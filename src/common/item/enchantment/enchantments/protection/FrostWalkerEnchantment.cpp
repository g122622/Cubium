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

#include "FrostWalkerEnchantment.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/item/enchantment/Enchantment.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"
#include <cmath>

namespace mc {
namespace item {
namespace enchant {

bool FrostWalkerEnchantment::isCompatibleWith(const Enchantment& other) const
{
    // 与深海探索者互斥
    if (other.id() == "minecraft:depth_strider") {
        return false;
    }
    return Enchantment::isCompatibleWith(other);
}

bool FrostWalkerEnchantment::onLocationChanged(
    LivingEntity& entity, const ItemStack& stack, i32 slot, i32 level, bool isActive) const
{
    (void)stack;
    (void)slot;
    (void)isActive;

    // 冰霜行者仅在以下条件下激活：
    // 1. 实体在地面上
    // 2. 实体没有骑乘
    // 3. 实体所在世界可用
    if (!entity.onGround()) {
        return false;
    }
    if (entity.isRiding()) {
        return false;
    }

    IWorld* world = entity.world();
    if (world == nullptr) {
        return false;
    }

    // 冰霜行者在每个方块位置变化时放置霜冰
    // 半径 = level + 1（I: 2, II: 3）
    i32 radius = getFrostRadius(level);

    BlockPos entityPos(static_cast<i32>(std::floor(entity.position().x)),
        static_cast<i32>(std::floor(entity.position().y)),
        static_cast<i32>(std::floor(entity.position().z)));

    // 在实体脚下 Y-1 层的圆形区域内放置霜冰
    placeFrostedIce(*world, entityPos, radius);

    return true; // 冰霜行者总是处于"活跃"状态（在地面时）
}

void FrostWalkerEnchantment::onLocationEffectDeactivated(
    LivingEntity& entity, const ItemStack& stack, i32 slot, i32 level) const
{
    (void)entity;
    (void)stack;
    (void)slot;
    (void)level;

    // 冰霜行者不需要停用清理：霜冰由 FrostedIceBlock 自行融化
    // 提供空实现以保证设计一致性（所有位置依赖附魔都应覆写此方法）
}

void FrostWalkerEnchantment::placeFrostedIce(IWorld& world, const BlockPos& center, i32 radius) const
{
    // 获取霜冰的默认方块状态
    if (VanillaBlocks::FROSTED_ICE == nullptr) {
        return;
    }
    const BlockState& frostedIceState = VanillaBlocks::FROSTED_ICE->defaultState();

    i32 y = center.y - 1; // 冻结实体脚下方块（ReplaceDisk 使用 Vec3i(0, -1, 0) 偏移）

    // 遍历以实体为中心的圆形区域
    for (i32 dx = -radius; dx <= radius; ++dx) {
        for (i32 dz = -radius; dz <= radius; ++dz) {
            // 圆形范围检测
            if (dx * dx + dz * dz > radius * radius) {
                continue;
            }

            BlockPos pos(center.x + dx, y, center.z + dz);

            // 检查上方是否有空气（不能在上方有实体方块时放置霜冰）
            const BlockState* aboveState = world.getBlockState(pos.x, pos.y + 1, pos.z);
            if (aboveState != nullptr && !aboveState->isAir()) {
                continue;
            }

            // 冰霜行者只冻结水源方块（不冻结流动水）
            // 使用 isWaterAt + isSource 双重检查，与 GlassBottleItem 一致
            const fluid::FluidState* fluidState = world.getFluidState(pos);
            if (fluidState == nullptr || fluidState->isEmpty() || !fluidState->isSource()) {
                continue;
            }
            if (!world.isWaterAt(pos)) {
                continue;
            }

            // 放置霜冰（仅在服务端）
            if (!world.isClientSide()) {
                world.setBlockState(pos, &frostedIceState);
            }
        }
    }
}

} // namespace enchant
} // namespace item
} // namespace mc
