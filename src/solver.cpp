#include "solver.h"
#include "hash_grid.h"

#include <glm/gtx/norm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>

#include <iostream>

Solver::Solver(std::shared_ptr<Config> config)
    : m_config(config)
{
}

void Solver::update_params()
{
    m_h             = m_config->smoothing_radius;
    m_h2            = m_h*m_h2;
    m_timestep      = m_config->timestep;
    m_cfm_epsilon   = m_config->cfm_epsilon;;
    m_poly6_factor  = get_poly6_factor(m_h);
    m_spiky_factor  = get_spiky_factor(m_h);
    m_solver_iters  = m_config->solver_iters;
    m_rest_density  = m_config->rest_density;
    m_rest_density2 = m_rest_density*m_rest_density; 
    m_particle_mass = m_config->particle_mass;
    m_kernel_radius = m_config->kernel_radius;
}

void Solver::step(std::vector<Particle>& particles, std::shared_ptr<HashGrid> hash_grid)
{
    const int nparticles = particles.size();
    update_params(); // update simulation paramters

    // TODO make into member
    std::vector<float> lambdas(nparticles);
    std::vector<std::vector<int>> particle_neighbors(nparticles);
    // std::vector<glm::vec3> gradients;

    // Compute Lambdas First
    for (int i = 0; i < nparticles; ++i)
    {
        Particle& particle = particles[i];

        // 1. Apply external forces and predict position 
        {
            particle.p_old = particle.p;
            particle.v += m_timestep*glm::vec3(0.0f,-9.8f, 0.0f); // "gravity"
            particle.p += m_timestep*particle.v;
        }

        // 2. Find all neighbors
        std::vector<int> neighbors = hash_grid->find_neighbors(particle.id, particles, m_kernel_radius);
        particle_neighbors[i] = neighbors;

        // 3. Compute Lambda
        float grad_Ci_norm2 = 0.0f;
        glm::vec3 grad_Ci_i(0.0f); 

        float density_i = 0.0f;
        for (int j : neighbors)
        {
            glm::vec3 diff = particle.p - particles[j].p;

            // SPH density estimator
            density_i += m_particle_mass * poly6_kernel(diff); 

            // gradient accumulation
            glm::vec3 tmp_grad = spiky_grad_kernel(diff)/m_rest_density;
            grad_Ci_norm2 -= glm::length2(tmp_grad); 
            grad_Ci_i += tmp_grad;
        }
        float C_i = density_i/m_rest_density - 1.0f;
        grad_Ci_norm2 += glm::length2(grad_Ci_i);
        lambdas[i] = -C_i/(grad_Ci_norm2 + m_cfm_epsilon);
    }

    // Update position and velocity
    const float k  = m_config->artificial_pressure_k;    
    const float n  = m_config->artificial_pressure_n;
    const float dq = m_config->artificial_pressure_dq;
    const float c = m_config->viscosity_c; 

    for (int i = 0; i < nparticles; ++i)
    {
        Particle& particle = particles[i];
        float grad_Ci_norm2 = 0.0f;
        glm::vec3 grad_Ci_i(0.0f); 
        glm::vec3 viscosity(0.0f); 
        glm::vec3 dp(0.0f);

        for (int j : particle_neighbors[i])
        {
            glm::vec3 diff = particle.p - particles[j].p;
            glm::vec3 diffv = particle.v - particles[j].v;

            // s_corr
            float s_corr = 0.0f;
            {
                s_corr = -k*glm::pow(poly6_kernel(diff)/poly6_kernel(dq), n);
            }
            dp += (lambdas[i]+lambdas[j]+s_corr)*spiky_grad_kernel(diff);
            viscosity += poly6_kernel(diff)*diffv;
        }
        dp /= m_rest_density;
        

        particle.p += dp;

        // boundary conditions
        float bound = 1.0f;
        if (particle.p.y > bound) particle.p.y = bound;
        if (particle.p.y < 0.0f) particle.p.y = 0.0f;
        if (particle.p.x > bound) particle.p.x = bound;
        if (particle.p.x < 0.0f) particle.p.x = 0.0f;
        if (particle.p.z > bound) particle.p.z = bound;
        if (particle.p.z < 0.0f) particle.p.z = 0.0f;

        // Update velocity 
        particle.v = (particle.p - particle.p_old) / m_timestep;

        particle.v += c*viscosity;
    }
}

float Solver::poly6_kernel(const glm::vec3& r)
{
    float r2 = glm::length2(r);
    const float base = m_h2-r2;
    if (r2 < m_h2)
        return m_poly6_factor * glm::pow(base, 3);
    else
        return  0.0f;
}

float Solver::poly6_kernel(const float r)
{
    const float base = m_h2-r*r;
    if (r < m_h) // (base < 1e-4)
        return m_poly6_factor * glm::pow(base, 3);
    else
        return  0.0f;
}

glm::vec3 Solver::spiky_grad_kernel(const glm::vec3& r)
{
    float len = glm::length(r);
    if (len > m_h || len < 1e-5)
        return glm::vec3(0.0f);

    float scale = (m_spiky_factor * glm::pow(m_h - len, 2)) / len;
    return scale*r;
}
