#pragma once

#include "common/core/Types.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/inventory/ContainerTypes.hpp"
#include "common/entity/inventory/Slot.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/physics/PhysicsEngine.hpp"
#include "common/resource/ResourcePackList.hpp"
#include "common/screen/IScreen.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/chunk/ChunkData.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/util/Direction.hpp"

#include "client/input/InputManager.hpp"

#include <algorithm>
#include <vector>

namespace mc::client::application::features {

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
 * @brief 判断屏幕是否匹配指定容器
 */
template <typename ScreenT>
bool isMatchingContainerScreen(IScreen* screen, ContainerId containerId)
{
    auto* typedScreen = dynamic_cast<ScreenT*>(screen);
    if (typedScreen == nullptr || typedScreen->getMenu() == nullptr) {
        return false;
    }

    return typedScreen->getMenu()->getId() == containerId;
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

} // namespace mc::client::application::features
