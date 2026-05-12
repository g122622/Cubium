#include "ZombieVillagerEntity.hpp"
#include "../../../attribute/Attributes.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../effect/EffectInstance.hpp"
#include "../../../effect/EffectType.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/enchantment/EnchantmentHelper.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../../world/block/VanillaBlocks.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../utils/ItemDropHelper.hpp"
#include "../../../core/LivingEntity.hpp"
#include <memory>
#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// 静态数据参数初始化
// ============================================================================

entity::DataParameter<bool> ZombieVillagerEntity::CONVERTING_PARAM =
    entity::EntityDataManager::createKey<bool>();

entity::DataParameter<i32> ZombieVillagerEntity::VILLAGER_TYPE_PARAM =
    entity::EntityDataManager::createKey<i32>();

entity::DataParameter<i32> ZombieVillagerEntity::VILLAGER_PROFESSION_PARAM =
    entity::EntityDataManager::createKey<i32>();

entity::DataParameter<i32> ZombieVillagerEntity::VILLAGER_LEVEL_PARAM =
    entity::EntityDataManager::createKey<i32>();

// ============================================================================
// 常量
// ============================================================================

namespace {
    // 基础治愈时间范围（ticks）
    constexpr i32 CONVERSION_TIME_MIN = 3600;   // 3分钟
    constexpr i32 CONVERSION_TIME_MAX = 6000;   // 5分钟

    // 力量效果加速：每级减少 10% 的治愈时间
    constexpr f32 STRENGTH_SPEEDUP_PER_LEVEL = 0.1f;

    // 铁栏杆/床加速检测范围和概率
    constexpr i32 SPEEDUP_CHECK_RANGE = 4;
    constexpr f32 SPEEDUP_CHANCE = 0.3f;
    constexpr i32 SPEEDUP_MAX_BONUS = 14;  // 最多加速14次

    // 恶心效果持续时间（治愈后）
    constexpr i32 NAUSEA_DURATION = 200;  // 10秒
}

// ============================================================================
// 构造函数
// ============================================================================

ZombieVillagerEntity::ZombieVillagerEntity(LegacyEntityType type, EntityId id)
    : ZombieEntity(type, id)
{
    // MC 1.16.5: 僵尸村民比普通僵尸慢
    // 职业随机设置（在 VanillaEntities 中设置）

    // 显式调用 registerData() 以注册僵尸村民特有的数据参数
    // 注意：由于 C++ 虚函数在基类构造函数中的行为，
    // Entity 构造函数调用的是 Entity::registerData() 而非派生类版本。
    // 因此必须在派生类构造函数中显式调用 registerData()。
    // registerData() 会先调用父类版本，确保参数按继承链正确注册。
    registerData();
}

std::unique_ptr<Entity> ZombieVillagerEntity::create(IWorld* /*world*/) {
    return std::make_unique<ZombieVillagerEntity>(LegacyEntityType::Unknown, 0);
}

// ============================================================================
// 数据同步
// ============================================================================

void ZombieVillagerEntity::registerData() {
    ZombieEntity::registerData();

    // 注册僵尸村民特有的数据参数
    m_dataManager.registerParam(CONVERTING_PARAM, false);
    m_dataManager.registerParam(VILLAGER_TYPE_PARAM, static_cast<i32>(entity::VillagerType::Plains));
    m_dataManager.registerParam(VILLAGER_PROFESSION_PARAM, static_cast<i32>(entity::VillagerProfession::None));
    m_dataManager.registerParam(VILLAGER_LEVEL_PARAM, 1);
}

void ZombieVillagerEntity::syncMetadataFromDataManager() {
    ZombieEntity::syncMetadataFromDataManager();

    // 从数据管理器同步治愈状态
    m_converting = m_dataManager.get<bool>(CONVERTING_PARAM);

    // 从数据管理器同步村民数据
    m_villagerData.setType(static_cast<entity::VillagerType>(m_dataManager.get<i32>(VILLAGER_TYPE_PARAM)));
    m_villagerData.setProfession(static_cast<entity::VillagerProfession>(m_dataManager.get<i32>(VILLAGER_PROFESSION_PARAM)));
    m_villagerData.setLevel(m_dataManager.get<i32>(VILLAGER_LEVEL_PARAM));
}

