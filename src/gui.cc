#include "gui.h"
#include "config.h"
#include "debuggl.h"
#include <iostream>
#include <algorithm>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

static const char* render_modes_[] = { "Particle", "Depth", "Normal", "Fluid" };

GUI::GUI(GLFWwindow* window, std::shared_ptr<Config> config)
    :window_(window), config(config)
{
    glfwSetWindowUserPointer(window_, this);
    glfwSetKeyCallback(window_, KeyCallback);
    glfwSetCursorPosCallback(window_, MousePosCallback);
    glfwSetMouseButtonCallback(window_, MouseButtonCallback);
    glfwSetCharCallback(window, ImGui_ImplGlfwGL3_CharCallback);
    glfwGetWindowSize(window_, &window_width_, &window_height_);
    view_width_ = window_width_;
    view_height_ = window_height_;
    float aspect_ = static_cast<float>(view_width_) / view_height_;
    projection_matrix_ = glm::perspective((float)(kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
}

GUI::~GUI()
{}

void GUI::setup()
{
    ImGui::SetNextWindowSize(ImVec2(400,460), ImGuiSetCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(20,20), ImGuiSetCond_FirstUseEver);
    ImGui::Begin("Simulation Parameters");
    ImGui::Combo("Render Mode", &current_mode_, render_modes_, 4);
    ImGui::SliderInt("Solver Iterations", &config->solver_iters, 1, 100);
    ImGui::SliderFloat("Particle Radius", &config->particle_radius, 0.1f, 10.0f);
    ImGui::InputFloat("Timestep", &config->timestep, 0.0001f, 1.0f);
    ImGui::SliderFloat("Hash Grid Cell Width", &config->grid_cell_width, 0.1f, 10.0f);
    ImGui::SliderFloat("Smoothing Radius (h)", &config->smoothing_radius, 0.0f, 30.0f);
    ImGui::SliderFloat("Kernel Radius", &config->kernel_radius, 0.01f, 10.0f);
    ImGui::SliderFloat("Particle Rest Density", &config->rest_density, 0.1f, 100000.0f);
    ImGui::SliderFloat("Particle Mass", &config->particle_mass, 0.1f, 10.0f);
    ImGui::SliderFloat("CFM Epsilon", &config->cfm_epsilon, 0.1f, 10000.0f);
    ImGui::InputFloat("Viscosity Scale (c)", &config->viscosity_c, 0.00000001f, 2.0f);
    ImGui::InputFloat("Artificial pressure (k)", &config->artificial_pressure_k, 0.00001f, 5.0f);
    ImGui::SliderInt("Artificial pressure (n)", &config->artificial_pressure_n, 1, 10);
    ImGui::InputFloat("Artificial pressure (dq)", &config->artificial_pressure_dq, 0.00001f, 5.0f);
    ImGui::InputInt3("Fluid Cube Dim", glm::value_ptr(config->fluid_dim));
    ImGui::SliderInt("Filter Radius", &config->filter_radius, 0, 20);
    ImGui::SliderInt("Filter Iters", &config->filter_iters, 1, 20);
    ImGui::SliderFloat("Filter Sigma", &config->filter_sigma, 0.0001f, 30.0f);

    ImGui::ColorEdit3("Clear Color", (float*)&clear_color_);
    if (ImGui::Button("Start/Stop")) pause_simulation_ ^= 1;
    if (ImGui::Button("Reset")) reset_simulation_ ^= 1;
    if (ImGui::Button("Test Window")) show_test_window_ ^= 1;
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    if (show_test_window_)
    {
        ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);
        ImGui::ShowTestWindow(&show_test_window_);
    }
}

void GUI::keyCallback(int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window_, GL_TRUE);
        return ;
    }
    if (mods == 0 && captureWASDUPDOWN(key, action))
        return ;
    if (key == GLFW_KEY_C && action != GLFW_RELEASE) {
        fps_mode_ = !fps_mode_;
    } 
}

