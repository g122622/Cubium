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

#pragma once

#include "common/advancement/MinMaxBounds.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/advancement/trigger/conditions/CriterionContext.hpp"
#include "common/advancement/trigger/conditions/DamagePredicate.hpp"
#include "common/advancement/trigger/conditions/EntityPredicate.hpp"
#include "common/advancement/trigger/conditions/ItemPredicate.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/Vector3.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {

// 前向声明
class ItemStack;

namespace advancement {

// 前向声明 Instance 类
class TameAnimalTriggerInstance;
class BredAnimalsTriggerInstance;
class SummonedEntityTriggerInstance;
class CuredZombieVillagerTriggerInstance;
class VillagerTradeTriggerInstance;
class PlayerInteractedWithEntityTriggerInstance;
class PlayerHurtEntityTriggerInstance;
class EntityHurtPlayerTriggerInstance;
class ChangedDimensionTriggerInstance;
class FishingRodHookedTriggerInstance;
class KilledByArrowTriggerInstance;
class PlayerInteractTriggerInstance;
class StartRidingTriggerInstance;
class LightningStrikeTriggerInstance;
class DistanceTriggerInstance;
class FallAfterExplosionTriggerInstance;

/**
 * @brief 驯服动物触发器
 *
 * 当玩家驯服动物时触发。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.TameAnimalTrigger
 */
class TameAnimalTrigger : public AbstractCriterionTrigger<TameAnimalTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:tame_animal";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player, const class Entity& entity);
};

/**
 * @brief 驯服动物触发器实例
 */
class TameAnimalTriggerInstance : public CriterionInstance<TameAnimalTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:tame_animal";

    TameAnimalTriggerInstance() = default;
    explicit TameAnimalTriggerInstance(EntityPredicate entity);

    [[nodiscard]] bool test(const class Entity& entity) const;

    /// 携带上下文判定（支持 ContextAwarePredicate 条件列表写法）
    [[nodiscard]] bool test(const CriterionContext& context) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    EntityPredicate m_entity;
    /// 实体谓词的上下文写法（裸谓词对象同样兼容）
    ContextAwarePredicate m_entityContext;
};

/**
 * @brief 动物繁殖触发器
 *
 * 当玩家繁殖动物时触发。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.BredAnimalsTrigger
 */
class BredAnimalsTrigger : public AbstractCriterionTrigger<BredAnimalsTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:bred_animals";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(
        class ServerPlayer& player, const class Entity& child, const class Entity& parent, const class Entity& partner);
};

/**
 * @brief 动物繁殖触发器实例
 */
class BredAnimalsTriggerInstance : public CriterionInstance<BredAnimalsTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:bred_animals";

    BredAnimalsTriggerInstance() = default;
    BredAnimalsTriggerInstance(EntityPredicate child, EntityPredicate parent, EntityPredicate partner);

    [[nodiscard]] bool test(const class Entity& child, const class Entity& parent, const class Entity& partner) const;

    /// 携带上下文判定（parent/partner/child 映射到 this 与 killer 槽位）
    [[nodiscard]] bool test(const CriterionContext& childContext,
        const CriterionContext& parentContext,
        const CriterionContext& partnerContext) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    EntityPredicate m_child;
    EntityPredicate m_parent;
    EntityPredicate m_partner;
    /// 三个实体的上下文写法（裸谓词对象同样兼容）
    ContextAwarePredicate m_childContext;
    ContextAwarePredicate m_parentContext;
    ContextAwarePredicate m_partnerContext;
};

/**
 * @brief 召唤实体触发器
 *
 * 当玩家召唤实体时触发（如铁傀儡、雪傀儡）。
 */
class SummonedEntityTrigger : public AbstractCriterionTrigger<SummonedEntityTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:summoned_entity";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player, const class Entity& entity);
};

/**
 * @brief 召唤实体触发器实例
 */
class SummonedEntityTriggerInstance : public CriterionInstance<SummonedEntityTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:summoned_entity";

    SummonedEntityTriggerInstance() = default;
    explicit SummonedEntityTriggerInstance(EntityPredicate entity);

    [[nodiscard]] bool test(const class Entity& entity) const;

    /// 携带上下文判定（支持 ContextAwarePredicate 条件列表写法）
    [[nodiscard]] bool test(const CriterionContext& context) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    EntityPredicate m_entity;
    /// 实体谓词的上下文写法（裸谓词对象同样兼容）
    ContextAwarePredicate m_entityContext;
};

/**
 * @brief 治愈僵尸村民触发器
 *
 * 当玩家治愈僵尸村民时触发。
 */
