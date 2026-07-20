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

#include "ThrowableEntity.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/blockentity/interactive/EndGatewayEntity.hpp"
#include "../../core/LivingEntity.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

namespace mc {
namespace entity {

namespace {

// 辅助函数：基于实体ID和tick创建随机数生成器
math::Random createRandomFromEntity(const Entity& entity)
{
    u64 seed = static_cast<u64>(entity.id()) << 32 | static_cast<u64>(entity.ticksExisted());
    return math::Random(seed);
}

} // anonymous namespace

ThrowableEntity::ThrowableEntity(EntityInstanceId id)
    : ProjectileEntity(id)
{}

void ThrowableEntity::tick()
{
    // 先执行射线追踪和碰撞检测
    if (!m_leftShooter) {
        m_leftShooter = checkLeftShooter();
    }

    // 执行射线追踪
    const RayTraceResult result = performRayTrace();

    // 处理传送门和末地传送门
    bool handledPortal = false;
    if (result.type == RayTraceResultType::Block && m_world != nullptr) {
        const BlockState* blockState = m_world->getBlockState(result.blockPos);
        if (blockState != nullptr) {
            // 获取方块（通过 getBlock() 返回引用，取地址获取指针）
            const Block* block = &blockState->getBlock();

            // 检查是否是下界传送门方块
            if (block == VanillaBlocks::NETHER_PORTAL) {
                // 投射物进入下界传送门，设置传送门状态
                // 投射物不需要等待时间，直接设置传送门状态
                setInPortal(true);
                setPortalPos(result.blockPos);
                handledPortal = true;
            }
            // 检查是否是末地折跃门方块
            else if (block == VanillaBlocks::END_GATEWAY) {
                // 投射物进入末地折跃门，获取方块实体并传送
                BlockEntity* blockEntity = m_world->getBlockEntity(result.blockPos);
                if (blockEntity != nullptr && blockEntity->getType() == BlockEntityType::EndGateway) {
                    auto* endGateway = static_cast<blockentity::EndGatewayEntity*>(blockEntity);
                    // 传送实体
                    endGateway->teleportEntity(*m_world, *this);
                    handledPortal = true;
                }
            }
        }
    }

    // 如果命中且未被传送门处理
    if (result.type != RayTraceResultType::Miss && !handledPortal) {
        onImpact(result);
        if (isRemoved()) {
            Entity::tick();
            return;
        }
    }

    // 执行方块碰撞检测
    // 对应 MC 原版 ThrowableProjectile.tick() 中的 applyEffectsFromBlocks() 调用
    // 用于处理蜘蛛网减速、气泡柱推拉等投射物与方块的碰撞效果
    doBlockCollisions();

    // 应用物理
    Vector3 velocity = m_velocity;

    // 更新旋转
    updateRotation();

    // 水中阻力 0.8，空气中阻力 0.99
    f32 drag = isInWater() ? 0.8f : 0.99f;

    // 水中生成气泡粒子
    if (isInWater()) {
        math::Random rng = createRandomFromEntity(*this);
        for (i32 i = 0; i < 4; ++i) {
            f32 offset = 0.25f;
            Vector3 pos(m_position.x - velocity.x * offset,
                m_position.y - velocity.y * offset,
                m_position.z - velocity.z * offset);
            m_world->addParticle(particle::ParticleTypeId::Bubble, pos, velocity);
        }
    }

    // 应用阻力
    velocity = velocity * drag;

    // 应用重力
    if (!m_noGravity) {
        velocity.y -= getGravity();
    }

    m_velocity = velocity;

    // 更新位置
    m_prevPosition = m_position;
    m_position = m_position + m_velocity;

    Entity::tick();
}

} // namespace entity
} // namespace mc
