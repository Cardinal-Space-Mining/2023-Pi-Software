#include "map_util.hpp"

#include <array>   //std::array
#include <cmath>   //std::sqrt
#include <cstring> //std::memcmp
#include <limits>  //std::numeric_limits
#include <utility> //std::pair

typedef struct point_t
{
    float x;
    float y;

    constexpr bool operator==(const point_t &other) const
    {
        return this->x == other.x && this->y == other.y;
    }
} point_t;

constexpr std::pair<point_t, point_t> INTERSECTION_FAILURE = {
    {std::numeric_limits<float>::max(), std::numeric_limits<float>::max()},
    {std::numeric_limits<float>::max(), std::numeric_limits<float>::max()}};

// Shoving all following items in a namespace tells the compiler that they are
// local to this compilation unit And allows for more agressive optimizations
// and thus more SPEEEEEEEEEEED
namespace
{

    float distance(point_t p1, point_t p2)
    {
        const float dx = p1.x - p2.x;
        const float dy = p1.y - p2.y;
        return std::sqrt((dx * dx) + (dy * dy));
    }

    constexpr float calc_pt_sign(point_t p1, float m, float b)
    {
        return p1.y - (m * p1.x) - b;
    }

    constexpr int count_negitives(const std::array<float, 4> &calcs)
    {
        int num_negitives = 0;
        for (size_t i = 0; i < calcs.size(); i++)
        {
            if (calcs[i] < 0)
                num_negitives += 1;
        }
        return num_negitives;
    }

    constexpr int count_positives(const std::array<float, 4> &calcs)
    {
        int num_positives = 0;
        for (size_t i = 0; i < calcs.size(); i++)
        {
            if (calcs[i] > 0)
                num_positives += 1;
        }
        return num_positives;
    }

    constexpr const point_t &get_point_with_min_sign(const point_t *sq,
                                                     const float *calcs,
                                                     size_t length)
    {
        float value = calcs[0];
        int index = 0;
        for (size_t i = 1; i < length; i++)
        {
            if (calcs[i] < value)
            {
                value = calcs[i];
                index = i;
            }
        }

        return sq[index];
    }

    constexpr const point_t &get_point_with_max_sign(const point_t *sq,
                                                     const float *calcs,
                                                     size_t length)
    {
        float value = calcs[0];
        int index = 0;
        for (size_t i = 1; i < length; i++)
        {
            if (calcs[i] > value)
            {
                value = calcs[i];
                index = i;
            }
        }

        return sq[index];
    }

    constexpr std::pair<point_t, point_t> calc_points_from_case_1_point(
        const point_t &pt, float m, float b)
    {
        return {{pt.x, (pt.x * m) + b}, {(pt.y - b) / m, pt.y}};
    }

    constexpr std::pair<point_t, point_t> get_intersection_pts(float sq_x, float sq_y,
                                                               float m, float b)
    {
        // Step 1
        const std::array<point_t, 4> sq = {{{sq_x - 0.5f, sq_y - 0.5f},
                                            {sq_x + 0.5f, sq_y - 0.5f},
                                            {sq_x - 0.5f, sq_y + 0.5f},
                                            {sq_x + 0.5f, sq_y + 0.5f}}};

        const std::array<float, 4> calcs = {
            calc_pt_sign(sq[0], m, b), calc_pt_sign(sq[1], m, b),
            calc_pt_sign(sq[2], m, b), calc_pt_sign(sq[3], m, b)};
        const int num_positives = count_positives(calcs);
        const int num_negitives = count_negitives(calcs);

        // Case 1
        if ((num_positives == 3) | (num_negitives == 3))
        {
            if (num_positives == 3)
            {
                const point_t &chosen_point =
                    get_point_with_min_sign(&sq[0], &calcs[0], sq.size());
                return calc_points_from_case_1_point(chosen_point, m, b);
            }
            else
            {
                const point_t &chosen_point =
                    get_point_with_max_sign(&sq[0], &calcs[0], sq.size());
                return calc_points_from_case_1_point(chosen_point, m, b);
            }
        }
        else if (num_positives == 2 || num_negitives == 2)
        {
            const point_t &chosen_point1 = sq[0];
            const point_t &chosen_point2 =
                (calcs[0] < 0)
                    ? get_point_with_min_sign(&sq[1], &calcs[1], sq.size() - 1)
                    : get_point_with_max_sign(&sq[1], &calcs[1], sq.size() - 1);

            if (std::abs(m) < 1)
            {
                const point_t inter_pt_1 = {chosen_point1.x,
                                            (m * chosen_point1.x) + b};
                const point_t inter_pt_2 = {chosen_point2.x,
                                            (m * chosen_point2.x) + b};
                return {inter_pt_1, inter_pt_2};
            }

            else
            {
                const point_t inter_pt_1 = {(chosen_point1.y - b) / m,
                                            chosen_point1.y};
                const point_t inter_pt_2 = {(chosen_point2.y - b) / m,
                                            chosen_point2.y};
                return {inter_pt_1, inter_pt_2};
            }
        }
        else
        {
            return INTERSECTION_FAILURE;
        }
    }
    /**
     * Shifts a line [m,b] by dy and dx.
     * The resulting slope is the same
    */
    constexpr float get_shifted_b(float dx, float dy, float m, float b){
        return (m * dx) + b + dy;
    }

} // namespace

float get_length_line_in_square(float sq_x, float sq_y, float m, float b)
{
    //We shift the line and square so that the square is centered at x=0,y=0
    //This simplifies later calculations and increases the accuracy of the result

    const auto intersection_pts = get_intersection_pts(sq_x, sq_y, m, b);
    if (intersection_pts.first == INTERSECTION_FAILURE.first)
    {
        return 0;
    }
    else
    {
        return distance(intersection_pts.first, intersection_pts.second);
    }
}