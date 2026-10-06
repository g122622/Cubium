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

#include "DebugStickItem.hpp"

#include "common/core/Types.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/Properties.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockUpdateFlags.hpp"
#include <string>
#include <utility>
#include <vector>

namespace mc {
namespace item::items {

namespace {

/// 物品 NBT 中保存"每个方块当前选中属性"的子标签键（对齐 vanilla DebugStickState 组件）
constexpr const char* DEBUG_STICK_STATE_KEY = "debug_stick_state";

/**
 * @brief 在属性值列表中按方向取相邻值索引
 * @param count 值总数
 * @param currentIndex 当前索引
 * @param backwards true=前一个，false=后一个
 */
size_t _relativeIndex(size_t count, size_t currentIndex, bool backwards)
{
    if (count == 0) {
        return 0;
    }
    if (backwards) {
        return (currentIndex + count - 1) % count;
    }
    return (currentIndex + 1) % count;
}

} // namespace

DebugStickItem::DebugStickItem(ItemProperties properties)
    : Item(std::move(properties))
{}

bool DebugStickItem::_handleInteraction(
    Player& player, ItemStack& stack, const BlockState& state, IWorld& world, const BlockPos& pos, bool isRightClick)
{
    // 对齐 vanilla DebugStickItem#handleInteraction：仅管理员可用
    if (!player.canUseGameMasterBlocks()) {
        return false;
    }

    const Block& block = state.getBlock();
    const auto& container = block.stateContainer();
    const auto& properties = container.properties();

    if (properties.empty()) {
        // 方块无可调试属性
        player.sendStatusMessage(
            std::string("debug Stick: ") + block.blockLocation().toString() + " has no properties", true);
        return false;
    }

    // 获取/初始化调试棒状态子标签：{ 方块id: 属性名 }
    // 注：ItemStack 仅提供 const 版 getChildTag，可修改访问须用 getOrCreateChildTag。
    nlohmann::json& debugState = stack.getOrCreateChildTag(DEBUG_STICK_STATE_KEY);
    if (!debugState.is_object()) {
        debugState = nlohmann::json::object();
    }
    const std::string blockId = block.blockLocation().toString();

    // 当前选中的属性名（可能为空，表示尚未选择）
    std::string currentPropName;
    if (debugState.contains(blockId) && debugState[blockId].is_string()) {
        currentPropName = debugState[blockId].get<std::string>();
    }

    // 取属性列表中"当前属性"的下一个（或上一个）属性。属性列表按名称有序（stateContainer 用有序 map）。
    std::vector<const IProperty*> orderedProps;
    orderedProps.reserve(properties.size());
    for (const auto& [name, prop] : properties) {
        orderedProps.push_back(prop);
    }

    size_t currentPropIndex = 0;
    bool found = false;
    for (size_t i = 0; i < orderedProps.size(); ++i) {
        if (orderedProps[i]->name() == currentPropName) {
            currentPropIndex = i;
            found = true;
            break;
        }
    }

    // 对齐 vanilla Player.isSecondaryUseActive()（潜行键反向循环）
    const bool backwards = player.isInputSneaking();

    if (!isRightClick) {
        // 左键：切换选中的属性
        if (!found) {
            currentPropIndex = 0;
        } else {
            currentPropIndex = _relativeIndex(orderedProps.size(), currentPropIndex, backwards);
        }
        const IProperty* selectedProp = orderedProps[currentPropIndex];
        debugState[blockId] = selectedProp->name();

        auto valueIndex = state.getValueIndex(*selectedProp);
        std::string valueName = valueIndex.has_value() ? selectedProp->valueToString(*valueIndex) : "";
        player.sendStatusMessage(
            std::string("debug Stick: selected property ") + selectedProp->name() + " = " + valueName, true);
        return true;
    }

    // 右键：切换选中属性的值并应用
    const IProperty* selectedProp = nullptr;
    if (found) {
        selectedProp = orderedProps[currentPropIndex];
    } else {
        selectedProp = orderedProps[0];
        debugState[blockId] = selectedProp->name();
    }

    auto currentValueIndex = state.getValueIndex(*selectedProp);
    if (!currentValueIndex.has_value()) {
        return false;
    }

    const size_t nextValueIndex = _relativeIndex(selectedProp->valueCount(), currentValueIndex.value(), backwards);
    const BlockState& newState = state.withValueIndex(*selectedProp, nextValueIndex);

    // 对齐 vanilla：flag=18（UPDATE_CLIENTS | UPDATE_KNOWN_SHAPE | UPDATE_SUPPRESS_DROPS | ...）。
    // 本项目使用 UPDATE_CLIENTS，其余副作用由 setBlockState 常规处理。
    world.setBlockState(pos, &newState, world::BlockUpdateFlags::UPDATE_CLIENTS);

    player.sendStatusMessage(
        std::string("debug Stick: ") + selectedProp->name() + " = " + selectedProp->valueToString(nextValueIndex),
        true);
    return true;
}

ActionResultType DebugStickItem::onItemUse(ItemUseContext& context)
{
    Player* player = context.getPlayer();
    if (player == nullptr) {
        return ActionResultType::Pass;
    }

    // 对齐 vanilla DebugStickItem#useOn：仅在服务端处理
    if (context.getWorld().isClientSide()) {
        return ActionResultType::Success;
    }

    const BlockPos& pos = context.blockPos();
    IWorld& world = context.getWorld();
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return ActionResultType::Success;
    }

    // 右键时物品堆为值拷贝，修改需回写权威手持槽
    ItemStack& held = player->getHeldItem(context.getHand());
    _handleInteraction(*player, held, *state, world, pos, true);
    return ActionResultType::Success;
}

bool DebugStickItem::canDestroyBlock(
    ItemStack& stack, const BlockState& state, IWorld& world, const BlockPos& pos, LivingEntity& breaker) const
{
    // 对齐 vanilla DebugStickItem#canDestroyBlock：服务端且为玩家时切换选中属性，返回 false。
    // 返回 false 表示不允许破坏该方块（调用方据此中止破坏流程），属性切换的副作用已在此完成。
    if (world.isClientSide()) {
        return false;
    }

    Player* player = dynamic_cast<Player*>(&breaker);
    if (player == nullptr) {
        return false;
    }

    _handleInteraction(*player, stack, state, world, pos, false);
    return false;
}

} // namespace item::items
} // namespace mc
