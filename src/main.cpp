#include <stdio.h>
#include <string>
#include <iostream>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw_gl3.h>

#include <Eigen/Dense>

#include "hash_grid.h"
#include "render_pass.h"
#include "gui.h"
#include "debuggl.h"
#include "config.h"
#include "solver.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

int window_width = 1280;
int window_height = 720;
std::string window_title = "PBF Demo";

const char* vertex_shader =
#include "shaders/default.vert"
;
const char* geometry_shader =
#include "shaders/default.geom"
;
const char* fragment_shader =
#include "shaders/default.frag"
;
const char* floor_fragment_shader =
#include "shaders/floor.frag"
;

const char* particle_vertex_shader =
#include "shaders/particle.vert"
;

const char* particle_fragment_shader =
#include "shaders/particle.frag"
;

GLFWwindow* init_glefw()
{
	if (!glfwInit())
		exit(EXIT_FAILURE);
	glfwSetErrorCallback(error_callback);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE); // Disable resizing, for simplicity
	glfwWindowHint(GLFW_SAMPLES, 4);
	auto ret = glfwCreateWindow(window_width, window_height, window_title.data(), nullptr, nullptr);

	glfwMakeContextCurrent(ret);
	glewExperimental = GL_TRUE;
	glewInit();
	glGetError();  // clear GLEW's error for it
	glfwSwapInterval(1);
	const GLubyte* renderer = glGetString(GL_RENDERER);  // get renderer string
	const GLubyte* version = glGetString(GL_VERSION);    // version as a string
	std::cout << "Renderer: " << renderer << "\n";
	std::cout << "OpenGL version supported:" << version << "\n";

	return ret;
}


void create_floor(std::vector<glm::vec4>& floor_vertices, std::vector<glm::uvec3>& floor_faces)
{
	floor_vertices.push_back(glm::vec4(kFloorXMin, kFloorY, kFloorZMax, 1.0f));
	floor_vertices.push_back(glm::vec4(kFloorXMax, kFloorY, kFloorZMax, 1.0f));
	floor_vertices.push_back(glm::vec4(kFloorXMax, kFloorY, kFloorZMin, 1.0f));
	floor_vertices.push_back(glm::vec4(kFloorXMin, kFloorY, kFloorZMin, 1.0f));
	floor_faces.push_back(glm::uvec3(0, 1, 2));
	floor_faces.push_back(glm::uvec3(2, 3, 0));
}

static const GLfloat g_vertex_buffer_data[] = {
    -0.5f, -0.5f, 0.0f,
    0.5f, -0.5f, 0.0f,
    -0.5f, 0.5f, 0.0f,
    0.5f, 0.5f, 0.0f,
};


