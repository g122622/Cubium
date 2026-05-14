#pragma once

#include "../CriterionTrigger.hpp"
#include "common/util/assert/AssertAll.hpp"

namespace mc::advancement {

// Forward declare the Instance
class ImpossibleTriggerInstance;

/**
 * @brief 不可能完成的触发器
 *
 * 用于创建无法自动完成的成就条件。
 * 通常用于配方解锁成就（需要手动授予）或调试目的。
 *
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.ImpossibleTrigger
 */
class ImpossibleTrigger : public AbstractCriterionTrigger<ImpossibleTriggerInstance> {
public:
    /**
     * @brief 触发器ID
     */
    static constexpr const char* TRIGGER_ID = "minecraft:impossible";

    /**
     * @brief 获取触发器ID
     */
    [[nodiscard]] ResourceLocation getId() const override { return ResourceLocation(TRIGGER_ID); }

    /**
     * @brief 触发检测
     *
     * 不可能触发器永远不会触发。
     */
    void trigger(::mc::server::PlayerAdvancements& advancements)
    {
        // 不可能触发器永远不会触发
        MC_UNUSED(advancements);
    }
};

/**
 * @brief 不可能触发器实例
 */
class ImpossibleTriggerInstance : public CriterionInstance<ImpossibleTriggerInstance> {
public:
    /**
     * @brief 触发器ID
     */
    static constexpr const char* TRIGGER_ID = ImpossibleTrigger::TRIGGER_ID;

    /**
     * @brief 从JSON解析
     * 不可能触发器没有条件，直接返回空实例
     */
    Result<void> fromJson(const nlohmann::json& json)
    {
        // 不可能触发器没有任何条件
        MC_UNUSED(json);
        return {};
    }

    /**
     * @brief 序列化条件为JSON
     */
    [[nodiscard]] nlohmann::json conditionsToJson() const
    {
        return nullptr; // 无条件
    }
};

} // namespace mc::advancement
