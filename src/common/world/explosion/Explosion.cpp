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
#include "ExplosionImmunityContext.hpp"
#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/misc/MiscEntities.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/entities/projectile/ProjectileEntity.hpp"
#include "common/entity/tag/EntityTypeTags.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/enchantment/EnchantmentHelper.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/loot/context/LootContext.hpp"
#include "common/item/loot/context/LootParameterSets.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/physics/collision/CollisionShape.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/blocks/nether/FireBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/chunk/data/ChunkSection.hpp"
#include "common/world/explosion/ExplosionContext.hpp"
#include "common/world/explosion/ExplosionMode.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/gamerule/GameRules.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <fmt/format.h>

namespace mc {
namespace world::explosion {

// 使用游戏常量命名空间
using namespace mc::game::explosion;

// 追踪命名空间：允许直接写 TraceEvents.Server.World
using namespace mc::trace;

namespace {

// ============================================================================
// 爆炸专用视线检测（仅服务 _getBlockDensity，绕过公共 raycastBlocks）
//
// 公共 raycastBlocks 的单次射线 chunk 缓存是函数内局部变量，无法跨采样点复用：
// _getBlockDensity 对单个实体发射约 45 条采样射线（采样点集中于同一实体 0.6×1.8×0.6
// 碰撞箱内、终点都指向同一爆炸中心，射线簇高度集中在 2-3 个区块内），每条射线都从冷缓存
// 开始重新对 m_chunksMutex 加锁 + unordered_map 哈希查找 getChunk，是 _getBlockDensity
// 占爆炸墙钟 97%（单次射线 124μs）的根因。
//
// 本匿名命名空间的专用路径把 chunk/section 缓存提升为跨采样点复用（由 _getBlockDensity
// 在采样循环外构造一次 ExplosionLosCache 传入），把约 45 次锁+哈希降到个位数；同时用
// ChunkSection::isEmpty() 做 section 级早退、用内联 Y 边界检查替代 isWithinWorldBounds
// 虚调用、用 hit-only bool 返回跳过 BlockRaycastResult 构造与命中点/距离的 sqrt。
//
// 行为等价性：DDA 骨架（adjustedStart/adjustedEnd 偏移、tMax/tDelta、三轴步进、起点预检、
// 零向量处理、isAir/isLiquid 跳过、state==nullptr 视为空气）逐行复刻 raycastBlocks，保证
// "是否被遮挡"判定不变，从而爆炸伤害数值不变。
// ============================================================================

/// 线段与 AABB 相交判定（slab 法），只返回是否相交，不算 t 值与命中面。
/// 与 raycastBlocks 内 intersectSegmentAabb 在"是否相交"上逻辑等价，但省去 Direction 计算、
/// clamp 与 std::swap（用 std::min/std::max 替代），供视线检测专用。
[[nodiscard]] bool _segmentIntersectsAabb(const Vector3& origin, const Vector3& delta, const AxisAlignedBB& box)
{
    constexpr f32 EPSILON = 1.0e-7f;

    f32 tMin = 0.0f;
    f32 tMax = 1.0f;

    const auto updateAxis = [&](f32 axisOrigin, f32 axisDelta, f32 axisMin, f32 axisMax) -> bool {
        if (std::abs(axisDelta) < EPSILON) {
            // 射线在该轴平行：起点必须在 slab 内
            return axisOrigin >= axisMin && axisOrigin <= axisMax;
        }
        f32 t1 = (axisMin - axisOrigin) / axisDelta;
        f32 t2 = (axisMax - axisOrigin) / axisDelta;
        if (t1 > t2) {
            std::swap(t1, t2);
        }
        if (t1 > tMin) {
            tMin = t1;
        }
        if (t2 < tMax) {
            tMax = t2;
        }
        return tMin <= tMax;
    };

    if (!updateAxis(origin.x, delta.x, box.minX, box.maxX)) {
        return false;
    }
    if (!updateAxis(origin.y, delta.y, box.minY, box.maxY)) {
        return false;
    }
    if (!updateAxis(origin.z, delta.z, box.minZ, box.maxZ)) {
        return false;
    }

    // 相交且交点在 [0,1] 参数区间内（线段而非无限射线）
    return tMax >= 0.0f && tMin <= 1.0f;
}

/// 命中判定专用：检测射线（adjustedStart→adjustedStart+ddaDelta）是否穿过方块碰撞箱。
/// 与 raycastBlocks 内 traceBlockShape 在"是否命中"上逻辑等价，但不构造 BlockRaycastResult、
/// 不算命中点/命中面/距离，命中任一 box 即返回 true。
[[nodiscard]] bool _traceBlockShapeHitOnly(
    const BlockState& state, i32 blockX, i32 blockY, i32 blockZ, const Vector3& adjustedStart, const Vector3& ddaDelta)
{
    const CollisionShape& shape = state.getShape();
    if (shape.isEmpty()) {
        return false;
    }
    for (const auto& localBox : shape.boxes()) {
        const AxisAlignedBB worldBox(static_cast<f32>(blockX) + localBox.minX,
            static_cast<f32>(blockY) + localBox.minY,
            static_cast<f32>(blockZ) + localBox.minZ,
            static_cast<f32>(blockX) + localBox.maxX,
            static_cast<f32>(blockY) + localBox.maxY,
            static_cast<f32>(blockZ) + localBox.maxZ);
        if (_segmentIntersectsAabb(adjustedStart, ddaDelta, worldBox)) {
            return true;
        }
    }
    return false;
}

/// 跨采样点复用的区块/区块段缓存定义见下方 world::explosion 命名空间（需与 Explosion.hpp
/// 前向声明同名同命名空间，否则 out-of-line 定义不匹配）。

/// 爆炸改投射物归属（对应 vanilla ServerExplosion.hurtEntities:199-200）。
///
/// vanilla 在 hurt + push 之后、onExplosionHit 之前，对爆炸范围内的可偏转投射物
/// （EntityTypeTags.REDIRECTABLE_PROJECTILE = fireball/wind_charge/breeze_wind_charge）
/// 额外调 projectile.setOwner(damageSource.getEntity())——不跳过伤害，只是把被波及投射物
/// 的所有者改为爆炸伤害源实体（例如恶魂火球被 TNT 爆炸波及时，其所有者改为点燃 TNT 的实体，
/// 使后续该火球造成的伤害归属正确）。Cubium 用 setShooter 对应 vanilla setOwner。
///
/// 注意：爆炸伤害源实体取自 damageSource->getEntity()（对应 vanilla damageSource.getEntity()）：
/// - 默认爆炸（无自定义 damageSource）走 EntityDamageSource(Explosion, m_source)，getEntity()=m_source；
/// - 自定义 damageSource（如末影水晶爆炸）由构造方绑定实体，getEntity() 返回该实体。
void _redirectProjectilesInBlast(Entity& entity, const DamageSource& damageSource)
{
    if (!EntityTypeTags::REDIRECTABLE_PROJECTILE().contains(entity.getTypeId())) {
        return;
    }
    auto* projectile = dynamic_cast<entity::ProjectileEntity*>(&entity);
    Entity* const newOwner = damageSource.getEntity();
    if (projectile != nullptr && newOwner != nullptr) {
        projectile->setShooter(newOwner);
    }
}

} // anonymous namespace

/// 跨采样点复用的区块/区块段缓存。由 _getBlockDensity 在采样循环外构造一次，
/// 传入 _isLineOfSightBlocked，使同一次密度计算内的约 45 条采样射线共享 chunk/section 指针。
///
/// 生命周期安全：缓存为栈局部变量，严格限定在 _getBlockDensity 单次调用内（微秒级）。
/// 爆炸在主线程同步执行，区块卸载由主线程 tick 驱动，不会在本调用执行期间发生，故裸指针
/// 在本调用内稳定安全（与 raycastBlocks 单次射线缓存安全性等价，只是复用范围扩大到采样簇）。
/// 跨区块/跨段边界时刷新指针，避免持有过期 chunk/section。
struct ExplosionLosCache {
    const ChunkData* cachedChunk = nullptr;
    ChunkCoord cachedChunkX = 0;
    ChunkCoord cachedChunkZ = 0;
    // 标记当前缓存坐标是否已确认 getChunk 返回 nullptr（避免对同一未加载区块反复 getChunk），
    // 同时作为回退判定：getChunk 返回 nullptr 时（区块未加载，或测试桩世界不实现 getChunk）
    // 回退到 world.getBlockState，保持与 raycastBlocks 行为一致。
    bool cachedChunkIsNull = false;

