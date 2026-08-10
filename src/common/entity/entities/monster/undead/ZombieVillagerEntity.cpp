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

#include "ZombieVillagerEntity.hpp"
#include "../../../../item/core/ItemStack.hpp"
#include "../../../../item/enchantment/EnchantmentHelper.hpp"
#include "../../../../sound/SoundEvents.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../world/IWorld.hpp"
#include "../../../../world/block/BlockPos.hpp"
#include "../../../../world/block/blocks/functional/BedBlock.hpp"
#include "../../../combat/DifficultyInstance.hpp"
#include "../../../core/EntityRegistry.hpp"
#include "../../../core/LivingEntity.hpp"
#include "../../../effect/EffectInstance.hpp"
#include "../../../effect/EffectType.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/entities/monster/MonsterEntity.hpp"
#include "common/entity/entities/monster/undead/ZombieEntity.hpp"
#include "common/entity/entities/villager/AbstractVillagerEntity.hpp"
#include "common/entity/entities/villager/VillagerEntity.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/spawn/EntitySpawnPlacementRegistry.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// 静态数据参数初始化
// ============================================================================

entity::DataParameter<bool> ZombieVillagerEntity::CONVERTING_PARAM = entity::EntityDataManager::createKey<bool>();

// 单一复合字段对齐 vanilla ZombieVillager.DATA_VILLAGER_DATA(VillagerData, serializerId=18)
entity::DataParameter<entity::VillagerDataValue> ZombieVillagerEntity::VILLAGER_DATA_PARAM =
    entity::EntityDataManager::createKey<entity::VillagerDataValue>();

const entity::EntityClassInfo& ZombieVillagerEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"ZombieVillagerEntity", &ZombieEntity::classInfo()};
    return s_classInfo;
}

// ============================================================================
// 常量
// ============================================================================

namespace {
// 基础治愈时间范围（ticks）
constexpr i32 CONVERSION_TIME_MIN = 3600; // 3分钟
constexpr i32 CONVERSION_TIME_MAX = 6000; // 5分钟

// 力量效果加速：每级减少 10% 的治愈时间
constexpr f32 STRENGTH_SPEEDUP_PER_LEVEL = 0.1f;

// 铁栏杆/床加速检测范围和概率
constexpr i32 SPEEDUP_CHECK_RANGE = 4;
constexpr f32 SPEEDUP_CHANCE = 0.3f;
constexpr i32 SPEEDUP_MAX_BONUS = 14; // 最多加速14次

// 恶心效果持续时间（治愈后）
constexpr i32 NAUSEA_DURATION = 200; // 10秒
} // namespace

// ============================================================================
// 构造函数
// ============================================================================

ZombieVillagerEntity::ZombieVillagerEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : ZombieEntity(id, registry)
{
    // 僵尸村民比普通僵尸慢
    // 职业随机设置（在 VanillaEntities 中设置）

    // 显式调用 registerData() 以注册僵尸村民特有的数据参数
    // 注意：由于 C++ 虚函数在基类构造函数中的行为，
    // Entity 构造函数调用的是 Entity::registerData() 而非派生类版本。
    // 因此必须在派生类构造函数中显式调用 registerData()。
    // registerData() 会先调用父类版本，确保参数按继承链正确注册。
    registerData();

    // 补调 registerGoals / registerAttributes：ZombieEntity 构造调基类版（vtable 指向 ZombieEntity），
    // 派生 override 永不执行，须在派生类构造显式调用。ZombieVillager 的 registerGoals 加专属目标，
    // registerAttributes 设僵尸村民移速等。
    registerGoals();
    registerAttributes();
}

std::unique_ptr<Entity> ZombieVillagerEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    return std::make_unique<ZombieVillagerEntity>(EntityInstanceId(0), registry);
}

// ============================================================================
// 数据同步
// ============================================================================

void ZombieVillagerEntity::registerData()
{
    ZombieEntity::registerData();

    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册僵尸村民特有的数据参数
    // 对齐 vanilla ZombieVillager.defineSynchedData: CONVERTING(Boolean,id19) + DATA_VILLAGER_DATA(VillagerData,id20)
    m_dataManager.registerParam(CONVERTING_PARAM, false);
    m_dataManager.registerParam(VILLAGER_DATA_PARAM,
        entity::VillagerDataValue{
            static_cast<i32>(entity::VillagerType::Plains), static_cast<i32>(entity::VillagerProfession::None), 1});
}

void ZombieVillagerEntity::syncMetadataFromDataManager()
{
    ZombieEntity::syncMetadataFromDataManager();

    // 从数据管理器同步治愈状态
    m_converting = m_dataManager.get<bool>(CONVERTING_PARAM);

    // 从数据管理器同步村民数据（单一复合字段 VillagerDataValue）
    const auto vd = m_dataManager.get<entity::VillagerDataValue>(VILLAGER_DATA_PARAM);
    m_villagerData.setType(static_cast<entity::VillagerType>(vd.type));
    m_villagerData.setProfession(static_cast<entity::VillagerProfession>(vd.profession));
    m_villagerData.setLevel(vd.level);
}

