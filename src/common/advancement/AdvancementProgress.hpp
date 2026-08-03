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

#include "Advancement.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <chrono>
#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

/**
 * @brief 单个条件的进度
 *
 * 追踪单个条件的完成状态和完成时间。
 */
class CriterionProgress {
public:
    /**
     * @brief 默认构造（未完成）
     */
    CriterionProgress() = default;

    /**
     * @brief 检查是否已完成
     */
    [[nodiscard]] bool isObtained() const noexcept { return m_obtainedTime.has_value(); }

    /**
     * @brief 标记为已完成
     */
    void obtain();

    /**
     * @brief 重置为未完成
     */
    void reset();

    /**
     * @brief 获取完成时间（毫秒时间戳）
     */
    [[nodiscard]] std::optional<i64> getObtainedTime() const noexcept { return m_obtainedTime; }

    /**
     * @brief 从JSON解析
     * @param json JSON值
     * @return 条件进度
     */
    static CriterionProgress fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

private:
    std::optional<i64> m_obtainedTime; // 完成时间戳（毫秒）
};

/**
 * @brief 单个成就的进度
 *
 * 追踪玩家在特定成就上的进度，管理所有条件的完成状态。
 */
class AdvancementProgress {
public:
    /**
     * @brief 默认构造
     */
    AdvancementProgress() = default;

    /**
     * @brief 构造进度
     * @param advancement 成就
     */
    explicit AdvancementProgress(Advancement::Ptr advancement);

    // ========== 进度操作 ==========

    /**
     * @brief 授予条件
     * @param criterion 条件名称
     * @return 是否有变化
     */
    bool grantCriterion(const std::string& criterion);

    /**
     * @brief 撤销条件
     * @param criterion 条件名称
     * @return 是否有变化
     */
    bool revokeCriterion(const std::string& criterion);

    // ========== 状态查询 ==========

    /**
     * @brief 检查是否完成
     */
    [[nodiscard]] bool isDone() const;

    /**
     * @brief 检查是否有进度
     */
    [[nodiscard]] bool hasProgress() const;

    /**
     * @brief 获取完成百分比
     * @return 0.0-1.0
     */
    [[nodiscard]] f32 getPercent() const;

    /**
     * @brief 获取进度文本
     * @return 如 "3/5" 或 "Done"
     */
    [[nodiscard]] std::string getProgressText() const;

    /**
     * @brief 获取完成的条件数量
     */
    [[nodiscard]] size_t countCompletedCriteria() const;

    /**
     * @brief 获取未完成的条件数量
     */
    [[nodiscard]] size_t countUncompletedCriteria() const;

    /**
     * @brief 获取剩余需求组数量
     */
    [[nodiscard]] size_t countRemainingRequirements() const;

    /**
     * @brief 获取完成的需求组数量
     */
    [[nodiscard]] size_t countCompletedRequirements() const;

    // ========== 条件访问 ==========

    /**
     * @brief 获取条件进度
     * @param name 条件名称
     * @return 条件进度，如果不存在返回nullptr
     */
    [[nodiscard]] CriterionProgress* getCriterion(const std::string& name);
    [[nodiscard]] const CriterionProgress* getCriterion(const std::string& name) const;

    /**
     * @brief 获取所有条件进度
     */
    [[nodiscard]] const std::map<std::string, CriterionProgress>& getCriteria() const noexcept { return m_criteria; }

    /**
     * @brief 获取关联的成就
     */
    [[nodiscard]] Advancement::Ptr getAdvancement() const noexcept { return m_advancement; }

    // ========== 更新 ==========

    /**
     * @brief 更新进度结构（当成就定义变化时）
     * @param advancement 新的成就定义
     */
    void update(Advancement::Ptr advancement);

    // ========== 序列化 ==========

    /**
     * @brief 从JSON解析
     * @param json JSON对象
     * @param advancement 成就
     * @return 进度或错误
     */
    static Result<AdvancementProgress> fromJson(const nlohmann::json& json, Advancement::Ptr advancement);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

private:
    Advancement::Ptr m_advancement;
    std::map<std::string, CriterionProgress> m_criteria;
    std::vector<std::vector<std::string>> m_requirements;

    /**
     * @brief 计算是否完成
     */
    [[nodiscard]] bool _computeDone() const;
};

} // namespace mc::advancement