    const ChunkSection* cachedSection = nullptr;
    i32 cachedSectionIndex = -1;
};

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

bool Explosion::_shouldAffectBlocklikeEntities() const
{
    // mobGriefing 开启时，所有爆炸都影响方块类实体；否则只有破坏方块的爆炸
    // （ExplosionMode 非 None）才影响。
    // 简化前提：风弹/风爆附魔不走 Explosion 类，故 m_source 永非风弹源，
    // vanilla shouldAffectBlocklikeEntities() 中的"非风弹源"判定恒真，退化为下式。
    // TODO: 若未来风弹改走 Explosion，需引入 Trigger 模式并恢复完整判定。
    const bool mobGriefing = m_world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING);
    return mobGriefing || m_mode != ExplosionMode::None;
}

// ============================================================================
// 核心方法
// ============================================================================

void Explosion::explode()
{
    // 以爆炸中心方块位置的 toId() 作为 Flow ID，贯穿本次爆炸的各子阶段，
    // 便于在 Perfetto UI 中追踪同一次爆炸的事件流转
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "Explosion::explode",
        "pos",
        fmt::format("({}, {}, {})", m_position.x, m_position.y, m_position.z),
        "radius",
        m_radius,
        "mode",
        static_cast<u8>(m_mode),
        "causesFire",
        m_causesFire,
        "sourceId",
        (m_source != nullptr) ? static_cast<i64>(m_source->id()) : -1,
        [flow = ::perfetto::Flow::ProcessScoped(BlockPos(m_position).toId())](
            ::perfetto::EventContext ctx) { flow(ctx); });

