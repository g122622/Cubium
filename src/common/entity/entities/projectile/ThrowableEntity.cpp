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
#include "../../core/LivingEntity.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"

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

ThrowableEntity::ThrowableEntity(LegacyEntityType type, EntityId id)
    : ProjectileEntity(type, id)
{}

void ThrowableEntity::tick()
{
    // 参考 MC 1.16.5 ThrowableEntity.tick() 第52-101行

    // 先执行射线追踪和碰撞检测
    if (!m_leftShooter) {
        m_leftShooter = checkLeftShooter();
    }

    // 执行射线追踪
    const RayTraceResult result = performRayTrace();

    // 处理传送门和末地传送门
    bool handledPortal = false;
    // TODO: 处理下界传送门和末地传送门

    // 如果命中且未被传送门处理
    if (result.type != RayTraceResultType::Miss && !handledPortal) {
        onImpact(result);
        if (isRemoved()) {
            Entity::tick();
            return;
        }
    }

    // 执行方块碰撞
    // doBlockCollisions();

    // 应用物理
    Vector3 velocity = m_velocity;

    // 更新旋转
    updateRotation();

    // MC 1.16.5: 水中阻力 0.8F，空气中阻力 0.99F
    f32 drag = isInWater() ? 0.8f : 0.99f;

    // 水中生成气泡粒子
    if (isInWater()) {
        math::Random rng = createRandomFromEntity(*this);
        for (int i = 0; i < 4; ++i) {
            f32 offset = 0.25f;
            Vector3 pos(m_position.x - velocity.x * offset,
                m_position.y - velocity.y * offset,
                m_position.z - velocity.z * offset);
            m_world->addParticle(client::renderer::trident::particle::ParticleTypeId::Bubble, pos, velocity);
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
