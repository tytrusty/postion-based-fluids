#ifndef HASH_GRID_H
#define HASH_GRID_H

#include <Eigen/Core>
#include <tuple>
#include <unordered_map>
#include <vector>


namespace HashGrid
{
    typedef std::tuple<int,int,int> triple;


    struct particle
    {
        Eigen::Vector3d m_pos;
        int m_idx;
        float radius;
    };

    class hash_grid
    {
    public:
        triple hash_position(const particle* p)
        {

           int i = p->m_pos(0) / m_cell_width;
           int j = p->m_pos(1) / m_cell_width;
           int k = p->m_pos(2) / m_cell_width;

           return std::make_tuple(i,j,k);
        }
    private:
        // std::unordered_map<triple, std::vector<particle*>> bins;
        float m_cell_width;

    };

} // namespace HashGrid

#endif // HASH_GRID_H
