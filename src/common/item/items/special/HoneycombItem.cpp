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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN THE EVENT OF LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "HoneycombItem.hpp"

#include "common/entity/entities/player/Player.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/SignEntity.hpp"
#include <optional>
#include <unordered_map>
#include <utility>

namespace mc {
namespace item::items {

HoneycombItem::HoneycombItem(ItemProperties properties)
    : Item(std::move(properties))
{}

ActionResultType HoneycombItem::onItemUse(ItemUseContext& context)
{
    IWorld& world = context.world();
    const BlockPos& pos = context.blockPos();
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return ActionResultType::Pass;
    }

    // 检查是否对告示牌使用蜜脾（涂蜡）
    BlockEntity* blockEntity = world.getBlockEntity(pos);
    if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::Sign) {
        auto* signEntity = static_cast<blockentity::SignEntity*>(blockEntity);
        // 如果另一玩家正在编辑告示牌，阻止涂蜡交互
        // 对应 MC Java SignBlock.useItemOn() 中的 otherPlayerIsEditingSign() 检查
        Player* player = context.getPlayer();
        if (player != nullptr && signEntity->otherPlayerIsEditing(*player)) {
            return ActionResultType::Consume;
        }
        if (!signEntity->isWaxed()) {
            if (signEntity->setWaxed(true)) {
                // 播放涂蜡粒子与音效
                world.playEvent(world::WorldEvents::WAX_ON, pos, 0);

                // 消耗一个蜜脾（非创造模式）
                ItemStack& stack = context.getItemStackMut();
                if (context.getPlayer() == nullptr || !context.getPlayer()->abilities().creativeMode) {
                    stack.shrink(1);
                }

                return ActionResultType::Success;
            }
        }
        // 告示牌已涂蜡，不处理
        return ActionResultType::Pass;
    }

    // 检查方块是否可涂蜡（铜块系列）
    auto waxedState = getWaxed(*state);
    if (!waxedState.has_value()) {
        return ActionResultType::Pass;
    }

    // 涂蜡成功：替换方块状态
    world.setBlockState(pos, &waxedState.value(), 11);

    // 播放涂蜡粒子与音效
    world.playEvent(world::WorldEvents::WAX_ON, pos, 0);

    // 消耗一个蜜脾（非创造模式）
    ItemStack& stack = context.getItemStackMut();
    if (context.getPlayer() == nullptr || !context.getPlayer()->abilities().creativeMode) {
        stack.shrink(1);
    }

    return ActionResultType::Success;
}

std::optional<BlockState> HoneycombItem::getWaxed(const BlockState& state)
{
    const Block* block = &state.owner();
    auto& map = getWaxablesMap();
    auto it = map.find(block);
    if (it == map.end()) {
        return std::nullopt;
    }

    // 复制原方块状态中兼容的属性到涂蜡方块
    const Block* waxedBlock = it->second;
    return waxedBlock->defaultState().withPropertiesOf(state);
}

std::optional<BlockState> HoneycombItem::getWaxedOff(const BlockState& state)
{
    const Block* block = &state.owner();
    auto& map = getWaxOffMap();
    auto it = map.find(block);
    if (it == map.end()) {
        return std::nullopt;
    }

    // 复制原方块状态中兼容的属性到未涂蜡方块
    const Block* unwaxedBlock = it->second;
    return unwaxedBlock->defaultState().withPropertiesOf(state);
}

std::unordered_map<const Block*, const Block*>& HoneycombItem::getWaxablesMap()
{
    static std::unordered_map<const Block*, const Block*> map = _buildWaxablesMap();
    return map;
}

std::unordered_map<const Block*, const Block*>& HoneycombItem::getWaxOffMap()
{
    static std::unordered_map<const Block*, const Block*> map = _buildWaxOffMap();
    return map;
}

