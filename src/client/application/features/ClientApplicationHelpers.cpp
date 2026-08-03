/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ClientApplicationHelpers.hpp"

#include "client/input/InputManager.hpp"
#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/world/block/BlockState.hpp"

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
