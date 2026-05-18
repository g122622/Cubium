/*
* Copyright (c) 2026 Guo Yi
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including, modification, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to permit
* persons to whom the Software is furnished to do so, subject to the following
* conditions:
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
#include "../../../item/core/ItemStack.hpp"
#include "../../../item/items/block/BlockItemRegistry.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../world/IWorld.hpp"
#include "../../../world/block/Block.hpp"
#include "../../../world/block/BlockRegistry.hpp"
#include "../../../world/block/blocks/FallingBlock.hpp"
#include "../../../world/block/Material.hpp"
#include "../../../world/explosion/Explosion.hpp"
#include "../../../world/explosion/ExplosionMode.hpp"
#include "../../../world/gamerule/GameRules.hpp"
#include "../../core/LivingEntity.hpp"
#include "../../damage/DamageSource.hpp"
#include "../../utils/ItemDropHelper.hpp"
#include "../player/Player.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include <cmath>

namespace mc {
namespace entity {

// ==================== FallingBlockEntity ====================

FallingBlockEntity::FallingBlockEntity()
    : Entity(EntityId(0))
{}

std::unique_ptr<Entity> FallingBlockEntity::create(IWorld* /*world*/)
{
    return std::make_unique<FallingBlockEntity>();
}

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
    IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        remove();
        return;
    }

    // MC 1.16.5: 获取落地点位置
    // 落地点是实体当前所在的方块位置
    BlockPos landingPos(
        static_cast<i32>(std::floor(x())),
        static_cast<i32>(std::floor(y())),
        static_cast<i32>(std::floor(z()))
    );

    // 获取下落的方块
    Block* block = Block::getBlock(m_blockId);
    if (block == nullptr) {
        // 方块ID无效，直接移除
        remove();
        return;
    }

    const BlockState* fallingState = &block->defaultState();
    if (fallingState == nullptr || fallingState->isAir()) {
        // 方块状态无效，直接移除
        remove();
        return;
    }

    // 获取落地点当前的方块状态
    const BlockState* hitState = worldPtr->getBlockState(landingPos);

    // MC 1.16.5: 伤害碰撞箱内的实体
    if (m_hurtEntities) {
        hurtEntities(worldPtr);
    }

    // MC 1.16.5: 如果 dontSetBlock 为 true，只调用 onBroken 回调
    if (m_dontSetBlock) {
        // 调用 FallingBlock 的 onBroken 回调
        if (auto* fallingBlock = dynamic_cast<blocks::FallingBlock*>(block)) {
            fallingBlock->onBroken(*worldPtr, landingPos, *this);
        }
        remove();
        return;
    }

    // MC 1.16.5: 尝试放置方块
    bool placed = false;
    if (m_placeBlock) {
        placed = tryPlaceBlock(worldPtr, landingPos, fallingState, hitState);
    }

    // MC 1.16.5: 放置失败，掉落物品
    if (!placed && m_shouldDropItem) {
        // 检查游戏规则 doEntityDrops
        if (worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS)) {
            dropItem(worldPtr, landingPos);
        }
    }

    remove();
}

bool FallingBlockEntity::tryPlaceBlock(IWorld* world, const BlockPos& landingPos, const BlockState* fallingState, const BlockState* hitState)
{
    if (world == nullptr || fallingState == nullptr) {
        return false;
    }

    // MC 1.16.5: 检查目标位置是否为移动中的活塞
    // 如果是活塞推动的方块，不能放置
    // 注意: 当前项目可能还没有实现活塞，这里暂时跳过这个检查

    // MC 1.16.5: 检查放置条件
    // 条件1: 目标位置可替换
    // 条件2: 下方方块不可穿透
    // 条件3: 方块状态在目标位置有效

    // 获取下方方块状态
    BlockPos belowPos(landingPos.x, landingPos.y - 1, landingPos.z);
    const BlockState* belowState = world->getBlockState(belowPos);

    // 检查是否可以穿透下方方块
    bool canFallThroughBelow = blocks::FallingBlock::canFallThrough(belowState);

    // 如果下方可穿透，则不能放置（方块会继续下落）
    if (canFallThroughBelow) {
        return false;
    }

    // 检查目标位置是否可替换
    // MC 1.16.5: blockstate.isReplaceable(new DirectionalPlaceContext(...))
    // 简化实现：检查是否为空气或可替换材质
    bool isReplaceable = false;
    if (hitState == nullptr || hitState->isAir()) {
        isReplaceable = true;
    } else if (hitState->getMaterial().isReplaceable()) {
        isReplaceable = true;
    } else if (!hitState->blocksMovement()) {
        // 不阻挡移动的方块可以穿透（如火把、草等）
        isReplaceable = true;
    }

    if (!isReplaceable) {
        return false;
    }

    // MC 1.16.5: 检查方块是否可以在该位置放置
    // 参考: BlockState.isValidPosition()
    // 简化实现：大多数下落方块可以在任何位置放置
    // 注意: 完整实现需要调用 fallingState->getBlock().isValidPosition()

    // MC 1.16.5: 处理水浸透方块（如沙子落入水中）
    // 如果方块有 waterlogged 属性且目标位置有水，设置 waterlogged = true
    // 注意: 当前项目可能还没有完整实现水浸透属性

    // 尝试放置方块
    // flags = 3 表示通知邻居 + 同步客户端
    bool success = world->setBlockState(landingPos, fallingState, 3);

    if (success) {
        // MC 1.16.5: 调用 FallingBlock 的 onEndFalling 回调
        Block* block = const_cast<Block*>(&fallingState->getBlock());
        if (auto* fallingBlock = dynamic_cast<blocks::FallingBlock*>(block)) {
            const BlockState& hitStateRef = hitState ? *hitState : *BlockRegistry::instance().airState();
            fallingBlock->onEndFalling(*world, landingPos, *fallingState, hitStateRef, *this);
        }

        // MC 1.16.5: 播放放置音效
        // world->playSound(...)
        // 注意: 完整实现需要播放方块的放置音效

        return true;
    }

    return false;
}

