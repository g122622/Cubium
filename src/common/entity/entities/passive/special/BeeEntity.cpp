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

#include "BeeEntity.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/ai/goal/goals/FollowParentGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/SwimGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/ai/goal/goals/special/BeeGoals.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/ai/util/RandomPositionGenerator.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/BlockTags.hpp"
#include "common/world/block/blocks/vegetation/DoublePlantBlock.hpp"
#include "common/world/block/registry/VegetationBlocks.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "common/world/blockentity/interactive/BeehiveBlockEntity.hpp"
#include <cstdlib>
#include <memory>
#include <optional>

namespace mc {

// ============================================================================
// 静态数据参数定义
// ============================================================================

entity::DataParameter<i8> BeeEntity::DATA_FLAGS_PARAM = entity::EntityDataManager::createKey<i8>();
entity::DataParameter<i64> BeeEntity::ANGER_TIME_PARAM = entity::EntityDataManager::createKey<i64>();

// ============================================================================
// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = AnimalEntity::classInfo()）
// ============================================================================
const entity::EntityClassInfo& BeeEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"BeeEntity", &AnimalEntity::classInfo()};
    return s_classInfo;
}

// ============================================================================
// 构造与生命周期
// ============================================================================

BeeEntity::BeeEntity(EntityInstanceId id)
    : AnimalEntity(id)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();

    // 补调 registerData：AnimalEntity 构造只调 registerAttributes 不调 registerData（vtable 在基类
    // 构造期间指向 AnimalEntity，派生 override 永不执行），须在派生类构造显式调用。
    // Bee 的 registerData 注册 DATA_FLAGS / ANGER_TIME 等同步参数。
    registerData();
}

std::unique_ptr<Entity> BeeEntity::create(IWorld* /*world*/)
{
    return std::make_unique<BeeEntity>(0);
}

// ============================================================================
// 花朵吸引判定
// ============================================================================

bool BeeEntity::attractsBees(const BlockState& state)
{
    // 1. 必须在 BEE_ATTRACTIVE 标签中（闭合眼眸花不在标签中，因此不吸引蜜蜂）
    if (!BlockTags::BEE_ATTRACTIVE().contains(state)) {
        return false;
    }

    // 2. 含水的可水合花朵不吸引蜜蜂
    if (state.getOptional(BlockStateProperties::WATERLOGGED()).value_or(false)) {
        return false;
    }

    // 3. 向日葵仅上半部分吸引蜜蜂
    if (state.is(block_registry::VegetationBlocks::SUNFLOWER)) {
        const auto half = state.get(BlockStateProperties::DOUBLE_BLOCK_HALF());
        return half == blocks::DoublePlantBlock::DoubleBlockHalf::Upper;
    }

    return true;
}

void BeeEntity::registerData()
{
    AnimalEntity::registerData();

    // 标记当前正在注册 BeeEntity 类的字段，使 registerParam 沿 BeeEntity 继承链
    // 分配 id（续接 AnimalEntity 之后）。RAII 守卫自动配对压栈/弹栈。
    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    m_dataManager.registerParam(DATA_FLAGS_PARAM, static_cast<i8>(0));
    // ANGER_TIME 以 i64(Long) 注册对齐 vanilla Bee.DATA_ANGER_END_TIME(serializerId=2)。
    // 业务层以 i32 语义运转,默认 0=非愤怒。旧 i32 致真客户端 field 类型校验崩。
    m_dataManager.registerParam(ANGER_TIME_PARAM, static_cast<i64>(0));
}

// ============================================================================
// 数据参数辅助方法
// ============================================================================

bool BeeEntity::_getBeeFlag(i8 flag) const
{
    return (m_dataManager.get(DATA_FLAGS_PARAM) & flag) != 0;
}

void BeeEntity::_setBeeFlag(i8 flag, bool value)
{
    i8 flags = m_dataManager.get(DATA_FLAGS_PARAM);
    if (value) {
        m_dataManager.set(DATA_FLAGS_PARAM, static_cast<i8>(flags | flag));
    } else {
        m_dataManager.set(DATA_FLAGS_PARAM, static_cast<i8>(flags & ~flag));
    }
}