    using clock = std::chrono::steady_clock;
    const clock::time_point t0 = clock::now();

    // 第一阶段：计算
    _calculateAffectedBlocks();
    _calculateAffectedEntities();

    // 第二阶段：执行
    _destroyBlocks();
    _applyKnockback();
    _spawnParticles();
    _playSound();

    if (m_causesFire && m_mode != ExplosionMode::None) {
        _spawnFire();
    }

    // 记录本次爆炸总耗时（微秒），便于在 Perfetto/Tracy UI 观察规模与耗时分布
    const i64 totalUs = std::chrono::duration_cast<std::chrono::microseconds>(clock::now() - t0).count();
    MC_TRACE_COUNTER(TraceEvents.Server.World, "Explosion.explode.totalUs", totalUs);
}

// ============================================================================
// 第一阶段：计算
// ============================================================================

void Explosion::_calculateAffectedBlocks()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "Explosion::_calculateAffectedBlocks",
        "pos",
        fmt::format("({}, {}, {})", m_position.x, m_position.y, m_position.z),
        "radius",
        m_radius,
        [flow = ::perfetto::Flow::ProcessScoped(BlockPos(m_position).toId())](
            ::perfetto::EventContext ctx) { flow(ctx); });

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

    // 记录本次爆炸受影响方块数（计数器，便于在 Perfetto/Tracy UI 观察规模分布）
    MC_TRACE_COUNTER(TraceEvents.Server.World, "Explosion.AffectedBlocks", static_cast<i64>(m_affectedBlocks.size()));
}

