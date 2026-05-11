#include "server/stats/Stat.hpp"

namespace mc {
namespace server {
namespace stats {

Stat::Stat(StatType type, ResourceLocation id) noexcept
    : m_type(type)
    , m_id(std::move(id))
    , m_value(0)
{
}

ResourceLocation Stat::getFullLocation() const {
    return buildStatLocation(m_type, m_id);
}

void Stat::setValue(ValueType value) noexcept {
    m_value = value;
}

void Stat::increment(ValueType delta) noexcept {
    // 防止溢出
    if (delta > 0 && m_value > std::numeric_limits<ValueType>::max() - delta) {
        m_value = std::numeric_limits<ValueType>::max();
    } else if (delta < 0 && m_value < std::numeric_limits<ValueType>::min() - delta) {
        m_value = std::numeric_limits<ValueType>::min();
    } else {
        m_value += delta;
    }
}

} // namespace stats
} // namespace server
} // namespace mc
