#ifndef CONFIG_H
#define CONFIG_H

struct Config
{
    Config()
    {
        particle_radius  = 0.2f;
        grid_cell_width  = 5.0f;
        smoothing_radius = 1.0f; 
        kernel_radius    = 1.0f; 
        rest_density     = 80000.0f;
        particle_mass    = 1.0f;
        cfm_epsilon      = 5000.1f;
        timestep         = 0.001f;
        nparticles   = 1000;
        solver_iters = 1;
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
    int nparticles;         // number of particles
    int solver_iters;       // simulation step solver iterations
};

/*
 * Global variables go here.
 */
const float kNear = 0.1f;
const float kFar = 1000.0f;
const float kFov = 45.0f;

// Floor info.
const float kFloorEps = 0.5 * (0.025 + 0.0175);
const float kFloorXMin = -100.0f;
const float kFloorXMax = 100.0f;
const float kFloorZMin = -100.0f;
const float kFloorZMax = 100.0f;
const float kFloorY = -0.75617 - kFloorEps;
#endif
