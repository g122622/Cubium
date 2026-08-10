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

#include "WolfEntity.hpp"

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/goal/GoalSelector.hpp"
#include "common/entity/ai/goal/goals/AvoidEntityGoal.hpp"
#include "common/entity/ai/goal/goals/BreedGoal.hpp"
#include "common/entity/ai/goal/goals/FollowParentGoal.hpp"
#include "common/entity/ai/goal/goals/LookAtGoal.hpp"
#include "common/entity/ai/goal/goals/MeleeAttackGoal.hpp"
#include "common/entity/ai/goal/goals/TemptGoal.hpp"
#include "common/entity/ai/goal/goals/interact/TameableGoals.hpp"
#include "common/entity/ai/goal/goals/movement/MovementGoals.hpp"
#include "common/entity/ai/goal/goals/target/TargetGoals.hpp"
#include "common/entity/ai/pathfinding/PathNavigator.hpp"
#include "common/entity/attribute/Attributes.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntityClassRegistry.hpp"
#include "common/entity/core/EntityDataManager.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/damage/tag/DamageTypeTags.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/entity/entities/monster/basic/CreeperEntity.hpp"
#include "common/entity/entities/monster/nether/NetherEntities.hpp"
#include "common/entity/entities/passive/basic/RabbitEntity.hpp"
#include "common/entity/entities/passive/basic/SheepEntity.hpp"
#include "common/entity/entities/passive/horse/AbstractHorseEntity.hpp"
#include "common/entity/entities/passive/horse/LlamaEntity.hpp"
#include "common/entity/entities/passive/special/FoxEntity.hpp"
#include "common/entity/entities/passive/special/TurtleEntity.hpp"
#include "common/entity/entities/passive/tamable/Crackiness.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/entity/serialization/EntityNbtKeys.hpp"
#include "common/entity/serialization/NbtHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/armor/DyeableArmorItem.hpp"
#include "common/item/tag/ItemTags.hpp"
#include "common/network/protocol/EntityEvents.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/color/DyeColor.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/gameevent/GameEvents.hpp"

#include <unordered_map>

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>

