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
 * The above copyright notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/item/loot/conditions/LootCondition.hpp"

namespace mc {
namespace loot {

/**
 * @brief 战利品谓词管理器
 *
 * 管理所有已注册的命名战利品条件谓词。
 * 战利品表中的 minecraft:reference 条件通过此管理器查找引用的谓词。
 *
 * 谓词从数据包的 predicates/ 目录加载，每个 JSON 文件定义一个命名的 LootCondition。
 * 路径映射遵循 MC 数据包规范：
 *   data/<namespace>/predicates/<path>.json -> <namespace>:<path>
 */
class LootPredicateManager {
public:
    LootPredicateManager() = default;
    ~LootPredicateManager() = default;

    // 禁止拷贝
    LootPredicateManager(const LootPredicateManager&) = delete;
    LootPredicateManager& operator=(const LootPredicateManager&) = delete;

    // 允许移动
    LootPredicateManager(LootPredicateManager&&) noexcept = default;
    LootPredicateManager& operator=(LootPredicateManager&&) noexcept = default;

    // ========== 谓词管理 ==========

    /**
     * @brief 注册命名谓词
     * @param id 谓词ID（如 "minecraft:gameplay/raid"）
     * @param condition 谓词条件
     */
    void registerPredicate(const std::string& id, std::unique_ptr<LootCondition> condition);

    /**
     * @brief 获取命名谓词
     * @param id 谓词ID
     * @return 谓词条件指针，不存在返回nullptr
     */
    [[nodiscard]] const LootCondition* getPredicate(const std::string& id) const;

    /**
     * @brief 检查是否有命名谓词
     */
    [[nodiscard]] bool hasPredicate(const std::string& id) const;

    /**
     * @brief 清空所有已注册的谓词
     */
    void clear();

    /**
     * @brief 获取所有已注册的谓词ID
     */
    [[nodiscard]] std::vector<std::string> getAllPredicateIds() const;

private:
    std::unordered_map<std::string, std::unique_ptr<LootCondition>> m_predicates;
};

} // namespace loot
} // namespace mc
