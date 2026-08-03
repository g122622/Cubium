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

#include "common/core/Types.hpp"
#include "common/core/settings/SettingsTypes.hpp"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc {

/**
 * @brief 资源包条目
 *
 * 表示资源包列表中的单个资源包配置。
 */
struct ResourcePackEntry {
    std::string path;    ///< 资源包路径（绝对路径或相对于资源包目录的相对路径）
    bool enabled = true; ///< 是否启用
    i32 priority = 0;    ///< 优先级（越大越优先，会被后加载覆盖较低优先级的资源）

    /**
     * @brief 默认构造函数
     */
    ResourcePackEntry() = default;

    /**
     * @brief 构造资源包条目
     * @param p 路径
     * @param e 是否启用
     * @param pr 优先级
     */
    ResourcePackEntry(std::string p, bool e = true, i32 pr = 0)
        : path(std::move(p))
        , enabled(e)
        , priority(pr)
    {}

    /**
     * @brief 从 JSON 解析
     */
    static ResourcePackEntry fromJson(const nlohmann::json& j)
    {
        ResourcePackEntry entry;
        if (j.contains("path") && j["path"].is_string()) {
            entry.path = j["path"].get<std::string>();
        }
        if (j.contains("enabled") && j["enabled"].is_boolean()) {
            entry.enabled = j["enabled"].get<bool>();
        }
        if (j.contains("priority") && j["priority"].is_number_integer()) {
            entry.priority = j["priority"].get<i32>();
        }
        return entry;
    }

    /**
     * @brief 序列化到 JSON
     */
    nlohmann::json toJson() const
    {
        nlohmann::json j;
        j["path"] = path;
        j["enabled"] = enabled;
        j["priority"] = priority;
        return j;
    }

    /**
     * @brief 相等比较
     */
    bool operator==(const ResourcePackEntry& other) const
    {
        return path == other.path && enabled == other.enabled && priority == other.priority;
    }

    /**
     * @brief 不等比较
     */
    bool operator!=(const ResourcePackEntry& other) const { return !(*this == other); }
};

/**
 * @brief 资源包列表设置选项
 *
 * 管理资源包列表的设置选项，支持启用/禁用、优先级排序等功能。
 * 序列化为 JSON 数组格式。
 *
 * 使用示例:
 * @code
 * ResourcePackListOption resourcePacks{"resourcePacks"};
 * resourcePacks.add(ResourcePackEntry{"packs/fancy.zip", true, 1});
 * resourcePacks.add(ResourcePackEntry{"packs/simple", true, 0});
 *
 * // 按优先级排序（高优先级在前）
 * auto sorted = resourcePacks.getSortedEntries();
 *
 * // 变更回调
 * resourcePacks.onChange([](const std::vector<ResourcePackEntry>& packs) {
 *     spdlog::info("Resource packs changed, count: {}", packs.size());
 * });
 * @endcode
 */
class ResourcePackListOption : public IOption {
public:
    /**
     * @brief 构造资源包列表选项
     * @param key 选项键名
     */
    explicit ResourcePackListOption(std::string key)
        : m_key(std::move(key))
    {}

    /**
     * @brief 从默认列表构造
     * @param key 选项键名
     * @param defaultEntries 默认资源包列表
     */
    ResourcePackListOption(std::string key, std::vector<ResourcePackEntry> defaultEntries)
        : m_key(std::move(key))
        , m_entries(std::move(defaultEntries))
        , m_default(m_entries)
    {}

    // ========================================================================
    // IOption 接口实现
    // ========================================================================

    [[nodiscard]] std::string getKey() const override { return m_key; }

    [[nodiscard]] SettingsValue getValue() const override
    {
        // 返回条目数量作为整数值
        return static_cast<i32>(m_entries.size());
    }

    bool setValue(const SettingsValue& /*value*/) override
    {
        // 不支持通过 setValue 设置列表
        // 列表应该通过 add/remove/clear 等方法操作
        return false;
    }