namespace mc {

// ==================== 静态成员初始化 ====================
entity::DataParameter<bool> WolfEntity::DATA_INTERESTED_PARAM = entity::EntityDataManager::createKey<bool>();
entity::DataParameter<i32> WolfEntity::DATA_COLLAR_COLOR_PARAM = entity::EntityDataManager::createKey<i32>();
entity::DataParameter<i64> WolfEntity::DATA_ANGER_TIME_PARAM = entity::EntityDataManager::createKey<i64>();

// ============================================================================
// 继承链标识（复刻 vanilla ClassTreeIdRegistry，parent = TameableEntity::classInfo()）
// ============================================================================
const entity::EntityClassInfo& WolfEntity::classInfo()
{
    static const entity::EntityClassInfo s_classInfo{"WolfEntity", &TameableEntity::classInfo()};
    return s_classInfo;
}

WolfEntity::WolfEntity(EntityInstanceId id, ecs::EntityRegistry& registry)
    : TameableEntity(id, registry)
{
    // 注册 AI 目标
    registerGoals();

    // 注册属性
    registerAttributes();

    // 显式调用 registerData() 注册同步数据参数
    // 由于 C++ 虚函数在基类构造函数中不会派发到派生类（Entity::Entity 内部调用
    // registerData() 时调用的是 Entity::registerData 而非 WolfEntity::registerData），
    // 必须在派生类构造函数中显式调用，参考 ZombieVillagerEntity 模式。
    registerData();
}

std::unique_ptr<Entity> WolfEntity::create(IWorld* /*world*/, ecs::EntityRegistry& registry)
{
    // 使用临时ID 0，实际ID由 EntityManager 分配
    return std::make_unique<WolfEntity>(0, registry);
}

bool WolfEntity::isTameItem(const ItemStack& itemStack) const
{
    // 狼用骨头驯服
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::BONE;
}

bool WolfEntity::isBreedingItem(const ItemStack& itemStack) const
{
    // 驯服后用肉类繁殖
    const Item* item = itemStack.getItem();
    if (item == nullptr) return false;
    return item == Items::PORKCHOP || item == Items::COOKED_PORKCHOP || item == Items::BEEF ||
        item == Items::COOKED_BEEF || item == Items::CHICKEN || item == Items::COOKED_CHICKEN ||
        item == Items::RABBIT || item == Items::COOKED_RABBIT || item == Items::MUTTON ||
        item == Items::COOKED_MUTTON || item == Items::ROTTEN_FLESH;
}

bool WolfEntity::isFoodItem(const ItemStack& itemStack) const
{
    // 同繁殖物品
    return isBreedingItem(itemStack);
}

namespace {
/// 染料颜色对应的 RGB 值
/// 参考: net.minecraft.world.item.DyeItem 中各染料的 textColor
[[nodiscard]] u32 dyeColorToRGB(DyeColor color)
{
    switch (color) {
        case DyeColor::White:
            return 0xFFFFFF;
        case DyeColor::Orange:
            return 0xD87F33;
        case DyeColor::Magenta:
            return 0xB24CD8;
        case DyeColor::LightBlue:
            return 0x6699D8;
        case DyeColor::Yellow:
            return 0xE5E533;
        case DyeColor::Lime:
            return 0x7FCC19;
        case DyeColor::Pink:
            return 0xF27FA5;
        case DyeColor::Gray:
            return 0x4C4C4C;
        case DyeColor::LightGray:
            return 0x999999;
        case DyeColor::Cyan:
            return 0x4C7F99;
        case DyeColor::Purple:
            return 0x7F3FB2;
        case DyeColor::Blue:
            return 0x334CB2;
        case DyeColor::Brown:
            return 0x664C33;
        case DyeColor::Green:
            return 0x667F33;
        case DyeColor::Red:
            return 0x993333;
        case DyeColor::Black:
            return 0x191919;
        default:
            return 0xFFFFFF;
    }
}

/// 混合两种颜色（取各 RGB 分量的平均值）
/// 参考: ArmorDyeRecipe::_mixColors
[[nodiscard]] u32 mixArmorColors(u32 color1, u32 color2)
{
    i32 r1 = static_cast<i32>((color1 >> 16) & 0xFF);
    i32 g1 = static_cast<i32>((color1 >> 8) & 0xFF);
    i32 b1 = static_cast<i32>(color1 & 0xFF);

    i32 r2 = static_cast<i32>((color2 >> 16) & 0xFF);
    i32 g2 = static_cast<i32>((color2 >> 8) & 0xFF);
    i32 b2 = static_cast<i32>(color2 & 0xFF);

    i32 r = (r1 + r2) / 2;
    i32 g = (g1 + g2) / 2;
    i32 b = (b1 + b2) / 2;

    return (0xFF << 24) | (static_cast<u32>(r) << 16) | (static_cast<u32>(g) << 8) | static_cast<u32>(b);
}
} // namespace

// ========== 交互 ==========

ActionResultType WolfEntity::interactMob(Player& player, Hand hand)
{
    ItemStack& itemStack = player.getHeldItem(hand);
    const Item* item = itemStack.getItem();

    if (isTamed()) {
        // ========== 已驯服的狼 ==========

        // 优先级1: 喂食治疗（食物 + 生命值未满）
        if (isFoodItem(itemStack) && health() < maxHealth()) {
            // 消耗物品（非创造模式）
            if (!player.abilities().creativeMode) {
                itemStack.shrink(1);
            }

            // 计算治疗量
            f32 healAmount = _getFoodHealAmount(item);
            heal(healAmount);

            // 播放吃东西声音
            if (!isSilent()) {
                auto soundEvent = makeSoundEventId("eat");
                if (soundEvent.has_value()) {
                    playSound(*soundEvent, 1.0f, 1.0f + (getRandom().nextFloat() - getRandom().nextFloat()) * 0.2f);
                }
            }

            return ActionResultType::Success;
        }

        // 优先级2: 狼铠装备（狼铠 + 未装备 + 主人 + 非幼年）
        // 参考: net.minecraft.world.entity.animal.wolf.Wolf.mobInteract() 装备分支
        if (item != nullptr && item == Items::WOLF_ARMOR && !isWearingBodyArmor() && isOwner(player.uuidBytes()) &&
            !isChild()) {
            // 装备狼铠：复制一份（数量1）放入身体槽位
            ItemStack armorStack = itemStack.split(1);
            setBodyArmorItem(armorStack);

            // 播放装备音效
            playSound(SoundEvents::ITEM_ARMOR_EQUIP_WOLF, 1.0f, 1.0f);

            return ActionResultType::Success;
        }

        // 优先级3: 狼铠修复（犰狳鳞甲 + 坐下 + 已装备 + 狼铠受损 + 主人）
        // 参考: net.minecraft.world.entity.animal.wolf.Wolf.mobInteract() 修复分支
        if (isSitting() && isWearingBodyArmor() && isOwner(player.uuidBytes())) {
            ItemStack& bodyArmor = getMutableEquipment(EquipmentSlot::Body);
            if (!bodyArmor.isEmpty() && bodyArmor.getItem() == Items::WOLF_ARMOR && bodyArmor.isDamaged()) {
                // 检查手持物品是否为犰狳鳞甲（或属于 REPAIRS_WOLF_ARMOR 标签）
                if (item != nullptr && item::tag::ItemTags::REPAIRS_WOLF_ARMOR().contains(itemStack)) {
                    // 消耗1个修复物品（非创造模式）
                    if (!player.abilities().creativeMode) {
                        itemStack.shrink(1);
                    }

                    // 播放修复音效
                    playSound(SoundEvents::ENTITY_WOLF_ARMOR_REPAIR, 1.0f, 1.0f);

                    // 修复狼铠耐久：恢复 maxDamage * 0.125 的耐久值
                    i32 maxDamage = bodyArmor.getMaxDamage();
                    i32 repairAmount = static_cast<i32>(static_cast<f32>(maxDamage) * ARMOR_REPAIR_UNIT);
                    i32 newDamage = std::max(0, bodyArmor.getDamage() - repairAmount);
                    bodyArmor.setDamage(newDamage);

                    return ActionResultType::Success;
                }
            }
        }

        // 优先级4: 狼铠染色（染料 + 已装备可染色狼铠 + 主人）
        // 参考: net.minecraft.world.entity.animal.wolf.Wolf.mobInteract() 染色分支
        if (item != nullptr && isWearingBodyArmor() && isOwner(player.uuidBytes())) {
            ItemStack& bodyArmor = getMutableEquipment(EquipmentSlot::Body);
            if (!bodyArmor.isEmpty() && bodyArmor.getItem() == Items::WOLF_ARMOR) {
                auto dyeColor = _getDyeColorFromItem(item);
                if (dyeColor.has_value()) {
                    auto* dyeableArmor = dynamic_cast<const item::items::DyeableArmorItem*>(bodyArmor.getItem());
                    if (dyeableArmor != nullptr) {
                        // 合成新颜色：当前颜色与染料颜色混合
                        u32 currentColor = dyeableArmor->getColor(bodyArmor);
                        u32 dyeRGB = dyeColorToRGB(dyeColor.value());
                        u32 newColor = mixArmorColors(currentColor, dyeRGB);
                        item::items::DyeableArmorItem::setColor(bodyArmor, newColor);
                        if (!player.abilities().creativeMode) {
                            itemStack.shrink(1);
                        }
                        return ActionResultType::Success;
                    }
                }
            }
        }

        // 优先级5: 颈圈染色（染料 + 主人）
        auto dyeColor = _getDyeColorFromItem(item);
        if (dyeColor.has_value() && isOwner(player.uuidBytes())) {
            if (dyeColor.value() != getCollarColor()) {
                setCollarColor(dyeColor.value());
                if (!player.abilities().creativeMode) {
                    itemStack.shrink(1);
                }
            }
            return ActionResultType::Success;
        }

        // 优先级6: 繁殖/成长（食物 + 满血）
        // 对应 MC 原版 Animal.mobInteract() 的逻辑
        if (isBreedingItem(itemStack)) {
            if (isChild()) {
                // 幼年狼喂食加速成长
                if (!player.abilities().creativeMode) {
                    itemStack.shrink(1);
                }
                // 加速成长：减少 10% 的剩余成长时间
                // getGrowingAge() 返回负值（tick），-getGrowingAge() 是剩余成长 tick
                i32 remainingTicks = -getGrowingAge();
                i32 accelerateSeconds = static_cast<i32>(remainingTicks * 0.1f) / 20;
                ageUp(accelerateSeconds);

                // 播放吃东西声音
                if (!isSilent()) {
                    auto soundEvent = makeSoundEventId("eat");
                    if (soundEvent.has_value()) {
                        playSound(*soundEvent, 1.0f, 1.0f + (getRandom().nextFloat() - getRandom().nextFloat()) * 0.2f);
                    }
                }
                return ActionResultType::Success;
            }

            if (canBreed()) {
                // 成年狼喂食进入求爱状态
                if (!player.abilities().creativeMode) {
                    itemStack.shrink(1);
                }
                setInLove(player.playerId());

                // 播放吃东西声音
                if (!isSilent()) {
                    auto soundEvent = makeSoundEventId("eat");
                    if (soundEvent.has_value()) {
                        playSound(*soundEvent, 1.0f, 1.0f + (getRandom().nextFloat() - getRandom().nextFloat()) * 0.2f);
                    }
                }
                return ActionResultType::Success;
            }
        }

        // 优先级7: 坐下/站起切换（主人 + 非特殊物品）
        if (isOwner(player.uuidBytes())) {
            setSitting(!isSitting());
            clearNavigation();
            setAttackTarget(nullptr);
            return ActionResultType::Success;
        }

        return ActionResultType::Pass;
    }

    // ========== 未驯服的狼 ==========

    // 手持骨头 + 未愤怒时尝试驯服
    if (isTameItem(itemStack) && !isAngry()) {
        if (!player.abilities().creativeMode) {
            itemStack.shrink(1);
        }

        // 播放吃东西声音
        if (!isSilent()) {
            auto soundEvent = makeSoundEventId("eat");
            if (soundEvent.has_value()) {
                playSound(*soundEvent, 1.0f, 1.0f + (getRandom().nextFloat() - getRandom().nextFloat()) * 0.2f);
            }
        }

        // 服务端处理驯服逻辑
        if (m_world != nullptr && !m_world->isClientSide()) {
            _tryToTame(player);
        }

        return ActionResultType::Success;
    }

    // 其他情况交给父类处理
    return TameableEntity::interactMob(player, hand);
}

void WolfEntity::_tryToTame(Player& player)
{
    // 1/3 概率驯服成功
    math::Random& rng = getRandom();
    if (rng.nextInt(3) == 0) {
        // 驯服成功
        setTamed(true);
        setOwnerId(player.uuidBytes());

        // 停止导航和攻击
        clearNavigation();
        setAttackTarget(nullptr);

        // 默认坐下
        setSitting(true);

        // 通知世界触发进度检测
        m_world->onTameAnimal(player.playerId(), this);

        // 广播驯服成功状态（心形粒子）
        m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatus::TamingSucceeded));
    } else {
        // 驯服失败，广播烟雾粒子
        m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatus::TamingFailed));
    }
}

