#pragma once

#include "../CriterionTrigger.hpp"

namespace mc::advancement {

// Forward declarations
class ServerPlayer;

/**
 * @brief Tick触发器实例
 */
class TickTriggerInstance : public CriterionInstance<TickTriggerInstance> {
public:
    /**
     * @brief 触发器ID
     */
    static constexpr const char* TRIGGER_ID = "minecraft:tick";

    /**
     * @brief 从JSON解析
     * Tick触发器没有条件
     */
    Result<void> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化条件为JSON
     */
    [[nodiscard]] nlohmann::json conditionsToJson() const;
};

/**
 * @brief Tick触发器
 *
 * 每游戏tick触发一次，用于检测持续条件。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.TickTrigger
 */
class TickTrigger : public AbstractCriterionTrigger<TickTriggerInstance> {
public:
    /**
     * @brief 触发器ID
     */
    static constexpr const char* TRIGGER_ID = "minecraft:tick";

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
     */
    void trigger(ServerPlayer& player);
};

} // namespace mc::advancement
