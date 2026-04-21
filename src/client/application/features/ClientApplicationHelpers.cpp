#include "ClientApplicationHelpers.hpp"

#include <algorithm>

namespace mc::client::application::features {

void releaseMouseForScreen(InputManager& input, bool& mouseCaptured)
{
    if (mouseCaptured) {
        input.setMouseLocked(false);
        mouseCaptured = false;
    }
}

void captureMouseAfterScreens(InputManager& input, bool& mouseCaptured)
{
    if (!mouseCaptured) {
        input.setMouseLocked(true);
        mouseCaptured = true;
    }
}

[[nodiscard]] f32 calculateBlockBreakingDelta(const Player& player, const BlockState& state)
{
    const f32 hardness = state.hardness();
    if (hardness < 0.0f) {
        return 0.0f;
    }

    if (hardness == 0.0f) {
        return 1.0f;
    }

    const ItemStack heldItem = player.inventory().getSelectedStack();
    const f32 destroySpeed = std::max(heldItem.getDestroySpeed(state), 1.0f);
    const bool canHarvest = heldItem.isEmpty() ? true : heldItem.canHarvestBlock(state);
    const f32 divisor = canHarvest ? 30.0f : 100.0f;
    return destroySpeed / hardness / divisor;
}

} // namespace mc::client::application::features