f32 WolfEntity::_getFoodHealAmount(const Item* item) const
{
    // MC 原版中治疗量为 2.0 * food.nutrition
    // 狼的食物营养值映射（硬编码，因为没有 FoodProperties 系统）：
    //   生猪肉/牛肉/鸡肉/兔肉/羊肉: nutrition = 2, 治疗 4.0
    //   熟猪肉/牛肉: nutrition = 8, 治疗 16.0
    //   熟鸡肉/兔肉/羊肉: nutrition = 6, 治疗 12.0
    //   腐肉: nutrition = 4, 治疗 8.0
    f32 nutrition = 0.0f;
    if (item == Items::PORKCHOP) {
        nutrition = 2.0f;
    } else if (item == Items::COOKED_PORKCHOP) {
        nutrition = 8.0f;
    } else if (item == Items::BEEF) {
        nutrition = 2.0f;
    } else if (item == Items::COOKED_BEEF) {
        nutrition = 8.0f;
    } else if (item == Items::CHICKEN) {
        nutrition = 2.0f;
    } else if (item == Items::COOKED_CHICKEN) {
        nutrition = 6.0f;
    } else if (item == Items::RABBIT) {
        nutrition = 2.0f;
    } else if (item == Items::COOKED_RABBIT) {
        nutrition = 6.0f;
    } else if (item == Items::MUTTON) {
        nutrition = 2.0f;
    } else if (item == Items::COOKED_MUTTON) {
        nutrition = 6.0f;
    } else if (item == Items::ROTTEN_FLESH) {
        nutrition = 4.0f;
    }
    return 2.0f * nutrition;
}