void ZombieVillagerEntity::setVillagerData(const entity::VillagerData& data) {
    m_villagerData = data;

    // 同步到数据管理器
    m_dataManager.set(VILLAGER_TYPE_PARAM, static_cast<i32>(data.type()));
    m_dataManager.set(VILLAGER_PROFESSION_PARAM, static_cast<i32>(data.profession()));
    m_dataManager.set(VILLAGER_LEVEL_PARAM, data.level());
}

void ZombieVillagerEntity::setProfession(entity::VillagerProfession profession) {
    m_villagerData.setProfession(profession);
    m_dataManager.set(VILLAGER_PROFESSION_PARAM, static_cast<i32>(profession));
}

void ZombieVillagerEntity::setVillagerType(entity::VillagerType type) {
    m_villagerData.setType(type);
    m_dataManager.set(VILLAGER_TYPE_PARAM, static_cast<i32>(type));
}

void ZombieVillagerEntity::setTradingLevel(i32 level) {
    m_villagerData.setLevel(level);
    m_dataManager.set(VILLAGER_LEVEL_PARAM, level);
}

void ZombieVillagerEntity::setTradingExperience(i32 exp) {
    m_villagerData.setExperience(exp);
    // 注意：经验值不需要同步到客户端，仅服务端保存
}

// ============================================================================
// 治愈系统
// ============================================================================

void ZombieVillagerEntity::setConversionTime(i32 time) {
    m_conversionTime = time;
    m_converting = time > 0;

    // 同步到数据管理器
    m_dataManager.set(CONVERTING_PARAM, m_converting);
}

void ZombieVillagerEntity::startConverting(const std::string& starterUuid, i32 time) {
    // 计算治愈时间
    if (time < 0) {
        // 随机时间：3600-6000 ticks (3-5分钟)
        math::Random rng(ticksExisted());
        time = CONVERSION_TIME_MIN + rng.nextInt(CONVERSION_TIME_MAX - CONVERSION_TIME_MIN + 1);
    }

    m_conversionStarterUuid = starterUuid;
    m_conversionTime = time;
    m_converting = true;

    // 同步到数据管理器
    m_dataManager.set(CONVERTING_PARAM, true);

    // 移除虚弱效果
    removeEffect(entity::effect::EffectType::Weakness);

    // 添加力量效果（持续整个治愈时间）
    // MC 1.16.5: 根据难度添加力量效果
    // 简单: 无力量, 普通: 力量 I, 困难: 力量 II
    i32 strengthLevel = 0;  // TODO: 从世界获取难度
    if (strengthLevel > 0) {
        addEffect(entity::effect::EffectInstance(
            entity::effect::EffectType::Strength,
            time,
            strengthLevel - 1,  // amplifier = level - 1
            false,  // ambient
            true,   // visible
            true    // showIcon
        ));
    }

    // 广播治愈状态（客户端播放音效）
    // world()->broadcastEntityStatus(id(), static_cast<u8>(16));

    spdlog::debug("ZombieVillagerEntity::startConverting: started conversion, time={} ticks, starter={}",
                  time, starterUuid.empty() ? "none" : starterUuid);
}

void ZombieVillagerEntity::stopConverting() {
    m_converting = false;
    m_conversionTime = 0;
    m_conversionStarterUuid.clear();

    // 同步到数据管理器
    m_dataManager.set(CONVERTING_PARAM, false);

    // 移除力量效果
    removeEffect(entity::effect::EffectType::Strength);

    spdlog::debug("ZombieVillagerEntity::stopConverting: stopped conversion");
}