int main(int, char**)
{
    GLFWwindow* window = init_glefw();	
    GUI gui(window);

    // Setup ImGui binding
    ImGui_ImplGlfwGL3_Init(window, false);

    bool show_test_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImColor(114, 144, 154);

    glm::vec4 light_position = glm::vec4(0.0f, 10.0f, 0.0f, 1.0f);
    MatrixPointers mats;

    std::vector<glm::vec4> floor_vertices;
    std::vector<glm::uvec3> floor_faces;
    create_floor(floor_vertices, floor_faces);

    //---------------------------------------//
    // Particle Temp
    const int nparticles = 1000;
    float particle_radius = 0.5f;
    Particle particle_container[nparticles];
    static GLfloat* particle_position_data = new GLfloat[nparticles*4];
    static GLubyte* particle_color_data    = new GLubyte[nparticles*4];
    //---------------------------------------//


	// Setup uniforms
    std::function<const glm::mat4*()> model_data = [&mats]() { return mats.model; };


	std::function<glm::mat4()> view_data = [&mats]() { return *mats.view; };
	std::function<glm::mat4()> proj_data = [&mats]() { return *mats.projection; };
	std::function<glm::mat4()> identity_mat = [](){ return glm::mat4(1.0f); };
	std::function<glm::vec3()> cam_data = [&gui](){ return gui.getCamera(); };
	std::function<glm::vec4()> lp_data = [&light_position]() { return light_position; };
	std::function<float()> alpha_data = [&gui]() { return gui.isTransparent() ? 0.5 : 1.0; };
	std::function<float()> radius_data = [&particle_radius]() { return particle_radius; };

	auto std_model = std::make_shared<ShaderUniform<const glm::mat4*>>("model", model_data);
	auto floor_model = make_uniform("model", identity_mat);
	auto std_view = make_uniform("view", view_data);
	auto std_camera = make_uniform("camera_position", cam_data);
	auto std_proj = make_uniform("projection", proj_data);
	auto std_light = make_uniform("light_position", lp_data);
	auto object_alpha = make_uniform("alpha", alpha_data);
    auto object_radius = make_uniform("radius", radius_data);


	// Floor render pass
	RenderDataInput floor_pass_input;
	floor_pass_input.assign(0, "vertex_position", floor_vertices.data(), floor_vertices.size(), 4, GL_FLOAT);
	floor_pass_input.assignIndex(floor_faces.data(), floor_faces.size(), 3);
	RenderPass floor_pass(-1,
			floor_pass_input,
			{ vertex_shader, geometry_shader, floor_fragment_shader},
			{ floor_model, std_view, std_proj, std_light },
			{ "fragment_color" }
			);

    // ----------------------------------------------------------------------//
    //                           Generate particle block                     //
    glm::vec3 start = glm::vec3(0.0f, 0.0f, 0.0f); 
    float step = particle_radius;
    int n = 0;
    for (int i = 0; i < 10; ++i)
    {
        start.x = step*i;
        for (int j = 0; j < 10; ++j)
        {
            start.y = step*j;
            for (int k = 0; k < 10; ++k)
            {
                start.z = step*k;

                particle_container[n].p = start;
                particle_container[n].r = rand() % 256;
                particle_container[n].g = rand() % 256;
                particle_container[n].b = rand() % 256;
                particle_container[n].a = (rand() % 256)/3;
                particle_container[n].size = 0.2f;// (rand()%1000)/2000.0f + 0.1f;

                particle_position_data[4*n+0] = start.x;
                particle_position_data[4*n+1] = start.y;
                particle_position_data[4*n+2] = start.z;
                particle_position_data[4*n+3] = particle_container[i].size;;

                particle_color_data[4*n+0] = particle_container[i].r;
                particle_color_data[4*n+1] = particle_container[i].g;
                particle_color_data[4*n+2] = particle_container[i].b;
                particle_color_data[4*n+3] = particle_container[i].a;

                ++n;
            }
        }
    }
    // ----------------------------------------------------------------------//

    // Particle pass
    RenderDataInput particle_pass_input;
    particle_pass_input.assign(0, "vertex_position", g_vertex_buffer_data, sizeof(g_vertex_buffer_data), 3, GL_FLOAT);
    particle_pass_input.assign(1, "particle_position", particle_position_data,
            nparticles*4*sizeof(GLfloat), 4, GL_FLOAT);
    particle_pass_input.assign(2, "particle_color", particle_color_data,
            nparticles*4*sizeof(GLubyte), 4, GL_UNSIGNED_INT);
	RenderPass particle_pass(-1,
			particle_pass_input,
			{ particle_vertex_shader, nullptr, particle_fragment_shader},
			{ std_view, std_proj, std_light, object_radius},
			{ "fragment_color" }
			);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        ImGui_ImplGlfwGL3_NewFrame();


        // 1. Show a simple window
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
        {
            static float f = 0.0f;
            ImGui::Text("Hello, world!");
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
            ImGui::ColorEdit3("clear color", (float*)&clear_color);
            if (ImGui::Button("Test Window")) show_test_window ^= 1;
            if (ImGui::Button("Another Window")) show_another_window ^= 1;
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        }

        // 2. Show another simple window, this time using an explicit Begin/End pair
        if (show_another_window)
        {
            ImGui::SetNextWindowSize(ImVec2(200,100), ImGuiSetCond_FirstUseEver);
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::Text("Hello");
            ImGui::End();
        }

        // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
        if (show_test_window)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
            ImGui::ShowTestWindow(&show_test_window);
        }

        // Render Setup
		glfwGetFramebufferSize(window, &window_width, &window_height);
        glViewport(0, 0, window_width, window_height);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_MULTISAMPLE);
		glEnable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LESS);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glCullFace(GL_BACK);

		gui.updateMatrices();
		mats = gui.getMatrixPointers();

		// Render
		floor_pass.setup();
		CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, floor_faces.size() * 3, GL_UNSIGNED_INT, 0));

        particle_pass.setup();
        glVertexAttribDivisor(0,0);
        glVertexAttribDivisor(1,1);
        glVertexAttribDivisor(2,1);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, nparticles);

		ImGui::Render();

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    glfwTerminate();

    return 0;
}
