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

#include "AdvancementDisplay.hpp"
#include "AdvancementRewards.hpp"
#include "Criterion.hpp"
#include "common/advancement/trigger/CriterionTrigger.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

// 前向声明
namespace mc::text {
class ITextComponent;
}

namespace mc::advancement {

/**
 * @brief 需求策略
 *
 * 定义条件之间的逻辑关系。
 */
enum class RequirementsStrategy : u8 {
    AND, ///< 所有条件都必须满足（默认）
    OR   ///< 任一条件满足即可
};

/**
 * @brief 成就定义
 *
 * 成就是游戏中可解锁的里程碑，由条件、显示信息和奖励组成。
 * 成就可以形成树形结构，子成就需要父成就完成后才可见。
 *
 * JSON格式示例：
 * @code
 * {
 *   "parent": "minecraft:story/root",
 *   "display": {
 *     "icon": {"item": "minecraft:diamond"},
 *     "title": "Diamonds!",
 *     "description": "Acquire diamonds",
 *     "frame": "task"
 *   },
 *   "criteria": {
 *     "diamond": {
 *       "trigger": "minecraft:inventory_changed",
 *       "conditions": {"items": [{"item": "minecraft:diamond"}]}
 *     }
 *   },
 *   "requirements": [["diamond"]]
 * }
 * @endcode
 */
class Advancement {
public:
    using Ptr = std::shared_ptr<const Advancement>;

    /**
     * @brief 默认构造
     */
    Advancement() = default;

    /**
     * @brief 构造成就
     * @param id 成就ID
     * @param parent 父成就ID（可选）
     * @param display 显示信息（可选）
     * @param rewards 奖励（可选）
     * @param criteria 条件映射
     * @param requirements 需求矩阵
     */
    Advancement(ResourceLocation id,
        std::optional<ResourceLocation> parent,
        std::optional<AdvancementDisplay> display,
        std::optional<AdvancementRewards> rewards,
        std::map<std::string, Criterion> criteria,
        std::vector<std::vector<std::string>> requirements);

    // ========== 基本信息 ==========

    /**
     * @brief 获取成就ID
     */
    [[nodiscard]] const ResourceLocation& getId() const noexcept { return m_id; }

    /**
     * @brief 获取父成就ID
     */
    [[nodiscard]] const std::optional<ResourceLocation>& getParent() const noexcept { return m_parent; }

    /**
     * @brief 获取显示信息
     */
    [[nodiscard]] const std::optional<AdvancementDisplay>& getDisplay() const noexcept { return m_display; }

    /**
     * @brief 获取奖励
     */
    [[nodiscard]] const std::optional<AdvancementRewards>& getRewards() const noexcept { return m_rewards; }

    // ========== 条件 ==========

    /**
     * @brief 获取所有条件
     */
    [[nodiscard]] const std::map<std::string, Criterion>& getCriteria() const noexcept { return m_criteria; }

    /**
     * @brief 获取需求矩阵
     *
     * 需求矩阵是一个二维数组：
     * - 外层数组的每个元素是一个"需求组"
     * - 每个需求组内的条件是OR关系（任一满足）
     * - 需求组之间是AND关系（所有组都必须满足）
     */
    [[nodiscard]] const std::vector<std::vector<std::string>>& getRequirements() const noexcept
    {
        return m_requirements;
    }

    // ========== 子成就（运行时填充） ==========

    /**
     * @brief 获取子成就列表
     */
    [[nodiscard]] const std::vector<Ptr>& getChildren() const noexcept { return m_children; }

    /**
     * @brief 添加子成就
     * @param child 子成就
     */
    void addChild(Ptr child) const;

    // ========== 辅助方法 ==========

    /**
     * @brief 检查是否为根成就（无父成就）
     */
    [[nodiscard]] bool isRoot() const noexcept { return !m_parent.has_value(); }

    /**
     * @brief 检查是否有显示信息
     */
    [[nodiscard]] bool hasDisplay() const noexcept { return m_display.has_value(); }

    /**
     * @brief 获取显示文本（用于聊天消息）
     */
    [[nodiscard]] std::unique_ptr<text::ITextComponent> getDisplayText() const;

    // ========== 序列化 ==========

    /**
     * @brief 从JSON解析
     * @param id 成就ID
     * @param json JSON对象
     * @return 成就或错误
     */
    static Result<Advancement> fromJson(const ResourceLocation& id, const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

    // ========== Builder模式 ==========

    /**
     * @brief 成就构建器
     */
    class Builder {
    public:
        Builder() = default;
        explicit Builder(ResourceLocation id)
            : m_id(std::move(id))
        {}

        Builder& parent(const ResourceLocation& parent);
        Builder& display(AdvancementDisplay display);
        Builder& rewards(AdvancementRewards rewards);
        Builder& criterion(const std::string& name, std::shared_ptr<ICriterionInstance> instance);
        Builder& requirements(const std::vector<std::vector<std::string>>& requirements);
        Builder& requirementsStrategy(RequirementsStrategy strategy);

        /**
         * @brief 构建成就
         */
        Result<Advancement> build();

        /**
         * @brief 注册到消费者
         * @param consumer 消费函数
         * @param id 成就ID
         * @return 构建的成就
         */
        Ptr registerTo(std::function<void(Ptr)> consumer, const ResourceLocation& id);

    private:
        mutable ResourceLocation m_id;
        std::optional<ResourceLocation> m_parent;
        std::optional<AdvancementDisplay> m_display;
        std::optional<AdvancementRewards> m_rewards;
        std::map<std::string, Criterion> m_criteria;
        std::vector<std::vector<std::string>> m_requirements;
        RequirementsStrategy m_requirementsStrategy = RequirementsStrategy::AND;
    };

private:
    ResourceLocation m_id;
    std::optional<ResourceLocation> m_parent;
    std::optional<AdvancementDisplay> m_display;
    std::optional<AdvancementRewards> m_rewards;
    std::map<std::string, Criterion> m_criteria;
    std::vector<std::vector<std::string>> m_requirements;

    // 运行时填充的子成就（mutable以便在const对象上修改）
    mutable std::vector<Ptr> m_children;
};

} // namespace mc::advancement
