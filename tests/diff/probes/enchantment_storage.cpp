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

/**
 * @brief 差分能力：`ItemEnchantments.Mutable` 的增删改语义（纯计算）。
 *
 * `list`：一组「操作序列」单元
 * `run` ：同上一行 → { "result": "a=2,b=0", "size": 2 }
 *
 * 复刻原版 `ItemEnchantments.Mutable`：
 *
 * ```java
 * public void set(Holder<Enchantment> ench, int level) {
 *     if (level <= 0) {
 *         this.enchantments.removeInt(ench);          // <= 0 就删除
 *     } else {
 *         this.enchantments.put(ench, Math.min(level, 255));   // 上限 255
 *     }
 * }
 *
 * public void upgrade(Holder<Enchantment> ench, int level) {
 *     if (level > 0) {
 *         this.enchantments.merge(ench, Math.min(level, 255), Integer::max);   // 取较大值
 *     }
 *     // level <= 0 时什么都不做
 * }
 *
 * public int getLevel(Holder<Enchantment> ench) {
 *     return this.enchantments.getOrDefault(ench, 0);
 * }
 * ```
 *
 * 而不可变的 `ItemEnchantments` 构造器校验的是 `0 <= level <= 255`：
 *
 * ```java
 * if (i < 0 || i > 255) { throw new IllegalArgumentException(...); }
 * ```
 *
 * 编解码用的 `LEVEL_CODEC` 则是 `Codec.intRange(1, 255)`。
 *
 * ## 四个关键点
 *
 * 1. **`set` 与 `upgrade` 对 `level <= 0` 的处理完全不同**：
 *    - `set(e, 0)` **删除**该附魔
 *    - `upgrade(e, 0)` **什么都不做**（已有的等级保留）
 *
 *    这是本能力最值得钉住的一处。`set` 是「赋值为 0 等于没有」，`upgrade` 是「加 0 级
 *    等于没要求」。代码里若把两者互换，删除附魔的操作会静默失效。
 *
 * 2. **`set` 的上限是 255，不是无限制**。`set(e, 300)` 得到 255。原版用
 *    `Math.min(level, 255)` 而非报错——超出的等级被静默截断。
 *
 * 3. **`upgrade` 取 `max`，不会降级**。`upgrade(e, 1)` 在已有 `e = 3` 时仍是 3。
 *
 * 4. **三层等级范围不一致**：
 *    - `Mutable.set` / `upgrade`：接受任意 `int`，`<= 0` 特殊处理，`> 255` 截断
 *    - `ItemEnchantments` 构造器：**允许 0**（`0 <= i <= 255`），但会保留「0 级」条目
 *    - `LEVEL_CODEC`：`intRange(1, 255)`，**拒绝 0**
 *
 *    所以「0 级附魔」在内存表示里是可能的（通过构造器直传），但无法被序列化。正常路径下
 *    `Mutable.set` 会把 0 删掉，因此不会产生这种状态。
 *
 * 单元格式：`<操作>[,<操作>]*`，每个操作为 `<op>:<附魔名>:<等级>`，附魔名用单字母 a~e
 * 代表不同附魔。
 *
 * 示例：`set:a:3,set:b:1,upgrade:a:2`
 *
 * TODO: 未覆盖 `removeIf`（依赖谓词，在 `Mutable` 上只是转发到 keySet 的 removeIf）、
 * `ItemEnchantments.CODEC` 的 0 级拒绝（属序列化层，需单独能力）。
 */
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 原版 `ItemEnchantments.Mutable.set` / `upgrade` 共用的等级上限。
constexpr i32 MAX_LEVEL = 255;

/**
 * @brief 复刻 `ItemEnchantments.Mutable`。
 */
class Mutable {
public:
    /// 原版 `Mutable.set`：`level <= 0` 删除，否则截断到 255 后写入。
    void set(char ench, i32 level)
    {
        if (level <= 0) {
            m_entries.erase(ench);
        } else {
            m_entries[ench] = std::min(level, MAX_LEVEL);
        }
    }

    /// 原版 `Mutable.upgrade`：`level <= 0` 什么都不做，否则取较大值。
    void upgrade(char ench, i32 level)
    {
        if (level > 0) {
            const i32 clamped = std::min(level, MAX_LEVEL);
            const auto it = m_entries.find(ench);
            if (it == m_entries.end()) {
                m_entries[ench] = clamped;
            } else {
                it->second = std::max(it->second, clamped);
            }
        }
    }

    /// 原版 `Mutable.getLevel`：不存在时返回 0。
    i32 getLevel(char ench) const
    {
        const auto it = m_entries.find(ench);
        return it == m_entries.end() ? 0 : it->second;
    }

    const std::map<char, i32>& entries() const
    {
        return m_entries;
    }

private:
    std::map<char, i32> m_entries;
};

std::vector<std::string> splitBy(const std::string& text, char separator)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == separator) {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(current);
    return tokens;
}

/// 单元表：每项是一串用 `,` 分隔的操作。
constexpr const char* UNITS[] = {
    // set：基本写入
    "set:a:3",
    "set:a:3,set:a:5",
    // set：level <= 0 删除
    "set:a:3,set:a:0",
    "set:a:3,set:a:-1",
    // set：上限 255
    "set:a:255",
    "set:a:256",
    "set:a:1000",
    // upgrade：取较大值
    "upgrade:a:3",
    "upgrade:a:3,upgrade:a:5",
    "upgrade:a:5,upgrade:a:3",
    // upgrade：level <= 0 不做事（与 set 相反）
    "upgrade:a:3,upgrade:a:0",
    "upgrade:a:3,upgrade:a:-1",
    // upgrade：对不存在的附魔
    "upgrade:a:0",
    // upgrade：上限
    "upgrade:a:300",
    "upgrade:a:300,upgrade:a:400",
    // 混合：多附魔
    "set:a:3,set:b:1,set:c:2",
    // 混合：set 后 upgrade
    "set:a:3,upgrade:a:2",
    "set:a:3,upgrade:a:5",
    // 混合：upgrade 后 set 覆盖
    "upgrade:a:5,set:a:2",
    // 混合：set 删除后再 upgrade 恢复
    "set:a:3,set:a:0,upgrade:a:1",
    // 混合：删除中间项
    "set:a:1,set:b:2,set:c:3,set:b:0",
};

void writeList(FILE* out)
{
    for (const auto* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    Mutable mutable_;
    for (const auto& opText : splitBy(unit, ',')) {
        const auto parts = splitBy(opText, ':');
        if (parts.size() != 3 || parts[1].size() != 1) {
            throw std::runtime_error("enchantment_storage: 非法操作 '" + opText + "'");
        }
        const char ench = parts[1][0];
        const i32 level = std::stoi(parts[2]);

        if (parts[0] == "set") {
            mutable_.set(ench, level);
        } else if (parts[0] == "upgrade") {
            mutable_.upgrade(ench, level);
        } else {
            throw std::runtime_error("enchantment_storage: 未知操作 '" + parts[0] + "'");
        }
    }

    // 输出：按附魔名排序的 `名=等级` 列表（空则给 "-"）。
    std::string summary;
    for (const auto& [ench, level] : mutable_.entries()) {
        if (!summary.empty()) {
            summary += ",";
        }
        summary += std::string(1, ench) + "=" + std::to_string(level);
    }
    if (summary.empty()) {
        summary = "-";
    }

    return json{{"unit", unit}, {"result", summary}, {"size", mutable_.entries().size()}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(enchantment_storage, &writeList, &runLine)

} // namespace mc::diff