void Explosion::_calculateAffectedEntities()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "Explosion::_calculateAffectedEntities",
        "pos",
        fmt::format("({}, {}, {})", m_position.x, m_position.y, m_position.z),
        [flow = ::perfetto::Flow::ProcessScoped(BlockPos(m_position).toId())](
            ::perfetto::EventContext ctx) { flow(ctx); });

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

    // 预构造爆炸免疫判定上下文（循环内对所有实体复用）：
    //   shouldAffectBlocklikeEntities 由 mobGriefing 与爆炸模式共同决定；
    //   indirectSource 追溯爆炸链（TNT→点燃者/投射物→发射者），供载具判定间接源是否为 Mob；
    //   directSource 即 m_source，供悬挂实体判定源是否在水中；
    //   mobGriefing 预填，避免各实体覆写内反查 world。
    const bool mobGriefing = m_world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::MOB_GRIEFING);
    const ExplosionImmunityContext immunityCtx{
        .shouldAffectBlocklikeEntities = _shouldAffectBlocklikeEntities(),
        .indirectSource = getIndirectSourceEntity(),
        .directSource = m_source,
        .mobGriefing = mobGriefing,
    };

    for (Entity* entity : entities) {
        if (!entity || entity->isRemoved()) {
            continue;
        }

        // 检查实体是否忽略此次爆炸（掉落物/盔甲架/悬挂实体/载具等按上下文判定）
        if (entity->ignoreExplosion(immunityCtx)) {
            continue;
        }

        // 计算实体到爆炸中心的距离
        Vector3 entityPos = entity->position();
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

        // hurtEntities 短路：不造成伤害且无击退倍率的实体跳过视线检测（seenPercent 视为 0），
        // 避免不必要的 raycast。默认 calculator 下 shouldDamage=true、knockbackMul=1.0，
        // 短路永不触发，seenPercent 照常计算。
        const bool shouldDamage = m_context->shouldDamageEntity(*this, *entity);
        const f32 knockbackMultiplier = m_context->getKnockbackMultiplier(*this, *entity);

        // 计算阻挡密度（视线检测）。短路条件下跳过 raycast。
        f32 seenPercent = 0.0f;
        if (shouldDamage || knockbackMultiplier != 0.0f) {
            seenPercent = _getBlockDensity(entity->boundingBox());
        }

        // 击退强度 = (1 - 距离比例) * 视线密度 * 击退倍率
        const f32 impact = (1.0f - distanceRatio) * seenPercent;
        f32 knockback = impact * knockbackMultiplier;

        // 爆炸击退抗性：LivingEntity 查 EXPLOSION_KNOCKBACK_RESISTANCE 属性（默认 0.0），
        // 非 LivingEntity 视为 0.0。爆炸保护魔咒经 enchantment.blast_protection 修饰符
        // （每级 +0.15 ADD_VALUE，4 盔甲槽位）增加此属性值，从而衰减被爆炸推开时的击退力度。
        // finalKnockback = baseKnockback * (1 - EXPLOSION_KNOCKBACK_RESISTANCE)。
        {
            LivingEntity* livingForKnockback = dynamic_cast<LivingEntity*>(entity);
            if (livingForKnockback != nullptr) {
                const f64 resistance = livingForKnockback->getAttributeValue(
                    entity::attribute::Attributes::EXPLOSION_KNOCKBACK_RESISTANCE, 0.0);
                knockback *= static_cast<f32>(1.0 - std::min(std::max(resistance, 0.0), 1.0));
            }
        }

        if (shouldDamage) {
            // 计算伤害（含 radius*2 修正，与 Java 一致）
            f32 damage = m_context->getEntityDamageAmount(*this, *entity, seenPercent);

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
                // ServerExplosion.hurtEntities 中 entity.push(vec32) 对所有实体调用，
                // 但 ServerPlayer 的 motion 是 client-authoritative，server 不会通过 SetEntityMotionPacket
                // 把自身速度同步给自己，因此不会出现双重应用。Cubium 的 EntityTracker 采用
                // "AndSelf" 模式（向 ServerPlayer 自身发送速度包），所以必须在玩家分支显式跳过
                // 服务端速度修改与同步。
                Player* player = dynamic_cast<Player*>(entity);
                if (player) {
                    // 观察者模式不受击退也不受伤害
                    if (player->isSpectator()) {
                        continue;
                    }
                    // 创造模式飞行中不受击退（仍受伤害）
                    const PlayerAbilities& abilities = player->abilities();
                    const bool creativeFlying = player->isCreative() && abilities.flying;

                    // 应用爆炸保护附魔减伤（EPF 减伤公式: damage * (1 - min(EPF, 20) / 25)）。
                    // 击退力度已在前面统一按 EXPLOSION_KNOCKBACK_RESISTANCE 属性缩减（爆炸保护经该属性
                    // 的修饰符提供抗性），此处不再用 EPF*0.15 重复衰减击退。
                    i32 blastProtection = item::enchant::EnchantmentHelper::getTotalArmorProtection(
                        player->getArmorSlots(), DamageFlags::EXPLOSION);
                    if (blastProtection > 0) {
                        damage = damage * (1.0f - std::min(static_cast<f32>(blastProtection), 20.0f) / 25.0f);
                    }
                    const f32 playerKnockback = knockback;

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
                    // 应用爆炸保护附魔减伤（EPF 减伤公式: damage * (1 - min(EPF, 20) / 25)）。
                    // 击退力度已在前面统一按 EXPLOSION_KNOCKBACK_RESISTANCE 属性缩减，此处不重复衰减。
                    i32 blastProtection = item::enchant::EnchantmentHelper::getTotalArmorProtection(
                        living->getArmorSlots(), DamageFlags::EXPLOSION);
                    if (blastProtection > 0) {
                        damage = damage * (1.0f - std::min(static_cast<f32>(blastProtection), 20.0f) / 25.0f);
                    }
                    const f32 livingKnockback = knockback;

                    living->hurt(*damageSource, damage);

                    // 应用击退（非玩家实体由服务端权威同步速度）
                    entity->addVelocity(dx * livingKnockback, dy * livingKnockback, dz * livingKnockback);
                    // LivingEntity::hurt 已设置 markHurt，EntityTracker 会通过 EntityVelocityPacket
                    // 把更新后的速度同步给追踪此实体的客户端。这里无需额外 markHurt。
                } else {
                    // 普通实体伤害
                    entity->hurt(*damageSource, damage);

                    // 应用击退
                    entity->addVelocity(dx * knockback, dy * knockback, dz * knockback);
                    // 非 LivingEntity 的 hurt 默认实现不调用 markHurt，需显式标记以同步速度
                    entity->markHurt();
                }

                // 爆炸改投射物归属（hurt+push 之后、onExplosionHit 之前，对应 vanilla
                // ServerExplosion:199-200）：被波及的可偏转投射物（fireball/wind_charge/
                // breeze_wind_charge）的所有者改为爆炸伤害源实体。不跳过伤害，只改归属。
                // Player 分支已 continue 跳过此处（玩家非投射物）。
                _redirectProjectilesInBlast(*entity, *damageSource);

                // 通知实体被爆炸击中（用于冲量坠落伤害免疫等机制）
                entity->onExplosionHit(m_source);
            }
        } else if (knockbackMultiplier > 0.0f) {
            // shouldDamage=false 但有击退倍率：不造成伤害，仅施加击退
            Player* player = dynamic_cast<Player*>(entity);
            if (player != nullptr) {
                if (player->isSpectator()) {
                    continue;
                }
                const PlayerAbilities& abilities = player->abilities();
                const bool creativeFlying = player->isCreative() && abilities.flying;
                if (!creativeFlying) {
                    // 玩家击退由客户端通过 Explosion IR 应用，服务端仅记录向量
                    player->clearHurtMarked();
                    m_playerKnockback[player->id()] = Vector3(dx * knockback, dy * knockback, dz * knockback);
                }
                entity->onExplosionHit(m_source);
                continue;
            }

            LivingEntity* living = dynamic_cast<LivingEntity*>(entity);
            if (living != nullptr) {
                entity->addVelocity(dx * knockback, dy * knockback, dz * knockback);
            } else {
                entity->addVelocity(dx * knockback, dy * knockback, dz * knockback);
                entity->markHurt();
            }
            // 爆炸改投射物归属（仅击退段同样执行，对应 vanilla 无条件 setOwner）。
            // 此段无 damageSource 局部变量，按爆炸默认规则构造：自定义 m_damageSource 优先，
            // 否则 EntityDamageSource(Explosion, m_source)。
            if (m_damageSource) {
                _redirectProjectilesInBlast(*entity, *m_damageSource);
            } else {
                const EntityDamageSource fallbackSource(DamageType::Explosion, m_source);
                _redirectProjectilesInBlast(*entity, fallbackSource);
            }
            entity->onExplosionHit(m_source);
        }
    }
}

