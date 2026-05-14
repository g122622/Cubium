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

#include "MiscEntities.hpp"
#include "../../../core/Types.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/explosion/Explosion.hpp"
#include "../../../world/explosion/ExplosionMode.hpp"
#include "../../core/LivingEntity.hpp"
#include "../player/Player.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include <cmath>

namespace mc {
namespace entity {

// ==================== FallingBlockEntity ====================

FallingBlockEntity::FallingBlockEntity()
    : Entity(LegacyEntityType::Unknown, EntityId(0))
{}

void FallingBlockEntity::tick()
{
    Entity::tick();

    m_fallTime++;

    // 应用重力
    Vector3 vel = velocity();
    vel.y -= 0.04f;

    // 移动
    move(vel.x, vel.y, vel.z);
    checkOnGround();

    // 减速
    vel.x *= 0.98f;
    vel.y *= 0.98f;
    vel.z *= 0.98f;
    setVelocity(vel);

    // 检查是否落地
    if (onGround()) {
        handleLanding();
    }

    // 超过一定时间后自动放置
    if (m_fallTime > 600) {
        m_placeBlock = true;
        handleLanding();
    }
}

void FallingBlockEntity::handleLanding()
{
    // 检查是否应该伤害实体
    if (m_hurtEntities) {
        f64 fallDistance = m_fallStartY - y();
        if (fallDistance > 0) {
            // 伤害下方的实体
        }
    }

    // 放置方块或掉落物品
    if (m_placeBlock) {
        // TODO: 尝试放置方块
    }

    // 无法放置，掉落物品
    remove();
}

// ==================== TNTEntity ====================

TNTEntity::TNTEntity()
    : Entity(LegacyEntityType::Unknown, EntityId(0))
{}

TNTEntity::TNTEntity(LegacyEntityType type, EntityId id)
    : Entity(type, id)
{}

std::unique_ptr<Entity> TNTEntity::create(IWorld* world)
{
    MC_UNUSED(world);
    // 创建时使用Unknown类型，会在spawnEntity时分配ID
    return std::make_unique<TNTEntity>();
}

void TNTEntity::tick()
{
    Entity::tick();

    // 引信倒计时
    if (m_fuse > 0) {
        m_fuse--;

        // MC 1.16.5: 客户端添加烟雾粒子效果
        // 参考: TNTEntity.tick() - world.addParticle(ParticleTypes.SMOKE, ...)
        if (world() != nullptr && world()->isClientSide()) {
            using namespace mc::client::renderer::trident::particle;

            // MC 1.16.5: 在 TNT 上方随机位置生成烟雾粒子
            // 每帧有 1/3 概率生成粒子
            math::Random& random = world()->getRandom();
            if (random.nextInt(3) == 0) {
                // 粒子位置：TNT 上方，带随机偏移
                f32 px = static_cast<f32>(x()) + random.nextFloat() * 0.6f - 0.3f;
                f32 py = static_cast<f32>(y()) + 0.5f + random.nextFloat() * 0.3f;
                f32 pz = static_cast<f32>(z()) + random.nextFloat() * 0.6f - 0.3f;

                // 粒子速度：轻微向上飘动
                f32 vx = random.nextFloat() * 0.02f - 0.01f;
                f32 vy = 0.02f + random.nextFloat() * 0.02f;
                f32 vz = random.nextFloat() * 0.02f - 0.01f;

                world()->addParticle(ParticleTypeId::Smoke, Vector3(px, py, pz), Vector3(vx, vy, vz));
            }
        }

        if (m_fuse <= 0 && !m_exploded) {
            explode();
        }
    }

    // 重力
    if (!hasNoGravity()) {
        Vector3 vel = velocity();
        vel.y -= 0.04f; // MC 1.16.5: 重力加速度
        setVelocity(vel);
    }

    // 移动
    Vector3 vel = velocity();
    move(vel.x, vel.y, vel.z);
    checkOnGround();

    // 空气阻力
    vel = velocity();
    vel.x *= 0.98f;
    vel.y *= 0.98f;
    vel.z *= 0.98f;
    setVelocity(vel);

    // 地面碰撞弹跳
    if (onGround()) {
        vel = velocity();
        vel.x *= 0.7f;
        vel.y *= -0.5f; // 反弹
        vel.z *= 0.7f;
        setVelocity(vel);
    }
}

void TNTEntity::ignite()
{
    m_fuse = DEFAULT_FUSE;
}

void TNTEntity::explode()
{
    if (m_exploded) return;
    m_exploded = true;

    IWorld* worldPtr = world();
    if (worldPtr != nullptr) {
        // TNT 爆炸半径 4.0，模式 BREAK（破坏方块但不掉落物品）
        // 爆炸位置在 TNT 底部（Y 偏移 0.0625，即 1/16 格）
        // 参考 MC 1.16.5: TNTEntity.explode()
        worldPtr->createExplosion(
            Vector3(static_cast<f32>(x()), static_cast<f32>(y()) + 0.0625f, static_cast<f32>(z())),
            m_explosionRadius,
            world::explosion::ExplosionMode::Break,
            false, // 不生成火焰
            this   // 爆炸源实体
        );
    }

    remove();
}

// ==================== WardenWarningEffect ====================

void WardenWarningEffect::tick()
{
    if (m_cooldown > 0) {
        m_cooldown--;
    } else {
        if (m_warningLevel > 0) {
            m_warningLevel--;
            m_cooldown = DECREASE_INTERVAL;
        }
    }
}

void WardenWarningEffect::increaseWarning()
{
    if (m_warningLevel < MAX_WARNING) {
        m_warningLevel++;
        m_cooldown = DECREASE_INTERVAL;
    }
}

void WardenWarningEffect::decreaseWarning()
{
    if (m_warningLevel > 0) {
        m_warningLevel--;
    }
}

} // namespace entity
} // namespace mc
