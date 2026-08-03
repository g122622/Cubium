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

#pragma once

#include "common/core/Types.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/block/BlockState.hpp"

#include "client/input/InputManager.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace mc {

class Player;

namespace client::application::features {

/**
 * @brief 容器内容应用到菜单
 */
template <typename Menu>
void applyContainerContents(Menu* menu, const std::vector<ItemStack>& items)
{
    if (menu == nullptr) {
        return;
    }

    const size_t slotCount = std::min(static_cast<size_t>(menu->getSlotCount()), items.size());
    for (size_t slotIndex = 0; slotIndex < slotCount; ++slotIndex) {
        Slot* slot = menu->getSlot(static_cast<i32>(slotIndex));
        if (slot != nullptr) {
            slot->set(items[slotIndex]);
        }
    }
}

/**
 * @brief 容器单个槽位内容应用到菜单
 */
template <typename Menu>
void applyContainerSlot(Menu* menu, i32 slotIndex, const ItemStack& item)
{
    if (menu == nullptr) {
        return;
    }

    Slot* slot = menu->getSlot(slotIndex);
    if (slot != nullptr) {
        slot->set(item);
    }
}

/**
 * @brief 释放鼠标捕获
 */
void releaseMouseForScreen(InputManager& input, bool& mouseCaptured);

/**
 * @brief 恢复鼠标捕获
 */
void captureMouseAfterScreens(InputManager& input, bool& mouseCaptured);

/**
 * @brief 计算方块破坏进度增量
 */
[[nodiscard]] f32 calculateBlockBreakingDelta(const Player& player, const BlockState& state);

} // namespace client::application::features
} // namespace mc
