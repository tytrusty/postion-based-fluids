#ifndef SOLVER_H
#define SOLVER_H

#include <glm/glm.hpp>

struct Particle
{
    glm::vec3 p, v;
    unsigned char r,g,b,a;
    float size;
};

class Solver 
{
public:
    /* SPH Kernels */
    static float poly6_kernel(const glm::vec3& r, const float h);
    static glm::vec3 spiky_grad_kernel(const glm::vec3& r, const float h);

    Solver(/* config* c */);

    //void step(std::vector<

private:
    void set_poly6_factor() { m_poly6_factor = 315.0f/(64.0f*M_PI*std::pow(m_h,9)); }
    void set_spiky_factor() { m_spiky_factor = -45.0f/(M_PI*std::pow(m_h,6)); }


    float m_h;  // smoothing radius
    float m_h2; // squared
    float m_poly6_factor;
    float m_spiky_factor;




};
#endif // SOLVER_H
