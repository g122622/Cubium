#include "Explosion.hpp"
#include "../IWorld.hpp"
#include "../block/Block.hpp"
#include "../fluid/Fluid.hpp"
#include "../../entity/core/Entity.hpp"
#include "../../entity/core/LivingEntity.hpp"
#include "../../entity/entities/player/Player.hpp"
#include "../../entity/entities/player/GameModeUtils.hpp"
#include "../../entity/damage/DamageSource.hpp"
#include "../../util/AxisAlignedBB.hpp"
#include "../../util/math/MathUtils.hpp"
#include "../../sound/SoundCategory.hpp"
#include "../../resource/ResourceLocation.hpp"
#include "../../core/Constants.hpp"
#include "client/renderer/trident/particle/ParticleTypes.hpp"

#include <cmath>
#include <algorithm>
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
                     std::unique_ptr<DamageSource> damageSource)
    : m_world(world)
    , m_position(position)
    , m_radius(radius)
    , m_mode(mode)
    , m_causesFire(causesFire)
    , m_source(source)
    , m_damageSource(std::move(damageSource))
    , m_context(std::make_unique<EntityExplosionContext>(source))
    , m_random(static_cast<u64>(std::abs(position.x * 3129871.0 + position.y * 116129781.0 + position.z * 172917.0))) {
}

// ============================================================================
// 核心方法
// ============================================================================

void Explosion::explode() {
    // 第一阶段：计算
    calculateAffectedBlocks();
    calculateAffectedEntities();

    // 第二阶段：执行
    destroyBlocks();
    applyKnockback();
    spawnParticles();
    playSound();

    // 如果需要生成火焰
    if (m_causesFire && m_mode != ExplosionMode::None) {
        spawnFire();
    }
}

// ============================================================================
// 第一阶段：计算
// ============================================================================

