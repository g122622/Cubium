#include "Food.hpp"

namespace mc {
namespace item::food {

Food::Food(i32 hunger, f32 saturation)
    : m_hunger(std::max(0, hunger))
    , m_saturation(std::max(0.0f, saturation)) {
}

} // namespace item::food
} // namespace mc