class CuredZombieVillagerTrigger : public AbstractCriterionTrigger<CuredZombieVillagerTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:cured_zombie_villager";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player, const class Entity& zombie, const class Entity& villager);
};

/**
 * @brief 治愈僵尸村民触发器实例
 */
class CuredZombieVillagerTriggerInstance : public CriterionInstance<CuredZombieVillagerTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:cured_zombie_villager";

    CuredZombieVillagerTriggerInstance() = default;
    CuredZombieVillagerTriggerInstance(EntityPredicate zombie, EntityPredicate villager);

    [[nodiscard]] bool test(const class Entity& zombie, const class Entity& villager) const;

    /// 携带上下文判定（zombie 为 killer 槽位，villager 为 this 槽位）
    [[nodiscard]] bool test(const CriterionContext& zombieContext, const CriterionContext& villagerContext) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    EntityPredicate m_zombie;
    EntityPredicate m_villager;
    /// zombie / villager 的上下文写法（裸谓词对象同样兼容）
    ContextAwarePredicate m_zombieContext;
    ContextAwarePredicate m_villagerContext;
};

/**
 * @brief 村民交易触发器
 *
 * 当玩家与村民交易时触发。
 */
class VillagerTradeTrigger : public AbstractCriterionTrigger<VillagerTradeTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:villager_trade";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player, const class Entity& villager, const class ItemStack& item);
};

/**
 * @brief 村民交易触发器实例
 */
class VillagerTradeTriggerInstance : public CriterionInstance<VillagerTradeTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:villager_trade";

    VillagerTradeTriggerInstance() = default;
    VillagerTradeTriggerInstance(EntityPredicate villager, ItemPredicate item);

    [[nodiscard]] bool test(const class Entity& villager, const ItemStack& item) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    EntityPredicate m_villager;
    ItemPredicate m_item;
};

/**
 * @brief 玩家与实体交互触发器
 *
 * 当玩家与实体交互时触发（如右键点击实体）。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.PlayerEntityInteractionTrigger
 */
class PlayerInteractedWithEntityTrigger : public AbstractCriterionTrigger<PlayerInteractedWithEntityTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:player_interacted_with_entity";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player, const class ItemStack& item, const class Entity& entity);
};

/**
 * @brief 玩家与实体交互触发器实例
 */
class PlayerInteractedWithEntityTriggerInstance : public CriterionInstance<PlayerInteractedWithEntityTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:player_interacted_with_entity";

    PlayerInteractedWithEntityTriggerInstance() = default;
    PlayerInteractedWithEntityTriggerInstance(ItemPredicate item, EntityPredicate entity);

    [[nodiscard]] bool test(const class ItemStack& item, const class Entity& entity) const;

    /// 携带上下文判定（支持 `[{"condition": "minecraft:entity_properties", ...}]` 写法）
    [[nodiscard]] bool test(const CriterionContext& context) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    ItemPredicate m_item;
    /// 实体上下文谓词（兼容裸 EntityPredicate 与 ContextAwarePredicate 条件列表）
    ContextAwarePredicate m_entityContext;
    EntityPredicate m_entity;
};

/**
 * @brief 玩家造成伤害触发器（对齐 vanilla PlayerHurtEntityTrigger）
 *
 * 玩家（伤害的真正造成者）对某实体造成伤害时触发。条件含：
 *  - damage：DamagePredicate（本次造成的伤害、受击方承受的伤害、造成者实体、是否格挡、伤害源）；
 *  - entity：受击实体谓词。
 *
 * 数据包 adventure/overoverkill（重锤一击 100 点）、shoot_arrow、throw_trident 依赖它。
 */
class PlayerHurtEntityTrigger : public AbstractCriterionTrigger<PlayerHurtEntityTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:player_hurt_entity";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    /// 触发（受击实体、伤害源、造成与承受的伤害、是否被格挡）
    void trigger(class ServerPlayer& player,
        const class Entity& entity,
        const class DamageSource& source,
        f32 dealt,
        f32 taken,
        bool blocked);
};

/**
 * @brief 玩家造成伤害触发器实例
 */
class PlayerHurtEntityTriggerInstance : public CriterionInstance<PlayerHurtEntityTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:player_hurt_entity";

    PlayerHurtEntityTriggerInstance() = default;

    [[nodiscard]] bool test(
        const class DamageSource& source, f32 dealt, f32 taken, bool blocked, const class Entity& entity) const;

    /// 携带上下文判定（entity 取 context.entity，player 条件同样生效）
    [[nodiscard]] bool test(
        const CriterionContext& context, const class DamageSource& source, f32 dealt, f32 taken, bool blocked) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    std::optional<DamagePredicate> m_damage;
    std::optional<EntityPredicate> m_entity;
    /// 可选的 player 条件（vanilla 各触发器的 player ContextAwarePredicate 字段）
    ContextAwarePredicate m_playerContext;
};