std::optional<DyeColor> WolfEntity::_getDyeColorFromItem(const Item* item)
{
    if (item == nullptr) {
        return std::nullopt;
    }

    static const std::unordered_map<const Item*, DyeColor> dyeMap = {
        {Items::INK_SAC, DyeColor::Black},
        {Items::RED_DYE, DyeColor::Red},
        {Items::GREEN_DYE, DyeColor::Green},
        {Items::COCOA_BEANS, DyeColor::Brown},
        {Items::LAPIS_LAZULI_DYE, DyeColor::Blue},
        {Items::PURPLE_DYE, DyeColor::Purple},
        {Items::CYAN_DYE, DyeColor::Cyan},
        {Items::LIGHT_GRAY_DYE, DyeColor::LightGray},
        {Items::GRAY_DYE, DyeColor::Gray},
        {Items::PINK_DYE, DyeColor::Pink},
        {Items::LIME_DYE, DyeColor::Lime},
        {Items::YELLOW_DYE, DyeColor::Yellow},
        {Items::LIGHT_BLUE_DYE, DyeColor::LightBlue},
        {Items::MAGENTA_DYE, DyeColor::Magenta},
        {Items::ORANGE_DYE, DyeColor::Orange},
        {Items::WHITE_DYE, DyeColor::White},
        {Items::BONE_MEAL, DyeColor::White},
    };

    auto it = dyeMap.find(item);
    if (it != dyeMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool WolfEntity::_canArmorAbsorb(const DamageSource& source) const
{
    // 狼铠吸收伤害的条件：
    // 1. 身体槽位装备了狼铠
    // 2. 伤害源不在 DamageTypeTags::BYPASSES_WOLF_ARMOR 标签中
    // 与 MC 1.21.11 Wolf.canArmorAbsorb 一致：
    //   return this.getBodyArmorItem().is(Items.WOLF_ARMOR) && !p_406249_.is(DamageTypeTags.BYPASSES_WOLF_ARMOR);
    const ItemStack& bodyArmor = getEquipment(EquipmentSlot::Body);
    if (bodyArmor.isEmpty() || bodyArmor.getItem() != Items::WOLF_ARMOR) {
        return false;
    }
    if (source.is(DamageTypeTags::BYPASSES_WOLF_ARMOR())) {
        return false;
    }
    return true;
}

bool WolfEntity::wantsToAttack(const LivingEntity& target, const LivingEntity* owner) const
{
    // 苦力怕、恶魂：永远不攻击（MC 使用 instanceof，此处使用 dynamic_cast）
    // 注：ArmorStandEntity 在本项目中继承自 Entity 而非 LivingEntity，
    // 因此不可能作为 LivingEntity 传入，无需检查
    if (dynamic_cast<const CreeperEntity*>(&target) != nullptr ||
        dynamic_cast<const GhastEntity*>(&target) != nullptr) {
        return false;
    }

    // 其他狼：只攻击未驯服的狼或主不同的狼
    const WolfEntity* otherWolf = dynamic_cast<const WolfEntity*>(&target);
    if (otherWolf != nullptr) {
        if (!otherWolf->isTamed()) {
            return true; // 未驯服的狼可以攻击
        }
        // 已驯服的狼：只有主不同时才攻击
        if (owner != nullptr) {
            return otherWolf->getOwner() != owner;
        }
        return false; // 没有主人，不攻击已驯服的狼
    }

    // 玩家PvP保护：如果目标和主人都是玩家，检查 PvP 规则和队伍友伤
    const Player* targetPlayer = dynamic_cast<const Player*>(&target);
    const Player* ownerPlayer = dynamic_cast<const Player*>(owner);
    if (targetPlayer != nullptr && ownerPlayer != nullptr && !ownerPlayer->canHarmPlayer(*targetPlayer)) {
        return false;
    }

    // 已驯服的马：不攻击
    const AbstractHorseEntity* horse = dynamic_cast<const AbstractHorseEntity*>(&target);
    if (horse != nullptr && horse->isTame()) {
        return false;
    }

    // 其他已驯服的驯服动物：不攻击
    const TameableEntity* tameable = dynamic_cast<const TameableEntity*>(&target);
    if (tameable != nullptr && tameable->isTamed()) {
        return false;
    }

    // 其他目标：允许攻击
    return true;
}

std::unique_ptr<AnimalEntity> WolfEntity::spawnBaby(AnimalEntity& /*partner*/)
{
    // ECS 迁移：实体构造需要 registry 句柄，ClientWorld 返回 nullptr 表客户端不接入 ECS
    auto* registry = &ecsRegistry();
    if (registry == nullptr) {
        return nullptr;
    }

    // 创建小狼
    auto baby = std::make_unique<WolfEntity>(0, *registry);

    // 设置为幼体
    baby->setChild(true);

    // 设置位置（在父体位置附近）
    baby->setPosition(x(), y(), z());

    return baby;
}

void WolfEntity::tick()
{
    TameableEntity::tick();

    if (!isAlive()) {
        return;
    }

    const f32 dx = x() - prevX();
    const f32 dz = z() - prevZ();
    const f32 horizontalDistance = std::sqrt(dx * dx + dz * dz);

    if (horizontalDistance > 0.0f) {
        m_stepSoundDistance += horizontalDistance * 0.6f;
        if (m_stepSoundDistance > m_nextStepSoundDistance && onGround() && !isInWater()) {
            m_nextStepSoundDistance = std::floor(m_stepSoundDistance) + 1.0f;
            const BlockPos stepPos(static_cast<i32>(std::floor(x())),
                static_cast<i32>(std::floor(y() - 0.2f)),
                static_cast<i32>(std::floor(z())));
            const BlockState* blockState = m_world != nullptr ? m_world->getBlockState(stepPos) : nullptr;
            playStepSound(stepPos, blockState);
        }
    }

    // ========== 甩水动画状态机（参考 MC 1.21.11 Wolf.tick() / Wolf.aiStep()） ==========

    // 1. interestedAngle 插值（向 1.0 或 0.0 趋近）
    //    对应 MC Wolf.tick() 第 318-323 行
    m_interestedAngleO = m_interestedAngle;
    if (isInterested()) {
        m_interestedAngle += (1.0f - m_interestedAngle) * 0.4f;
    } else {
        m_interestedAngle += (0.0f - m_interestedAngle) * 0.4f;
    }

    // 2. 甩水触发（对应 MC Wolf.aiStep() 第 300-307 行）
    //    条件：服务端 + 已湿 + 未在甩水 + 未在寻路 + 在地面
    //    注：isInWaterOrRain() 包含水中和雨中
    if (m_world != nullptr && !m_world->isClientSide() && m_isWet && !m_isShaking && onGround()) {
        // 检查是否在寻路（导航未完成时不触发甩水）
        const auto* nav = navigator();
        const bool isPathFinding = (nav != nullptr && nav->isInProgress());
        if (!isPathFinding) {
            m_isShaking = true;
            m_shakeAnim = 0.0f;
            m_shakeAnimO = 0.0f;
            m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatus::ShakeOffWater));
        }
    }

    // 3. isWet / 甩水进度更新（对应 MC Wolf.tick() 第 325-358 行）
    const bool inWaterOrRain = isInWaterOrRain();
    if (inWaterOrRain) {
        m_isWet = true;
        // 已在甩水时再次接触水：取消甩水并广播 byte 56
        if (m_isShaking && m_world != nullptr && !m_world->isClientSide()) {
            m_world->broadcastEntityStatus(id(), static_cast<u8>(network::EntityStatus::WolfStopShaking));
            _cancelShake();
        }
    } else if ((m_isWet || m_isShaking) && m_isShaking) {
        // 甩水动画开始时播放一次甩水音效并触发 ENTITY_ACTION 游戏事件
        // 对应 MC Wolf.tick() 第 332-335 行
        if (m_shakeAnim == 0.0f) {
            playShakingSound();
            if (m_world != nullptr) {
                m_world->gameEvent(gameevent::GameEvents::ENTITY_ACTION,
                    BlockPos(static_cast<i32>(std::floor(x())),
                        static_cast<i32>(std::floor(y())),
                        static_cast<i32>(std::floor(z()))),
                    gameevent::GameEvent::Context::of(this));
            }
        }

        // 甩水进度推进（每 tick +0.05）
        m_shakeAnimO = m_shakeAnim;
        m_shakeAnim += 0.05f;

        // 甩水完成（shakeAnimO >= 2.0）
        // 对应 MC Wolf.tick() 第 339-344 行
        if (m_shakeAnimO >= 2.0f) {
            m_isWet = false;
            m_isShaking = false;
            m_shakeAnimO = 0.0f;
            m_shakeAnim = 0.0f;
        }

        // SPLASH 粒子发射（shakeAnim > 0.4 时）
        // 对应 MC Wolf.tick() 第 346-356 行
        if (m_shakeAnim > 0.4f && m_world != nullptr) {
            const f32 particleY = static_cast<f32>(y());
            const i32 particleCount = static_cast<i32>(std::sin((m_shakeAnim - 0.4f) * math::PI) * 7.0f);
            const f32 bbWidth = width();
            const Vector3 delta = velocity();
            for (i32 j = 0; j < particleCount; ++j) {
                const f32 f1 = (getRandom().nextFloat() * 2.0f - 1.0f) * bbWidth * 0.5f;
                const f32 f2 = (getRandom().nextFloat() * 2.0f - 1.0f) * bbWidth * 0.5f;
                m_world->addParticle(particle::ParticleTypeId::Splash,
                    Vector3(x() + f1, particleY + 0.8f, z() + f2),
                    Vector3(static_cast<f32>(delta.x), static_cast<f32>(delta.y), static_cast<f32>(delta.z)));
            }
        }
    }
}