// ============================================================================
// 第二阶段：执行
// ============================================================================

void Explosion::_destroyBlocks()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "Explosion::_destroyBlocks",
        "affectedCount",
        static_cast<i64>(m_affectedBlocks.size()),
        "mode",
        static_cast<u8>(m_mode),
        [flow = ::perfetto::Flow::ProcessScoped(BlockPos(m_position).toId())](
            ::perfetto::EventContext ctx) { flow(ctx); });

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

    // 记录本次爆炸合并后的掉落物数量（计数器）
    MC_TRACE_COUNTER(TraceEvents.Server.World, "Explosion.ItemDrops", static_cast<i64>(allDrops.size()));
}

void Explosion::_applyKnockback()
{
    // 击退已经在 _calculateAffectedEntities() 中应用
    // 这里可以处理额外的玩家击退同步
}

void Explosion::_spawnParticles()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "Explosion::_spawnParticles",
        "radius",
        m_radius,
        "bigExplosion",
        (m_radius >= 2.0f && m_mode != ExplosionMode::None));

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

    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World, "Explosion::_playSound", "pitch", pitch);

    m_world.playSound(ResourceLocation("minecraft:entity.generic.explode"),
        sound::SoundCategory::Blocks,
        m_position,
        EXPLOSION_VOLUME,
        pitch);
}

