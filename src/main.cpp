#include <stdio.h>
#include <string>
#include <iostream>
#include <memory>
#include <omp.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw_gl3.h>
#include "hash_grid.h"
#include "render_pass.h"
#include "gui.h"
#include "debuggl.h"
#include "config.h"
#include "solver.h"
#include "texture_to_render.h"
#include "shaders.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

int window_width = 1280;
int window_height = 720;
std::string window_title = "PBF Demo";

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

static const GLfloat s_quad_vertices[] = {
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    1.0f,  1.0f, 0.0f,
};


void create_fluid_cube(std::vector<Particle>& particles,
        GLfloat*& position_data, GLubyte*& color_data, std::shared_ptr<Config> c)
{
    glm::ivec3 dim = c->fluid_dim;
    int nparticles = dim[0]*dim[1]*dim[2];

    // If particle count changes, resize buffers
    if (nparticles != particles.size())
    {
        particles.resize(nparticles, Particle());
        for (int i = 0; i < nparticles; ++i) { particles[i].id = i; }
        position_data = new GLfloat[nparticles*3];
        color_data    = new GLubyte[nparticles*4];
    }

    glm::vec3 start = glm::vec3(0.0f, 0.0f, 0.0f); 
    float step = c->particle_radius;
    int n = 0;
    for (int i = 0; i < dim[0]; ++i)
    {
        start.x = step*i;
        for (int j = 0; j < dim[1]; ++j)
        {
            start.y = step*j;
            for (int k = 0; k < dim[2]; ++k)
            {
                start.z = step*k;

                particles[n].p = start;
                particles[n].v = glm::vec3(0.0f);
                particles[n].r = 64;// rand() % 256;
                particles[n].g = 124;// rand() % 256;
                particles[n].b = 253;// rand() % 256;
                particles[n].a = (rand() % 256)/3;

                color_data[4*n+0] = particles[n].r;
                color_data[4*n+1] = particles[n].g;
                color_data[4*n+2] = particles[n].b;
                color_data[4*n+3] = particles[n].a;
                ++n;
            }
        }
    }
}

static GLfloat* particle_position_data;
static GLubyte* particle_color_data;    

