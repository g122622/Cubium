#include "UniformIntDistribution.hpp"
#include "Random.hpp"

namespace mc::math {

i32 UniformIntDistribution::generate() const
{
    Random rng;
    return rng.nextInt(m_min, m_max);
}

} // namespace mc::math
