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
 * @brief 差分能力：`ClickEvent.Action` 的序列化过滤与 `ChangePage` 的页码校验（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.network.chat.ClickEvent`：
 *
 * ```java
 * public static enum Action implements StringRepresentable {
 *     OPEN_URL("open_url", true, ...),
 *     OPEN_FILE("open_file", false, ...),        // 唯一一个 allowFromServer = false
 *     RUN_COMMAND("run_command", true, ...),
 *     SUGGEST_COMMAND("suggest_command", true, ...),
 *     SHOW_DIALOG("show_dialog", true, ...),
 *     CHANGE_PAGE("change_page", true, ...),
 *     COPY_TO_CLIPBOARD("copy_to_clipboard", true, ...),
 *     CUSTOM("custom", true, ...);
 *
 *     private final boolean allowFromServer;
 *     private final String name;
 *
 *     public boolean isAllowedFromServer() { return this.allowFromServer; }
 *     public String getSerializedName() { return this.name; }
 *
 *     public static DataResult<Action> filterForSerialization(Action action) {
 *         return !action.isAllowedFromServer()
 *             ? DataResult.error(() -> "Click event type not allowed: " + action)
 *             : DataResult.success(action, Lifecycle.stable());
 *     }
 * }
 *
 * public record ChangePage(int page) implements ClickEvent {
 *     // CODEC 用 ExtraCodecs.POSITIVE_INT.fieldOf("page")
 * }
 *
 * public static final Codec<Integer> POSITIVE_INT =
 *     intRangeWithMessage(1, Integer.MAX_VALUE, v -> "Value must be positive: " + v);
 * ```
 *
 * ## 四个关键点
 *
 * 1. **`OPEN_FILE` 是唯一 `allowFromServer = false` 的动作。**
 *
 *    | 动作 | 序列化名 | `allowFromServer` |
 *    |---|---|---|
 *    | `OPEN_URL` | `open_url` | `true` |
 *    | **`OPEN_FILE`** | **`open_file`** | **`false`** |
 *    | `RUN_COMMAND` | `run_command` | `true` |
 *    | `SUGGEST_COMMAND` | `suggest_command` | `true` |
 *    | `SHOW_DIALOG` | `show_dialog` | `true` |
 *    | `CHANGE_PAGE` | `change_page` | `true` |
 *    | `COPY_TO_CLIPBOARD` | `copy_to_clipboard` | `true` |
 *    | `CUSTOM` | `custom` | `true` |
 *
 *    **`OPEN_FILE` 之所以禁掉，是因为它会打开服务器本地的文件路径**——这是安全边界。
 *
 * 2. **`filterForSerialization` 对 `OPEN_FILE` 返回错误，其余返回成功。**
 *
 *    ```java
 *    return !action.isAllowedFromServer()
 *        ? DataResult.error(() -> "Click event type not allowed: " + action)
 *        : DataResult.success(action, Lifecycle.stable());
 *    ```
 *
 *    注意 `UNSAFE_CODEC` 不做这个过滤——**只有 `CODEC` 过滤**。两者是不同的 codec。
 *
 * 3. **`ChangePage` 的页码用 `POSITIVE_INT`，范围是 `[1, Integer.MAX_VALUE]`。**
 *
 *    ```java
 *    public static final Codec<Integer> POSITIVE_INT =
 *        intRangeWithMessage(1, Integer.MAX_VALUE, ...);
 *    ```
 *
 *    **`0` 与负数都非法**——书页从 `1` 开始编号。
 *
 * 4. **动作名是 `snake_case`，且 `CUSTOM` 的序列化名就是 `"custom"`。**
 *
 *    没有前缀、没有命名空间——纯小写加下划线。
 *
 * 单元格式：
 * - `serial|<action>` —— `getSerializedName`，`action` 用枚举名（如 `OPEN_URL`）
 * - `allowed|<action>` —— `isAllowedFromServer`
 * - `filter|<action>` —— `filterForSerialization` 是否成功
 * - `names` —— 全部动作名，按序数排序、逗号分隔
 * - `page|<int>` —— `ChangePage` 页码是否合法（`POSITIVE_INT`）
 */
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// `ClickEvent.Action` 的 8 个枚举项，按序数排列。
struct Action {
    const char* enumName;
    const char* serializedName;
    bool allowedFromServer;
};

constexpr Action ACTIONS[] = {
    {"OPEN_URL", "open_url", true},
    {"OPEN_FILE", "open_file", false},
    {"RUN_COMMAND", "run_command", true},
    {"SUGGEST_COMMAND", "suggest_command", true},
    {"SHOW_DIALOG", "show_dialog", true},
    {"CHANGE_PAGE", "change_page", true},
    {"COPY_TO_CLIPBOARD", "copy_to_clipboard", true},
    {"CUSTOM", "custom", true},
};

constexpr i32 ACTION_COUNT = 8;

const Action* findAction(const std::string& name)
{
    for (const Action& action : ACTIONS) {
        if (name == action.enumName) {
            return &action;
        }
    }
    throw std::runtime_error("click_event: 未知动作：" + name);
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // serial：全部动作
        "serial|OPEN_URL",
        "serial|OPEN_FILE",
        "serial|RUN_COMMAND",
        "serial|SUGGEST_COMMAND",
        "serial|SHOW_DIALOG",
        "serial|CHANGE_PAGE",
        "serial|COPY_TO_CLIPBOARD",
        "serial|CUSTOM",
        // allowed
        "allowed|OPEN_URL",
        "allowed|OPEN_FILE",
        "allowed|RUN_COMMAND",
        "allowed|CUSTOM",
        // filter
        "filter|OPEN_URL",
        "filter|OPEN_FILE",
        "filter|RUN_COMMAND",
        "filter|CHANGE_PAGE",
        // names
        "names",
        // page：边界
        "page|0",
        "page|1",
        "page|2",
        "page|-1",
        "page|2147483647",
        "page|-2147483648",
    };
    for (const auto* unit : UNITS) {
        std::fprintf(out, "%s\n", unit);
    }
}

std::vector<std::string> splitPipe(const std::string& text)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == '|') {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(current);
    return tokens;
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.empty()) {
        throw std::runtime_error("click_event: 空单元");
    }

    const std::string mode = parts[0];

    if (mode == "names" && parts.size() == 1) {
        std::string names;
        for (const Action& action : ACTIONS) {
            if (!names.empty()) {
                names.push_back(',');
            }
            names += action.serializedName;
        }
        return json{{"unit", unit}, {"value", names}}.dump();
    }
    if (parts.size() != 2) {
        throw std::runtime_error("click_event: 单元段数不符：" + unit);
    }

    if (mode == "serial") {
        return json{{"unit", unit}, {"value", findAction(parts[1])->serializedName}}.dump();
    }
    if (mode == "allowed") {
        return json{{"unit", unit}, {"value", findAction(parts[1])->allowedFromServer}}.dump();
    }
    if (mode == "filter") {
        return json{{"unit", unit}, {"value", findAction(parts[1])->allowedFromServer}}.dump();
    }
    if (mode == "page") {
        // 原版用 ExtraCodecs.POSITIVE_INT：范围 [1, Integer.MAX_VALUE]。
        const i64 page = std::stoll(parts[1]);
        const bool valid = page >= 1 && page <= std::numeric_limits<i32>::max();
        return json{{"unit", unit}, {"value", valid}}.dump();
    }

    throw std::runtime_error("click_event: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(click_event, &writeList, &runLine)

} // namespace mc::diff