int main(int, char**)
{
    GLFWwindow* window = init_glefw();  
    std::shared_ptr<Config> config = std::make_shared<Config>();
    GUI gui(window, config);

    // Setup ImGui binding
    ImGui_ImplGlfwGL3_Init(window, false);
    glm::vec4 light_position = glm::vec4(0.0f, 15.0f, 0.0f, 1.0f);
    MatrixPointers mats;

    // FBOs
    TextureToRender rtt_depth(true);
    rtt_depth.create(window_width, window_height);
    TextureToRender rtt_filter;
    rtt_filter.create(window_width, window_height);
    TextureToRender rtt_filter_2; // NOTE: this is a hack ... 
    rtt_filter_2.create(window_width, window_height);

    TextureToRender rtt_normal;
    rtt_normal.create(window_width, window_height);

    int active_texture = -1;

    //--------------------------------Geometry--------------------------------//
    std::vector<glm::vec4> floor_vertices;
    std::vector<glm::uvec3> floor_faces;
    create_floor(floor_vertices, floor_faces);

    int nparticles = config->fluid_dim[0]*config->fluid_dim[1]*config->fluid_dim[2];
    std::vector<Particle> particles;
    create_fluid_cube(particles, particle_position_data, particle_color_data, config);
    //------------------------------------------------------------------------//

    //--------------------------------Uniforms--------------------------------//
    std::function<const glm::mat4*()> model_data = [&mats]() { return mats.model; };
    std::function<glm::mat4()> view_data = [&mats]() { return *mats.view; };
    std::function<glm::mat4()> proj_data = [&mats]() { return *mats.projection; };
    std::function<glm::mat4()> identity_mat = [](){ return glm::mat4(1.0f); };
    std::function<glm::vec3()> cam_data = [&gui](){ return gui.getCamera(); };
    std::function<glm::vec4()> lp_data = [&light_position]() { return light_position; };
    std::function<float()> alpha_data = [&gui]() { return gui.isTransparent() ? 0.5 : 1.0; };
    std::function<float()> radius_data = [&config]() { return config->particle_radius; };
    std::function<unsigned()> sampler_data = []() { return 0;  };
    std::function<unsigned()> tex_data = [&active_texture]() { return active_texture; };
    std::function<glm::vec2()> pixel_size_data = [=]() { return glm::vec2(1./window_width, 1./window_height);};
    std::function<int()> filter_radius_data = [&config]() { return config->filter_radius; };
    std::function<float()> filter_sigma_data = [&config]() { return config->filter_sigma; };


    auto std_model = std::make_shared<ShaderUniform<const glm::mat4*>>("model", model_data);
    auto floor_model = make_uniform("model", identity_mat);
    auto std_view = make_uniform("view", view_data);
    auto std_camera = make_uniform("camera_position", cam_data);
    auto std_proj = make_uniform("projection", proj_data);
    auto std_light = make_uniform("light_position", lp_data);
    auto object_alpha = make_uniform("alpha", alpha_data);
    auto object_radius = make_uniform("radius", radius_data);
    auto depth_tex = make_texture("tex_depth", sampler_data, 0, tex_data);
    auto pixel_size = make_uniform("pixel_size", pixel_size_data);
    auto filter_radius = make_uniform("filter_radius", filter_radius_data);
    auto filter_sigma = make_uniform("filter_sigma", filter_sigma_data);
    //------------------------------------------------------------------------//

    //-----------------------------Render Passes------------------------------//
    RenderDataInput floor_pass_input;
    floor_pass_input.assign(0, "vertex_position", floor_vertices.data(), floor_vertices.size(), 4, GL_FLOAT, GL_FALSE);
    floor_pass_input.assignIndex(floor_faces.data(), floor_faces.size(), 3);
    RenderPass floor_pass(-1,
            floor_pass_input,
            { vertex_shader, geometry_shader, floor_fragment_shader},
            { floor_model, std_view, std_proj, std_light },
            { "fragment_color" }
            );

    // Particle pass
    RenderDataInput particle_pass_input;
    particle_pass_input.assign(0, "vertex_position", g_vertex_buffer_data, sizeof(g_vertex_buffer_data), 3, GL_FLOAT, GL_FALSE);
    particle_pass_input.assign(1, "particle_position", nullptr, nparticles*3*sizeof(GLfloat), 3, GL_FLOAT, GL_FALSE);
    particle_pass_input.assign(2, "particle_color", nullptr, nparticles*4*sizeof(GLubyte), 4, GL_UNSIGNED_BYTE, GL_TRUE);
    RenderPass particle_pass(-1,
            particle_pass_input,
            { particle_vertex_shader, nullptr, particle_fragment_shader},
            { std_view, std_proj, std_light, object_radius},
            { "fragment_color" }
            );
    RenderDataInput filter_pass_input;
    filter_pass_input.assign(0, "vertex_position", s_quad_vertices, sizeof(s_quad_vertices), 3, GL_FLOAT, GL_FALSE);
    RenderPass filter_pass(-1,
            filter_pass_input,
            { quad_vertex_shader, nullptr, filter_fragment_shader},
            { std_view, std_proj, depth_tex, object_radius, pixel_size, filter_radius, filter_sigma },
            { "out_depth" }
            );
    RenderDataInput normal_pass_input;
    normal_pass_input.assign(0, "vertex_position", s_quad_vertices, sizeof(s_quad_vertices), 3, GL_FLOAT, GL_FALSE);
    RenderPass normal_pass(-1,
            normal_pass_input,
            { quad_vertex_shader, nullptr, normal_fragment_shader},
            { std_view, std_proj, depth_tex, pixel_size },
            { "out_normal" }
            );
    RenderDataInput depth_pass_input;
    depth_pass_input.assign(0, "vertex_position", s_quad_vertices, sizeof(s_quad_vertices), 3, GL_FLOAT, GL_FALSE);
    RenderPass depth_pass(-1,
            depth_pass_input,
            { quad_vertex_shader, nullptr, depth_fragment_shader},
            { std_view, std_proj, depth_tex },
            { "fragment_color" }
            );
    RenderDataInput fluid_pass_input;
    fluid_pass_input.assign(0, "vertex_position", s_quad_vertices, sizeof(s_quad_vertices), 3, GL_FLOAT, GL_FALSE);
    RenderPass fluid_pass(-1,
            fluid_pass_input,
            { quad_vertex_shader, nullptr, fluid_fragment_shader},
            { std_view, std_proj, std_light, depth_tex, std_light },
            { "fragment_color" }
            );
    //------------------------------------------------------------------------//

    std::shared_ptr<Solver> solver = std::make_shared<Solver>(config);
    std::shared_ptr<HashGrid> grid = std::make_shared<HashGrid>(config->grid_cell_width);
    grid->init(particles);
    std::cout << "Enterring main loop" << std::endl;

    std::cout << "OMP max threads: " << omp_get_max_threads() << std::endl;
    #pragma omp parallel
    {
        // dummy parallel region to spin up threads
    }

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        ImGui_ImplGlfwGL3_NewFrame();

        grid->update_cell_width(config->grid_cell_width);
        grid->init(particles);

        if (gui.isResetting())
        {
            create_fluid_cube(particles, particle_position_data, particle_color_data, config);
            nparticles = config->fluid_dim[0]*config->fluid_dim[1]*config->fluid_dim[2];
            gui.setReset(false);
        }

        if (!gui.isPaused()) 
            solver->step(particles, grid);
       
        #pragma omp parallel for
        for (int i = 0; i < nparticles; ++i)
        {
            const Particle& p = particles[i];
            particle_position_data[3*i+0] = p.p.x;
            particle_position_data[3*i+1] = p.p.y;
            particle_position_data[3*i+2] = p.p.z;
            particle_color_data[4*i+0] = particles[i].r;
            particle_color_data[4*i+1] = particles[i].g;
            particle_color_data[4*i+2] = particles[i].b;
        }

        gui.setup();
        ImVec4 clear_color = gui.getClearColor();

        //---------------------------Render-----------------------------------//
        RenderMode mode = gui.getRenderMode();

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

        // Render floor 
        floor_pass.setup();
        CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES, floor_faces.size() * 3, GL_UNSIGNED_INT, 0));

        // FBO bind
        if (mode != RenderMode::Particle)
        {
            rtt_depth.bind();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        // Render particles
        particle_pass.updateVBO(1, particle_position_data, nparticles);
        particle_pass.updateVBO(2, particle_color_data, nparticles);
        particle_pass.setup();
        glVertexAttribDivisor(0,0);
        glVertexAttribDivisor(1,1);
        glVertexAttribDivisor(2,1);
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, nparticles);

        if (mode != RenderMode::Particle)
        {
            rtt_depth.unbind();

            rtt_filter.bind();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            active_texture = rtt_depth.getTexture(); 
            filter_pass.setup();
            glDrawArrays(GL_TRIANGLES, 0, 6);
            rtt_filter.unbind();

            
            TextureToRender* curr = &rtt_filter_2;
            TextureToRender* prev = &rtt_filter;
            for (int i = 1; i < config->filter_iters; ++i)
            {
                curr->bind();
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                active_texture = prev->getTexture(); 
                filter_pass.setup();
                glDrawArrays(GL_TRIANGLES, 0, 6);
                curr->unbind();
                std::swap(curr,prev);
            }

            if (mode == RenderMode::Depth)
            {
                glClear(GL_DEPTH_BUFFER_BIT);
                active_texture = prev->getTexture(); 
                depth_pass.setup();
                glDrawArrays(GL_TRIANGLES, 0, 6);

            }
            else 
            {
                if (mode == RenderMode::Fluid)
                {
                    rtt_normal.bind();
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                }
                active_texture = prev->getTexture(); 
                normal_pass.setup(); 
                glDrawArrays(GL_TRIANGLES, 0, 6);

                if (mode == RenderMode::Fluid)
                {
                    rtt_normal.unbind();
                    glClear(GL_DEPTH_BUFFER_BIT);
                    active_texture = rtt_normal.getTexture(); 
                    fluid_pass.setup();
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }
        }

        // Render UI 
        ImGui::Render();

        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    glfwTerminate();

    return 0;
}
