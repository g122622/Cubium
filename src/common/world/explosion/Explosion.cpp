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

#include "Explosion.hpp"
#include "common/core/Constants.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParameterSets.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/ray/Raycast.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/blocks/nether/FireBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/fluid/Fluid.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace mc {
namespace world::explosion {

// 使用游戏常量命名空间
using namespace mc::game::explosion;

// ============================================================================
// 构造函数
// ============================================================================

Explosion::Explosion(IWorld& world,
    const Vector3& position,
    f32 radius,
    ExplosionMode mode,
    bool causesFire,
    Entity* source,
    std::unique_ptr<DamageSource> damageSource,
    const loot::LootTableManager* lootTableManager)
    : m_world(world)
    , m_position(position)
    , m_radius(radius)
    , m_mode(mode)
    , m_causesFire(causesFire)
    , m_source(source)
    , m_damageSource(std::move(damageSource))
    , m_context(std::make_unique<EntityExplosionContext>(source))
    , m_lootTableManager(lootTableManager)
    , m_random(static_cast<u64>(std::abs(position.x * 3129871.0 + position.y * 116129781.0 + position.z * 172917.0)))
{}

Explosion::Explosion(IWorld& world,
    const Vector3& position,
    f32 radius,
    ExplosionMode mode,
    bool causesFire,
    Entity* source,
    std::unique_ptr<DamageSource> damageSource,
    const loot::LootTableManager* lootTableManager,
    std::unique_ptr<ExplosionContext> context)
    : m_world(world)
    , m_position(position)
    , m_radius(radius)
    , m_mode(mode)
    , m_causesFire(causesFire)
    , m_source(source)
    , m_damageSource(std::move(damageSource))
    , m_context(std::move(context))
    , m_lootTableManager(lootTableManager)
    , m_random(static_cast<u64>(std::abs(position.x * 3129871.0 + position.y * 116129781.0 + position.z * 172917.0)))
{
    MC_ASSERT_RELEASE(m_context != nullptr);
}

LivingEntity* Explosion::getIndirectSourceEntity() const
{
    if (m_source == nullptr) {
        return nullptr;
    }

    // 如果直接源是 LivingEntity，直接返回
    auto* living = dynamic_cast<LivingEntity*>(m_source);
    if (living != nullptr) {
        return living;
    }

    // 如果直接源是 TNT 实体，返回其点燃者
    auto* tnt = dynamic_cast<entity::TNTEntity*>(m_source);
    if (tnt != nullptr) {
        Entity* owner = tnt->getOwner();
        if (owner != nullptr) {
            auto* ownerLiving = dynamic_cast<LivingEntity*>(owner);
            if (ownerLiving != nullptr) {
                return ownerLiving;
            }
        }
    }

    // 如果直接源是投掷物，返回其发射者（如果是 LivingEntity）
    auto* projectile = dynamic_cast<entity::ProjectileEntity*>(m_source);
    if (projectile != nullptr) {
        Entity* shooter = projectile->getShooter();
        if (shooter != nullptr) {
            auto* shooterLiving = dynamic_cast<LivingEntity*>(shooter);
            if (shooterLiving != nullptr) {
                return shooterLiving;
            }
        }
    }

    return nullptr;
}

// ============================================================================
// 核心方法
// ============================================================================

void Explosion::explode()
{
    // 第一阶段：计算
    _calculateAffectedBlocks();
    _calculateAffectedEntities();

    // 第二阶段：执行
    _destroyBlocks();
    _applyKnockback();
    _spawnParticles();
    _playSound();

    // 如果需要生成火焰
    if (m_causesFire && m_mode != ExplosionMode::None) {
        _spawnFire();
    }
}

// ============================================================================
// 第一阶段：计算
// ============================================================================

void Explosion::_calculateAffectedBlocks()
{
    // 使用 std::unordered_set 避免重复
    std::unordered_set<i64> affectedPositions;

    // 16x16x16 立方体表面，共 1352 条射线
    // 只处理表面（j == 0 || j == 15 || k == 0 || k == 15 || l == 0 || l == 15）
    for (i32 j = 0; j < RAY_GRID_SIZE; ++j) {
        for (i32 k = 0; k < RAY_GRID_SIZE; ++k) {
            for (i32 l = 0; l < RAY_GRID_SIZE; ++l) {
                if (j == 0 || j == 15 || k == 0 || k == 15 || l == 0 || l == 15) {
                    // 计算射线方向
                    f32 d0 = static_cast<f32>(j) / 15.0f * 2.0f - 1.0f;
                    f32 d1 = static_cast<f32>(k) / 15.0f * 2.0f - 1.0f;
                    f32 d2 = static_cast<f32>(l) / 15.0f * 2.0f - 1.0f;

                    // 归一化
                    f32 d3 = std::sqrt(d0 * d0 + d1 * d1 + d2 * d2);
                    d0 /= d3;
                    d1 /= d3;
                    d2 /= d3;

                    // 初始爆炸强度 = radius * (0.7 + random * 0.6)
                    f32 f = m_radius * (INITIAL_STRENGTH_MIN + m_random.nextFloat() * INITIAL_STRENGTH_RANGE);

                    // 沿射线步进
                    f32 x = m_position.x;
                    f32 y = m_position.y;
                    f32 z = m_position.z;

                    for (f32 step = 0.3f; f > 0.0f; f -= 0.22500001f) {
                        BlockPos pos(static_cast<i32>(std::floor(x)),
                            static_cast<i32>(std::floor(y)),
                            static_cast<i32>(std::floor(z)));

                        // 获取方块状态
                        const BlockState* blockState = m_world.getBlockState(pos);
                        if (!blockState || blockState->isAir()) {
                            // 空气不消耗强度
                            x += d0 * RAY_STEP_SIZE;
                            y += d1 * RAY_STEP_SIZE;
                            z += d2 * RAY_STEP_SIZE;
                            continue;
                        }

                        // 获取流体状态（用于爆炸抗性计算）
                        const fluid::FluidState* fluidState = m_world.getFluidState(pos);

                        // 获取爆炸抗性
                        auto resistance = m_context->getExplosionResistance(*blockState, fluidState);
                        if (resistance.has_value()) {
                            // 强度衰减 = (resistance + 0.3) * 0.3
                            f -= (resistance.value() + RESISTANCE_COEFFICIENT) * RESISTANCE_COEFFICIENT;
                        }

                        // 如果强度仍然 > 0 且方块可被破坏
                        if (f > 0.0f && m_context->canDestroyBlock(*blockState, f)) {
                            // 添加到受影响方块列表（使用位置哈希去重）
                            i64 posKey = (static_cast<i64>(pos.x) & 0xFFFFFFLL) |
                                ((static_cast<i64>(pos.y) & 0xFFFFLL) << 24) |
                                ((static_cast<i64>(pos.z) & 0xFFFFFFLL) << 40);
                            affectedPositions.insert(posKey);
                        }

                        x += d0 * RAY_STEP_SIZE;
                        y += d1 * RAY_STEP_SIZE;
                        z += d2 * RAY_STEP_SIZE;
                    }
                }
            }
        }
    }

    // 将哈希转换回 BlockPos
    m_affectedBlocks.reserve(affectedPositions.size());
    for (i64 posKey : affectedPositions) {
        i32 bx = static_cast<i32>(posKey & 0xFFFFFFLL);
        i32 by = static_cast<i32>((posKey >> 24) & 0xFFFFLL);
        i32 bz = static_cast<i32>((posKey >> 40) & 0xFFFFFFLL);
        // 处理符号位
        if (bx >= 0x800000) bx -= 0x1000000;
        if (by >= 0x8000) by -= 0x10000;
        if (bz >= 0x800000) bz -= 0x1000000;
        m_affectedBlocks.emplace_back(bx, by, bz);
    }
}

void Explosion::_calculateAffectedEntities()
{
    // 实体影响范围 = radius * 2
    f32 range = m_radius * ENTITY_RANGE_MULTIPLIER;

    // 创建搜索 AABB
    AxisAlignedBB searchBox(m_position.x - range,
        m_position.y - range,
        m_position.z - range,
        m_position.x + range,
        m_position.y + range,
        m_position.z + range);

    // 获取范围内的所有实体
    std::vector<Entity*> entities = m_world.getEntitiesInAABB(searchBox, m_source);

    for (Entity* entity : entities) {
        if (!entity || entity->isRemoved()) {
            continue;
        }

        // 检查实体是否免疫爆炸
        if (entity->isImmuneToExplosions()) {
            continue;
        }

        // 计算实体到爆炸中心的距离
        Vector3 entityPos = entity->position();
        // 对于 TNT 实体，使用眼睛位置；其他实体使用普通位置
        f32 dx = entityPos.x - m_position.x;
        f32 dy = entityPos.y - m_position.y;
        f32 dz = entityPos.z - m_position.z;
        f32 distanceSq = dx * dx + dy * dy + dz * dz;

        // 距离比例
        f32 distanceRatio = std::sqrt(distanceSq) / range;

        if (distanceRatio > 1.0f) {
            continue; // 超出影响范围
        }

        // 归一化方向向量
        f32 length = std::sqrt(distanceSq);
        if (length < 0.001f) {
            // 实体就在爆炸中心，随机方向
            dx = m_random.nextFloat() * 2.0f - 1.0f;
            dy = m_random.nextFloat() * 2.0f - 1.0f;
            dz = m_random.nextFloat() * 2.0f - 1.0f;
            length = std::sqrt(dx * dx + dy * dy + dz * dz);
        }
        dx /= length;
        dy /= length;
        dz /= length;

        // 计算阻挡密度（视线检测）
        f32 density = _getBlockDensity(entity->boundingBox());

        // 伤害系数
        f32 impact = (1.0f - distanceRatio) * density;

        // 造成伤害
        f32 damage = std::floor((impact * impact + impact) / 2.0f * DAMAGE_MULTIPLIER * m_radius + 1.0f);

        if (damage > 0.0f) {
            // 创建伤害来源
            std::unique_ptr<DamageSource> damageSource;
            if (m_damageSource) {
                damageSource = m_damageSource->clone();
            } else {
                // 默认使用爆炸伤害
                damageSource = std::make_unique<EntityDamageSource>(DamageType::Explosion, m_source);
            }

            // ========== 玩家分支 ==========
            // 玩家的击退完全由客户端通过 Explosion IR 应用（client-authoritative），
            // 服务端不调用 addVelocity，避免 EntityVelocityPacket 与 Explosion IR 双重应用。
            // 同时清除 hurtMarked，防止 EntityTracker 发送 EntityVelocityPacket 覆盖客户端速度。
            // 对应 MC Java: ServerExplosion.hurtEntities 中 entity.push(vec32) 对所有实体调用，
            // 但 ServerPlayer 的 motion 是 client-authoritative，server 不会通过 SetEntityMotionPacket
            // 把自身速度同步给自己，因此 MC 端不会出现双重应用。Cubium 的 EntityTracker 采用
            // "AndSelf" 模式（向 ServerPlayer 自身发送速度包），所以必须在玩家分支显式跳过
            // 服务端速度修改与同步。
            Player* player = dynamic_cast<Player*>(entity);
            if (player) {
                // 观察者模式不受击退也不受伤害（与 MC 一致：旁观者免疫爆炸）
                if (player->isSpectator()) {
                    continue;
                }
                // 创造模式飞行中不受击退（仍受伤害，与原版一致）
                const PlayerAbilities& abilities = player->abilities();
                const bool creativeFlying = player->isCreative() && abilities.flying;

                // 应用爆炸保护附魔减伤与击退衰减
                // EPF 减伤公式: damage * (1 - min(EPF, 20) / 25)
                // 击退减少: knockback * (1 - EPF * 0.15)
                f32 playerKnockback = impact;
                i32 blastProtection = item::enchant::EnchantmentHelper::getTotalArmorProtection(
                    player->getArmorSlots(), DamageFlags::EXPLOSION);
                if (blastProtection > 0) {
                    damage = damage * (1.0f - std::min(static_cast<f32>(blastProtection), 20.0f) / 25.0f);
                    playerKnockback = playerKnockback * (1.0f - static_cast<f32>(blastProtection) * 0.15f);
                }

                // 造成伤害（LivingEntity::hurt 会设置 hurtMarked）
                player->hurt(*damageSource, damage);

                if (!creativeFlying) {
                    // 清除 hurtMarked，防止 EntityTracker 发送 EntityVelocityPacket
                    // （玩家速度由客户端通过 Explosion IR 应用，服务端不应同步速度）
                    player->clearHurtMarked();
                    // 记录玩家击退向量，将通过 Explosion IR 发送给客户端
                    // 客户端收到后调用 addDeltaMovement/addVelocity 累加到现有速度
                    m_playerKnockback[player->id()] =
                        Vector3(dx * playerKnockback, dy * playerKnockback, dz * playerKnockback);
                } else {
                    // 创造飞行玩家不受击退，但仍需清除 hurtMarked（hurt 调用已设置它）
                    // 否则 EntityTracker 会发送 EntityVelocityPacket，可能干扰飞行状态
                    player->clearHurtMarked();
                }

                // 通知实体被爆炸击中（用于冲量坠落伤害免疫等机制）
                entity->onExplosionHit(m_source);
                continue;
            }

            // ========== 非玩家生物实体分支 ==========
            LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
            if (living) {
                // 应用爆炸保护附魔减伤
                f32 knockback = impact;
                i32 blastProtection = item::enchant::EnchantmentHelper::getTotalArmorProtection(
                    living->getArmorSlots(), DamageFlags::EXPLOSION);
                if (blastProtection > 0) {
                    // EPF 减伤公式: damage * (1 - min(EPF, 20) / 25)
                    // 击退减少: knockback * (1 - EPF * 0.15)
                    damage = damage * (1.0f - std::min(static_cast<f32>(blastProtection), 20.0f) / 25.0f);
                    knockback = knockback * (1.0f - static_cast<f32>(blastProtection) * 0.15f);
                }

                living->hurt(*damageSource, damage);

                // 应用击退（非玩家实体由服务端权威同步速度）
                entity->addVelocity(dx * knockback, dy * knockback, dz * knockback);
                // LivingEntity::hurt 已设置 markHurt，EntityTracker 会通过 EntityVelocityPacket
                // 把更新后的速度同步给追踪此实体的客户端。这里无需额外 markHurt。
            } else {
                // 普通实体伤害
                entity->hurt(*damageSource, damage);

                // 应用击退
                entity->addVelocity(dx * impact, dy * impact, dz * impact);
                // 非 LivingEntity 的 hurt 默认实现不调用 markHurt，需显式标记以同步速度
                entity->markHurt();
            }

            // 通知实体被爆炸击中（用于冲量坠落伤害免疫等机制）
            entity->onExplosionHit(m_source);
        }
    }
}

// ============================================================================
// 第二阶段：执行
// ============================================================================

void Explosion::_destroyBlocks()
{
    if (m_mode == ExplosionMode::None) {
        return; // 不破坏方块
    }

    // 随机打乱方块顺序（Fisher-Yates 洗牌算法）
    for (size_t i = m_affectedBlocks.size(); i > 1; --i) {
        size_t j = static_cast<size_t>(m_random.nextInt(static_cast<i32>(i)));
        std::swap(m_affectedBlocks[i - 1], m_affectedBlocks[j]);
    }

    // 获取空气方块引用
    const BlockState* airState = &VanillaBlocks::AIR->defaultState();

    // 收集所有掉落物（用于合并）
    std::vector<std::pair<ItemStack, BlockPos>> allDrops;

    for (const BlockPos& pos : m_affectedBlocks) {
        const BlockState* state = m_world.getBlockState(pos);
        if (!state || state->isAir()) {
            continue;
        }

        // 获取方块对象
        const Block& block = state->getBlock();

        // 调用方块的爆炸回调
        block.onBlockExploded(m_world, pos, *state, this);

        // 检查方块是否可以被爆炸掉落
        bool canDrop = block.canDropFromExplosion(*state);

        // 移除方块
        if (m_mode == ExplosionMode::Break) {
            // 破坏但不掉落
            m_world.setBlockState(pos, airState, 3);
        } else if (m_mode == ExplosionMode::Destroy && canDrop) {
            // 破坏并掉落
            if (m_lootTableManager != nullptr) {
                // 生成掉落物
                auto drops = _generateBlockDrops(pos, *state);
                if (!drops.empty()) {
                    for (auto& drop : drops) {
                        // 尝试与已有掉落物合并
                        bool merged = false;
                        for (auto& [existingStack, existingPos] : allDrops) {
                            // 检查是否可以合并（相同物品、相同位置附近，2格范围内）
                            if (existingStack.canMergeWith(drop) && existingPos.distanceSq(pos) <= 4) {
                                // 尝试合并
                                i32 space = existingStack.getMaxStackSize() - existingStack.getCount();
                                if (space > 0) {
                                    i32 toAdd = std::min(space, drop.getCount());
                                    existingStack.grow(toAdd);
                                    drop.shrink(toAdd);
                                    if (drop.isEmpty()) {
                                        merged = true;
                                        break;
                                    }
                                }
                            }
                        }
                        if (!merged && !drop.isEmpty()) {
                            allDrops.emplace_back(std::move(drop), pos);
                        }
                    }
                }
            }
            m_world.setBlockState(pos, airState, 3);
        } else {
            // DESTROY 模式但方块不掉落（如玻璃）
            m_world.setBlockState(pos, airState, 3);
        }

        // 调用方块的破坏后生成回调（如 InfestedBlock 生成蠹虫）
        // 必须在方块被移除（设为空气）后调用，与 MC Java 行为一致
        // 爆炸破坏时工具为 nullptr（精准采集检查会被跳过，蠹虫正常生成）
        // dropExp 为 false，因为爆炸不会产生经验掉落
        block.spawnAfterBreak(m_world, pos, *state, nullptr, false);
    }

    // 在世界中生成所有物品实体
    for (const auto& [drop, pos] : allDrops) {
        if (!drop.isEmpty()) {
            // 使用 ItemDropHelper 生成物品实体
            std::vector<ItemStack> singleDrop;
            singleDrop.push_back(drop.copy());

            std::string throwerUuid;
            if (m_source != nullptr) {
                throwerUuid = m_source->uuid();
            }

            ItemDropHelper::spawnItemEntities(&m_world, pos, singleDrop, m_random, throwerUuid);
        }
    }
}

void Explosion::_applyKnockback()
{
    // 击退已经在 _calculateAffectedEntities() 中应用
    // 这里可以处理额外的玩家击退同步
}

void Explosion::_spawnParticles()
{
    if (m_radius >= 2.0f && m_mode != ExplosionMode::None) {
        // 大爆炸：使用发射器粒子
        m_world.addParticle(particle::ParticleTypeId::HugeExplosion, m_position, Vector3(1.0f, 0.0f, 0.0f));
    } else {
        // 小爆炸：使用普通爆炸粒子
        m_world.addParticle(particle::ParticleTypeId::Explosion, m_position, Vector3(1.0f, 0.0f, 0.0f));
    }
}

void Explosion::_playSound()
{
    // 播放爆炸音效
    f32 pitch = EXPLOSION_PITCH_BASE + (m_random.nextFloat() * 2.0f - 1.0f) * EXPLOSION_PITCH_RANGE * 0.5f;

    m_world.playSound(ResourceLocation("minecraft:entity.generic.explode"),
        sound::SoundCategory::Blocks,
        m_position,
        EXPLOSION_VOLUME,
        pitch);
}

// ============================================================================
// 辅助方法
// ============================================================================

f32 Explosion::_getBlockDensity(const AxisAlignedBB& entityBox)
{
    // 参考 MC 1.16.5 Explosion.getBlockDensity
    // 在实体碰撞箱内采样点，检测有多少可以看到爆炸中心

    // 计算采样步长
    f32 dx = (entityBox.maxX - entityBox.minX) * 2.0f + 1.0f;
    f32 dy = (entityBox.maxY - entityBox.minY) * 2.0f + 1.0f;
    f32 dz = (entityBox.maxZ - entityBox.minZ) * 2.0f + 1.0f;

    f32 stepX = 1.0f / dx;
    f32 stepY = 1.0f / dy;
    f32 stepZ = 1.0f / dz;

    // 计算偏移量以居中采样点
    f32 offsetX = (1.0f - std::floor(1.0f / stepX) * stepX) * 0.5f;
    f32 offsetZ = (1.0f - std::floor(1.0f / stepZ) * stepZ) * 0.5f;

    // 检查步长是否有效
    if (stepX <= 0.0f || stepY <= 0.0f || stepZ <= 0.0f) {
        return 0.0f;
    }

    i32 visible = 0;
    i32 total = 0;

    // 采样点
    for (f32 fx = 0.0f; fx <= 1.0f; fx += stepX) {
        for (f32 fy = 0.0f; fy <= 1.0f; fy += stepY) {
            for (f32 fz = 0.0f; fz <= 1.0f; fz += stepZ) {
                // 采样点位置（世界坐标）
                Vector3 samplePoint(entityBox.minX + fx * (entityBox.maxX - entityBox.minX) + offsetX,
                    entityBox.minY + fy * (entityBox.maxY - entityBox.minY),
                    entityBox.minZ + fz * (entityBox.maxZ - entityBox.minZ) + offsetZ);

                // 使用射线检测是否有方块阻挡
                Ray ray(samplePoint,
                    Vector3(m_position.x - samplePoint.x, m_position.y - samplePoint.y, m_position.z - samplePoint.z));
                f32 distance = (m_position - samplePoint).length();
                RaycastContext context(ray, distance);

                BlockRaycastResult result = raycastBlocks(context, m_world);

                // 如果射线未击中任何方块，说明该采样点可以看到爆炸中心
                if (result.isMiss()) {
                    ++visible;
                }
                ++total;
            }
        }
    }

    return total > 0 ? static_cast<f32>(visible) / static_cast<f32>(total) : 0.0f;
}

std::optional<f32> Explosion::_getExplosionResistance(const BlockPos& pos)
{
    const BlockState* blockState = m_world.getBlockState(pos);
    if (!blockState || blockState->isAir()) {
        return std::nullopt;
    }

    // 获取流体状态
    const fluid::FluidState* fluidState = m_world.getFluidState(pos);

    return m_context->getExplosionResistance(*blockState, fluidState);
}

f32 Explosion::_calculateDamage(Entity& entity, f32 distance, f32 density)
{
    // 伤害公式
    // damage = floor((impact^2 + impact) / 2 * 7 * radius + 1)
    f32 impact = (1.0f - distance / (m_radius * ENTITY_RANGE_MULTIPLIER)) * density;
    return std::floor((impact * impact + impact) / 2.0f * DAMAGE_MULTIPLIER * m_radius + 1.0f);
}

void Explosion::_spawnFire()
{
    // 1/3 概率在空位置生成火焰，前提是下方方块是不透明固体方块

    for (const BlockPos& pos : m_affectedBlocks) {
        // 1/3 概率
        if (m_random.nextInt(3) != 0) {
            continue;
        }

        const BlockState* state = m_world.getBlockState(pos);
        if (state && state->isAir()) {
            // 检查下方方块是否是不透明固体方块
            BlockPos belowPos(pos.x, pos.y - 1, pos.z);
            const BlockState* belowState = m_world.getBlockState(belowPos);

            if (belowState && belowState->isOpaqueCube(m_world, belowPos)) {
                // 根据下方方块类型选择火焰种类：灵魂沙/灵魂土上方生成灵魂火，否则生成普通火
                const BlockState& fireState = blocks::FireBlock::getFireState(m_world, pos);
                m_world.setBlockState(pos, &fireState, 11);
            }
        }
    }
}

std::vector<ItemStack> Explosion::_generateBlockDrops(const BlockPos& pos, const BlockState& state)
{

    if (m_lootTableManager == nullptr) {
        return {};
    }

    const Block& block = state.getBlock();

    // 获取方块的掉落表
    std::string lootTableId = block.getLootTableId();
    if (lootTableId.empty()) {
        return {};
    }

    const loot::LootTable* lootTable = m_lootTableManager->getTable(lootTableId);
    if (lootTable == nullptr) {
        return {};
    }

    // 构建掉落上下文
    BlockState* mutableState = const_cast<BlockState*>(&state);
    BlockPos* mutablePos = const_cast<BlockPos*>(&pos);
    ItemStack emptyTool; // 空工具（爆炸不使用工具）

    auto contextBuilder = loot::LootContextBuilder(m_world)
                              .withRandom(m_random)
                              .withParameter(loot::LootParams::BLOCK_STATE, mutableState)
                              .withParameter(loot::LootParams::BLOCK_POS, mutablePos)
                              .withParameter(loot::LootParams::TOOL, &emptyTool)
                              .withOwnedValue(loot::LootParams::EXPLOSION_RADIUS, m_radius); // 爆炸半径参数

    // 设置掉落表解析器（用于处理嵌套掉落表）
    contextBuilder.withLootTableResolver([this](const std::string& id) -> const loot::LootTable* {
        if (m_lootTableManager == nullptr) {
            return nullptr;
        }
        return m_lootTableManager->getTable(id);
    });
    contextBuilder.withPredicateResolver([this](const std::string& id) -> const loot::LootCondition* {
        if (m_lootTableManager == nullptr) {
            return nullptr;
        }
        return m_lootTableManager->getPredicate(id);
    });

    // 如果有爆炸源实体，添加到上下文
    if (m_source != nullptr) {
        contextBuilder.withNullableParameter(loot::LootParams::THIS_ENTITY, m_source);
    }

    std::unique_ptr<loot::LootContext> context = contextBuilder.build(loot::LootParameterSets::block());
    if (!context) {
        return {};
    }

    // 生成掉落物
    std::vector<ItemStack> drops = lootTable->generate(*context);

    // 应用爆炸衰减：爆炸时物品有概率消失
    for (auto& drop : drops) {
        // 每个物品有 (1 - 1/explosionRadius) 的概率存活
        f32 survivalChance = 1.0f - 1.0f / m_radius;
        survivalChance = std::max(0.0f, std::min(1.0f, survivalChance));

        // 对每个物品进行存活判定
        i32 survivingCount = 0;
        for (i32 i = 0; i < drop.getCount(); ++i) {
            if (m_random.nextFloat() < survivalChance) {
                ++survivingCount;
            }
        }
        drop.setCount(survivingCount);
    }

    // 移除空掉落物
    drops.erase(std::remove_if(drops.begin(), drops.end(), [](const ItemStack& stack) { return stack.isEmpty(); }),
        drops.end());

    return drops;
}

void Explosion::_spawnItemEntities(const BlockPos& pos, const std::vector<ItemStack>& drops)
{
    // 此方法已被 _destroyBlocks 中的内联实现替代
    // 保留此方法以供未来可能的扩展使用
    MC_UNUSED(pos);
    MC_UNUSED(drops);
}

} // namespace world::explosion
} // namespace mc
