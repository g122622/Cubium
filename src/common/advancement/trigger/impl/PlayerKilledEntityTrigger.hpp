#pragma once

#include "../CriterionTrigger.hpp"
#include "../conditions/EntityPredicate.hpp"
#include "../conditions/LocationPredicate.hpp"
#include <memory>

// 前向声明
namespace mc {
class Entity;
class DamageSource;
} // namespace mc

namespace mc::advancement {

// 前向声明 Instance 类
class PlayerKilledEntityTriggerInstance;
class EntityKilledPlayerTriggerInstance;

/**
 * @brief 玩家击杀实体触发器
 *
 * 当玩家击杀实体时触发。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.KilledTrigger
 */
class PlayerKilledEntityTrigger : public AbstractCriterionTrigger<PlayerKilledEntityTriggerInstance> {
public:
    /**
     * @brief 触发器ID
     */
    static constexpr const char* TRIGGER_ID = "minecraft:player_killed_entity";

    /**
     * @brief 获取触发器ID
     */
    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    /**
     * @brief 从JSON反序列化实例
     */
    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    /**
     * @brief 触发检测
     * @param player 玩家
     * @param entity 被击杀的实体
     * @param source 伤害源
     */
    void trigger(class ServerPlayer& player, const Entity& entity, const DamageSource& source);

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<PlayerKilledEntityTriggerInstance> entityKilled();
    static std::shared_ptr<PlayerKilledEntityTriggerInstance> entityKilled(const EntityPredicate& entity);
    static std::shared_ptr<PlayerKilledEntityTriggerInstance> killedByEntity(const EntityPredicate& killer);
};

/**
 * @brief 玩家击杀实体触发器实例
 */
class PlayerKilledEntityTriggerInstance : public CriterionInstance<PlayerKilledEntityTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:player_killed_entity";

    PlayerKilledEntityTriggerInstance() = default;

    /**
     * @brief 构造实例
     * @param entity 实体谓词
     * @param killingBlow 伤害源谓词
     */
    PlayerKilledEntityTriggerInstance(EntityPredicate entity, DamageSourcePredicate killingBlow);

    /**
     * @brief 检查条件是否满足
     * @param entity 被击杀的实体
     * @param source 伤害源
     * @return 是否满足
     */
    [[nodiscard]] bool test(const Entity& entity, const DamageSource& source) const;

    /**
     * @brief 从JSON解析
     */
    Result<void> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化条件为JSON
     */
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    EntityPredicate m_entity;
    DamageSourcePredicate m_killingBlow;
};

/**
 * @brief 实体击杀玩家触发器
 *
 * 当实体击杀玩家时触发。
 */
class EntityKilledPlayerTrigger : public AbstractCriterionTrigger<EntityKilledPlayerTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:entity_killed_player";

    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    [[nodiscard]] Result<std::shared_ptr<ICriterionInstance>> fromJson(const nlohmann::json& json) override;

    void trigger(class ServerPlayer& player, const Entity& entity, const DamageSource& source);
};

/**
 * @brief 实体击杀玩家触发器实例
 */
class EntityKilledPlayerTriggerInstance : public CriterionInstance<EntityKilledPlayerTriggerInstance> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:entity_killed_player";

    EntityKilledPlayerTriggerInstance() = default;
    EntityKilledPlayerTriggerInstance(EntityPredicate entity, DamageSourcePredicate killingBlow);

    /**
     * @brief 检查条件是否满足
     * @param entity 击杀者实体
     * @param source 伤害源
     * @return 是否满足
     */
    [[nodiscard]] bool test(const Entity& entity, const DamageSource& source) const;

    Result<void> fromJson(const nlohmann::json& json);
    [[nodiscard]] nlohmann::json conditionsToJson() const;

private:
    EntityPredicate m_entity;
    DamageSourcePredicate m_killingBlow;
};

} // namespace mc::advancement