void ZombieVillagerEntity::finishConverting() {
    if (m_world == nullptr) {
        spdlog::warn("ZombieVillagerEntity::finishConverting: world is null");
        return;
    }

    spdlog::debug("ZombieVillagerEntity::finishConverting: converting to villager at ({}, {}, {})",
                  x(), y(), z());

    // 创建村民实体
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* villagerType = registry.getType("minecraft:villager");

    std::unique_ptr<Entity> newEntity;
    if (villagerType && villagerType->canSummon()) {
        newEntity = villagerType->create(m_world);
    } else {
        // 回退：直接创建 VillagerEntity
        newEntity = std::make_unique<entity::VillagerEntity>(LegacyEntityType::Unknown, 0);
    }

    if (!newEntity) {
        spdlog::error("ZombieVillagerEntity::finishConverting: failed to create villager entity");
        return;
    }

    // 转换为 VillagerEntity
    entity::VillagerEntity* villager = dynamic_cast<entity::VillagerEntity*>(newEntity.get());
    if (!villager) {
        spdlog::error("ZombieVillagerEntity::finishConverting: created entity is not a VillagerEntity");
        return;
    }

    // 设置位置和旋转
    villager->setPosition(m_position);
    villager->setRotation(m_yaw, m_pitch);

    // 继承村民数据
    villager->setVillagerData(m_villagerData);

    // 处理装备
    math::Random rng(ticksExisted());

    for (size_t i = 0; i < static_cast<size_t>(EquipmentSlot::Count); ++i) {
        EquipmentSlot slot = static_cast<EquipmentSlot>(i);
        const ItemStack& equipment = getEquipment(slot);

        if (equipment.isEmpty()) {
            continue;
        }

        // 检查绑定诅咒
        bool hasBindingCurse = item::enchant::EnchantmentHelper::hasBindingCurse(equipment);

        if (hasBindingCurse) {
            // 绑定诅咒的装备转移到村民的对应槽位
            // MC 1.16.5: 使用 index + 300 的槽位
            villager->setEquipment(slot, equipment);
        } else {
            // 其他装备根据掉落概率丢弃
            // MC 1.16.5: 只有 dropChance > 1.0 的装备才会丢弃
            // 简化实现：直接丢弃所有非绑定装备
            ItemDropHelper::spawnItemAtEntity(
                this,
                equipment,
                0.5f,  // Y offset
                rng,
                ItemDropHelper::DEFAULT_PICKUP_DELAY
            );
        }
    }

    // 清空僵尸村民的装备（防止重复掉落）
    for (size_t i = 0; i < static_cast<size_t>(EquipmentSlot::Count); ++i) {
        setEquipment(static_cast<EquipmentSlot>(i), ItemStack());
    }

    // 设置婴儿状态
    villager->setChild(isBaby());

    // 释放所有权并生成到世界
    newEntity.release();
    EntityId newId = m_world->spawnEntity(std::unique_ptr<Entity>(villager));

    if (newId == 0) {
        spdlog::error("ZombieVillagerEntity::finishConverting: failed to spawn villager entity");
        delete villager;
        return;
    }

    // 给村民添加恶心效果（10秒）
    villager->addEffect(entity::effect::EffectInstance(
        entity::effect::EffectType::Nausea,
        NAUSEA_DURATION,
        0,  // amplifier = 0 (level I)
        false,  // ambient
        true,   // visible
        true    // showIcon
    ));

    // 播放治愈音效
    playSound(SoundEvents::ENTITY_ZOMBIE_VILLAGER_CURE, 1.0f, 1.0f);

    // 广播治愈事件（1027 = 僵尸村民治愈）
    // m_world->broadcastEntityStatus(m_id, 1027);

    // 触发成就 CriteriaTriggers.CURED_ZOMBIE_VILLAGER
    // 参考 MC 1.16.5: ZombieVillagerEntity.finishConverting()
    if (m_world && !m_conversionStarterUuid.empty()) {
        m_world->onZombieVillagerCured(m_conversionStarterUuid, this, villager);
    }

    // TODO: 更新村庄声望 (MajorPositive)

    // 移除僵尸村民
    remove();

    spdlog::debug("ZombieVillagerEntity::finishConverting: successfully converted to villager id={}", newId);
}