std::optional<ResourceLocation> WolfEntity::getAmbientSound() const
{
    math::Random& random = getRandom();

    if (isAngry()) {
        return makeSoundEventId("growl");
    }

    if (random.nextInt(3) == 0) {
        if (isTamed() && health() < 10.0f) {
            return makeSoundEventId("whine");
        }

        return makeSoundEventId("pant");
    }

    return makeSoundEventId("ambient");
}

std::optional<ResourceLocation> WolfEntity::getHurtSound(DamageSource& source) const
{
    // 穿戴狼铠且伤害由狼铠吸收时，播放狼铠受伤音效
    if (_canArmorAbsorb(source)) {
        return SoundEvents::ENTITY_WOLF_ARMOR_DAMAGE;
    }
    return makeSoundEventId("hurt");
}

bool WolfEntity::canShearEquipment(const Player& player) const
{
    // 狼只允许主人剪切狼铠
    // 参考: net.minecraft.world.entity.animal.wolf.Wolf.canShearEquipment()
    return isOwner(player.uuidBytes());
}

void WolfEntity::actuallyHurt(DamageSource& source, f32 amount)
{
    // 狼铠伤害吸收逻辑
    // 参考: net.minecraft.world.entity.animal.wolf.Wolf.actuallyHurt()
    if (!_canArmorAbsorb(source)) {
        // 狼铠不吸收此伤害，走父类正常受伤流程
        TameableEntity::actuallyHurt(source, amount);
        return;
    }

    // 狼铠吸收伤害：狼不扣血，狼铠耐久降低
    ItemStack& bodyArmor = getMutableEquipment(EquipmentSlot::Body);
    if (bodyArmor.isEmpty() || !bodyArmor.isDamageable()) {
        // 无狼铠或狼铠不可损坏，走父类正常受伤流程
        TameableEntity::actuallyHurt(source, amount);
        return;
    }

    // 记录受损前的裂纹等级
    i32 damageBefore = bodyArmor.getDamage();
    i32 maxDamage = bodyArmor.getMaxDamage();
    auto crackBefore = entity::Crackiness::WOLF_ARMOR.byDamage(damageBefore, maxDamage);

    // 狼铠耐久降低（向上取整）
    i32 armorDamage = static_cast<i32>(std::ceil(amount));
    bool armorBroken = LivingEntity::hurtAndBreak(bodyArmor, armorDamage, this, EquipmentSlot::Body);

    if (armorBroken) {
        // 狼铠破损时播放破损音效（取代普通受损音效）
        // 参考: MC 1.21.11 WolfArmorItem 的 BREAK_SOUND 组件
        playSound(SoundEvents::ENTITY_WOLF_ARMOR_BREAK, 1.0f, 1.0f);
    } else {
        // 狼铠受损但未破损：播放狼铠受损音效（getHurtSound 返回 ENTITY_WOLF_ARMOR_DAMAGE）
        // 参考: MC 1.21.11 Wolf.getHurtSound() 在狼铠吸收时返回 wolf_armor.damage
        playHurtSound(source);

        // 检查受损后的裂纹等级，等级变化时播放裂纹音效
        auto crackAfter = entity::Crackiness::WOLF_ARMOR.byDamage(bodyArmor.getDamage(), bodyArmor.getMaxDamage());
        if (crackBefore != crackAfter) {
            // 播放裂纹音效
            playSound(SoundEvents::ENTITY_WOLF_ARMOR_CRACK, 1.0f, 1.0f);
        }
    }

    // 狼铠吸收伤害时，狼不扣血
}