std::unordered_map<const Block*, const Block*> HoneycombItem::_buildWaxablesMap()
{
    std::unordered_map<const Block*, const Block*> m;

    // 铜块系列
    if (VanillaBlocks::COPPER_BLOCK && VanillaBlocks::WAXED_COPPER_BLOCK)
        m[VanillaBlocks::COPPER_BLOCK] = VanillaBlocks::WAXED_COPPER_BLOCK;
    if (VanillaBlocks::EXPOSED_COPPER && VanillaBlocks::WAXED_EXPOSED_COPPER)
        m[VanillaBlocks::EXPOSED_COPPER] = VanillaBlocks::WAXED_EXPOSED_COPPER;
    if (VanillaBlocks::WEATHERED_COPPER && VanillaBlocks::WAXED_WEATHERED_COPPER)
        m[VanillaBlocks::WEATHERED_COPPER] = VanillaBlocks::WAXED_WEATHERED_COPPER;
    if (VanillaBlocks::OXIDIZED_COPPER && VanillaBlocks::WAXED_OXIDIZED_COPPER)
        m[VanillaBlocks::OXIDIZED_COPPER] = VanillaBlocks::WAXED_OXIDIZED_COPPER;

    // 切制铜系列
    if (VanillaBlocks::CUT_COPPER && VanillaBlocks::WAXED_CUT_COPPER)
        m[VanillaBlocks::CUT_COPPER] = VanillaBlocks::WAXED_CUT_COPPER;
    if (VanillaBlocks::EXPOSED_CUT_COPPER && VanillaBlocks::WAXED_EXPOSED_CUT_COPPER)
        m[VanillaBlocks::EXPOSED_CUT_COPPER] = VanillaBlocks::WAXED_EXPOSED_CUT_COPPER;
    if (VanillaBlocks::WEATHERED_CUT_COPPER && VanillaBlocks::WAXED_WEATHERED_CUT_COPPER)
        m[VanillaBlocks::WEATHERED_CUT_COPPER] = VanillaBlocks::WAXED_WEATHERED_CUT_COPPER;
    if (VanillaBlocks::OXIDIZED_CUT_COPPER && VanillaBlocks::WAXED_OXIDIZED_CUT_COPPER)
        m[VanillaBlocks::OXIDIZED_CUT_COPPER] = VanillaBlocks::WAXED_OXIDIZED_CUT_COPPER;

    // 切制铜楼梯
    if (VanillaBlocks::CUT_COPPER_STAIRS && VanillaBlocks::WAXED_CUT_COPPER_STAIRS)
        m[VanillaBlocks::CUT_COPPER_STAIRS] = VanillaBlocks::WAXED_CUT_COPPER_STAIRS;
    if (VanillaBlocks::EXPOSED_CUT_COPPER_STAIRS && VanillaBlocks::WAXED_EXPOSED_CUT_COPPER_STAIRS)
        m[VanillaBlocks::EXPOSED_CUT_COPPER_STAIRS] = VanillaBlocks::WAXED_EXPOSED_CUT_COPPER_STAIRS;
    if (VanillaBlocks::WEATHERED_CUT_COPPER_STAIRS && VanillaBlocks::WAXED_WEATHERED_CUT_COPPER_STAIRS)
        m[VanillaBlocks::WEATHERED_CUT_COPPER_STAIRS] = VanillaBlocks::WAXED_WEATHERED_CUT_COPPER_STAIRS;
    if (VanillaBlocks::OXIDIZED_CUT_COPPER_STAIRS && VanillaBlocks::WAXED_OXIDIZED_CUT_COPPER_STAIRS)
        m[VanillaBlocks::OXIDIZED_CUT_COPPER_STAIRS] = VanillaBlocks::WAXED_OXIDIZED_CUT_COPPER_STAIRS;

    // 切制铜台阶
    if (VanillaBlocks::CUT_COPPER_SLAB && VanillaBlocks::WAXED_CUT_COPPER_SLAB)
        m[VanillaBlocks::CUT_COPPER_SLAB] = VanillaBlocks::WAXED_CUT_COPPER_SLAB;
    if (VanillaBlocks::EXPOSED_CUT_COPPER_SLAB && VanillaBlocks::WAXED_EXPOSED_CUT_COPPER_SLAB)
        m[VanillaBlocks::EXPOSED_CUT_COPPER_SLAB] = VanillaBlocks::WAXED_EXPOSED_CUT_COPPER_SLAB;
    if (VanillaBlocks::WEATHERED_CUT_COPPER_SLAB && VanillaBlocks::WAXED_WEATHERED_CUT_COPPER_SLAB)
        m[VanillaBlocks::WEATHERED_CUT_COPPER_SLAB] = VanillaBlocks::WAXED_WEATHERED_CUT_COPPER_SLAB;
    if (VanillaBlocks::OXIDIZED_CUT_COPPER_SLAB && VanillaBlocks::WAXED_OXIDIZED_CUT_COPPER_SLAB)
        m[VanillaBlocks::OXIDIZED_CUT_COPPER_SLAB] = VanillaBlocks::WAXED_OXIDIZED_CUT_COPPER_SLAB;

    // 铜门
    if (VanillaBlocks::COPPER_DOOR && VanillaBlocks::WAXED_COPPER_DOOR)
        m[VanillaBlocks::COPPER_DOOR] = VanillaBlocks::WAXED_COPPER_DOOR;
    if (VanillaBlocks::EXPOSED_COPPER_DOOR && VanillaBlocks::WAXED_EXPOSED_COPPER_DOOR)
        m[VanillaBlocks::EXPOSED_COPPER_DOOR] = VanillaBlocks::WAXED_EXPOSED_COPPER_DOOR;
    if (VanillaBlocks::WEATHERED_COPPER_DOOR && VanillaBlocks::WAXED_WEATHERED_COPPER_DOOR)
        m[VanillaBlocks::WEATHERED_COPPER_DOOR] = VanillaBlocks::WAXED_WEATHERED_COPPER_DOOR;
    if (VanillaBlocks::OXIDIZED_COPPER_DOOR && VanillaBlocks::WAXED_OXIDIZED_COPPER_DOOR)
        m[VanillaBlocks::OXIDIZED_COPPER_DOOR] = VanillaBlocks::WAXED_OXIDIZED_COPPER_DOOR;

    // 铜活板门
    if (VanillaBlocks::COPPER_TRAPDOOR && VanillaBlocks::WAXED_COPPER_TRAPDOOR)
        m[VanillaBlocks::COPPER_TRAPDOOR] = VanillaBlocks::WAXED_COPPER_TRAPDOOR;
    if (VanillaBlocks::EXPOSED_COPPER_TRAPDOOR && VanillaBlocks::WAXED_EXPOSED_COPPER_TRAPDOOR)
        m[VanillaBlocks::EXPOSED_COPPER_TRAPDOOR] = VanillaBlocks::WAXED_EXPOSED_COPPER_TRAPDOOR;
    if (VanillaBlocks::WEATHERED_COPPER_TRAPDOOR && VanillaBlocks::WAXED_WEATHERED_COPPER_TRAPDOOR)
        m[VanillaBlocks::WEATHERED_COPPER_TRAPDOOR] = VanillaBlocks::WAXED_WEATHERED_COPPER_TRAPDOOR;
    if (VanillaBlocks::OXIDIZED_COPPER_TRAPDOOR && VanillaBlocks::WAXED_OXIDIZED_COPPER_TRAPDOOR)
        m[VanillaBlocks::OXIDIZED_COPPER_TRAPDOOR] = VanillaBlocks::WAXED_OXIDIZED_COPPER_TRAPDOOR;

    // 铜格栅
    if (VanillaBlocks::COPPER_GRATE && VanillaBlocks::WAXED_COPPER_GRATE)
        m[VanillaBlocks::COPPER_GRATE] = VanillaBlocks::WAXED_COPPER_GRATE;
    if (VanillaBlocks::EXPOSED_COPPER_GRATE && VanillaBlocks::WAXED_EXPOSED_COPPER_GRATE)
        m[VanillaBlocks::EXPOSED_COPPER_GRATE] = VanillaBlocks::WAXED_EXPOSED_COPPER_GRATE;
    if (VanillaBlocks::WEATHERED_COPPER_GRATE && VanillaBlocks::WAXED_WEATHERED_COPPER_GRATE)
        m[VanillaBlocks::WEATHERED_COPPER_GRATE] = VanillaBlocks::WAXED_WEATHERED_COPPER_GRATE;
    if (VanillaBlocks::OXIDIZED_COPPER_GRATE && VanillaBlocks::WAXED_OXIDIZED_COPPER_GRATE)
        m[VanillaBlocks::OXIDIZED_COPPER_GRATE] = VanillaBlocks::WAXED_OXIDIZED_COPPER_GRATE;

    // 铜灯
    if (VanillaBlocks::COPPER_BULB && VanillaBlocks::WAXED_COPPER_BULB)
        m[VanillaBlocks::COPPER_BULB] = VanillaBlocks::WAXED_COPPER_BULB;
    if (VanillaBlocks::EXPOSED_COPPER_BULB && VanillaBlocks::WAXED_EXPOSED_COPPER_BULB)
        m[VanillaBlocks::EXPOSED_COPPER_BULB] = VanillaBlocks::WAXED_EXPOSED_COPPER_BULB;
    if (VanillaBlocks::WEATHERED_COPPER_BULB && VanillaBlocks::WAXED_WEATHERED_COPPER_BULB)
        m[VanillaBlocks::WEATHERED_COPPER_BULB] = VanillaBlocks::WAXED_WEATHERED_COPPER_BULB;
    if (VanillaBlocks::OXIDIZED_COPPER_BULB && VanillaBlocks::WAXED_OXIDIZED_COPPER_BULB)
        m[VanillaBlocks::OXIDIZED_COPPER_BULB] = VanillaBlocks::WAXED_OXIDIZED_COPPER_BULB;

    // 凿制铜
    if (VanillaBlocks::CHISELED_COPPER && VanillaBlocks::WAXED_CHISELED_COPPER)
        m[VanillaBlocks::CHISELED_COPPER] = VanillaBlocks::WAXED_CHISELED_COPPER;
    if (VanillaBlocks::EXPOSED_CHISELED_COPPER && VanillaBlocks::WAXED_EXPOSED_CHISELED_COPPER)
        m[VanillaBlocks::EXPOSED_CHISELED_COPPER] = VanillaBlocks::WAXED_EXPOSED_CHISELED_COPPER;
    if (VanillaBlocks::WEATHERED_CHISELED_COPPER && VanillaBlocks::WAXED_WEATHERED_CHISELED_COPPER)
        m[VanillaBlocks::WEATHERED_CHISELED_COPPER] = VanillaBlocks::WAXED_WEATHERED_CHISELED_COPPER;
    if (VanillaBlocks::OXIDIZED_CHISELED_COPPER && VanillaBlocks::WAXED_OXIDIZED_CHISELED_COPPER)
        m[VanillaBlocks::OXIDIZED_CHISELED_COPPER] = VanillaBlocks::WAXED_OXIDIZED_CHISELED_COPPER;

    // 铜栏杆
    if (VanillaBlocks::COPPER_BARS && VanillaBlocks::WAXED_COPPER_BARS)
        m[VanillaBlocks::COPPER_BARS] = VanillaBlocks::WAXED_COPPER_BARS;
    if (VanillaBlocks::EXPOSED_COPPER_BARS && VanillaBlocks::WAXED_EXPOSED_COPPER_BARS)
        m[VanillaBlocks::EXPOSED_COPPER_BARS] = VanillaBlocks::WAXED_EXPOSED_COPPER_BARS;
    if (VanillaBlocks::WEATHERED_COPPER_BARS && VanillaBlocks::WAXED_WEATHERED_COPPER_BARS)
        m[VanillaBlocks::WEATHERED_COPPER_BARS] = VanillaBlocks::WAXED_WEATHERED_COPPER_BARS;
    if (VanillaBlocks::OXIDIZED_COPPER_BARS && VanillaBlocks::WAXED_OXIDIZED_COPPER_BARS)
        m[VanillaBlocks::OXIDIZED_COPPER_BARS] = VanillaBlocks::WAXED_OXIDIZED_COPPER_BARS;

    // 铜链
    if (VanillaBlocks::COPPER_CHAIN && VanillaBlocks::WAXED_COPPER_CHAIN)
        m[VanillaBlocks::COPPER_CHAIN] = VanillaBlocks::WAXED_COPPER_CHAIN;
    if (VanillaBlocks::EXPOSED_COPPER_CHAIN && VanillaBlocks::WAXED_EXPOSED_COPPER_CHAIN)
        m[VanillaBlocks::EXPOSED_COPPER_CHAIN] = VanillaBlocks::WAXED_EXPOSED_COPPER_CHAIN;
    if (VanillaBlocks::WEATHERED_COPPER_CHAIN && VanillaBlocks::WAXED_WEATHERED_COPPER_CHAIN)
        m[VanillaBlocks::WEATHERED_COPPER_CHAIN] = VanillaBlocks::WAXED_WEATHERED_COPPER_CHAIN;
    if (VanillaBlocks::OXIDIZED_COPPER_CHAIN && VanillaBlocks::WAXED_OXIDIZED_COPPER_CHAIN)
        m[VanillaBlocks::OXIDIZED_COPPER_CHAIN] = VanillaBlocks::WAXED_OXIDIZED_COPPER_CHAIN;

    // 铜灯笼
    if (VanillaBlocks::COPPER_LANTERN && VanillaBlocks::WAXED_COPPER_LANTERN)
        m[VanillaBlocks::COPPER_LANTERN] = VanillaBlocks::WAXED_COPPER_LANTERN;
    if (VanillaBlocks::EXPOSED_COPPER_LANTERN && VanillaBlocks::WAXED_EXPOSED_COPPER_LANTERN)
        m[VanillaBlocks::EXPOSED_COPPER_LANTERN] = VanillaBlocks::WAXED_EXPOSED_COPPER_LANTERN;
    if (VanillaBlocks::WEATHERED_COPPER_LANTERN && VanillaBlocks::WAXED_WEATHERED_COPPER_LANTERN)
        m[VanillaBlocks::WEATHERED_COPPER_LANTERN] = VanillaBlocks::WAXED_WEATHERED_COPPER_LANTERN;
    if (VanillaBlocks::OXIDIZED_COPPER_LANTERN && VanillaBlocks::WAXED_OXIDIZED_COPPER_LANTERN)
        m[VanillaBlocks::OXIDIZED_COPPER_LANTERN] = VanillaBlocks::WAXED_OXIDIZED_COPPER_LANTERN;

    // 避雷针（MC 1.21+ 新增氧化变种）
    if (VanillaBlocks::LIGHTNING_ROD && VanillaBlocks::WAXED_LIGHTNING_ROD)
        m[VanillaBlocks::LIGHTNING_ROD] = VanillaBlocks::WAXED_LIGHTNING_ROD;
    if (VanillaBlocks::EXPOSED_LIGHTNING_ROD && VanillaBlocks::WAXED_EXPOSED_LIGHTNING_ROD)
        m[VanillaBlocks::EXPOSED_LIGHTNING_ROD] = VanillaBlocks::WAXED_EXPOSED_LIGHTNING_ROD;
    if (VanillaBlocks::WEATHERED_LIGHTNING_ROD && VanillaBlocks::WAXED_WEATHERED_LIGHTNING_ROD)
        m[VanillaBlocks::WEATHERED_LIGHTNING_ROD] = VanillaBlocks::WAXED_WEATHERED_LIGHTNING_ROD;
    if (VanillaBlocks::OXIDIZED_LIGHTNING_ROD && VanillaBlocks::WAXED_OXIDIZED_LIGHTNING_ROD)
        m[VanillaBlocks::OXIDIZED_LIGHTNING_ROD] = VanillaBlocks::WAXED_OXIDIZED_LIGHTNING_ROD;

    // 铜傀儡雕像（MC 1.21.11）
    if (VanillaBlocks::COPPER_GOLEM_STATUE && VanillaBlocks::WAXED_COPPER_GOLEM_STATUE)
        m[VanillaBlocks::COPPER_GOLEM_STATUE] = VanillaBlocks::WAXED_COPPER_GOLEM_STATUE;
    if (VanillaBlocks::EXPOSED_COPPER_GOLEM_STATUE && VanillaBlocks::WAXED_EXPOSED_COPPER_GOLEM_STATUE)
        m[VanillaBlocks::EXPOSED_COPPER_GOLEM_STATUE] = VanillaBlocks::WAXED_EXPOSED_COPPER_GOLEM_STATUE;
    if (VanillaBlocks::WEATHERED_COPPER_GOLEM_STATUE && VanillaBlocks::WAXED_WEATHERED_COPPER_GOLEM_STATUE)
        m[VanillaBlocks::WEATHERED_COPPER_GOLEM_STATUE] = VanillaBlocks::WAXED_WEATHERED_COPPER_GOLEM_STATUE;
    if (VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE && VanillaBlocks::WAXED_OXIDIZED_COPPER_GOLEM_STATUE)
        m[VanillaBlocks::OXIDIZED_COPPER_GOLEM_STATUE] = VanillaBlocks::WAXED_OXIDIZED_COPPER_GOLEM_STATUE;

    // 铜箱子（MC 1.21.11）
    if (VanillaBlocks::COPPER_CHEST && VanillaBlocks::WAXED_COPPER_CHEST)
        m[VanillaBlocks::COPPER_CHEST] = VanillaBlocks::WAXED_COPPER_CHEST;
    if (VanillaBlocks::EXPOSED_COPPER_CHEST && VanillaBlocks::WAXED_EXPOSED_COPPER_CHEST)
        m[VanillaBlocks::EXPOSED_COPPER_CHEST] = VanillaBlocks::WAXED_EXPOSED_COPPER_CHEST;
    if (VanillaBlocks::WEATHERED_COPPER_CHEST && VanillaBlocks::WAXED_WEATHERED_COPPER_CHEST)
        m[VanillaBlocks::WEATHERED_COPPER_CHEST] = VanillaBlocks::WAXED_WEATHERED_COPPER_CHEST;
    if (VanillaBlocks::OXIDIZED_COPPER_CHEST && VanillaBlocks::WAXED_OXIDIZED_COPPER_CHEST)
        m[VanillaBlocks::OXIDIZED_COPPER_CHEST] = VanillaBlocks::WAXED_OXIDIZED_COPPER_CHEST;

    return m;
}

std::unordered_map<const Block*, const Block*> HoneycombItem::_buildWaxOffMap()
{
    // 除蜡映射 = 涂蜡映射的反向
    const auto& waxables = getWaxablesMap();
    std::unordered_map<const Block*, const Block*> m;
    for (const auto& [unwaxed, waxed] : waxables) {
        m[waxed] = unwaxed;
    }
    return m;
}

} // namespace item::items
} // namespace mc