void ZombieVillagerEntity::setVillagerData(const entity::VillagerData& data)
{
    m_villagerData = data;

    // 同步到数据管理器（单一复合字段,对齐 vanilla DATA_VILLAGER_DATA wire 类型）
    m_dataManager.set(VILLAGER_DATA_PARAM,
        entity::VillagerDataValue{static_cast<i32>(data.type()), static_cast<i32>(data.profession()), data.level()});
}

void ZombieVillagerEntity::setProfession(entity::VillagerProfession profession)
{
    m_villagerData.setProfession(profession);
    // 经 setVillagerData 复合写回,保证 wire 上 type/profession/level 三段一致
    m_dataManager.set(VILLAGER_DATA_PARAM,
        entity::VillagerDataValue{
            static_cast<i32>(m_villagerData.type()), static_cast<i32>(profession), m_villagerData.level()});
}

void ZombieVillagerEntity::setVillagerType(entity::VillagerType type)
{
    m_villagerData.setType(type);
    m_dataManager.set(VILLAGER_DATA_PARAM,
        entity::VillagerDataValue{
            static_cast<i32>(type), static_cast<i32>(m_villagerData.profession()), m_villagerData.level()});
}

void ZombieVillagerEntity::setTradingLevel(i32 level)
{
    m_villagerData.setLevel(level);
    m_dataManager.set(VILLAGER_DATA_PARAM,
        entity::VillagerDataValue{
            static_cast<i32>(m_villagerData.type()), static_cast<i32>(m_villagerData.profession()), level});
}

void ZombieVillagerEntity::setTradingExperience(i32 exp)
{
    m_villagerData.setExperience(exp);
    // 注意：经验值不需要同步到客户端，仅服务端保存
}

// ============================================================================
// 治愈系统
// ============================================================================

void ZombieVillagerEntity::setConversionTime(i32 time)
{
    m_conversionTime = time;
    m_converting = time > 0;

    // 同步到数据管理器
    m_dataManager.set(CONVERTING_PARAM, m_converting);
}

void ZombieVillagerEntity::startConverting(const std::string& starterUuid, i32 time)
{
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
    // 根据难度添加力量效果：简单无力量，普通力量I，困难力量II
    i32 strengthLevel = 0;
    if (m_world != nullptr) {
        i32 difficultyId = static_cast<i32>(m_world->difficulty());
        strengthLevel = std::max(difficultyId - 1, 0);
    }
    if (strengthLevel > 0) {
        addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Strength,
            time,
            strengthLevel - 1, // amplifier = level - 1
            false,             // ambient
            true,              // visible
            true               // showIcon
            ));
    }

    // 广播治愈状态（客户端播放音效）
    // world()->broadcastEntityStatus(id(), static_cast<u8>(16));
}

void ZombieVillagerEntity::stopConverting()
{
    m_converting = false;
    m_conversionTime = 0;
    m_conversionStarterUuid.clear();

    // 同步到数据管理器
    m_dataManager.set(CONVERTING_PARAM, false);

    // 移除力量效果
    removeEffect(entity::effect::EffectType::Strength);
}

