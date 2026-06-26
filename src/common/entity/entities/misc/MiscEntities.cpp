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
 * LIABILITY, IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "MiscEntities.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/ConcretePowderBlock.hpp"
#include "common/world/block/blocks/FallingBlock.hpp"
#include "common/world/block/blocks/functional/AnvilBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/explosion/Explosion.hpp"
#include "common/world/explosion/ExplosionMode.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/gamerule/GameRules.hpp"
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

bool FallingBlockEntity::hurt(DamageSource& source, f32 /*amount*/)
{
    // MC Java: FallingBlockEntity.hurtServer()
    // 下落方块不可被伤害，但当来源非无敌时标记 hurtMarked 以同步速度到客户端。
    // 这使得下落方块在被击中时会产生击退效果（速度同步）。
    if (!isInvulnerableTo(source)) {
        markHurt();
    }
    return false;
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

    // 混凝土粉末下落过程中遇水提前固化
    // 对齐 MC 1.21.11 FallingBlockEntity.tick() 中 ConcretePowderBlock 特殊处理：
    // 如果下落方块是混凝土粉末，且当前位置有水，则立即固化
    if (auto* block = Block::getBlock(m_blockId)) {
        if (auto* concretePowder = dynamic_cast<blocks::ConcretePowderBlock*>(block)) {
            IWorld* worldPtr = world();
            if (worldPtr != nullptr) {
                BlockPos currentPos(static_cast<i32>(std::floor(x())),
                    static_cast<i32>(std::floor(y())),
                    static_cast<i32>(std::floor(z())));

                // 检查当前位置是否有水流体
                const fluid::FluidState* fluidState = worldPtr->getFluidState(currentPos);
                if (fluidState != nullptr && !fluidState->isEmpty() &&
                    fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
                    // 混凝土粉末接触水，立即固化为混凝土
                    const BlockState* concreteState = &concretePowder->getConcreteBlock()->defaultState();
                    worldPtr->setBlockState(currentPos, concreteState, 3);
                    remove();
                    return;
                }
            }
        }
    }

    // 检查是否落地
    if (onGround()) {
        _handleLanding();
    }

    // 超过最大下落时间后自动放置
    if (m_fallTime > MAX_FALL_TIME) {
        m_placeBlock = true;
        _handleLanding();
    }
}

void FallingBlockEntity::_handleLanding()
{
    IWorld* worldPtr = world();
    if (worldPtr == nullptr) {
        remove();
        return;
    }

    // 获取落地点位置（落地点是实体当前所在的方块位置）
    BlockPos landingPos(
        static_cast<i32>(std::floor(x())), static_cast<i32>(std::floor(y())), static_cast<i32>(std::floor(z())));

    // 获取下落的方块
    Block* block = Block::getBlock(m_blockId);
    if (block == nullptr) {
        // 方块ID无效，直接移除
        remove();
        return;
    }

    // 获取下落时的方块状态
    // 优先使用保存的完整状态（含属性如朝向），否则回退到默认状态
    const BlockState* fallingState = m_fallingState;
    if (fallingState == nullptr || fallingState->isAir()) {
        fallingState = &block->defaultState();
        if (fallingState == nullptr || fallingState->isAir()) {
            remove();
            return;
        }
    }

    // 获取落地点当前的方块状态
    const BlockState* hitState = worldPtr->getBlockState(landingPos);

    // 伤害碰撞箱内的实体（铁砧等 hurtEntities=true 的方块）
    if (m_hurtEntities) {
        _hurtEntities(worldPtr);
    }

    // _hurtEntities 可能更新了 m_blockId（铁砧降级），需要重新获取 block 指针
    block = Block::getBlock(m_blockId);

    // 如果 dontSetBlock 为 true（铁砧完全摧毁或外部设置），不放置方块，但调用 onBroken 回调
    // 注意：铁砧完全摧毁时仅设置 m_dontSetBlock=true（而非 m_cancelDrop），
    // 以确保 onBroken 回调被触发（播放铁砧破碎音效 WorldEvents::ANVIL_DESTROYED_SOUND）
    // 同时 m_shouldDropItem 在此路径下不会触发物品掉落（因为不走 _dropItem 分支）
    if (m_dontSetBlock) {
        // 调用 FallingBlock 的 onBroken 回调（如铁砧播放破碎音效）
        if (auto* fallingBlock = dynamic_cast<blocks::FallingBlock*>(block)) {
            fallingBlock->onBroken(*worldPtr, landingPos, *this);
        }
        remove();
        return;
    }

    // cancelDrop 为 true 时直接移除实体，不调用回调也不掉落物品
    // 此标志由外部逻辑设置（非铁砧损坏场景），表示完全取消一切后续处理
    if (m_cancelDrop) {
        remove();
        return;
    }

    // 尝试放置方块
    bool placed = false;
    if (m_placeBlock) {
        placed = _tryPlaceBlock(worldPtr, landingPos, fallingState, hitState);
    }

    if (placed) {
        // 放置成功：onEndFalling 回调已在 _tryPlaceBlock 中调用
    } else {
        // 放置失败：调用 onBroken 回调（如铁砧播放破碎音效）
        if (auto* fallingBlock = dynamic_cast<blocks::FallingBlock*>(block)) {
            fallingBlock->onBroken(*worldPtr, landingPos, *this);
        }

        // 掉落物品
        if (m_shouldDropItem) {
            // 检查游戏规则 doEntityDrops
            if (worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::DO_ENTITY_DROPS)) {
                _dropItem(worldPtr, landingPos);
            }
        }
    }

    remove();
}

