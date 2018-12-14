#include "solver.h"
#include "hash_grid.h"

#include <glm/gtx/norm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>

#include <iostream>

Solver::Solver()
{
    m_h = 1.0f;
    m_h2 = m_h*m_h;
    set_poly6_factor();
    set_spiky_factor();
    m_timestep = 0.001;
    m_solver_iters = 1;
}

void Solver::step(std::vector<Particle>& particles, std::shared_ptr<HashGrid> hash_grid)
{
    // TODO make into member
    const int nparticles = particles.size();
    std::vector<float> lambdas(nparticles);
    std::vector<std::vector<int>> particle_neighbors(nparticles);
    std::vector<glm::vec3> gradients;

    const float kernel_radius = 1.0f;
    const float rest_density  = 0.5f;
    const float particle_mass = 1.0f;
    const float rest_density2 = rest_density*rest_density; 
    const float cfm_epsilon   = 0.1f;

    // Compute Lambdas First
    for (int i = 0; i < nparticles; ++i)
    {
        Particle& particle = particles[i];

        // 1. Apply external forces
        particle.p_old = particle.p;
        particle.p += m_timestep*particle.v;


        // 2. Find all neighbors
        std::vector<int> neighbors = hash_grid->find_neighbors(particle.id, particles, kernel_radius);
        particle_neighbors[i] = neighbors;

        // 3. Compute Lambda
        float grad_Ci_norm2 = 0.0f;
        glm::vec3 grad_Ci_i(0.0f); 

        float density_i = 0.0f;
        for (int j : neighbors)
        {
            glm::vec3 diff = particle.p - particles[j].p;

            // SPH density estimator
            density_i += particle_mass * poly6_kernel(diff); 

            // gradient accumulation
            glm::vec3 tmp_grad = spiky_grad_kernel(diff)/rest_density;
            grad_Ci_norm2 += glm::length2(tmp_grad);
            grad_Ci_i += tmp_grad;
        }
        float C_i = density_i/rest_density - 1.0f;
        grad_Ci_norm2 += glm::length2(grad_Ci_i);
        lambdas[i] = -C_i/(grad_Ci_norm2 + cfm_epsilon);
        //if (lambdas[i] != 0) {
        //    std::cout << lambdas[i] << std::endl;
        //    std::cout << "C_i:" << C_i << std::endl;
        //    std::cout << "rho_i:" << density_i << std::endl;
  
        //}    
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
        dp /= rest_density;

        particle.p += dp;
        if (dp != glm::vec3(0,0,0))
        std::cout << "dp_" << i << ": " << glm::to_string(dp) << std::endl;

        particle.v = (particle.p - particle.p_old) / m_timestep;
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