// ============================================================================
// 辅助方法
// ============================================================================

bool Explosion::_isLineOfSightBlocked(
    const Vector3& samplePoint, const Vector3& explosionCenter, ExplosionLosCache& cache, i32 minY, i32 maxY) const
{
    // 以下 DDA 骨架逐行复刻 raycastBlocks（Raycast.cpp），仅替换：
    //   - 零向量/重合 → 返回 false（无遮挡，等价 miss）
    //   - getCachedBlockState → 跨采样点复用 cache + section isEmpty 早退
    //   - isWithinWorldBounds 虚调用 → 内联 Y 比较（X/Z 不检查，与现状等价）
    //   - traceBlockShape → _traceBlockShapeHitOnly（hit-only bool，不构造结果）
    //   - 命中 → return true（遮挡）；miss → return false

    const Vector3 start = samplePoint;
    const Vector3 dir(
        explosionCenter.x - samplePoint.x, explosionCenter.y - samplePoint.y, explosionCenter.z - samplePoint.z);

    if (dir.lengthSquared() < 0.0001f) {
        // 方向为零向量：采样点与爆炸中心重合，视为无遮挡
        return false;
    }

    // 射线终点精确为爆炸中心（对齐 vanilla ServerExplosion.getSeenPercent 的
    // level.clip(samplePoint, center)）。原实现误把未归一化 dir（=explosionCenter-samplePoint）
    // 既当方向又取 |dir| 当距离，导致 end = samplePoint + dir*|dir|，终点被延伸到爆炸中心
    // 远侧 |dir| 倍，多经过的方块（如受害者脚下的地板）被误判遮挡，使 seenPercent 系统性偏低。
    // RaycastContext 契约要求 direction 归一化（见 Ray.hpp），endPosition() = origin + dir*maxDistance，
    // 故归一化 dir 并令 maxDistance = |dir| 后 end 恰为爆炸中心。
    const f32 distance = dir.length();
    const f32 invDistance = (distance > 0.0f) ? (1.0f / distance) : 0.0f;
    const Vector3 normalizedDir(dir.x * invDistance, dir.y * invDistance, dir.z * invDistance);
    const Vector3 end(start.x + normalizedDir.x * distance,
        start.y + normalizedDir.y * distance,
        start.z + normalizedDir.z * distance);

    if (start.distanceSquared(end) < 0.0001f) {
        // 起点和终点重合，视为无遮挡
        return false;
    }

    // 端点偏移避免边界精度问题，与 raycastBlocks 一致
    const f32 eps = 1.0e-7f;
    const Vector3 adjustedEnd(
        end.x + (start.x - end.x) * eps, end.y + (start.y - end.y) * eps, end.z + (start.z - end.z) * eps);
    const Vector3 adjustedStart(
        start.x + (end.x - start.x) * eps, start.y + (end.y - start.y) * eps, start.z + (end.z - start.z) * eps);

    const f32 dx = adjustedEnd.x - adjustedStart.x;
    const f32 dy = adjustedEnd.y - adjustedStart.y;
    const f32 dz = adjustedEnd.z - adjustedStart.z;
    const Vector3 ddaDelta(dx, dy, dz);

    i32 currentX = static_cast<i32>(std::floor(adjustedStart.x));
    i32 currentY = static_cast<i32>(std::floor(adjustedStart.y));
    i32 currentZ = static_cast<i32>(std::floor(adjustedStart.z));

    // 缓存式 getBlockState：命中 cache.cachedChunk 则直读 section，否则刷新缓存；
    // getChunk 返回 nullptr 时回退到 world.getBlockState（测试桩世界/未加载区块）。
    // section 级早退：当前 section 为空（m_blockCount==0）或未创建时直接返回 nullptr，
    // 调用方据此跳过逐方块查询。section/chunk 切换时刷新 cachedSection 指针。
    const auto getCachedBlockState = [&](i32 x, i32 y, i32 z) -> const BlockState* {
        const ChunkCoord chunkX = world::toChunkCoord(x);
        const ChunkCoord chunkZ = world::toChunkCoord(z);
        if (cache.cachedChunk == nullptr || chunkX != cache.cachedChunkX || chunkZ != cache.cachedChunkZ) {
            cache.cachedChunkX = chunkX;
            cache.cachedChunkZ = chunkZ;
            cache.cachedChunk = m_world.getChunk(chunkX, chunkZ);
            cache.cachedChunkIsNull = (cache.cachedChunk == nullptr);
            // 区块变更后段缓存失效，强制重取
            cache.cachedSectionIndex = -1;
            cache.cachedSection = nullptr;
        }
        if (cache.cachedChunkIsNull) {
            return m_world.getBlockState(x, y, z);
        }
        const i32 sectionIndex = world::toSectionIndex(y);
        if (sectionIndex != cache.cachedSectionIndex) {
            cache.cachedSectionIndex = sectionIndex;
            cache.cachedSection = cache.cachedChunk->getSection(sectionIndex);
        }
        // 空段（未创建或 m_blockCount==0）整段都是空气，返回 nullptr 触发调用方 continue
        if (cache.cachedSection == nullptr || cache.cachedSection->isEmpty()) {
            return nullptr;
        }
        const BlockCoord localX = x - chunkX * world::CHUNK_WIDTH;
        const BlockCoord localZ = z - chunkZ * world::CHUNK_WIDTH;
        return cache.cachedSection->getBlockState(localX, world::toSectionLocalY(y), localZ);
    };

    // 起点预检：起点在非空气非液体方块内且 shape 命中 → 遮挡
    if (currentY >= minY && currentY < maxY) {
        const BlockState* state = getCachedBlockState(currentX, currentY, currentZ);
        if (state != nullptr && !state->isAir() && !state->isLiquid()) {
            if (_traceBlockShapeHitOnly(*state, currentX, currentY, currentZ, adjustedStart, ddaDelta)) {
                return true;
            }
        }
    }

    const i32 stepX = (dx > 0.0f) ? 1 : ((dx < 0.0f) ? -1 : 0);
    const i32 stepY = (dy > 0.0f) ? 1 : ((dy < 0.0f) ? -1 : 0);
    const i32 stepZ = (dz > 0.0f) ? 1 : ((dz < 0.0f) ? -1 : 0);

    const f32 tDeltaX = (stepX == 0) ? std::numeric_limits<f32>::max() : static_cast<f32>(stepX) / dx;
    const f32 tDeltaY = (stepY == 0) ? std::numeric_limits<f32>::max() : static_cast<f32>(stepY) / dy;
    const f32 tDeltaZ = (stepZ == 0) ? std::numeric_limits<f32>::max() : static_cast<f32>(stepZ) / dz;

    const auto fractVal = [](f32 v) { return v - std::floor(v); };
    f32 tMaxX = (stepX == 0) ? std::numeric_limits<f32>::max()
                             : tDeltaX * (stepX > 0 ? (1.0f - fractVal(adjustedStart.x)) : fractVal(adjustedStart.x));
    f32 tMaxY = (stepY == 0) ? std::numeric_limits<f32>::max()
                             : tDeltaY * (stepY > 0 ? (1.0f - fractVal(adjustedStart.y)) : fractVal(adjustedStart.y));
    f32 tMaxZ = (stepZ == 0) ? std::numeric_limits<f32>::max()
                             : tDeltaZ * (stepZ > 0 ? (1.0f - fractVal(adjustedStart.z)) : fractVal(adjustedStart.z));

    while (tMaxX <= 1.0f || tMaxY <= 1.0f || tMaxZ <= 1.0f) {
        // 选择最小 t 值前进（与 raycastBlocks 三轴选择逻辑一致）
        if (tMaxX < tMaxY) {
            if (tMaxX < tMaxZ) {
                currentX += stepX;
                tMaxX += tDeltaX;
            } else {
                currentZ += stepZ;
                tMaxZ += tDeltaZ;
            }
        } else {
            if (tMaxY < tMaxZ) {
                currentY += stepY;
                tMaxY += tDeltaY;
            } else {
                currentZ += stepZ;
                tMaxZ += tDeltaZ;
            }
        }

        // Y 边界检查（内联，替代 isWithinWorldBounds 虚调用；X/Z 不检查与现状等价）
        if (currentY < minY || currentY >= maxY) {
            // 超出世界 Y 边界，视为无遮挡结束
            return false;
        }

        const BlockState* state = getCachedBlockState(currentX, currentY, currentZ);

        // 空气/液体/未加载区块不遮挡
        if (state == nullptr || state->isAir() || state->isLiquid()) {
            continue;
        }

        if (_traceBlockShapeHitOnly(*state, currentX, currentY, currentZ, adjustedStart, ddaDelta)) {
            return true;
        }
    }

    // 未击中任何方块：无遮挡
    return false;
}