bool FallingBlockEntity::_tryPlaceBlock(
    IWorld* world, const BlockPos& landingPos, const BlockState* fallingState, const BlockState* hitState)
{
    if (world == nullptr || fallingState == nullptr) {
        return false;
    }

    // 检查目标位置是否为移动中的活塞（活塞推动的方块不能放置）
    if (hitState != nullptr && hitState->is(VanillaBlocks::MOVING_PISTON)) {
        return false;
    }

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
    // canBeReplaced() 涵盖空气、可替换材质（花草、雪层等）
    // !blocksMovement() 涵盖不阻挡移动但非 replaceable 的方块（如火把）
    bool isReplaceable = (hitState == nullptr || hitState->canBeReplaced() || !hitState->blocksMovement());

    if (!isReplaceable) {
        return false;
    }

    // 检查方块是否可以在该位置放置（如脚手架需要支撑、火把需要墙壁等）
    // 对齐 MC 1.21.11 FallingBlockEntity.tick() 中的 canSurvive 检查
    IBlockReader& blockReader = static_cast<IBlockReader&>(*world);
    if (!fallingState->getBlock().isValidPosition(*fallingState, blockReader, landingPos)) {
        return false;
    }

    // 处理水浸透方块：如果方块支持 waterlogged 属性且目标位置有水源，设置 waterlogged = true
    // 对齐 MC 1.21.11 FallingBlockEntity.tick() 中的水浸透处理
    const BlockState* placementState = fallingState;
    if (fallingState->hasProperty(BlockStateProperties::WATERLOGGED())) {
        const fluid::FluidState* fluidState = world->getFluidState(landingPos);
        if (fluidState != nullptr && !fluidState->isEmpty() && fluidState->isSource() &&
            fluidState->getFluid().isIn(fluid::FluidTags::WATER())) {
            placementState = &fallingState->with(BlockStateProperties::WATERLOGGED(), true);
        }
    }

    // 尝试放置方块
    // flags = 3 表示通知邻居 + 同步客户端
    bool success = world->setBlockState(landingPos, placementState, 3);

    if (success) {
        // 调用 FallingBlock 的 onEndFalling 回调
        // 注意：各方块的音效由 onEndFalling 回调自行处理（如铁砧播放落地音效），
        // 沙子/砾石等方块 onEndFalling 为空实现（MC 原版中这些方块落地无声），
        // 不在此处播放通用放置音效以避免铁砧双重音效问题
        Block* block = const_cast<Block*>(&fallingState->getBlock());
        if (auto* fallingBlock = dynamic_cast<blocks::FallingBlock*>(block)) {
            const BlockState& hitStateRef = hitState ? *hitState : *BlockRegistry::instance().airState();
            fallingBlock->onEndFalling(*world, landingPos, *fallingState, hitStateRef, *this);
        }

        return true;
    }

    return false;
}

void FallingBlockEntity::_dropItem(IWorld* world, const BlockPos& pos)
{
    if (world == nullptr) {
        return;
    }

    // 获取方块对应的物品
    // 优先使用 m_fallingState 中的 blockId（铁砧降级后已更新），
    // 回退到 m_blockId（兼容未设置 fallingState 的情况）
    u32 itemId = m_blockId;
    if (m_fallingState != nullptr && !m_fallingState->isAir()) {
        itemId = m_fallingState->blockId();
    }
    const BlockItem* blockItem = BlockItemRegistry::instance().getBlockItem(itemId);
    if (blockItem == nullptr) {
        // 某些方块（如基岩）可能没有对应的物品
        return;
    }

    // 创建物品堆
    ItemStack stack(blockItem, 1);

    // 使用 ItemDropHelper 在方块位置生成物品实体
    math::Random& rng = world->getRandom();
    ItemDropHelper::spawnItemEntity(world,
        stack,
        static_cast<f64>(pos.x) + 0.5,
        static_cast<f64>(pos.y) + 0.5,
        static_cast<f64>(pos.z) + 0.5,
        rng,
        ItemDropHelper::DEFAULT_PICKUP_DELAY);
}

