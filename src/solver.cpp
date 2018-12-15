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
            grad_Ci_norm2 += glm::length2(tmp_grad);
            grad_Ci_i += tmp_grad;
        }
        float C_i = density_i/m_rest_density - 1.0f;
        grad_Ci_norm2 -= glm::length2(grad_Ci_i);
        lambdas[i] = -C_i/(grad_Ci_norm2 + m_cfm_epsilon);
    }

    // Update position and velocity
    for (int i = 0; i < nparticles; ++i)
    {
        Particle& particle = particles[i];

        glm::vec3 dp(0.0f);

        // 3. Compute Lambda
        float grad_Ci_norm2 = 0.0f;
        glm::vec3 grad_Ci_i(0.0f); 

        for (int j : particle_neighbors[i])
        {
            glm::vec3 diff = particle.p - particles[j].p;
            dp += (lambdas[i]+lambdas[j])*spiky_grad_kernel(diff);
        }
        dp /= m_rest_density;

        particle.p += dp;
        //if (dp != glm::vec3(0,0,0))
        //if (i % 200 == 0)
        //std::cout << "dp_" << i << ": " << glm::to_string(dp) << std::endl;

        particle.v = (particle.p - particle.p_old) / m_timestep;

        // handle collisions
        if (particle.p.y < 0.0f)
        {
            float penaltyStiffness = 1e3;
            //particle.v += m_timestep*glm::vec3(0.0f, penaltyStiffness*particle.p.y*particle.p.y, 0);
            particle.p.y = 0.0f;
        }

    }
}

float Solver::poly6_kernel(const glm::vec3& r)
{
    //std::cout << "factor: " << m_poly6_factor << std::endl;
    //std::cout << "m_h2 : " << m_h2 << std::endl;
    //std::cout << "r: " << glm::to_string(r) << std::endl;
    return m_poly6_factor * glm::pow(m_h2 - glm::length2(r), 3);
}

glm::vec3 Solver::spiky_grad_kernel(const glm::vec3& r)
{
    float len = glm::length(r);
    float scale = (m_spiky_factor * glm::pow(m_h - len, 2)) / len;
    return scale*r;
}