void GUI::mousePosCallback(double mouse_x, double mouse_y)
{
    last_x_ = current_x_;
    last_y_ = current_y_;
    current_x_ = mouse_x;
    current_y_ = window_height_ - mouse_y;
    float delta_x = current_x_ - last_x_;
    float delta_y = current_y_ - last_y_;
    if (sqrt(delta_x * delta_x + delta_y * delta_y) < 1e-15)
        return;
    if (mouse_x > view_width_)
        return ;
    glm::vec3 mouse_direction = glm::normalize(glm::vec3(delta_x, delta_y, 0.0f));

    bool drag_camera = drag_state_ && current_button_ == GLFW_MOUSE_BUTTON_LEFT;

    if (drag_camera) {
        glm::vec3 axis = glm::normalize(
                orientation_ *
                glm::vec3(mouse_direction.y, -mouse_direction.x, 0.0f)
                );
        orientation_ =
            glm::mat3(glm::rotate(rotation_speed_, axis) * glm::mat4(orientation_));
        tangent_ = glm::column(orientation_, 0);
        up_ = glm::column(orientation_, 1);
        look_ = glm::column(orientation_, 2);
    } 
}

void GUI::mouseButtonCallback(int button, int action, int mods)
{
    if (current_x_ <= view_width_) {
        drag_state_ = (action == GLFW_PRESS);
        current_button_ = button;
        return ;
    }
}

void GUI::updateMatrices()
{
    // Compute our view, and projection matrices.
    if (fps_mode_)
        center_ = eye_ + camera_distance_ * look_;
    else
        eye_ = center_ - camera_distance_ * look_;

    view_matrix_ = glm::lookAt(eye_, center_, up_);
    light_position_ = glm::vec4(eye_, 1.0f);

    aspect_ = static_cast<float>(view_width_) / view_height_;
    projection_matrix_ = glm::perspective((float)(kFov * (M_PI / 180.0f)), aspect_, kNear, kFar);
    model_matrix_ = glm::mat4(1.0f);
}

MatrixPointers GUI::getMatrixPointers() const
{
    MatrixPointers ret;
    ret.projection = &projection_matrix_;
    ret.model= &model_matrix_;
    ret.view = &view_matrix_;
    return ret;
}

bool GUI::captureWASDUPDOWN(int key, int action)
{
    if (key == GLFW_KEY_W) {
        if (fps_mode_)
            eye_ += zoom_speed_ * look_;
        else
            camera_distance_ -= zoom_speed_;
        return true;
    } else if (key == GLFW_KEY_S) {
        if (fps_mode_)
            eye_ -= zoom_speed_ * look_;
        else
            camera_distance_ += zoom_speed_;
        return true;
    } else if (key == GLFW_KEY_A) {
        if (fps_mode_)
            eye_ -= pan_speed_ * tangent_;
        else
            center_ -= pan_speed_ * tangent_;
        return true;
    } else if (key == GLFW_KEY_D) {
        if (fps_mode_)
            eye_ += pan_speed_ * tangent_;
        else
            center_ += pan_speed_ * tangent_;
        return true;
    } else if (key == GLFW_KEY_DOWN) {
        if (fps_mode_)
            eye_ -= pan_speed_ * up_;
        else
            center_ -= pan_speed_ * up_;
        return true;
    } else if (key == GLFW_KEY_UP) {
        if (fps_mode_)
            eye_ += pan_speed_ * up_;
        else
            center_ += pan_speed_ * up_;
        return true;
    }
    return false;
}


// Delegrate to the actual GUI object.
void GUI::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
    gui->keyCallback(key, scancode, action, mods);
    ImGui_ImplGlfwGL3_KeyCallback(window,key,scancode,action,mods);
}

void GUI::MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y)
{
    GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
    gui->mousePosCallback(mouse_x, mouse_y);
}

void GUI::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    GUI* gui = (GUI*)glfwGetWindowUserPointer(window);
    gui->mouseButtonCallback(button, action, mods);
    ImGui_ImplGlfwGL3_MouseButtonCallback(window,button,action,mods);
}