void Explosion::calculateAffectedBlocks() {
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

                        // 获取爆炸抗性
                        auto resistance = m_context->getExplosionResistance(*blockState, nullptr);
                        if (resistance.has_value()) {
                            // 强度衰减 = (resistance + 0.3) * 0.3
                            f -= (resistance.value() + RESISTANCE_COEFFICIENT) * RESISTANCE_COEFFICIENT;
                        }

                        // 如果强度仍然 > 0 且方块可被破坏
                        if (f > 0.0f && m_context->canDestroyBlock(*blockState, f)) {
                            // 添加到受影响方块列表（使用位置哈希去重）
                            i64 posKey = static_cast<i64>(pos.x) & 0xFFFFFFLL |
                                        (static_cast<i64>(pos.y) & 0xFFFFLL) << 24 |
                                        (static_cast<i64>(pos.z) & 0xFFFFFFLL) << 40;
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

void Explosion::calculateAffectedEntities() {
    // 实体影响范围 = radius * 2
    f32 range = m_radius * ENTITY_RANGE_MULTIPLIER;

    // 创建搜索 AABB
    AxisAlignedBB searchBox(
        m_position.x - range, m_position.y - range, m_position.z - range,
        m_position.x + range, m_position.y + range, m_position.z + range
    );

    // 获取范围内的所有实体
    std::vector<Entity*> entities = m_world.getEntitiesInAABB(searchBox, m_source);

    for (Entity* entity : entities) {
        if (!entity || entity->isRemoved()) {
            continue;
        }

        // 检查实体是否免疫爆炸
        // TODO: 添加 isImmuneToExplosions() 方法

        // 计算实体到爆炸中心的距离
        Vector3 entityPos = entity->position();
        f32 dx = entityPos.x - m_position.x;
        f32 dy = entityPos.y - m_position.y;
        f32 dz = entityPos.z - m_position.z;
        f32 distanceSq = dx * dx + dy * dy + dz * dz;

        // 距离比例
        f32 distanceRatio = std::sqrt(distanceSq) / range;

        if (distanceRatio > 1.0f) {
            continue;  // 超出影响范围
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
        f32 density = getBlockDensity(entity->boundingBox());

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

            // 对生物实体造成伤害
            LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
            if (living) {
                // TODO: 爆炸保护附魔减少伤害
                living->hurt(*damageSource, damage);
            } else {
                // 普通实体伤害
                // TODO: Entity::hurt() 方法
            }

            // 计算击退
            f32 knockback = impact;
            if (living) {
                // TODO: 爆炸保护附魔减少击退
            }

            // 应用击退速度
            entity->addVelocity(dx * knockback, dy * knockback, dz * knockback);

            // 记录玩家击退
            Player* player = dynamic_cast<Player*>(entity);
            if (player) {
                // 观察者模式不受击退
                if (entity::GameModeUtils::isSpectator(player->gameMode())) {
                    continue;
                }
                // 创造模式飞行中不受击退
                const PlayerAbilities& abilities = player->abilities();
                if (entity::GameModeUtils::isCreative(player->gameMode()) && abilities.flying) {
                    continue;
                }
                m_playerKnockback[player->id()] = Vector3(dx * impact, dy * impact, dz * impact);
            }
        }
    }
}

// ============================================================================
// 第二阶段：执行
// ============================================================================

void Explosion::destroyBlocks() {
    if (m_mode == ExplosionMode::None) {
        return;  // 不破坏方块
    }

    // 随机打乱方块顺序（Fisher-Yates 洗牌算法）
    for (size_t i = m_affectedBlocks.size(); i > 1; --i) {
        size_t j = static_cast<size_t>(m_random.nextInt(static_cast<i32>(i)));
        std::swap(m_affectedBlocks[i - 1], m_affectedBlocks[j]);
    }

    // 空气方块引用
    const BlockState* airState = nullptr;  // TODO: 从 BlockRegistry 获取空气方块

    for (const BlockPos& pos : m_affectedBlocks) {
        const BlockState* state = m_world.getBlockState(pos);
        if (!state || state->isAir()) {
            continue;
        }

        // 调用方块的爆炸回调
        // TODO: Block::onBlockExploded()

        // 移除方块
        if (m_mode == ExplosionMode::Break) {
            // 破坏但不掉落
            m_world.setBlockState(pos, airState, 3);
        } else if (m_mode == ExplosionMode::Destroy) {
            // 破坏并掉落
            // TODO: 使用 BlockDropHandler 生成掉落物
            m_world.setBlockState(pos, airState, 3);
        }
    }
}

void Explosion::applyKnockback() {
    // 击退已经在 calculateAffectedEntities() 中应用
    // 这里可以处理额外的玩家击退同步
}

void Explosion::spawnParticles() {
    // 根据爆炸半径选择粒子类型
    using ParticleTypeId = client::renderer::trident::particle::ParticleTypeId;

    if (m_radius >= 2.0f && m_mode != ExplosionMode::None) {
        // 大爆炸：使用发射器粒子
        m_world.addParticle(ParticleTypeId::HugeExplosion, m_position, Vector3(1.0f, 0.0f, 0.0f));
    } else {
        // 小爆炸：使用普通爆炸粒子
        m_world.addParticle(ParticleTypeId::Explosion, m_position, Vector3(1.0f, 0.0f, 0.0f));
    }
}

void Explosion::playSound() {
    // 播放爆炸音效
    f32 pitch = EXPLOSION_PITCH_BASE + (m_random.nextFloat() * 2.0f - 1.0f) * EXPLOSION_PITCH_RANGE * 0.5f;

    m_world.playSound(
        ResourceLocation("minecraft:entity.generic.explode"),
        sound::SoundCategory::Blocks,
        m_position,
        EXPLOSION_VOLUME,
        pitch
    );
}

// ============================================================================
// 辅助方法
// ============================================================================

f32 Explosion::getBlockDensity(const AxisAlignedBB& entityBox) {
    // 在实体碰撞箱内采样点，检测有多少可以看到爆炸中心
    f32 dx = (entityBox.maxX - entityBox.minX) * 2.0f + 1.0f;
    f32 dy = (entityBox.maxY - entityBox.minY) * 2.0f + 1.0f;
    f32 dz = (entityBox.maxZ - entityBox.minZ) * 2.0f + 1.0f;

    f32 stepX = 1.0f / dx;
    f32 stepY = 1.0f / dy;
    f32 stepZ = 1.0f / dz;

    i32 visible = 0;
    i32 total = 0;

    // 采样点
    for (f32 fx = 0.0f; fx <= 1.0f; fx += stepX) {
        for (f32 fy = 0.0f; fy <= 1.0f; fy += stepY) {
            for (f32 fz = 0.0f; fz <= 1.0f; fz += stepZ) {
                Vector3 samplePoint(
                    entityBox.minX + fx * (entityBox.maxX - entityBox.minX),
                    entityBox.minY + fy * (entityBox.maxY - entityBox.minY),
                    entityBox.minZ + fz * (entityBox.maxZ - entityBox.minZ)
                );

                // TODO: 使用射线检测是否有方块阻挡
                // 目前简化：假设所有采样点都可见
                ++visible;
                ++total;
            }
        }
    }

    return total > 0 ? static_cast<f32>(visible) / static_cast<f32>(total) : 0.0f;
}

std::optional<f32> Explosion::getExplosionResistance(const BlockPos& pos) {
    const BlockState* blockState = m_world.getBlockState(pos);
    if (!blockState || blockState->isAir()) {
        return std::nullopt;
    }

    // TODO: 获取流体状态
    return m_context->getExplosionResistance(*blockState, nullptr);
}

f32 Explosion::calculateDamage(Entity& entity, f32 distance, f32 density) {
    // MC 1.16.5 伤害公式
    // damage = floor((impact^2 + impact) / 2 * 7 * radius + 1)
    f32 impact = (1.0f - distance / (m_radius * ENTITY_RANGE_MULTIPLIER)) * density;
    return std::floor((impact * impact + impact) / 2.0f * DAMAGE_MULTIPLIER * m_radius + 1.0f);
}

void Explosion::spawnFire() {
    // 1/3 概率在空位置生成火焰
    // TODO: 实现火焰生成
}

} // namespace world::explosion
} // namespace mc