// ============================================================================
// 花粉状态（使用 DataParameter 同步）
// ============================================================================

bool BeeEntity::hasNectar() const
{
    return _getBeeFlag(FLAG_HAS_NECTAR);
}

void BeeEntity::setHasNectar(bool nectar)
{
    if (nectar != m_hasNectar) {
        m_hasNectar = nectar;
        _setBeeFlag(FLAG_HAS_NECTAR, nectar);
    }
}

bool BeeEntity::hasStung() const
{
    return _getBeeFlag(FLAG_HAS_STUNG);
}

void BeeEntity::setHasStung(bool stung)
{
    if (stung != m_hasStung) {
        m_hasStung = stung;
        _setBeeFlag(FLAG_HAS_STUNG, stung);
    }
}

// ============================================================================
// IAngerable 接口实现
// ============================================================================

i32 BeeEntity::getAngerTime() const
{
    // wire 以 i64(Long) 存储(对齐 vanilla),业务层返回 i32
    return static_cast<i32>(m_dataManager.get(ANGER_TIME_PARAM));
}

void BeeEntity::setAngerTime(i32 time)
{
    m_angerTime = time;
    // 写入 i64(Long) 对齐 vanilla wire 类型(serializerId=2 VAR_LONG)
    m_dataManager.set(ANGER_TIME_PARAM, static_cast<i64>(time));
}

void BeeEntity::setAngry(bool angry)
{
    if (angry) {
        // 设置随机愤怒时间 (20-39 ticks)
        setAngerTime(MAX_ANGER_TIME);
    } else {
        setAngerTime(0);
    }
}

void BeeEntity::setRevengeTarget(LivingEntity* target)
{
    setAttackTarget(target);
    if (target != nullptr) {
        setAngry(true);
        m_revengeTargetId = target->id();
        m_revengeTimer = MAX_ANGER_TIME;
    } else {
        m_revengeTargetId = std::nullopt;
        m_revengeTimer = 0;
    }
}

LivingEntity* BeeEntity::getRevengeTarget() const
{
    if (!m_revengeTargetId.has_value()) {
        return nullptr;
    }
    // 从世界获取复仇目标
    IWorld* worldPtr = const_cast<IWorld*>(world());
    if (!worldPtr) {
        return nullptr;
    }
    Entity* entity = worldPtr->getEntity(m_revengeTargetId.value());
    if (!entity || !entity->isAlive()) {
        return nullptr;
    }
    return dynamic_cast<LivingEntity*>(entity);
}

void BeeEntity::updateAnger()
{
    i32 angerTime = getAngerTime();
    if (angerTime > 0) {
        setAngerTime(angerTime - 1);
        if (getAngerTime() == 0) {
            // 愤怒结束，清除攻击目标
            setAttackTarget(nullptr);
            m_attacking = false;
            m_targetPlayerId = 0;
            m_revengeTargetId = std::nullopt;
        }
    }
    // 更新复仇计时器
    if (m_revengeTimer > 0) {
        m_revengeTimer--;
    }
}

// ============================================================================
// 生命周期
// ============================================================================

void BeeEntity::setHivePos(const BlockPos& pos)
{
    m_hivePos = pos;
    m_hasHive = true;
}

void BeeEntity::setFlowerPos(const BlockPos& pos)
{
    m_flowerPos = pos;
    m_hasFlower = true;
}

bool BeeEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 检查物品是否在花朵标签中
    const Item* item = itemStack.getItem();
    if (item == nullptr) {
        return false;
    }
    return item->isIn(item::tag::ItemTags::FLOWERS());
}

std::unique_ptr<AnimalEntity> BeeEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    auto baby = std::make_unique<BeeEntity>(0);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置（在父体位置附近）
    baby->setPosition(x(), y(), z());

    return baby;
}

// ============================================================================
// 生命周期
// ============================================================================

