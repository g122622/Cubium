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

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mc::mod::bedrock::addon {

/** @brief 单个脚本上下文内的类名、编号和借用原型索引。 */
class ScriptClassRegistry {
public:
    ScriptClassRegistry() noexcept = default;

    /**
     * @brief 登记 classId → 原型句柄 + 类名（注册期调）。
     *
     * 同 classId 重复登记覆盖（引擎重建后重新登记场景）。
     */
    void registerClass(u64 classId, void* proto, std::string_view name) noexcept;

    /**
     * @brief 注销 classId 的登记（原型句柄所有权方销毁句柄前调）。
     *
     * 本注册表只存裸指针不持有所有权，故持有句柄的绑定上下文在释放句柄**之前**必须调用本方法，
     * 否则表中残留指向已释放句柄的条目，运行期 `proto`/`protoByName` 会把悬垂句柄交给
     * `ScriptObjectRegistry::wrap` / `setPrototypeOf`。由上下文在析构时逐条注销（见
     * `QuickJSBindingContext::~QuickJSBindingContext`），可覆盖 shutdown / reload / 单插件卸载 /
     * 上下文移动赋值等全部销毁路径，无需各调用点自行 `clear()`。
     *
     * 幂等：未登记时直接返回。同名类被后来的上下文重新登记时，只在该名仍指向本 classId 时才删除
     * 名索引，避免误删新一代上下文的映射。
     */
    void unregisterClass(u64 classId) noexcept;

    /**
     * @brief 按 classId 取原型句柄（运行期 wrap 调）；未登记返回 nullptr。
     */
    [[nodiscard]] void* proto(u64 classId) const noexcept;

    /**
     * @brief 按类名取原型句柄；未登记返回 nullptr。
     */
    [[nodiscard]] void* protoByName(std::string_view name) const noexcept;

    /**
     * @brief 按类名取 classId；未登记返回 0。
     */
    [[nodiscard]] u64 classIdByName(std::string_view name) const noexcept;

    /**
     * @brief 按类名取全部 classId（同一上下文内同名类可能登记多次）。
     *
     * 上下文内每个插件各自执行一次模组工厂（例如每个 GameTest 行为包都会装一遍
     * `@minecraft/server`），同名类因此会分多次 `registerClass` 落到不同 classId 上。名索引只保留
     * 最后一次登记，按名解开对象时若只用最后那个就会漏掉其它批次包装出的对象。
     *
     * 顺序：首次 off 的项为最近一次登记（与 `classIdByName` 一致），其后为更早的登记。
     */
    [[nodiscard]] std::vector<u64> classIdsByName(std::string_view name) const;

    /**
     * @brief 清空所有条目（脚本引擎重建/测试隔离用）。
     */
    void clear() noexcept;

private:
    struct Entry {
        void* proto;
        std::string name;
    };

    std::unordered_map<u64, Entry> m_byId;
    std::unordered_map<std::string, u64> m_idByName;
    /// 同名类的全部 classId，最近登记在前。与 m_byId/m_idByName 同步维护。
    std::unordered_map<std::string, std::vector<u64>> m_idsByName;
};

} // namespace mc::mod::bedrock::addon
