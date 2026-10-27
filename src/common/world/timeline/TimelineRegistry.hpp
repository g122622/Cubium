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

#include "common/core/Result.hpp"
#include "common/world/timeline/Timeline.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace mc {
namespace world {
namespace timeline {

/**
 * @brief 时间线注册表
 *
 * MC 1.21.11 Registries.TIMELINE。
 * 以注册名索引时间线，维度类型通过时间线标签（`minecraft:universal` /
 * `minecraft:in_overworld` / `minecraft:in_nether` / `minecraft:in_end` /
 * `minecraft:villager_schedule`）引用它们。
 *
 * 内置时间线为数据包缺失时的兜底：vanilla 数据包自带同名 JSON，数据包重载时
 * 由数据包加载器覆盖。
 */
class TimelineRegistry {
public:
    /// 注册表中的一项
    struct Entry {
        Entry(std::string name, const Timeline* timeline);

        /// 完整注册名（`minecraft:<path>`）
        std::string name;
        /// 时间线（内置项指向静态实例，数据包项由注册表持有）
        const Timeline* timeline;
    };

    /**
     * @brief 按注册名查找
     *
     * @param name 注册名（接受 `minecraft:xxx` 与 `xxx` 两种写法）
     * @return 条目指针，未注册时返回 nullptr
     */
    [[nodiscard]] static const Entry* find(std::string_view name);

    /**
     * @brief 全部已注册时间线（按注册顺序，内置项在前）
     */
    [[nodiscard]] static const std::vector<Entry>& all();

    /**
     * @brief 注册内置时间线
     *
     * 注册顺序固定为 DAY / MOON / VILLAGER_SCHEDULE / EARLY_GAME；
     * 重复注册会被跳过。
     *
     * @return 本次新登记的数量
     */
    static Result<size_t> registerAll();

    /**
     * @brief 以数据包加载的时间线覆盖同名内置项，或新增条目
     *
     * @param name 注册名（不含命名空间前缀）
     * @param timeline 时间线（注册表接管所有权）
     * @return 注册结果
     */
    static Result<void> registerFromDataPack(std::string_view name, std::unique_ptr<Timeline> timeline);

private:
    static Result<void> _register(std::string_view name, const Timeline* builtin, std::unique_ptr<Timeline> owned);
};

} // namespace timeline
} // namespace world
} // namespace mc