void BeeEntity::tick()
{
    AnimalEntity::tick();

    // 更新愤怒计时器
    updateAnger();

    // 递减蜂巢相关冷却计时器（对应MC原版 Bee.aiStep()，仅服务端执行）
    if (m_world != nullptr && !m_world->isClientSide()) {
        if (m_stayOutOfHiveCountdown > 0) {
            --m_stayOutOfHiveCountdown;
        }
        if (m_remainingCooldownBeforeLocatingNewHive > 0) {
            --m_remainingCooldownBeforeLocatingNewHive;
        }
        if (m_remainingCooldownBeforeLocatingNewFlower > 0) {
            --m_remainingCooldownBeforeLocatingNewFlower;
        }
    }

    // 离巢后无花粉计时递增
    if (!hasNectar() && m_hasHive) {
        ++m_ticksWithoutNectarSinceExitingHive;
    }

    // 螫刺后逐渐死亡
    // 越久越容易死亡，最长存活 1200 tick = 60 秒
    if (m_hasStung) {
        ++m_timeSinceSting;
        // 每 5 tick 检查一次死亡概率
        if (m_timeSinceSting % 5 == 0 && m_world != nullptr) {
            // 概率随时间增加：MathHelper.clamp(1200 - timeSinceSting, 1, 1200)
            i32 deathChance = math::clamp(1200 - m_timeSinceSting, 1, 1200);

            // 获取随机数生成器
            math::Random& rng = m_world->getRandom();

            // rand.nextInt(deathChance) == 0 时死亡
            if (rng.nextInt(deathChance) == 0) {
                // 造成 GENERIC 伤害，伤害量为当前生命值
                auto damageSource = DamageSources::generic();
                hurt(damageSource, health());
            }
        }
    }

    // 水下溺水逻辑
    if (isInWater()) {
        ++m_underWaterTimer;
        if (m_underWaterTimer > 20 && m_world != nullptr) {
            // 开始溺水伤害
            auto damageSource = DamageSources::drown();
            hurt(damageSource, 1.0f);
        }
    } else {
        m_underWaterTimer = 0;
    }
}

void BeeEntity::registerGoals()
{
    // 调用父类方法注册基础动物 AI
    AnimalEntity::registerGoals();

    // 优先级越小越高

    // ========== Goal Selector (行为目标) ==========

    // 优先级 0: 蛰刺攻击（最高优先级）
    m_goalSelector.addGoal(0, std::make_unique<entity::ai::goal::BeeStingGoal>(this));

    // 优先级 1: 进入蜂巢
    m_goalSelector.addGoal(1, std::make_unique<entity::ai::goal::BeeEnterHiveGoal>(this));

    // 优先级 2: 繁殖
    m_goalSelector.addGoal(2, std::make_unique<entity::ai::goal::BreedGoal>(this, 1.0));

    // 优先级 3: 花朵诱惑（使用花朵物品）
    m_goalSelector.addGoal(3,
        std::make_unique<entity::ai::goal::TemptGoal>(
            this,
            1.25,
            [](const ItemStack& stack) -> bool {
                const Item* item = stack.getItem();
                return item != nullptr && item->isIn(item::tag::ItemTags::FLOWERS());
            },
            false));

    // 优先级 4: 授粉
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::BeePollinateGoal>(this));

    // 优先级 5: 跟随父母
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::FollowParentGoal>(this, 1.25));

    // 优先级 5: 更新蜂巢位置
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::BeeUpdateHiveGoal>(this));

    // 优先级 5: 寻找蜂巢
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::BeeFindHiveGoal>(this));

    // 优先级 6: 寻找花朵
    m_goalSelector.addGoal(6, std::make_unique<entity::ai::goal::BeeFindFlowerGoal>(this));

    // 优先级 7: 寻找授粉目标（农作物）
    m_goalSelector.addGoal(7, std::make_unique<entity::ai::goal::BeeFindPollinationTargetGoal>(this));

    // 优先级 8: 随机飞行
    m_goalSelector.addGoal(8, std::make_unique<entity::ai::goal::BeeWanderGoal>(this));

    // 优先级 9: 游泳
    m_goalSelector.addGoal(9, std::make_unique<entity::ai::goal::SwimGoal>(this));

    // ========== Target Selector (目标选择) ==========

    // 优先级 1: 愤怒复仇（被攻击时召唤其他蜜蜂）
    m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::BeeAngerGoal>(this));

    // 优先级 2: 攻击玩家
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::BeeAttackPlayerGoal>(this, 10));

    // 优先级 3: 重置愤怒
    m_targetSelector.addGoal(3, std::make_unique<entity::ai::goal::BeeResetAngerGoal>(this));
}