/**
 * @brief 实体伤害玩家触发器（对齐 vanilla EntityHurtPlayerTrigger）
 *
 * 玩家受到任意伤害时触发（与 player_hurt_entity 成对，vanilla 同在
 * LivingEntity.hurtServer 末尾派发）。条件为可选的 damage 谓词。
 */
class EntityHurtPlayerTrigger : public AbstractCriterionTrigger<EntityHurtPlayerTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:entity_hurt_player";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player, const class DamageSource& source, f32 dealt, f32 taken, bool blocked);
};

/**
 * @brief 实体伤害玩家触发器实例
 */
class EntityHurtPlayerTriggerInstance : public CriterionInstance<EntityHurtPlayerTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:entity_hurt_player";

    EntityHurtPlayerTriggerInstance() = default;

    [[nodiscard]] bool test(const class DamageSource& source, f32 dealt, f32 taken, bool blocked) const;

    /// 携带上下文判定（player 条件生效）
    [[nodiscard]] bool test(
        const CriterionContext& context, const class DamageSource& source, f32 dealt, f32 taken, bool blocked) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    std::optional<DamagePredicate> m_damage;
    /// 可选的 player 条件（vanilla 各触发器的 player ContextAwarePredicate 字段）
    ContextAwarePredicate m_playerContext;
};

/**
 * @brief 维度切换触发器（对齐 vanilla ChangeDimensionTrigger）
 *
 * 玩家跨维度迁移成功时触发（主世界↔下界↔末地、数据包自定义维度）。
 * 条件可选 from / to 两个维度资源位置。数据包 end/root、nether 目录下的 loot_* 进度依赖它。
 */
class ChangedDimensionTrigger : public AbstractCriterionTrigger<ChangedDimensionTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:changed_dimension";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player, const ResourceLocation& from, const ResourceLocation& to);
};

/**
 * @brief 维度切换触发器实例
 */
class ChangedDimensionTriggerInstance : public CriterionInstance<ChangedDimensionTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:changed_dimension";

    ChangedDimensionTriggerInstance() = default;

    /// vanilla matches(from, to)：from 不匹配即失败；to 缺省则匹配任意目标维度
    [[nodiscard]] bool test(const ResourceLocation& from, const ResourceLocation& to) const;

    /// 携带上下文判定（player 条件生效）
    [[nodiscard]] bool test(
        const CriterionContext& context, const ResourceLocation& from, const ResourceLocation& to) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

    [[nodiscard]] const std::optional<ResourceLocation>& getFrom() const noexcept { return m_from; }
    [[nodiscard]] const std::optional<ResourceLocation>& getTo() const noexcept { return m_to; }

private:
    std::optional<ResourceLocation> m_from;
    std::optional<ResourceLocation> m_to;
    /// 可选的 player 条件（vanilla 各触发器的 player ContextAwarePredicate 字段）
    ContextAwarePredicate m_playerContext;
};

/**
 * @brief 钓竿钓到东西触发器（对齐 vanilla FishingRodHookedTrigger）
 *
 * 玩家收杆时触发：钩住实体（item 收集为空）或钓到战利品（item 为本次掉落物）。
 * 条件可选 rod / entity / item 三个谓词。数据包 husbandry/fishy_business 等 4 处依赖它。
 */
class FishingRodHookedTrigger : public AbstractCriterionTrigger<FishingRodHookedTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:fishing_rod_hooked";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    /**
     * @brief 触发
     * @param player 钓鱼者
     * @param rod 使用中的钓竿（消耗耐久前的物品堆）
     * @param hookedEntity 被钩住的实体（无则 nullptr，对齐 vanilla getHookedIn 为空时传浮标自身）
     * @param items 本次钓到的掉落物
     */
    void trigger(class ServerPlayer& player,
        const class ItemStack& rod,
        const class Entity* hookedEntity,
        const class std::vector<class ItemStack>& items);
};

/**
 * @brief 钓竿钓到东西触发器实例
 */