f32 Explosion::_getBlockDensity(const AxisAlignedBB& entityBox)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World, "Explosion::_getBlockDensity");

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

    // 预读世界 Y 边界，传入专用视线检测供循环内内联比较（替代 isWithinWorldBounds 虚调用）
    const i32 minY = m_world.getMinBuildHeight();
    const i32 maxY = m_world.getMaxBuildHeight();

    // 跨采样点复用的区块/区块段缓存：本实体的约 45 条采样射线共享 chunk/section 指针，
    // 把逐射线的锁+哈希查找降到个位数（专用视线检测的核心收益点）
    ExplosionLosCache cache;

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

                // 专用视线检测：被遮挡则该采样点看不到爆炸中心
                const bool blocked = _isLineOfSightBlocked(samplePoint, m_position, cache, minY, maxY);
                if (!blocked) {
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

void Explosion::_spawnFire()
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "Explosion::_spawnFire",
        "affectedCount",
        static_cast<i64>(m_affectedBlocks.size()),
        [flow = ::perfetto::Flow::ProcessScoped(BlockPos(m_position).toId())](
            ::perfetto::EventContext ctx) { flow(ctx); });

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
    MC_TRACE_SCOPED_EVENT(TraceEvents.Server.World,
        "Explosion::_generateBlockDrops",
        "pos",
        fmt::format("({}, {}, {})", pos.x, pos.y, pos.z),
        [flow = ::perfetto::Flow::ProcessScoped(BlockPos(m_position).toId())](
            ::perfetto::EventContext ctx) { flow(ctx); });

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