    void serialize(nlohmann::json& j) const override
    {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& entry : m_entries) {
            arr.push_back(entry.toJson());
        }
        j[m_key] = arr;
    }

    void deserialize(const nlohmann::json& j) override
    {
        if (j.contains(m_key) && j[m_key].is_array()) {
            m_entries.clear();
            for (const auto& item : j[m_key]) {
                m_entries.push_back(ResourcePackEntry::fromJson(item));
            }
            notifyChange();
        }
    }

    void reset() override
    {
        m_entries = m_default;
        notifyChange();
    }

    [[nodiscard]] bool isDefault() const override { return m_entries == m_default; }

    // ========================================================================
    // 列表操作方法
    // ========================================================================

    /**
     * @brief 获取所有条目
     * @return 条目列表的常量引用
     */
    [[nodiscard]] const std::vector<ResourcePackEntry>& entries() const { return m_entries; }

    /**
     * @brief 获取条目数量
     */
    [[nodiscard]] size_t size() const { return m_entries.size(); }

    /**
     * @brief 检查列表是否为空
     */
    [[nodiscard]] bool empty() const { return m_entries.empty(); }

    /**
     * @brief 添加资源包条目
     * @param entry 要添加的条目
     */
    void add(const ResourcePackEntry& entry)
    {
        // 检查是否已存在相同路径的条目
        auto it = std::find_if(
            m_entries.begin(), m_entries.end(), [&](const ResourcePackEntry& e) { return e.path == entry.path; });
        if (it != m_entries.end()) {
            // 更新已存在的条目
            *it = entry;
        } else {
            m_entries.push_back(entry);
        }
        notifyChange();
    }

    /**
     * @brief 移除资源包条目
     * @param path 资源包路径
     * @return 是否成功移除
     */
    bool remove(const std::string& path)
    {
        auto it = std::find_if(
            m_entries.begin(), m_entries.end(), [&](const ResourcePackEntry& e) { return e.path == path; });
        if (it != m_entries.end()) {
            m_entries.erase(it);
            notifyChange();
            return true;
        }
        return false;
    }

    /**
     * @brief 查找资源包条目
     * @param path 资源包路径
     * @return 条目指针，未找到返回 nullptr
     */
    [[nodiscard]] const ResourcePackEntry* find(const std::string& path) const
    {
        auto it = std::find_if(
            m_entries.begin(), m_entries.end(), [&](const ResourcePackEntry& e) { return e.path == path; });
        return it != m_entries.end() ? &(*it) : nullptr;
    }

    /**
     * @brief 更新资源包条目
     * @param path 资源包路径
     * @param entry 新的条目值
     * @return 是否成功更新
     */
    bool update(const std::string& path, const ResourcePackEntry& entry)
    {
        auto it = std::find_if(
            m_entries.begin(), m_entries.end(), [&](const ResourcePackEntry& e) { return e.path == path; });
        if (it != m_entries.end()) {
            *it = entry;
            notifyChange();
            return true;
        }
        return false;
    }

    /**
     * @brief 启用资源包
     * @param path 资源包路径
     * @param enabled 是否启用
     * @return 是否成功更新
     */
    bool setEnabled(const std::string& path, bool enabled)
    {
        auto it = std::find_if(
            m_entries.begin(), m_entries.end(), [&](const ResourcePackEntry& e) { return e.path == path; });
        if (it != m_entries.end() && it->enabled != enabled) {
            it->enabled = enabled;
            notifyChange();
            return true;
        }
        return false;
    }

    /**
     * @brief 设置资源包优先级
     * @param path 资源包路径
     * @param priority 新优先级
     * @return 是否成功更新
     */
    bool setPriority(const std::string& path, i32 priority)
    {
        auto it = std::find_if(
            m_entries.begin(), m_entries.end(), [&](const ResourcePackEntry& e) { return e.path == path; });
        if (it != m_entries.end() && it->priority != priority) {
            it->priority = priority;
            notifyChange();
            return true;
        }
        return false;
    }

    /**
     * @brief 清空所有条目
     */
    void clear()
    {
        if (!m_entries.empty()) {
            m_entries.clear();
            notifyChange();
        }
    }

    /**
     * @brief 设置所有条目
     * @param entries 新的条目列表
     */
    void setEntries(std::vector<ResourcePackEntry> entries)
    {
        m_entries = std::move(entries);
        notifyChange();
    }

    /**
     * @brief 获取按优先级排序的条目列表
     *
     * 返回按优先级降序排列的列表（高优先级在前）。
     * 注意：MC 的资源加载顺序是先加载低优先级，后加载高优先级，
     * 高优先级的资源会覆盖低优先级的同名资源。
     *
     * @return 排序后的条目列表
     */
    [[nodiscard]] std::vector<ResourcePackEntry> getSortedEntries() const
    {
        std::vector<ResourcePackEntry> sorted = m_entries;
        std::sort(sorted.begin(), sorted.end(), [](const ResourcePackEntry& a, const ResourcePackEntry& b) {
            return a.priority > b.priority; // 降序，高优先级在前
        });
        return sorted;
    }

    /**
     * @brief 获取已启用的条目
     * @return 已启用的条目列表
     */
    [[nodiscard]] std::vector<ResourcePackEntry> getEnabledEntries() const
    {
        std::vector<ResourcePackEntry> enabled;
        for (const auto& entry : m_entries) {
            if (entry.enabled) {
                enabled.push_back(entry);
            }
        }
        return enabled;
    }

    /**
     * @brief 获取已启用并按优先级排序的条目
     * @return 排序后的已启用条目列表
     */
    [[nodiscard]] std::vector<ResourcePackEntry> getSortedEnabledEntries() const
    {
        std::vector<ResourcePackEntry> enabled = getEnabledEntries();
        std::sort(enabled.begin(), enabled.end(), [](const ResourcePackEntry& a, const ResourcePackEntry& b) {
            return a.priority > b.priority;
        });
        return enabled;
    }

    // ========================================================================
    // 迭代器支持
    // ========================================================================

    [[nodiscard]] std::vector<ResourcePackEntry>::const_iterator begin() const { return m_entries.begin(); }
    [[nodiscard]] std::vector<ResourcePackEntry>::const_iterator end() const { return m_entries.end(); }
    [[nodiscard]] std::vector<ResourcePackEntry>::const_iterator cbegin() const { return m_entries.cbegin(); }
    [[nodiscard]] std::vector<ResourcePackEntry>::const_iterator cend() const { return m_entries.cend(); }

    // ========================================================================
    // 变更回调
    // ========================================================================

    /**
     * @brief 设置变更回调
     * @param callback 变更时调用的函数
     */
    void onChange(std::function<void(const std::vector<ResourcePackEntry>&)> callback)
    {
        m_callback = std::move(callback);
    }

private:
    std::string m_key;
    std::vector<ResourcePackEntry> m_entries;
    std::vector<ResourcePackEntry> m_default;
    std::function<void(const std::vector<ResourcePackEntry>&)> m_callback;

    /**
     * @brief 通知变更
     */
    void notifyChange()
    {
        if (m_callback) {
            m_callback(m_entries);
        }
    }
};

} // namespace mc