void FallingBlockEntity::_hurtEntities(IWorld* world)
{
    if (world == nullptr) {
        return;
    }

    // 计算下落距离和伤害
    f64 fallDistance = m_fallStartY - y();
    if (fallDistance <= 0) {
        return;
    }

    // 有效下落距离 = 总距离 - 1（第一格不造成伤害）
    // 参考 MC 原版: Mth.ceil(fallDistance - 1.0)
    i32 effectiveDistance = static_cast<i32>(std::ceil(fallDistance - 1.0));
    if (effectiveDistance <= 0) {
        return;
    }

    // 计算伤害值
    // 伤害 = min(floor(有效距离 * 每格伤害), 最大伤害)
    f32 damagePerDist = m_hurtEntities ? m_fallDamagePerDistance : HURT_AMOUNT;
    i32 maxDmg = m_hurtEntities ? m_fallDamageMax : MAX_HURT_AMOUNT;
    i32 damage =
        static_cast<i32>(std::min(static_cast<f32>(effectiveDistance) * damagePerDist, static_cast<f32>(maxDmg)));

    if (damage <= 0) {
        return;
    }

    // 获取碰撞箱内的所有实体
    AxisAlignedBB hurtBox = boundingBox();
    std::vector<Entity*> entities = world->getEntitiesInAABB(hurtBox, this);

    // 判断是否为铁砧（使用 BlockTags::ANVIL 检查）
    Block* block = Block::getBlock(m_blockId);
    bool isAnvil = (block != nullptr && BlockTags::ANVIL().contains(*block));

    // 创建伤害来源（根据伤害类型选择）
    // 钟乳石掉落使用 fallingStalactite 伤害类型，携带实体引用
    // 铁砧使用 anvil 伤害类型，其他使用 fallingBlock
    std::unique_ptr<DamageSource> damageSource;
    if (m_fallDamageType == DamageType::FallingStalactite) {
        damageSource = std::make_unique<EntityDamageSource>(DamageSources::fallingStalactite(this));
    } else if (isAnvil) {
        damageSource = std::make_unique<EnvironmentalDamage>(DamageSources::anvil());
    } else {
        damageSource = std::make_unique<EnvironmentalDamage>(DamageSources::fallingBlock());
    }

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

    // ========== 铁砧损坏逻辑 ==========
    // 参考 MC 原版 FallingBlockEntity.causeFallDamage:
    // 当方块是铁砧、伤害 > 0、且随机概率满足时，铁砧损坏
    // 概率公式: 0.05 + effectiveDistance * 0.05
    if (isAnvil && damage > 0 && m_fallingState != nullptr) {
        math::Random& rng = world->getRandom();
        f32 damageChance = 0.05f + static_cast<f32>(effectiveDistance) * 0.05f;
        if (rng.nextFloat() < damageChance) {
            // 尝试降级铁砧
            const BlockState* damagedState = blocks::AnvilBlock::damageAnvil(*m_fallingState);
            if (damagedState != nullptr) {
                // 铁砧降级成功（anvil → chipped_anvil 或 chipped_anvil → damaged_anvil）
                m_fallingState = damagedState;
                m_blockId = damagedState->blockId();
            } else {
                // 铁砧已在最大损坏状态（damaged_anvil），完全摧毁
                // 设置 m_dontSetBlock=true 使 _handleLanding 走 onBroken 回调路径
                // （播放铁砧破碎音效 WorldEvents::ANVIL_DESTROYED_SOUND），不放置方块也不掉落物品
                // 不使用 m_cancelDrop，因为那会跳过 onBroken 回调
                m_dontSetBlock = true;
                m_shouldDropItem = false;
            }
        }
    }
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

        // 客户端添加烟雾粒子效果
        if (world() != nullptr && world()->isClientSide()) {
            using namespace mc::client::renderer::trident::particle;

            // 在 TNT 上方随机位置生成烟雾粒子
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
        vel.y -= 0.04f; // 重力加速度
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

void TNTEntity::ignite(i32 fuseTicks)
{
    m_fuse = fuseTicks;
}

void TNTEntity::explode()
{
    if (m_exploded) return;
    m_exploded = true;

    IWorld* worldPtr = world();
    if (worldPtr != nullptr) {
        // 检查 tntExplodes 游戏规则
        // 对应 MC Java 的 PrimedTnt.explode() 中的 GameRules.TNT_EXPLODES 检查
        // 当规则为 false 时，TNT 实体仍然消失（remove），但不产生爆炸
        if (worldPtr->getGameRules().getBoolean(world::gamerule::GameRuleKeys::TNT_EXPLODES)) {
            // TNT 爆炸半径 4.0，模式 BREAK（破坏方块但不掉落物品）
            // 爆炸位置在 TNT 底部（Y 偏移 0.0625，即 1/16 格）
            worldPtr->createExplosion(
                Vector3(static_cast<f32>(x()), static_cast<f32>(y()) + 0.0625f, static_cast<f32>(z())),
                m_explosionRadius,
                world::explosion::ExplosionMode::Break,
                false, // 不生成火焰
                this   // 爆炸源实体
            );
        }
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
