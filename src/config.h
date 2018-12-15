#ifndef CONFIG_H
#define CONFIG_H

#include <glm/glm.hpp>

struct Config
{
    Config()
    {
        particle_radius  = 0.2f;
        grid_cell_width  = 1.0f;
        smoothing_radius = 2.0f; 
        kernel_radius    = 1.2f; 
        rest_density     = 60.0f; // 63780.0f;
        particle_mass    = 1.0f;
        cfm_epsilon      = 100.0f; // 6000.0f;
        timestep         = 0.016f;
        viscosity_c      = 0.0000001;
        artificial_pressure_k  = 0.0001f;
        artificial_pressure_dq = 0.03f;
        artificial_pressure_n  = 4;
        solver_iters = 4;
        fluid_dim = glm::ivec3(10,30,10);
    }

    // Simulation Parameters
    float particle_radius;  // radius of each particle
    float grid_cell_width;  // hash grid cell width
    float smoothing_radius; // SPH kernel smoothing radius (h)
    float kernel_radius;    // SPH kernel radius (neighbor radius)
    float rest_density;     // SPH particle rest density
    float particle_mass;    // SPH particle mass
    float cfm_epsilon;      // constraint force mixing epsilon for PBF
    float timestep;         // simulation timestep 
    float artificial_pressure_k; 
    float artificial_pressure_dq;
    int artificial_pressure_n;
    float viscosity_c;
    int solver_iters;       // simulation step solver iterations
    glm::ivec3 fluid_dim;
};

/*
 * Global variables go here.
 */
const float kNear = 0.1f;
const float kFar = 1000.0f;
const float kFov = 70.0f;

// Floor info.
const float kFloorEps = 0.5 * (0.025 + 0.0175);
const float kFloorXMin = -100.0f;
const float kFloorXMax = 100.0f;
const float kFloorZMin = -100.0f;
const float kFloorZMax = 100.0f;
const float kFloorY = -0.75617 - kFloorEps;
#endif
