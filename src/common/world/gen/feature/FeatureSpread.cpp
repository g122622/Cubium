#include "FeatureSpread.hpp"

namespace mc {

i32 FeatureSpread::get(math::Random& random) const
{
    if (m_spread == 0) {
        return m_base;
    }
    return m_base + random.nextInt(0, m_spread);
}

} // namespace mc