void WolfEntity::damageArmor(DamageSource& source, f32 amount)
{
    // 当狼未穿戴狼铠或伤害绕过护甲时，走父类默认逻辑（空实现）
    // 当狼穿戴狼铠且伤害不绕过护甲时，伤害已在 actuallyHurt 中由狼铠吸收，
    // 此处不再额外调用 doHurtEquipment（避免双重耐久损耗）
    // 参考: net.minecraft.world.entity.animal.wolf.Wolf.hurtArmor() 调用 doHurtEquipment
    // 在 MC 1.21.11 中，Wolf.hurtArmor 始终调用 doHurtEquipment，但 actuallyHurt 在
    // canArmorAbsorb 时会跳过 super.actuallyHurt（不再触发 damageArmor）。
    // 本项目的 LivingEntity::actuallyHurt 在 !bypassesArmor 时调用 damageArmor，
    // 而 actuallyHurt 已被重写并在 canArmorAbsorb 时直接返回，不会调用 damageArmor。
    // 因此 damageArmor 仅在无狼铠或绕过护甲时被调用，此时不做任何处理。
    (void)source;
    (void)amount;
}

std::optional<ResourceLocation> WolfEntity::getDeathSound() const
{
    return makeSoundEventId("death");
}

void WolfEntity::playStepSound(const BlockPos& /*pos*/, const BlockState* /*blockState*/)
{
    auto soundEvent = makeSoundEventId("step");
    if (!soundEvent.has_value()) {
        return;
    }

    playSound(*soundEvent, 0.15f, 1.0f);
}

void WolfEntity::playStepSound()
{
    const BlockPos stepPos(
        static_cast<i32>(std::floor(x())), static_cast<i32>(std::floor(y() - 0.2f)), static_cast<i32>(std::floor(z())));
    const BlockState* blockState = m_world != nullptr ? m_world->getBlockState(stepPos) : nullptr;
    playStepSound(stepPos, blockState);
}

f32 WolfEntity::getTailAngle() const
{
    // 根据生命值计算尾巴角度
    if (isAngry()) {
        // 愤怒时尾巴竖起
        return 1.539f; // 约88度
    }

    // 根据生命值计算
    f32 healthRatio = health() / maxHealth();
    return TAIL_ANGLE_UNHEALTHY + (healthRatio * (TAIL_ANGLE_HEALTHY - TAIL_ANGLE_UNHEALTHY));
}

void WolfEntity::setAngry(bool angry)
{
    // 重写 TameableEntity::setAngry，本方法本身不直接操作状态，
    // 而是委托给基类实现。基类内部通过虚函数 setAngerTime 写入愤怒时间，
    // 由于 WolfEntity 重写了 setAngerTime，写入会路由到 DATA_ANGER_TIME_PARAM，
    // 从而让 EntityTracker 自动广播到客户端，驱动尾巴角度、纹理选择等渲染表现。
    //
    // 基类 setAngry 还会清理攻击目标与复仇目标（m_revengeTargetId），这些副作用
    // 与愤怒状态的清除语义一致，无需在 WolfEntity 中重复。
    TameableEntity::setAngry(angry);
}

bool WolfEntity::isInWater() const
{
    // 调用父类实现检查是否在水中
    return TameableEntity::isInWater();
}

bool WolfEntity::isInWaterOrRain() const
{
    // 对应 MC Wolf 中使用的 Entity.isInWaterOrRain()
    // 用于判断狼是否接触水（水中或雨中），驱动甩水状态机
    return isInWater() || isInRain();
}

f32 WolfEntity::getShakeAnim(f32 partialTick) const
{
    // 对应 MC Wolf.getShakeAnim(): Mth.lerp(partialTick, shakeAnimO, shakeAnim)
    return m_shakeAnimO + (m_shakeAnim - m_shakeAnimO) * partialTick;
}

f32 WolfEntity::getWetShade(f32 partialTick) const
{
    // 对应 MC Wolf.getWetShade():
    //   !isWet ? 1.0F : min(0.75F + lerp(partialTick, shakeAnimO, shakeAnim) / 2.0F * 0.25F, 1.0F)
    if (!m_isWet) {
        return 1.0f;
    }
    const f32 shake = getShakeAnim(partialTick);
    return std::min(0.75f + shake / 2.0f * 0.25f, 1.0f);
}

f32 WolfEntity::getHeadRollAngle(f32 partialTick) const
{
    // 对应 MC Wolf.getHeadRollAngle():
    //   Mth.lerp(partialTick, interestedAngleO, interestedAngle) * 0.15F * PI
    const f32 interested = m_interestedAngleO + (m_interestedAngle - m_interestedAngleO) * partialTick;
    return interested * 0.15f * math::PI;
}