void FallingBlockEntity::dropItem(IWorld* world, const BlockPos& pos)
{
    if (world == nullptr) {
        return;
    }

    // 获取方块对应的物品
    const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(m_blockId);
    if (blockItem == nullptr) {
        // 如果方块没有对应的物品，尝试使用方块本身
        // 某些方块（如基岩）可能没有对应的物品
        return;
    }

    // 创建物品堆
    ItemStack stack(blockItem, 1);

    // 使用 ItemDropHelper 在方块位置生成物品实体
    math::Random& rng = world->getRandom();
    ItemDropHelper::spawnItemEntity(
        world,
        stack,
        static_cast<f64>(pos.x) + 0.5,
        static_cast<f64>(pos.y) + 0.5,
        static_cast<f64>(pos.z) + 0.5,
        rng,
        ItemDropHelper::DEFAULT_PICKUP_DELAY
    );
}

void FallingBlockEntity::hurtEntities(IWorld* world)
{
    if (world == nullptr) {
        return;
    }

    // MC 1.16.5: 计算下落距离和伤害
    f64 fallDistance = m_fallStartY - y();
    if (fallDistance <= 0) {
        return;
    }

    // 有效下落距离 = 总距离 - 1（第一格不造成伤害）
    i32 effectiveDistance = static_cast<i32>(std::floor(fallDistance - 1.0));
    if (effectiveDistance <= 0) {
        return;
    }

    // 计算伤害值
    // 伤害 = min(floor(下落距离 * HURT_AMOUNT), MAX_HURT_AMOUNT)
    i32 damage = static_cast<i32>(std::min(
        static_cast<f32>(effectiveDistance) * HURT_AMOUNT,
        static_cast<f32>(MAX_HURT_AMOUNT)
    ));

    if (damage <= 0) {
        return;
    }

    // MC 1.16.5: 获取碰撞箱内的所有实体
    AxisAlignedBB hurtBox = boundingBox();
    std::vector<Entity*> entities = world->getEntitiesInAABB(hurtBox, this);

    // MC 1.16.5: 判断是否为铁砧
    Block* block = Block::getBlock(m_blockId);
    bool isAnvil = (block != nullptr && block->blockLocation().toString().find("anvil") != std::string::npos);

    // 创建伤害来源
    EnvironmentalDamage anvilDamage = DamageSources::anvil();
    EnvironmentalDamage fallingBlockDamageSource = DamageSources::fallingBlock();
    DamageSource* damageSource = isAnvil ? static_cast<DamageSource*>(&anvilDamage) : static_cast<DamageSource*>(&fallingBlockDamageSource);

    // 对每个实体造成伤害
    for (Entity* entity : entities) {
        if (entity == nullptr || entity == this) {
            continue;
        }

        // 只对生物实体造成伤害
        LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
        if (living != nullptr) {
            living->hurt(*damageSource, static_cast<f32>(damage));
        }
    }

    // MC 1.16.5: 铁砧损坏机制
    // 如果是铁砧，有概率损坏
    // 注意: 完整实现需要检查铁砧的损坏状态并更新
    // 参考: AnvilBlock.damage()
}

// ==================== TNTEntity ====================

TNTEntity::TNTEntity()
    : Entity(EntityId(0))
{}

TNTEntity::TNTEntity(EntityId id)
    : Entity(id)
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
