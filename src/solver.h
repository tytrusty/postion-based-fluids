#ifndef SOLVER_H
#define SOLVER_H

#include <glm/glm.hpp>
#include <vector>
#include <memory>

#include "config.h"

class HashGrid;

struct Particle
{
    glm::vec3 p, p_old; // position
    glm::vec3 v; // velocity
    unsigned char r,g,b,a;
    float size;
    int id;
};

class Solver 
{
public:

    Solver(std::shared_ptr<Config>);

    void step(std::vector<Particle>& particles, std::shared_ptr<HashGrid> hash_grid);

private:
    /* SPH Kernels */
    float poly6_kernel(const glm::vec3& r);
    float poly6_kernel(const float r2);
    glm::vec3 spiky_grad_kernel(const glm::vec3& r);
    float spiky_grad_norm2(const float r2);
    void update_params();

    /* Set SPH Kernels scale factors */
    static float get_poly6_factor(float h) { return 315.0f/(64.0f*M_PI*std::pow(h,9)); }
    static float get_spiky_factor(float h) { return -45.0f/(M_PI*std::pow(h,6)); }

    std::shared_ptr<Config> m_config;
    float m_kernel_radius;
    float m_rest_density; 
    float m_rest_density2; 
    float m_particle_mass;
    float m_cfm_epsilon;  
    float m_h;  
    float m_h2; 
    float m_poly6_factor; 
    float m_spiky_factor;
    float m_timestep;
    size_t m_solver_iters;
    glm::vec3 m_bounds;
};
#endif // SOLVER_H