void ZombieVillagerEntity::finishConverting()
{
    if (m_world == nullptr) {
        spdlog::warn("ZombieVillagerEntity::finishConverting: world is null");
        return;
    }

    // 创建村民实体
    auto& registry = entity::EntityRegistry::instance();
    const entity::EntityType* villagerType = registry.getType("minecraft:villager");

    // 实体自身的 ECS registry 句柄（构造时绑定于 EntityContext，永远非空）。
    // 注意局部变量名须与方法名 ecsRegistry() 区分（此处已有名为 registry 的类型注册表引用）。
    auto* ecsReg = &ecsRegistry();

    std::unique_ptr<Entity> newEntity;
    if (villagerType && villagerType->canSummon()) {
        newEntity = villagerType->create(m_world, *ecsReg);
    } else {
        // 回退：直接创建 VillagerEntity
        newEntity = std::make_unique<entity::VillagerEntity>(EntityInstanceId(0), *ecsReg);
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
    villager->setPosition(m_builtIn.stateVector->m_pos);
    villager->setRotation(m_builtIn.rotation->m_rot.x, m_builtIn.rotation->m_rot.y);

    // 继承村民数据
    villager->setVillagerData(m_villagerData);

    // 处理装备（对应 MC Java 的 ZombieVillager.finishConversion 中的 dropPreservedEquipment 逻辑）
    //
    // MC Java 的逻辑：
    // 1. 调用 dropPreservedEquipment()，谓词为 "没有绑定诅咒的物品"
    // 2. 不满足谓词的物品（有绑定诅咒）→ 保留在实体上，其槽位被返回
    // 3. 满足谓词且保留的物品（掉落概率 > 1.0）→ 掉落在地上
    // 4. 满足谓词但不保留的物品（默认 8.5% 掉落概率）→ 静默消失
    // 5. 对返回的槽位中的物品（绑定诅咒物品），转移到新实体
    //
    // 然后清空原实体的所有装备（防止死亡时重复掉落）

    // 谓词：物品没有绑定诅咒 → 可以被处理（掉落或消失）
    auto noBindingCurse = [](const ItemStack& stack) -> bool {
        return !item::enchant::EnchantmentHelper::hasBindingCurse(stack);
    };

    // 掉落保留装备，获取需要转移到新实体的槽位（绑定诅咒物品的槽位）
    std::vector<EquipmentSlot> preservedSlots = dropPreservedEquipment(noBindingCurse);

    // 将绑定诅咒的装备转移到村民的对应槽位
    for (EquipmentSlot slot : preservedSlots) {
        const ItemStack& equipment = getEquipment(slot);
        if (!equipment.isEmpty()) {
            villager->setEquipment(slot, equipment);
        }
    }

    // 清空僵尸村民的所有装备（防止重复掉落）
    // 注意：dropPreservedEquipment 已经清空了保留状态的槽位，
    // 这里清空剩余的（绑定诅咒和非保留非空槽位）
    for (size_t i = 0; i < static_cast<size_t>(EquipmentSlot::Count); ++i) {
        setEquipment(static_cast<EquipmentSlot>(i), ItemStack());
    }

    // 设置婴儿状态
    villager->setChild(isBaby());

    // 对村民调用 finalizeSpawn 进行基于难度的初始化（使用位置感知的区域难度）
    {
        entity::combat::DifficultyInstance difficultyInstance = entity::combat::DifficultyInstance::at(*m_world,
            BlockPos(static_cast<i32>(std::floor(x())), static_cast<i32>(y()), static_cast<i32>(std::floor(z()))));
        villager->finalizeSpawn(*m_world, difficultyInstance, world::spawn::SpawnReason::Conversion);
    }

    // 释放所有权并生成到世界
    newEntity.release();
    EntityInstanceId newId = m_world->spawnEntity(std::unique_ptr<Entity>(villager));

    if (newId == 0) {
        spdlog::error("ZombieVillagerEntity::finishConverting: failed to spawn villager entity");
        delete villager;
        return;
    }

    // 给村民添加恶心效果（10秒）
    villager->addEffect(entity::effect::EffectInstance(entity::effect::EffectType::Nausea,
        NAUSEA_DURATION,
        0,     // amplifier = 0 (level I)
        false, // ambient
        true,  // visible
        true   // showIcon
        ));

    // 播放治愈音效
    playSound(SoundEvents::ENTITY_ZOMBIE_VILLAGER_CURE, 1.0f, 1.0f);

    // 广播治愈事件（1027 = 僵尸村民治愈）
    // m_world->broadcastEntityStatus(m_id, 1027);

    // 触发成就 CriteriaTriggers.CURED_ZOMBIE_VILLAGER
    // 村庄声望更新在 AdvancementEventHandler::onCuredZombieVillager() 中处理
    if (m_world && !m_conversionStarterUuid.empty()) {
        m_world->onZombieVillagerCured(m_conversionStarterUuid, this, villager);
    }

    // 移除僵尸村民
    remove();
}

i32 ZombieVillagerEntity::getConversionProgress() const
{
    if (m_world == nullptr) {
        return 1;
    }

    i32 progress = 1;

    // 在 4x4x4 范围内检测铁栏杆和床
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
                // 床也和铁栏杆一样加速治愈
                else if (blocks::BedBlock::isBed(*m_world, checkPos)) {
                    // 床同样有 30% 概率增加进度
                    if (rng.nextFloat() < SPEEDUP_CHANCE) {
                        ++progress;
                    }
                    ++speedupCount;
                }
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

bool ZombieVillagerEntity::canDespawn(f64 distanceToClosestPlayer) const
{
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

void ZombieVillagerEntity::tick()
{
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

void ZombieVillagerEntity::registerGoals()
{
    ZombieEntity::registerGoals();

    // 僵尸村民没有额外 AI（与普通僵尸相同）
}

void ZombieVillagerEntity::registerAttributes()
{
    ZombieEntity::registerAttributes();

    // 僵尸村民的属性与普通僵尸相同
}

// ============================================================================
// 声音
// ============================================================================

std::optional<ResourceLocation> ZombieVillagerEntity::getAmbientSound() const
{
    return SoundEvents::ENTITY_ZOMBIE_VILLAGER_AMBIENT;
}

std::optional<ResourceLocation> ZombieVillagerEntity::getHurtSound(DamageSource& /*source*/) const
{
    return SoundEvents::ENTITY_ZOMBIE_VILLAGER_HURT;
}

std::optional<ResourceLocation> ZombieVillagerEntity::getDeathSound() const
{
    return SoundEvents::ENTITY_ZOMBIE_VILLAGER_DEATH;
}

std::optional<ResourceLocation> ZombieVillagerEntity::getStepSound() const
{
    return SoundEvents::ENTITY_ZOMBIE_VILLAGER_STEP;
}

// ============================================================================
// 属性
// ============================================================================

f32 ZombieVillagerEntity::eyeHeight() const
{
    return isBaby() ? 0.93f : 1.79f;
}

} // namespace mc
