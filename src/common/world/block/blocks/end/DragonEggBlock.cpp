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

#include "DragonEggBlock.hpp"
#include "../../../../core/BlockRaycastResult.hpp"
#include "../../../../entity/entities/player/Player.hpp"
#include "../../../../item/context/BlockItemUseContext.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorld.hpp"
#include "../../../block/BlockRegistry.hpp"
#include "common/core/Types.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/blocks/FallingBlock.hpp"

namespace mc {
namespace blocks {

DragonEggBlock::DragonEggBlock(const BlockProperties& properties)
    : FallingBlock(properties)
{
    // 碰撞箱: (1/16, 0, 1/16) 到 (15/16, 1, 15/16)
    m_shape = CollisionShape::box(0.0625f, 0.0f, 0.0625f, 0.9375f, 1.0f, 0.9375f);
}

BlockState DragonEggBlock::getStateForPlacement(BlockItemUseContext& context)
{
    MC_UNUSED(context);
    return defaultState();
}

BlockActionResult DragonEggBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(player);
    MC_UNUSED(hand);
    MC_UNUSED(hit);

    // 右键点击触发传送
    _teleport(world, pos, state);

    // 返回 Success 表示成功处理，阻止后续操作
    return ActionResultType::Success;
}

void DragonEggBlock::attack(const BlockState& state, IWorld& world, const BlockPos& pos, Player& player)
{
    MC_UNUSED(player);

    // 左键点击也触发传送
    _teleport(world, pos, state);
}

bool DragonEggBlock::_teleport(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 随机寻找新位置，最多尝试 1000 次
    math::Random& random = world.getRandom();

    for (i32 attempt = 0; attempt < MAX_TELEPORT_ATTEMPTS; ++attempt) {
        // 计算随机偏移，X/Z 范围: -15 ~ +15，Y 范围: -7 ~ +7
        i32 dx = random.nextInt(HORIZONTAL_RANGE + 1) - random.nextInt(HORIZONTAL_RANGE + 1);
        i32 dy = random.nextInt(VERTICAL_RANGE + 1) - random.nextInt(VERTICAL_RANGE + 1);
        i32 dz = random.nextInt(HORIZONTAL_RANGE + 1) - random.nextInt(HORIZONTAL_RANGE + 1);

        BlockPos targetPos(pos.x + dx, pos.y + dy, pos.z + dz);

        // 检查目标位置是否为空气
        const BlockState* targetState = world.getBlockState(targetPos);
        if (targetState == nullptr || !targetState->isAir()) {
            continue;
        }

        // 目标位置是空气，可以传送
        // 客户端：生成粒子效果；服务端：移动方块
        if (!world.isClientSide()) {
            // 服务端逻辑：在新位置放置龙蛋，移除原位置龙蛋
            world.setBlockState(targetPos, &state, 2);

            // 移除原位置的龙蛋（设置空气，不触发方块更新以避免递归）
            const BlockState* airState = BlockRegistry::instance().airState();
            if (airState != nullptr) {
                world.setBlockState(pos, airState, 2);
            }
        } else {
            // 客户端逻辑：生成传送门粒子效果
            // 在新旧位置之间生成粒子轨迹
            for (i32 j = 0; j < 128; ++j) {
                // 粒子位置在新旧位置之间插值
                f64 t = random.nextDouble();
                f32 velocityX = (random.nextFloat() - 0.5f) * 0.2f;
                f32 velocityY = (random.nextFloat() - 0.5f) * 0.2f;
                f32 velocityZ = (random.nextFloat() - 0.5f) * 0.2f;

                f64 particleX = math::lerp(static_cast<f64>(targetPos.x), static_cast<f64>(pos.x), t) +
                    (random.nextDouble() - 0.5) + 0.5;
                f64 particleY =
                    math::lerp(static_cast<f64>(targetPos.y), static_cast<f64>(pos.y), t) + random.nextDouble() - 0.5;
                f64 particleZ = math::lerp(static_cast<f64>(targetPos.z), static_cast<f64>(pos.z), t) +
                    (random.nextDouble() - 0.5) + 0.5;

                // 生成传送门粒子效果
                world.addParticle(particle::ParticleTypeId::Portal,
                    Vector3(static_cast<f32>(particleX), static_cast<f32>(particleY), static_cast<f32>(particleZ)),
                    Vector3(velocityX, velocityY, velocityZ));
            }
        }

        return true;
    }

    // 1000 次尝试都未找到有效位置，传送失败
    return false;
}

const CollisionShape& DragonEggBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_shape;
}

} // namespace blocks
} // namespace mc