class FishingRodHookedTriggerInstance : public CriterionInstance<FishingRodHookedTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:fishing_rod_hooked";

    FishingRodHookedTriggerInstance() = default;

    [[nodiscard]] bool test(const class ItemStack& rod,
        const class Entity* hookedEntity,
        const class std::vector<class ItemStack>& items) const;

    /// 携带上下文判定（player 条件生效）
    [[nodiscard]] bool test(const CriterionContext& context,
        const class ItemStack& rod,
        const class Entity* hookedEntity,
        const class std::vector<class ItemStack>& items) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    std::optional<advancement::ItemPredicate> m_rod;
    std::optional<advancement::EntityPredicate> m_entity;
    std::optional<advancement::ItemPredicate> m_item;
    /// 可选的 player 条件（vanilla 各触发器的 player ContextAwarePredicate 字段）
    ContextAwarePredicate m_playerContext;
};

/**
 * @brief 箭矢击杀触发器（对齐 vanilla KilledByArrowTrigger）
 *
 * 玩家射出的箭/三叉戟击杀实体时触发。条件：
 *  - victims：受害者实体谓词列表（多重集合匹配）；
 *  - unique_entity_types：被击杀的不同实体类型数量范围；
 *  - fired_from_weapon：射出武器的物品谓词（弩多杀进度用它限定弩）。
 */
class KilledByArrowTrigger : public AbstractCriterionTrigger<KilledByArrowTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:killed_by_arrow";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    /// 触发（本次击杀的实体集合 + 射出武器）
    void trigger(class ServerPlayer& player,
        const class std::vector<const class Entity*>& victims,
        const class ItemStack& firedFromWeapon);
};

/**
 * @brief 箭矢击杀触发器实例
 */
class KilledByArrowTriggerInstance : public CriterionInstance<KilledByArrowTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:killed_by_arrow";

    KilledByArrowTriggerInstance() = default;
    KilledByArrowTriggerInstance(std::vector<EntityPredicate> victims,
        IntBounds uniqueEntityTypes,
        std::optional<ItemPredicate> firedFromWeapon);

    [[nodiscard]] bool test(
        const std::vector<const class Entity*>& victims, const class ItemStack& firedFromWeapon) const;

    /// 携带上下文判定（player 条件生效）
    [[nodiscard]] bool test(const CriterionContext& context,
        const class std::vector<const class Entity*>& victims,
        const class ItemStack& firedFromWeapon) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    std::vector<EntityPredicate> m_victims;
    IntBounds m_uniqueEntityTypes;
    std::optional<ItemPredicate> m_firedFromWeapon;
    /// 可选的 player 条件（vanilla 各触发器的 player ContextAwarePredicate 字段）
    ContextAwarePredicate m_playerContext;
};

/**
 * @brief 玩家交互触发器（对齐 vanilla PlayerInteractTrigger）
 *
 * 玩家与实体交互（剪羊毛、给动物梳毛、给狼装/卸护甲等）时触发。
 * 条件为可选的 item 物品谓词与 entity（ContextAwarePredicate）。
 * player_sheared_equipment 复用同一实例类型。
 */
class PlayerInteractTrigger : public AbstractCriterionTrigger<PlayerInteractTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:player_sheared_equipment";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player, const class ItemStack& item, const class Entity& entity);
};

/**
 * @brief 玩家交互触发器实例
 */
class PlayerInteractTriggerInstance : public CriterionInstance<PlayerInteractTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:player_sheared_equipment";

    PlayerInteractTriggerInstance() = default;

    [[nodiscard]] bool test(const class ItemStack& item, const class Entity& entity) const;

    /// 携带上下文判定（item / entity / player 条件同时生效）
    [[nodiscard]] bool test(const CriterionContext& context) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    ItemPredicate m_item;
    ContextAwarePredicate m_entityContext;
    /// 可选的 player 条件（vanilla 各触发器的 player ContextAwarePredicate 字段）
    ContextAwarePredicate m_playerContext;
};

/**
 * @brief 开始骑乘触发器（对齐 vanilla StartRidingTrigger）
 *
 * 玩家开始骑乘实体时触发。条件只有可选的 player（ContextAwarePredicate），
 * 数据包 husbandry/ride_a_boat_with_a_goat 用 player.vehicle.passenger 限定
 * "船上载着山羊时开船"。
 */
class StartRidingTrigger : public AbstractCriterionTrigger<StartRidingTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:started_riding";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player);
};

/**
 * @brief 开始骑乘触发器实例
 */
class StartRidingTriggerInstance : public CriterionInstance<StartRidingTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:started_riding";

    StartRidingTriggerInstance() = default;

    [[nodiscard]] bool test(const CriterionContext& context) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    ContextAwarePredicate m_playerContext;
};

/**
 * @brief 闪电击中触发器（对齐 vanilla LightningStrikeTrigger）
 *
 * 玩家附近有闪电落地时触发。条件为可选的 lightning / bystander 实体谓词。
 */
