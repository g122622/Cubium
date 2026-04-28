#include "Food.hpp"

namespace mc {
namespace item::food {

Food::Food(i32 hunger, f32 saturationModifier)
    : m_hunger(std::max(0, hunger))
    , m_saturationModifier(std::max(0.0f, saturationModifier)) {
}

} // namespace item::food
} // namespace mc