void WolfEntity::_cancelShake()
{
    // 对应 MC Wolf.cancelShake():
    //   isShaking = false; shakeAnim = 0; shakeAnimO = 0;
    m_isShaking = false;
    m_shakeAnim = 0.0f;
    m_shakeAnimO = 0.0f;
}

void WolfEntity::playShakingSound()
{
    // 对应 MC Wolf.playShakingSound():
    //   playSound(SoundEvents.WOLF_SHAKE, getSoundVolume(),
    //             (random.nextFloat() - random.nextFloat()) * 0.2F + 1.0F);
    math::Random& random = getRandom();
    playSound(
        SoundEvents::ENTITY_WOLF_SHAKE, getSoundVolume(), (random.nextFloat() - random.nextFloat()) * 0.2f + 1.0f);
}

void WolfEntity::die(DamageSource& cause)
{
    // 对应 MC Wolf.die():
    //   isWet = false; isShaking = false; shakeAnimO = 0; shakeAnim = 0;
    //   super.die(cause);
    m_isWet = false;
    m_isShaking = false;
    m_shakeAnimO = 0.0f;
    m_shakeAnim = 0.0f;
    TameableEntity::die(cause);
}

void WolfEntity::registerGoals()
{
    // 调用父类方法（已包含 SwimGoal, PanicGoal, BreedGoal, FollowParentGoal, RandomWalkingGoal, LookAtGoal,
    // LookRandomlyGoal）
    TameableEntity::registerGoals();

    // ========================================================================
    // 行为目标 (goalSelector)
    // ========================================================================

    // 优先级 1: 坐下目标（驯服后）- 与PanicGoal同优先级，但SitGoal会检查是否驯服
    m_goalSelector.addGoal(1, new entity::ai::goal::SitGoal(this));

    // 优先级 3: 未驯服时避开羊驼
    // 羊驼有强度属性，强度高的羊驼可以吓跑狼
    m_goalSelector.addGoal(3,
        std::make_unique<entity::ai::goal::AvoidEntityGoal>(
            this, 24.0f, 1.5, 1.5, [this](const LivingEntity* entity) -> bool {
                // 只在未驯服时避开羊驼
                if (isTamed()) return false;
                // 检查是否是羊驼
                if (entity->entityType() != entity::VanillaEntityTypeKeys::LLAMA &&
                    entity->entityType() != entity::VanillaEntityTypeKeys::TRADER_LLAMA) {
                    return false;
                }
                // 检查羊驼的强度
                const LlamaEntity* llama = dynamic_cast<const LlamaEntity*>(entity);
                if (!llama) return false;
                // 羊驼强度 >= 随机值(0-4) 时，狼会躲避
                // 强度1: 20%概率吓跑，强度4: 80%概率吓跑
                math::Random& rng = getRandom();
                return llama->getStrength() >= rng.nextInt(5);
            }));

    // 优先级 4: 跳跃攻击
    m_goalSelector.addGoal(4, std::make_unique<entity::ai::goal::LeapAtTargetGoal>(this, 0.4f));

    // 优先级 5: 近战攻击
    m_goalSelector.addGoal(5, std::make_unique<entity::ai::goal::MeleeAttackGoal>(this, 1.0, true));

    // 优先级 6: 跟随主人（驯服后）
    m_goalSelector.addGoal(6, new entity::ai::goal::FollowOwnerGoal(this, 1.0, 3.0f, 10.0f, 32.0f));

    // 优先级 9: 乞求目标（看向手持骨头或肉类的玩家）
    // 狼使用 BegGoal（乞求，只看不动），而非 TemptGoal（诱惑，会跟随玩家）
    // 这是因为未驯服的狼不会主动接近玩家，驯服后的狼已跟随主人，不需要 TemptGoal
    // [COMPLETED] 2026-05-15 - 骨头乞求行为已通过 BegGoal 实现
    m_goalSelector.addGoal(9, new entity::ai::goal::BegGoal(this, 8.0f));

    // ========================================================================
    // 目标选择器 (targetSelector)
    // ========================================================================

    // 优先级 1: 主人被攻击时反击
    // 当主人被攻击时，狼会攻击攻击者
    m_targetSelector.addGoal(1, std::make_unique<entity::ai::goal::OwnerHurtByTargetGoal>(this));

    // 优先级 2: 攻击主人正在攻击的目标
    // 当主人攻击某实体时，狼会协助攻击
    m_targetSelector.addGoal(2, std::make_unique<entity::ai::goal::OwnerHurtTargetGoal>(this));

    // 优先级 3: 被攻击后反击，并呼叫同伴
    // setCallsForHelp = true，召唤附近的狼一起攻击
    m_targetSelector.addGoal(3, std::make_unique<entity::ai::goal::HurtByTargetGoal>(this, true));

    // 优先级 4: 愤怒时攻击玩家
    // 需要配合 IAngerable 接口，当玩家攻击狼后，狼会记住玩家并攻击
    // 当前简化实现：不注册此目标，因为狼默认不会主动攻击玩家
    // m_targetSelector.addGoal(4, std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<Player>>(
    //     this, true, 10, /* angerPredicate */));

    // 优先级 5: 未驯服时攻击羊、兔子、狐狸
    // TARGET_ENTITIES 谓词：羊、兔子、狐狸
    m_targetSelector.addGoal(5,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<LivingEntity>>(this,
            true, // checkSight
            0,    // chance (每tick检查)
            [](const LivingEntity* entity) -> bool {
                if (!entity || !entity->isAlive()) return false;
                // 羊、兔子、狐狸
                auto type = entity->entityType();
                return type == entity::VanillaEntityTypeKeys::SHEEP || type == entity::VanillaEntityTypeKeys::RABBIT ||
                    type == entity::VanillaEntityTypeKeys::FOX;
            }));

    // 优先级 6: 未驯服时攻击幼海龟（不在水中）
    // 使用 NonTamedTargetGoal，只在未驯服时执行
    // TurtleEntity.TARGET_DRY_BABY 谓词：幼体且不在水中
    m_targetSelector.addGoal(6,
        std::make_unique<entity::ai::goal::NonTamedTargetGoal<TurtleEntity>>(this,
            true, // checkSight
            [](const LivingEntity* entity) -> bool {
                // TARGET_DRY_BABY: 幼体且不在水中
                const TurtleEntity* turtle = dynamic_cast<const TurtleEntity*>(entity);
                if (!turtle) return false;
                return turtle->isChild() && !turtle->isInWater();
            }));

    // 优先级 7: 攻击骷髅类怪物
    // 无论是否驯服，狼都会攻击骷髅类怪物
    m_targetSelector.addGoal(7,
        std::make_unique<entity::ai::goal::NearestAttackableTargetGoal<LivingEntity>>(this,
            false, // checkSight - 不需要视线检查，骷髅是敌对生物
            0,     // chance
            [](const LivingEntity* entity) -> bool {
                if (!entity || !entity->isAlive()) return false;
                // 骷髅、流浪者、凋灵骷髅
                auto type = entity->entityType();
                return type == entity::VanillaEntityTypeKeys::SKELETON ||
                    type == entity::VanillaEntityTypeKeys::STRAY ||
                    type == entity::VanillaEntityTypeKeys::WITHER_SKELETON;
            }));

    // 优先级 8: 愤怒重置目标（驯服后未设置攻击目标时重置愤怒状态）
    m_targetSelector.addGoal(8, std::make_unique<entity::ai::goal::ResetAngerGoal<WolfEntity>>(this, false));
}

