#include "common/test/framework/function/BaseGameTestFunction.hpp"

#include <algorithm>

namespace mc::test {

bool BaseGameTestFunction::hasTag(std::string_view tag) const noexcept
{
    return std::find(m_tags.begin(), m_tags.end(), tag) != m_tags.end();
}

} // namespace mc::test