void BeeEntity::registerAttributes()
{
    // 调用父类方法
    AnimalEntity::registerAttributes();

    // MAX_HEALTH: 10.0, FLYING_SPEED: 0.6, MOVEMENT_SPEED: 0.3,
    // ATTACK_DAMAGE: 2.0, FOLLOW_RANGE: 48.0

    // 注意：AnimalEntity 不注册 FLYING_SPEED 和 ATTACK_DAMAGE
    // 需要先注册这些属性才能设置值
    m_attributes.registerAttribute(*entity::attribute::Attributes::flyingSpeed());
    m_attributes.registerAttribute(*entity::attribute::Attributes::attackDamage());

    // 设置属性值
    m_attributes.setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 10.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    m_attributes.setBaseValue(entity::attribute::Attributes::FLYING_SPEED, 0.6);
    m_attributes.setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0);
    m_attributes.setBaseValue(entity::attribute::Attributes::FOLLOW_RANGE, 48.0);
}

std::optional<ResourceLocation> BeeEntity::getAmbientSound() const
{
    // 蜜蜂无环境音，对齐原版 Bee.getAmbientSound 返回 null。
    // sounds.json 中无 entity.bee.ambient，飞行音由客户端循环音效实例处理。
    return std::nullopt;
}

// ============================================================================
// 蜂巢验证与交互
// ============================================================================

bool BeeEntity::isHiveValid() const
{
    return getBeehiveBlockEntity() != nullptr;
}

blockentity::BeehiveBlockEntity* BeeEntity::getBeehiveBlockEntity() const
{
    if (!m_hasHive) {
        return nullptr;
    }

    auto* worldPtr = const_cast<IWorld*>(world());
    if (!worldPtr) {
        return nullptr;
    }

    // 检查距离是否过远（超过48格）
    Vector3 beePos = position();
    f32 dx = static_cast<f32>(m_hivePos.x) + 0.5f - beePos.x;
    f32 dy = static_cast<f32>(m_hivePos.y) + 0.5f - beePos.y;
    f32 dz = static_cast<f32>(m_hivePos.z) + 0.5f - beePos.z;
    f32 distSq = dx * dx + dy * dy + dz * dz;
    if (distSq > 48.0f * 48.0f) {
        return nullptr;
    }

    // 检查该位置的方块是否仍是蜂巢
    const BlockState* state = worldPtr->getBlockState(m_hivePos);
    if (!state || !BlockTags::BEEHIVES().contains(*state)) {
        return nullptr;
    }

    // 检查方块实体是否存在
    auto* blockEntity = worldPtr->getBlockEntity(m_hivePos);
    if (!blockEntity || blockEntity->getType() != BlockEntityType::Beehive) {
        return nullptr;
    }

    return static_cast<blockentity::BeehiveBlockEntity*>(blockEntity);
}

bool BeeEntity::wantsToEnterHive() const
{
    if (m_stayOutOfHiveCountdown > 0) {
        return false;
    }
    if (m_pollinating) {
        return false;
    }
    if (hasStung()) {
        return false;
    }
    if (attackTarget() != nullptr) {
        return false;
    }

    // 检查是否满足进入蜂巢的条件：
    // 1. 有花粉 → 回巢存放
    // 2. 寻找花蜜超过 3600 tick（3分钟）→ 疲惫回巢
    // 3. 雨天或雷暴 → 天气恶劣回巢
    // 4. 夜间 → 天黑回巢
    bool shouldEnter = hasNectar();
    if (!shouldEnter) {
        shouldEnter = isTiredOfLookingForNectar();
    }
    if (!shouldEnter) {
        const IWorld* worldPtr = world();
        if (worldPtr != nullptr) {
            shouldEnter = worldPtr->isRaining() || worldPtr->isThundering() || !worldPtr->isDaytime();
        }
    }

    if (!shouldEnter) {
        return false;
    }

    // 检查蜂巢附近是否有火
    return !isHiveNearFire();
}

