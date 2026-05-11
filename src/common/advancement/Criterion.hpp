#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "trigger/CriterionTrigger.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

namespace mc::advancement {

/**
 * @brief 成就条件
 *
 * 每个成就可以有多个条件，条件的完成情况决定成就是否完成。
 * 参考 MC 1.16.5: net.minecraft.advancements.Criterion
 *
 * 条件由触发器实例组成，当触发器触发时检查条件是否满足。
 * 例如："diamond" 条件使用 inventory_changed 触发器检查玩家是否获得钻石。
 */
class Criterion {
public:
    Criterion() = default;

    /**
     * @brief 构造条件
     * @param name 条件名称
     * @param triggerInstance 触发器实例
     */
    Criterion(std::string name, std::shared_ptr<ICriterionInstance> triggerInstance);

    /**
     * @brief 获取条件名称
     */
    [[nodiscard]] const std::string& getName() const noexcept { return m_name; }

    /**
     * @brief 获取触发器实例
     */
    [[nodiscard]] const std::shared_ptr<ICriterionInstance>& getTriggerInstance() const noexcept {
        return m_triggerInstance;
    }

    /**
     * @brief 获取触发器ID
     */
    [[nodiscard]] ResourceLocation getTrigger() const noexcept {
        return m_triggerInstance ? m_triggerInstance->getId() : ResourceLocation();
    }

    /**
     * @brief 从JSON解析
     * @param name 条件名称
     * @param json JSON对象
     * @return 条件或错误
     */
    static Result<Criterion> fromJson(const std::string& name, const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

private:
    std::string m_name;
    std::shared_ptr<ICriterionInstance> m_triggerInstance;
};

} // namespace mc::advancement
