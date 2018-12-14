#ifndef HASH_GRID_H
#define HASH_GRID_H

#include <tuple>
#include <vector>
#include <map>
#include <iostream>

#include <glm/gtx/norm.hpp>

#include "solver.h"

class HashGrid
{
public:
    typedef std::tuple<int,int,int> triple;

    HashGrid(float cell_width) 
        : m_cell_width(cell_width)
    {}

    void update_cell_width(float cell_width) { m_cell_width = cell_width; }

    void init(const std::vector<Particle>& particles)
    {
        for (const auto& p : particles)
            insert_particle(p);
    }

    triple hash_position(const Particle& particle)
    {
       int i = particle.p.x / m_cell_width;
       int j = particle.p.y / m_cell_width;
       int k = particle.p.z / m_cell_width;
       return std::make_tuple(i,j,k);
    }

    void insert_particle(const Particle& particle)
    {
        triple key = hash_position(particle);
        auto it = bins.find(key);
        if (it != bins.end())
        {
            it->second.emplace_back(particle.id);
        }
        else
        {
            bins.insert(std::pair<triple,std::vector<int>>(key, {particle.id}));
        }
    }

    std::vector<int> find_neighbors(int idx, const std::vector<Particle>& particles, float radius)
    {
        triple key = hash_position(particles[idx]);
        std::vector<int> neighbors;
        float radius2 = radius*radius; 

        int r = (int)(radius/m_cell_width) + 1;
        int key_x = std::get<0>(key);
        int key_y = std::get<1>(key);
        int key_z = std::get<2>(key);

        // Search surrounding buckets for neighbors within radius
        for (int x = key_x-r; x <= key_x+r; x++)
            for (int y = key_y-r; y <= key_y+r; y++)
                for (int z = key_z-r; z <= key_z+r; z++)
                {
                    // Get bins
                    auto it = bins.find(std::make_tuple(x,y,z));
                    if (it != bins.end())
                    {
                        const std::vector<int>& bin = it->second;
                        for (int nidx : bin)
                        {
                            const Particle& p = particles[nidx]; 
                            if (nidx != idx && glm::distance2(p.p, particles[idx].p) < radius2)
                                neighbors.emplace_back(nidx);
                        }
                    }
                }

        return neighbors;
    }
private:
    std::map<triple, std::vector<int>> bins;
    float m_cell_width;
};

#endif // HASH_GRID_H
