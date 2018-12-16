#include "solver.h"
#include "hash_grid.h"

#include <GLFW/glfw3.h>

#include <glm/gtx/norm.hpp>
#include <glm/ext.hpp>
#include <glm/gtx/string_cast.hpp>

#include <iostream>
#include <omp.h>

using namespace std; // haters gonna hate

Solver::Solver(shared_ptr<Config> config)
    : m_config(config)
{
}

void Solver::update_params()
{
    m_h             = m_config->smoothing_radius;
    m_h2            = m_h*m_h;
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

static int step_cnt = 0;
void Solver::step(vector<Particle>& particles, shared_ptr<HashGrid> hash_grid)
{
    step_cnt++;
    double t0 = glfwGetTime();

    const int nparticles = particles.size();
    update_params(); // update simulation paramters

    vector<float> lambdas(nparticles);
    vector<vector<pair<int,float>>> particle_neighbors(nparticles);
    // vector<glm::vec3> gradients;
    double t_init = glfwGetTime()-t0;
    double t_predict_pos = 0.0f;
    double t_neighbors = 0.0f;
    double t_lambda = 0.0f;
    double t_update = 0.0f;

    double t_total_lambda = 0.0f;
    double t_total0 = glfwGetTime();

    // Compute Lambdas First
    #pragma omp parallel
    {
    #pragma omp for schedule(dynamic, 1)
    for (int i = 0; i < nparticles; ++i)
    {
        Particle& particle = particles[i];
        t0 = glfwGetTime(); 
        // 1. Apply external forces and predict position 
        {
            particle.p_old = particle.p;
            particle.v += m_timestep*glm::vec3(0.0f,-9.8f, 0.0f); // "gravity"
            particle.p += m_timestep*particle.v;
            // cout << "p.v." << glm::to_string(particle.v) << endl;
        }
        t_predict_pos += glfwGetTime()-t0;


        t0 = glfwGetTime();
        // 2. Find all neighbors
        vector<pair<int,float>> neighbors = hash_grid->find_neighbors(particle.id, particles, m_kernel_radius);
        particle_neighbors[i] = neighbors;

        t_neighbors += glfwGetTime()-t0;
        t0 = glfwGetTime();

        // 3. Compute Lambda
        float grad_Ci_norm2 = 0.0f;
        glm::vec3 grad_Ci_i(0.0f); 

        float density_i = 0.0f;
        for (auto& j_pair : neighbors)
        {
            int j = j_pair.first;
            float distsqr = j_pair.second;
            glm::vec3 diff = particle.p - particles[j].p;

            // SPH density estimator
            density_i += m_particle_mass * poly6_kernel(distsqr); 

            // gradient accumulation
            glm::vec3 tmp_grad = spiky_grad_kernel(diff);
            grad_Ci_norm2 -= glm::length2(tmp_grad);  
            grad_Ci_i += tmp_grad;
        }
        float C_i = density_i/m_rest_density - 1.0f;
        //particle.r = 0.0f;
        //particle.g = 0.0f;
        //particle.b = glm::clamp<int>((C_i+0.7)*256, 0, 256);
        grad_Ci_norm2 += glm::length2(grad_Ci_i);
        lambdas[i] = -C_i/((grad_Ci_norm2/m_rest_density2) + m_cfm_epsilon);

        t_lambda += glfwGetTime()-t0;
        t0 = glfwGetTime();
    }
    t_total_lambda = glfwGetTime() - t_total0;

    // Update position and velocity
    const float k  = m_config->artificial_pressure_k;    
    const float n  = m_config->artificial_pressure_n;
    const float dq = m_config->artificial_pressure_dq;
    const float c = m_config->viscosity_c; 
    float dq_poly6 = poly6_kernel(dq);

    t0 = glfwGetTime();
    #pragma omp for schedule(dynamic, 1)
    for (int i = 0; i < nparticles; ++i)
    {
        Particle& particle = particles[i];
        float grad_Ci_norm2 = 0.0f;
        glm::vec3 grad_Ci_i(0.0f); 
        glm::vec3 viscosity(0.0f); 
        glm::vec3 dp(0.0f);

        for (auto& j_pair : particle_neighbors[i])
        {
            int j = j_pair.first;
            float distsqr = j_pair.second;
            glm::vec3 diff = particle.p - particles[j].p;
            glm::vec3 diffv = particles[j].v - particle.v;

            // s_corr
            float s_corr = 0.0f;
            {
                s_corr = -k*glm::pow(poly6_kernel(distsqr)/dq_poly6, n);
            }
            dp += (lambdas[i]+lambdas[j]+s_corr)*spiky_grad_kernel(diff);
            viscosity += poly6_kernel(distsqr)*diffv;
        }
        dp /= m_rest_density;
        

        particle.p += dp;

        // boundary conditions
        float bound = m_config->particle_radius*20.0f;
        if (particle.p.y > bound*3) particle.p.y = bound*3;
        if (particle.p.y < 0.0f) particle.p.y = 0.0f;
        if (particle.p.x > bound*2) particle.p.x = bound*2;
        if (particle.p.x < 0.0f) particle.p.x = 0.0f;
        if (particle.p.z > bound) particle.p.z = bound;
        if (particle.p.z < 0.0f) particle.p.z = 0.0f;

        // Update velocity 
        particle.v = (particle.p - particle.p_old) / m_timestep;
        particle.v += c*viscosity;
    }
    t_update = glfwGetTime()-t0;
    }

    ///// Diagnostics /////
    if (step_cnt % 10 == 0)
    {
        cout << "///////////////////" << endl;
        cout << "(1) Init time:     " << t_init << endl;
        cout << "(2) Predict time:  " << t_predict_pos << endl;
        cout << "(3) Neighbor time: " << t_neighbors << endl;
        cout << "(4) Lambda time:   " << t_lambda << endl;
        cout << "(5) Total lambda:  " << t_total_lambda << endl;
        cout << "(5) Update p time: " << t_update << endl;
        cout << "///////////////////" << endl;
    double t_init = glfwGetTime()-t0;
    double t_predict_pos = 0.0f;
    double t_neighbors = 0.0f;
    double t_lambda = 0.0f;
    double t_update = 0.0f;

    }
}

float Solver::poly6_kernel(const glm::vec3& r)
{
    float r2 = glm::length2(r);
    if (r2 < m_h2)
        return m_poly6_factor * glm::pow(m_h2-r2, 3);
    else
        return 0.0f;
}

float Solver::poly6_kernel(const float r2)
{
    if (r2 < m_h2)
        return m_poly6_factor * glm::pow(m_h2-r2, 3);
    else
        return 0.0f;
}

glm::vec3 Solver::spiky_grad_kernel(const glm::vec3& r)
{
    float len = glm::length(r);
    if (len > m_h || len < 1e-5)
        return glm::vec3(0.0f);

    float scale = (m_spiky_factor * glm::pow(m_h - len, 2)) / len;
    return scale*r;
}

float Solver::spiky_grad_norm2(const float r2)
{
    float len = glm::sqrt(r2);
    if (len > m_h || len < 1e-5)
        return 0.0f;

    float scale = (m_spiky_factor * glm::pow(m_h - len, 2))/ len;
    return scale*scale;
}