bool BeeEntity::isHiveNearFire() const
{
    if (!m_hasHive) {
        return false;
    }
    auto* worldPtr = const_cast<IWorld*>(world());
    if (!worldPtr) {
        return false;
    }
    return blockentity::BeehiveBlockEntity::isFireNearby(*worldPtr, m_hivePos);
}

void BeeEntity::dropHive()
{
    m_hivePos = BlockPos::zero();
    m_hasHive = false;
    m_stayOutOfHiveCountdown = 200; // 10秒冷却
}

// ============================================================================
// 导航辅助
// ============================================================================

bool BeeEntity::pathfindRandomlyTowards(const BlockPos& targetPos)
{
    // 对应 MC 1.21.11 Bee.pathfindRandomlyTowards()
    // 将目标位置转换为底部中心坐标
    f64 targetX = static_cast<f64>(targetPos.x) + 0.5;
    f64 targetY = static_cast<f64>(targetPos.y);
    f64 targetZ = static_cast<f64>(targetPos.z) + 0.5;

    // 计算Y轴高度差偏移
    math::Vector3f beePos = position();
    i32 yDiff = static_cast<i32>(targetY - beePos.y);
    i32 yOffset = 0;
    if (yDiff > 2) {
        yOffset = 4;
    } else if (yDiff < -2) {
        yOffset = -4;
    }

    // 计算搜索范围：近距离时缩小范围
    // 对应 MC 的 i1 = distManhattan, if (i1 < 15) { k = i1/2; l = i1/2; }
    i32 k = 6; // xzRange
    i32 l = 8; // yRange
    i32 manhattanDist =
        static_cast<i32>(std::abs(beePos.x - targetX) + std::abs(beePos.y - targetY) + std::abs(beePos.z - targetZ));
    if (manhattanDist < 15) {
        k = manhattanDist / 2;
        l = manhattanDist / 2;
    }

    // 使用 AirRandomPos.getPosTowards 生成目标方向 PI/10 (18度) 锥形内的随机空中位置
    math::Vector3f targetVec(static_cast<f32>(targetX), static_cast<f32>(targetY), static_cast<f32>(targetZ));
    math::Vector3f airPos;
    if (entity::ai::util::RandomPositionGenerator::findAirPositionTowards(
            this, k, l, yOffset, targetVec, math::PI / 10.0f, airPos)) {
        if (auto* nav = navigator()) {
            // MC 1.21.11: navigation.setMaxVisitedNodesMultiplier(0.5F)
            // 漂移飞行时降低寻路开销，将搜索节点数减半
            nav->setMaxVisitedNodesMultiplier(0.5f);
            return nav->moveTo(airPos.x, airPos.y, airPos.z, 1.0);
        }
    }

    return false;
}

bool BeeEntity::pathfindDirectlyTowards(const BlockPos& targetPos)
{
    // 对应 MC 1.21.11 BeeGoToHiveGoal.pathfindDirectlyTowards()
    // 近距离（16格内）使用精确导航
    // MC: int i = closerThan(pos, 3) ? 1 : 2; navigation.setMaxVisitedNodesMultiplier(10.0F);
    // 使用距离决定速度倍率：3格内用1.0，否则用2.0
    math::Vector3f beePos = position();
    f64 dx = beePos.x - (static_cast<f64>(targetPos.x) + 0.5);
    f64 dy = beePos.y - static_cast<f64>(targetPos.y);
    f64 dz = beePos.z - (static_cast<f64>(targetPos.z) + 0.5);
    f64 distSq = dx * dx + dy * dy + dz * dz;

    f64 speed = distSq < 9.0 ? 1.0 : 2.0;

    if (auto* nav = navigator()) {
        // MC 1.21.11: navigation.setMaxVisitedNodesMultiplier(10.0F)
        // 精确导航时提高寻路精度，将搜索节点数扩大10倍确保路径可达
        nav->setMaxVisitedNodesMultiplier(10.0f);
        bool hasPath = nav->moveTo(static_cast<f64>(targetPos.x) + 0.5,
            static_cast<f64>(targetPos.y) + 0.5,
            static_cast<f64>(targetPos.z) + 0.5,
            speed);
        return hasPath && nav->hasPath();
    }
    return false;
}

} // namespace mc
