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

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "LootTable.hpp"
#include "common/item/loot/conditions/LootCondition.hpp"

namespace mc {
namespace loot {

// 前置声明
class LootPredicateManager;

/**
 * @brief 掉落表管理器
 *
 * 管理所有注册的掉落表，同时可选地持有谓词管理器引用，
 * 以便为 ReferenceCondition 提供谓词查找能力。
 */
class LootTableManager {
public:
    LootTableManager() = default;
    ~LootTableManager() = default;

    // 禁止拷贝
    LootTableManager(const LootTableManager&) = delete;
    LootTableManager& operator=(const LootTableManager&) = delete;

    // 允许移动
    LootTableManager(LootTableManager&&) noexcept = default;
    LootTableManager& operator=(LootTableManager&&) noexcept = default;

    // ========== 表管理 ==========

    /**
     * @brief 注册掉落表
     * @param id 掉落表ID（如 "minecraft:entities/pig"）
     * @param table 掉落表
     */
    void registerTable(const std::string& id, std::unique_ptr<LootTable> table);

    /**
     * @brief 获取掉落表
     * @param id 掉落表ID
     * @return 掉落表指针，不存在返回nullptr
     */
    [[nodiscard]] const LootTable* getTable(const std::string& id) const;

    /**
     * @brief 检查是否有掉落表
     */
    [[nodiscard]] bool hasTable(const std::string& id) const;

    /**
     * @brief 清空所有已注册的掉落表
     */
    void clear();

    /**
     * @brief 获取所有已注册的掉落表ID
     */
    [[nodiscard]] std::vector<std::string> getAllTableIds() const;

    // ========== 默认表 ==========

    /**
     * @brief 获取空掉落表
     */
    [[nodiscard]] static const LootTable& getEmptyTable();

    // ========== 谓词管理 ==========

    /**
     * @brief 设置谓词管理器引用
     *
     * ReferenceCondition 通过此引用查找命名谓词。
     *
     * @param manager 谓词管理器指针（生命周期须长于此对象）
     */
    void setPredicateManager(const LootPredicateManager* manager) noexcept { m_predicateManager = manager; }

    /**
     * @brief 获取谓词管理器
     */
    [[nodiscard]] const LootPredicateManager* getPredicateManager() const noexcept { return m_predicateManager; }

    /**
     * @brief 查找命名谓词
     *
     * 便捷方法，委托给谓词管理器。
     *
     * @param id 谓词ID
     * @return 谓词条件指针，不存在或无谓词管理器时返回nullptr
     */
    [[nodiscard]] const LootCondition* getPredicate(const std::string& id) const noexcept;

private:
    std::unordered_map<std::string, std::unique_ptr<LootTable>> m_tables;
    const LootPredicateManager* m_predicateManager = nullptr;
};

} // namespace loot
} // namespace mc
