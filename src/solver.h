#ifndef SOLVER_H
#define SOLVER_H

#include <glm/glm.hpp>
#include <vector>
#include <memory>

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

    Solver(/* config* c */);

    void step(std::vector<Particle>& particles);
    void step(std::vector<Particle>& particles, std::shared_ptr<HashGrid> hash_grid);

private:
    /* SPH Kernels */
    float poly6_kernel(const glm::vec3& r);
    glm::vec3 spiky_grad_kernel(const glm::vec3& r);

    /* Set SPH Kernels scale factors */
    void set_poly6_factor() { m_poly6_factor = 315.0f/(64.0f*M_PI*std::pow(m_h,9)); }
    void set_spiky_factor() { m_spiky_factor = -45.0f/(M_PI*std::pow(m_h,6)); }

    float m_h;  // smoothing radius
    float m_h2; // squared
    float m_poly6_factor; 
    float m_spiky_factor;
    float m_timestep;
    size_t m_solver_iters;
};
#endif // SOLVER_H