class LightningStrikeTrigger : public AbstractCriterionTrigger<LightningStrikeTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:lightning_strike";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    /// 触发（闪电视为主要触发对象，旁观玩家作为 bystander 上下文）
    void trigger(class ServerPlayer& player, const class Entity& lightning, const class Entity& bystander);
};

/**
 * @brief 闪电击中触发器实例
 */
class LightningStrikeTriggerInstance : public CriterionInstance<LightningStrikeTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:lightning_strike";

    LightningStrikeTriggerInstance() = default;

    [[nodiscard]] bool test(const class Entity& lightning, const class Entity& bystander) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    ContextAwarePredicate m_lightningContext;
    ContextAwarePredicate m_bystanderContext;
    /// 可选的 player 条件（vanilla LightningStrikeTrigger.player）
    ContextAwarePredicate m_playerContext;
};

/**
 * @brief 距离触发器（对齐 vanilla DistanceTrigger）
 *
 * 同一套条件结构服务三个触发器 ID：
 *  - fall_from_height：玩家落地时对比"开始下落位置"与当前位置；
 *  - ride_entity_in_lava：在载具上身处熔岩时对比进入熔岩位置与当前位置；
 *  - nether_travel：从下界返回主世界时对比进入下界位置与当前位置。
 *
 * 条件：player（ContextAwarePredicate）/ start_position（LocationPredicate）/ distance
 * （DistancePredicate，absX/absY/absZ/x/y/z/horizontal 等）。
 */
class DistanceTrigger : public AbstractCriterionTrigger<DistanceTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:fall_from_height";

    explicit DistanceTrigger(const char* triggerId = TRIGGER_ID);

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(m_triggerId); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    /// 触发（起点位置 + 当前位置）
    void trigger(class ServerPlayer& player, const Vector3& startPos, const Vector3& endPos);

private:
    std::string m_triggerId;
};

/// nether_travel 触发器（与 DistanceTrigger 同条件结构，仅 ID 不同）
class NetherTravelTrigger : public DistanceTrigger {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:nether_travel";

    NetherTravelTrigger()
        : DistanceTrigger(TRIGGER_ID)
    {}
};

/// ride_entity_in_lava 触发器
class RideEntityInLavaTrigger : public DistanceTrigger {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:ride_entity_in_lava";

    RideEntityInLavaTrigger()
        : DistanceTrigger(TRIGGER_ID)
    {}
};

/**
 * @brief 距离触发器实例
 */
class DistanceTriggerInstance : public CriterionInstance<DistanceTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:fall_from_height";

    DistanceTriggerInstance() = default;

    [[nodiscard]] bool test(const Vector3& startPos, const Vector3& endPos) const;

    /// 携带上下文判定（player 条件为 ContextAwarePredicate 条件列表）
    [[nodiscard]] bool test(const CriterionContext& context, const Vector3& startPos, const Vector3& endPos) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    ContextAwarePredicate m_playerContext;
    std::optional<LocationPredicate> m_startPosition;
    std::optional<DistancePredicate> m_distance;
};

/**
 * @brief 爆炸冲量后坠落触发器（对齐 vanilla FallAfterExplosionTrigger）
 *
 * 玩家被爆炸（如风弹）弹起后开始下落、且下落起点不低于冲量发生点高度时触发。
 * 条件：player / start_position / distance / cause（爆炸来源实体，如 wind_charge）。
 * 数据包 adventure/who_needs_rockets 依赖它。
 */
class FallAfterExplosionTrigger : public AbstractCriterionTrigger<FallAfterExplosionTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:fall_after_explosion";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    /// 触发（爆炸冲量点 + 下落起点 + 当前位置 + 爆炸来源实体，可为 nullptr）
    void trigger(class ServerPlayer& player,
        const Vector3& impulsePos,
        const Vector3& startPos,
        const Vector3& endPos,
        const class Entity* cause);
};

/**
 * @brief 爆炸冲量后坠落触发器实例
 */
class FallAfterExplosionTriggerInstance : public CriterionInstance<FallAfterExplosionTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:fall_after_explosion";

    FallAfterExplosionTriggerInstance() = default;

    [[nodiscard]] bool test(
        const Vector3& impulsePos, const Vector3& startPos, const Vector3& endPos, const class Entity* cause) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    ContextAwarePredicate m_playerContext;
    std::optional<LocationPredicate> m_startPosition;
    std::optional<DistancePredicate> m_distance;
    ContextAwarePredicate m_causeContext;
};

} // namespace advancement
} // namespace mc