void WolfEntity::registerAttributes()
{
    // 调用父类方法
    TameableEntity::registerAttributes();

    // 狼的属性
    attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 8.0); // 驯服前8血
    attributes().setBaseValue(entity::attribute::Attributes::MOVEMENT_SPEED, 0.3);
    attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0); // 2点攻击力

    // 驯服后会增加到20血，由 onTamed 处理
}

void WolfEntity::registerData()
{
    // 调用父类方法，确保基类数据参数已注册（包括 TameableEntity::DATA_TAMED_PARAM）
    TameableEntity::registerData();

    // 标记当前正在注册 WolfEntity 类的字段，使 registerParam 沿 WolfEntity 继承链
    // 分配 id（续接 TameableEntity 之后）。RAII 守卫自动配对压栈/弹栈。
    entity::EntityDataManager::ClassRegisterGuard guard(m_dataManager, classInfo());

    // 注册兴趣状态数据参数，用于客户端-服务端同步
    // 对应 MC 1.21.11 Wolf.defineSynchedData() 中的 DATA_INTERESTED_ID
    m_dataManager.registerParam(DATA_INTERESTED_PARAM, false);

    // 注册颈圈颜色数据参数，用于客户端-服务端同步
    // 对应 MC 1.21.11 Wolf.defineSynchedData() 中的 DATA_COLLAR_COLOR
    // 默认值为红色（DyeColor::Red），与 MC 原版 DEFAULT_COLLAR_COLOR 一致
    m_dataManager.registerParam(DATA_COLLAR_COLOR_PARAM, static_cast<i32>(DyeColor::Red));

    // 注册愤怒时间数据参数，用于客户端-服务端同步
    // 对应 MC 1.21.11 Wolf.defineSynchedData() 中的 DATA_ANGER_END_TIME
    // vanilla 该字段为 Long（默认 -1L）；本项目声明 i64 以对齐 wire 类型（serializerId=2 VAR_LONG），
    // 业务层以「剩余 ticks」i32 语义运转（默认 0=非愤怒，由 setAngry/setAngerTime 写入、updateAnger 递减）。
    // 旧实现误用 i32 致真客户端 field21 类型校验崩（Integer vs 期望 Long）。
    m_dataManager.registerParam(DATA_ANGER_TIME_PARAM, static_cast<i64>(0));
}

void WolfEntity::onTamed(bool tamed)
{
    if (tamed) {
        // 驯服后增加生命值上限（从8血变为20血）
        attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 20.0);
        setHealth(20.0f);

        // 驯服后增加攻击力
        attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 4.0);
    } else {
        // 放弃驯服后恢复
        attributes().setBaseValue(entity::attribute::Attributes::MAX_HEALTH, 8.0);
        setHealth(8.0f);
        attributes().setBaseValue(entity::attribute::Attributes::ATTACK_DAMAGE, 2.0);
    }
}

// ============================================================================
// NBT 序列化
// ============================================================================

void WolfEntity::addAdditionalSaveData(nbt::tags::compound_tag& tag) const
{
    using namespace mc::entity::serialization;

    // 先调用父类实现
    TameableEntity::addAdditionalSaveData(tag);

    // 保存颈圈颜色（对应 MC 1.21.11 Wolf.addAdditionalSaveData 中的 "CollarColor"）
    tag.put(nbt_keys::COLLAR_COLOR, static_cast<i32>(getCollarColor()));
}

Result<void> WolfEntity::readAdditionalSaveData(const nbt::tags::compound_tag& tag)
{
    using namespace mc::entity::serialization;

    // 先调用父类实现
    MC_TRY(TameableEntity::readAdditionalSaveData(tag));

    // 读取颈圈颜色（对应 MC 1.21.11 Wolf.readAdditionalSaveData 中的 "CollarColor"）
    if (auto val = nbt_helper::tryGetInt(tag, nbt_keys::COLLAR_COLOR)) {
        const i32 colorValue = *val;
        if (colorValue >= 0 && colorValue <= 15) {
            setCollarColor(static_cast<DyeColor>(colorValue));
        }
    }

    return Result<void>::ok();
}

} // namespace mc