i32 ZombieVillagerEntity::getConversionProgress() const {
    if (m_world == nullptr) {
        return 1;
    }

    i32 progress = 1;

    // MC 1.16.5: 在 4x4x4 范围内检测铁栏杆和床
    // 有 1% 的概率执行检测（每tick只有1%概率）
    math::Random rng(ticksExisted());
    if (rng.nextFloat() >= 0.01f) {
        return progress;
    }

    i32 speedupCount = 0;

    // 遍历 4x4x4 范围
    i32 centerX = static_cast<i32>(std::floor(x()));
    i32 centerY = static_cast<i32>(std::floor(y()));
    i32 centerZ = static_cast<i32>(std::floor(z()));

    for (i32 dx = -SPEEDUP_CHECK_RANGE; dx <= SPEEDUP_CHECK_RANGE && speedupCount < SPEEDUP_MAX_BONUS; ++dx) {
        for (i32 dy = -SPEEDUP_CHECK_RANGE; dy <= SPEEDUP_CHECK_RANGE && speedupCount < SPEEDUP_MAX_BONUS; ++dy) {
            for (i32 dz = -SPEEDUP_CHECK_RANGE; dz <= SPEEDUP_CHECK_RANGE && speedupCount < SPEEDUP_MAX_BONUS; ++dz) {
                BlockPos checkPos(centerX + dx, centerY + dy, centerZ + dz);

                const BlockState* blockState = m_world->getBlockState(checkPos);
                if (blockState == nullptr) {
                    continue;
                }

                // 检查是否是铁栏杆
                const Block& block = blockState->getBlock();
                if (&block == VanillaBlocks::IRON_BARS) {
                    // 有 30% 概率增加进度
                    if (rng.nextFloat() < SPEEDUP_CHANCE) {
                        ++progress;
                    }
                    ++speedupCount;
                }
                // TODO: 检查是否是床（需要 BedBlock 类实现后）
                // BedBlock 也应该加速治愈
            }
        }
    }

    // 力量效果加速
    i32 strengthLevel = getEffectLevel(entity::effect::EffectType::Strength);
    if (strengthLevel > 0) {
        // 每级减少 10% 的治愈时间，相当于进度增加
        progress += static_cast<i32>(progress * STRENGTH_SPEEDUP_PER_LEVEL * strengthLevel);
    }

    return progress;
}

bool ZombieVillagerEntity::canDespawn(f64 distanceToClosestPlayer) const {
    // 正在治愈的僵尸村民不能消失
    if (m_converting) {
        return false;
    }

    // 有交易经验的僵尸村民不能消失（表示曾经是交易过的村民）
    if (m_villagerData.experience() > 0) {
        return false;
    }

    // 其他情况由父类决定
    return MonsterEntity::canDespawn(distanceToClosestPlayer);
}

// ============================================================================
// 生命周期
// ============================================================================

void ZombieVillagerEntity::tick() {
    ZombieEntity::tick();

    // 更新治愈倒计时
    if (m_converting && m_conversionTime > 0) {
        // 计算治愈进度（考虑铁栏杆/床和力量效果）
        i32 progress = getConversionProgress();
        m_conversionTime -= progress;

        if (m_conversionTime <= 0) {
            finishConverting();
        }
    }
}

// ============================================================================
// AI 目标
// ============================================================================

void ZombieVillagerEntity::registerGoals() {
    ZombieEntity::registerGoals();

    // 僵尸村民没有额外 AI（与普通僵尸相同）
}

void ZombieVillagerEntity::registerAttributes() {
    ZombieEntity::registerAttributes();

    // 僵尸村民的属性与普通僵尸相同
}

// ============================================================================
// 声音
// ============================================================================

std::optional<ResourceLocation> ZombieVillagerEntity::getAmbientSound() const {
    return SoundEvents::ENTITY_ZOMBIE_VILLAGER_AMBIENT;
}

std::optional<ResourceLocation> ZombieVillagerEntity::getHurtSound(DamageSource& /*source*/) const {
    return SoundEvents::ENTITY_ZOMBIE_VILLAGER_HURT;
}

std::optional<ResourceLocation> ZombieVillagerEntity::getDeathSound() const {
    return SoundEvents::ENTITY_ZOMBIE_VILLAGER_DEATH;
}

std::optional<ResourceLocation> ZombieVillagerEntity::getStepSound() const {
    return SoundEvents::ENTITY_ZOMBIE_VILLAGER_STEP;
}

// ============================================================================
// 属性
// ============================================================================

f32 ZombieVillagerEntity::eyeHeight() const {
    return isBaby() ? 0.93f : 1.79f;
}

} // namespace mc
