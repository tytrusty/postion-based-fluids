#ifndef GUI_H
#define GUI_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>

#include <memory>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw_gl3.h>

#include "config.h"

struct MatrixPointers {
    const glm::mat4 *projection, *model, *view;
};

enum class RenderMode
{
    Particle,
    Depth,
    Normal,
    Fluid
};

class GUI {
public:
    GUI(GLFWwindow*, std::shared_ptr<Config>);
    ~GUI();

    void setup();
    void keyCallback(int key, int scancode, int action, int mods);
    void mousePosCallback(double mouse_x, double mouse_y);
    void mouseButtonCallback(int button, int action, int mods);
    void updateMatrices();
    MatrixPointers getMatrixPointers() const;

    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MousePosCallback(GLFWwindow* window, double mouse_x, double mouse_y);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    glm::vec3 getCenter() const { return center_; }
    const glm::vec3& getCamera() const { return eye_; }
    const float* getLightPositionPtr() const { return &light_position_[0]; }

    bool isTransparent() const { return transparent_; }
    bool isPaused() const { return pause_simulation_; }
    bool isResetting() const { return reset_simulation_; }
    void setReset(bool reset) { reset_simulation_ = reset; }
    ImVec4 getClearColor() const { return clear_color_; }

    RenderMode getRenderMode()
    {
        switch(current_mode_)
        {
            case 0:
                return RenderMode::Particle;
            case 1: 
                return RenderMode::Depth;
            case 2:
                return RenderMode::Normal;
            case 3:
                return RenderMode::Fluid;
        }
    }

private:
    GLFWwindow* window_;
    std::shared_ptr<Config> config;

    int window_width_, window_height_;
    int view_width_, view_height_;

    bool drag_state_ = false;
    bool fps_mode_ = true;
    bool pose_changed_ = true;
    bool transparent_ = false;
    int current_button_ = -1;
    float roll_speed_ = M_PI / 64.0f;
    float last_x_ = 0.0f, last_y_ = 0.0f, current_x_ = 0.0f, current_y_ = 0.0f;
    float camera_distance_ = 8.0;
    float pan_speed_ = 0.2f;
    float rotation_speed_ = 0.02f;
    float zoom_speed_ = 0.1f;
    float aspect_;
    bool pause_simulation_ = true;
    bool reset_simulation_ = false;
    bool show_test_window_ = false;
    ImVec4 clear_color_; // ImColor(114, 144, 154);

    int current_mode_ = 0;

    glm::vec3 eye_ = glm::vec3(3.0f, 5.0f, camera_distance_);
    glm::vec3 up_ = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 look_ = glm::normalize(glm::vec3(-0.2f, -0.4f, -1.0f));
    glm::vec3 tangent_ = glm::cross(look_, up_);
    glm::vec3 center_ = eye_ - camera_distance_ * look_;
    glm::mat3 orientation_ = glm::mat3(tangent_, up_, look_);
    glm::vec4 light_position_;

    glm::mat4 view_matrix_ = glm::lookAt(eye_, center_, up_);
    glm::mat4 projection_matrix_;
    glm::mat4 model_matrix_ = glm::mat4(1.0f);

    bool captureWASDUPDOWN(int key, int action);
};

#endif
