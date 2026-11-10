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

#include "RedstoneHelper.hpp"

#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/core/Block.hpp"
#include <algorithm>
#include <vector>

namespace mc {
namespace world {
namespace redstone {

bool RedstoneHelper::isNormalCube(const BlockState& state)
{
    // 实体方块：固体、不透明、非空气
    return state.isSolid() && state.isOpaque() && !state.isAir();
}

bool RedstoneHelper::canConnectRedstone(const BlockState& state)
{
    // 检查方块是否可以输出红石信号
    return state.getBlock().canProvidePower(state);
}

namespace {

/// 原版显式注册成 Blocks::never 的方块（形状完整或半完整，但明确不是导体）。
/// 清单来自 block_redstone_conductor 差分对 29671 个状态的实测。
bool _isExplicitNonConductor(const ResourceLocation& id)
{
    static const std::unordered_set<std::string> kNever = {
        // 红石元件自身：若当导体会把周围的粉重复充能
        "minecraft:redstone_block",
        "minecraft:piston",
        "minecraft:sticky_piston",
        "minecraft:observer",
        "minecraft:tnt",
        // 铜家族：格栅是镂空、灯是半透明
        "minecraft:copper_grate",
        "minecraft:exposed_copper_grate",
        "minecraft:weathered_copper_grate",
        "minecraft:oxidized_copper_grate",
        "minecraft:waxed_copper_grate",
        "minecraft:waxed_exposed_copper_grate",
        "minecraft:waxed_weathered_copper_grate",
        "minecraft:waxed_oxidized_copper_grate",
        "minecraft:copper_bulb",
        "minecraft:exposed_copper_bulb",
        "minecraft:weathered_copper_bulb",
        "minecraft:oxidized_copper_bulb",
        "minecraft:waxed_copper_bulb",
        "minecraft:waxed_exposed_copper_bulb",
        "minecraft:waxed_weathered_copper_bulb",
        "minecraft:waxed_oxidized_copper_bulb",
    };
    return kNever.count(id.toString()) != 0;
}

/// 原版默认为导体、但碰撞形状不完全覆盖整格的方块。
/// 判据只看形状时这些会被误判为非导体，导致下方红石粉无法被充能。
bool _isExplicitConductor(const ResourceLocation& id)
{
    static const std::unordered_set<std::string> kAlways = {
        "minecraft:amethyst_block",
        "minecraft:budding_amethyst",
        "minecraft:barrier",
        "minecraft:blue_ice",
        "minecraft:packed_ice",
        "minecraft:dried_kelp_block",
        "minecraft:loom",
        "minecraft:moss_block",
        "minecraft:pale_moss_block",
        "minecraft:mud",
        "minecraft:respawn_anchor",
        "minecraft:sculk",
        "minecraft:sculk_catalyst",
        "minecraft:soul_sand",
    };
    return kAlways.count(id.toString()) != 0;
}

} // namespace

bool RedstoneHelper::isRedstoneConductor(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 对齐原版语义：BlockBehaviour 的默认判据是「碰撞形状完整覆盖整格」
    // （isCollisionShapeFullBlock），随后由各方块注册时的 BlockProperties#isRedstoneConductor
    // 逐个覆盖；原版 Blocks.java 里共 22 处显式覆盖，绝大多数是 Blocks::never。
    //
    // 两个判据缺一不可，任意单独使用都会出错：
    // - 只看「isSolid && isOpaque」：单层台阶、楼梯碰撞形状不完整但材质不透明，会被误判为
    //   导体，于是「火把在台阶正下方」时凭空向四周输出满功率（原版此处为 0）。
    // - 只看「碰撞形状完整」：玻璃、红石块碰撞形状完整，但原版显式注册成 never，不是导体。
    //
    // 形状判据能覆盖绝大多数方块，但有两批必须靠显式清单修正（原版 Blocks.java 的
    // BlockProperties#isRedstoneConductor 覆盖）。block_redstone_conductor 差分实测出的
    // 全部偏差集合，只此 35 种，其余 1228 种形状判据已经正确。
    //
    // 一、原版显式 never 却形状完整的方块（判为导体是错的）
    //    红石块最典型：它不透明、形状完整，但若当导体会把自己周围的粉重复充能。
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return isRedstoneConductorState(state);
}

bool RedstoneHelper::isRedstoneConductorState(const BlockState& state)
{
    const ResourceLocation& id = state.getBlock().blockLocation();
    if (_isExplicitNonConductor(id)) {
        return false;
    }
    if (_isExplicitConductor(id)) {
        return true;
    }
    return state.getCollisionShape().coversFullBlock() && state.isOpaque();
}

i32 RedstoneHelper::getEntitySignal(IWorld& world, const BlockPos& pos)
{
    // 在指定方块位置构建 1x1x1 检测区域
    AxisAlignedBB searchBox = AxisAlignedBB::fromBlock(pos.x, pos.y, pos.z);
    return getEntitySignal(world, searchBox);
}

i32 RedstoneHelper::getEntitySignal(IWorld& world, const AxisAlignedBB& searchBox)
{
    // 获取区域内的所有实体
    std::vector<Entity*> entities = world.getEntitiesInAABB(searchBox, nullptr);

    // 遍历实体，查找最大的比较器信号值
    i32 maxSignal = 0;
    for (Entity* entity : entities) {
        if (!entity || entity->isRemoved()) {
            continue;
        }
        i32 signal = entity->getComparatorOutput();
        if (signal > maxSignal) {
            maxSignal = signal;
        }
    }

    return maxSignal;
}

i32 RedstoneHelper::calcRedstoneFromInventory(const IInventory& inventory)
{
    const i32 containerSize = inventory.getContainerSize();
    if (containerSize <= 0) {
        return 0;
    }

    f32 fillRatio = 0.0f;

    for (i32 i = 0; i < containerSize; ++i) {
        const ItemStack& stack = inventory.getItem(i);
        if (!stack.isEmpty()) {
            fillRatio += static_cast<f32>(stack.getCount()) / static_cast<f32>(stack.getMaxStackSize());
        }
    }

    fillRatio /= static_cast<f32>(containerSize);

    // lerpDiscrete(fillRatio, 0, 14) + (fillRatio > 0 ? 1 : 0)
    // 即 floor(fillRatio * 14) + (有物品 ? 1 : 0)
    i32 signal = static_cast<i32>(math::floorTo<i32>(fillRatio * 14.0f));
    if (fillRatio > 0.0f) {
        signal += 1;
    }

    return std::min(signal, MAX_POWER);
}

} // namespace redstone
} // namespace world
} // namespace mc
