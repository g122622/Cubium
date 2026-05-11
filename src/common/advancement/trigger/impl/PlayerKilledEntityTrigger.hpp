#pragma once

#include "../CriterionTrigger.hpp"
#include "../conditions/EntityPredicate.hpp"
#include "../conditions/LocationPredicate.hpp"
#include "../conditions/DistancePredicate.hpp"
#include <memory>

// 前向声明
namespace mc {
    class Entity;
    struct DamageSource;
}

namespace mc::advancement {

/**
 * @brief 玩家击杀实体触发器
 *
 * 当玩家击杀实体时触发。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.KilledTrigger
 */
class PlayerKilledEntityTrigger : public AbstractCriterionTrigger<PlayerKilledEntityTrigger> {
public:
    /**
     * @brief 触发器ID
     */
    static constexpr const char* TRIGGER_ID = "minecraft:player_killed_entity";

    /**
     * @brief 触发器实例
     */
    class Instance : public CriterionInstance<Instance> {
    public:
        Instance() = default;

        /**
         * @brief 构造实例
         * @param entity 实体谓词
         * @param killingBlow 伤害源谓词
         */
        Instance(EntityPredicate entity, DamageSourcePredicate killingBlow);

        /**
         * @brief 检查条件是否满足
         * @param player 玩家
         * @param entity 被击杀的实体
         * @param source 伤害源
         * @return 是否满足
         */
        [[nodiscard]] bool test(
            class ServerPlayer& player,
            const Entity& entity,
            const DamageSource& source
        ) const;

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
     * @brief 获取触发器ID
     */
    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    /**
     * @brief 从JSON反序列化实例
     */
    [[nodiscard]] Result<std::shared_ptr<Instance>> fromJson(const nlohmann::json& json);

    /**
     * @brief 触发检测
     * @param player 玩家
     * @param entity 被击杀的实体
     * @param source 伤害源
     */
    void trigger(
        class ServerPlayer& player,
        const Entity& entity,
        const DamageSource& source
    );

    // ========== 静态工厂方法 ==========

    static std::shared_ptr<Instance> entityKilled();
    static std::shared_ptr<Instance> entityKilled(const EntityPredicate& entity);
    static std::shared_ptr<Instance> killedByEntity(const EntityPredicate& killer);
};

/**
 * @brief 实体击杀玩家触发器
 *
 * 当实体击杀玩家时触发。
 */
class EntityKilledPlayerTrigger : public AbstractCriterionTrigger<EntityKilledPlayerTrigger> {
public:
    static constexpr const char* TRIGGER_ID = "minecraft:entity_killed_player";

    class Instance : public CriterionInstance<Instance> {
    public:
        Instance() = default;
        Instance(EntityPredicate entity, DamageSourcePredicate killingBlow);

        [[nodiscard]] bool test(
            class ServerPlayer& player,
            const Entity& entity,
            const DamageSource& source
        ) const;

        Result<void> fromJson(const nlohmann::json& json);
        [[nodiscard]] nlohmann::json conditionsToJson() const;

    private:
        EntityPredicate m_entity;
        DamageSourcePredicate m_killingBlow;
    };

    [[nodiscard]] ResourceLocation getId() const override {
        return ResourceLocation(TRIGGER_ID);
    }

    [[nodiscard]] Result<std::shared_ptr<Instance>> fromJson(const nlohmann::json& json);

    void trigger(
        class ServerPlayer& player,
        const Entity& entity,
        const DamageSource& source
    );
};

} // namespace mc::advancement
