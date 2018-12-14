#include "solver.h"

#include <glm/gtx/norm.hpp>

float Solver::poly6_kernel(const glm::vec3& r, const float h)
{
    return m_poly6_factor * glm::pow(m_h2 - glm::length2(r), 3);
}

glm::vec3 Solver::spiky_grad_kernel(const glm::vec3& r, const float h)
{
    float len = glm::length(r);
    float scale = (m_spiky_factor * glm::pow(m_h - len, 2)) / len;
    return scale*r;
}

Solver::Solver()
{
    m_h = 0.2f;
    m_h2 = m_h*m_h;
    set_poly6_factor();
    set_spiky_factor();
}
