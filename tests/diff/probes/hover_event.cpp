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
 * @brief 差分能力：`HoverEvent.Action` 的序列化与 `toString`（纯计算）。
 *
 * `list`：一组「模式|参数…」单元
 * `run` ：同上一行 → { "value": ... }
 *
 * 复刻原版 `net.minecraft.network.chat.HoverEvent`：
 *
 * ```java
 * public static enum Action implements StringRepresentable {
 *     SHOW_TEXT("show_text", true, HoverEvent.ShowText.CODEC),
 *     SHOW_ITEM("show_item", true, HoverEvent.ShowItem.CODEC),
 *     SHOW_ENTITY("show_entity", true, HoverEvent.ShowEntity.CODEC);
 *
 *     public static final Codec<Action> UNSAFE_CODEC =
 *         StringRepresentable.fromValues(Action::values);
 *     public static final Codec<Action> CODEC =
 *         UNSAFE_CODEC.validate(Action::filterForSerialization);
 *
 *     private final String name;
 *     private final boolean allowFromServer;
 *
 *     public boolean isAllowedFromServer() { return this.allowFromServer; }
 *     public String getSerializedName() { return this.name; }
 *
 *     @Override
 *     public String toString() { return "<action " + this.name + ">"; }
 *
 *     private static DataResult<Action> filterForSerialization(Action action) {
 *         return !action.isAllowedFromServer()
 *             ? DataResult.error(() -> "Action not allowed: " + action)
 *             : DataResult.success(action, Lifecycle.stable());
 *     }
 * }
 * ```
 *
 * ## 四个关键点
 *
 * 1. **只有 3 个动作，且全部 `allowFromServer = true`。**
 *
 *    | 动作 | 序列化名 | `allowFromServer` |
 *    |---|---|---|
 *    | `SHOW_TEXT` | `show_text` | `true` |
 *    | `SHOW_ITEM` | `show_item` | `true` |
 *    | `SHOW_ENTITY` | `show_entity` | `true` |
 *
 *    **与 [`click_event`](./click_event.md) 形成鲜明对比**：那里 8 个动作中有 1 个
 *    （`OPEN_FILE`）被禁用。
 *
 * 2. **因此 `filterForSerialization` 对任何 `HoverEvent.Action` 都不会失败。**
 *
 *    这是一条**「恒真的安全校验」**——代码结构保留（与 `ClickEvent` 对称），
 *    但当前数据下永不触发。
 *
 *    **若 Cubium 认为「悬停事件来自服务端所以才需要校验」并给某个动作加了禁用，
 *    就会与原版不同**——原版明确允许全部三个。
 *
 * 3. **`toString()` 是 `"<action " + name + ">"`，不是枚举名。**
 *
 *    | 动作 | `toString()` |
 *    |---|---|
 *    | `SHOW_TEXT` | `<action show_text>` |
 *    | `SHOW_ITEM` | `<action show_item>` |
 *    | `SHOW_ENTITY` | `<action show_entity>` |
 *
 *    **这是 `HoverEvent.Action` 独有的**：`ClickEvent.Action` 没有覆盖 `toString`，
 *    返回的是枚举名 `OPEN_URL`。
 *
 *    且这里的 `name` 是**序列化名**（`show_text`），不是枚举名（`SHOW_TEXT`）。
 *
 * 4. **用的是 `StringRepresentable.fromValues`，`ClickEvent` 用的是 `fromEnum`。**
 *
 *    两者在 codec 语义上等价（都按 `values()` 建映射），但**方法名不同**。
 *    这不影响输出，记录在此避免误以为行为有差异。
 *
 * 单元格式：
 * - `serial|<action>` —— `getSerializedName`
 * - `allowed|<action>` —— `isAllowedFromServer`
 * - `filter|<action>` —— `filterForSerialization` 是否成功
 * - `tostring|<action>` —— `toString()`
 * - `names` —— 全部序列化名，逗号分隔
 * - `count` —— 动作总数
 */
#include <cstdio>
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

/// `HoverEvent.Action` 的 3 个枚举项，按序数排列。
struct Action {
    const char* enumName;
    const char* serializedName;
    bool allowedFromServer;
};

constexpr Action ACTIONS[] = {
    {"SHOW_TEXT", "show_text", true},
    {"SHOW_ITEM", "show_item", true},
    {"SHOW_ENTITY", "show_entity", true},
};

constexpr i32 ACTION_COUNT = 3;

const Action* findAction(const std::string& name)
{
    for (const Action& action : ACTIONS) {
        if (name == action.enumName) {
            return &action;
        }
    }
    throw std::runtime_error("hover_event: 未知动作：" + name);
}

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // serial
        "serial|SHOW_TEXT",
        "serial|SHOW_ITEM",
        "serial|SHOW_ENTITY",
        // allowed
        "allowed|SHOW_TEXT",
        "allowed|SHOW_ITEM",
        "allowed|SHOW_ENTITY",
        // filter
        "filter|SHOW_TEXT",
        "filter|SHOW_ITEM",
        "filter|SHOW_ENTITY",
        // tostring
        "tostring|SHOW_TEXT",
        "tostring|SHOW_ITEM",
        "tostring|SHOW_ENTITY",
        // names / count
        "names",
        "count",
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
        throw std::runtime_error("hover_event: 空单元");
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
    if (mode == "count" && parts.size() == 1) {
        return json{{"unit", unit}, {"value", ACTION_COUNT}}.dump();
    }
    if (parts.size() != 2) {
        throw std::runtime_error("hover_event: 单元段数不符：" + unit);
    }

    const Action* action = findAction(parts[1]);
    if (mode == "serial") {
        return json{{"unit", unit}, {"value", action->serializedName}}.dump();
    }
    if (mode == "allowed") {
        return json{{"unit", unit}, {"value", action->allowedFromServer}}.dump();
    }
    if (mode == "filter") {
        return json{{"unit", unit}, {"value", action->allowedFromServer}}.dump();
    }
    if (mode == "tostring") {
        return json{{"unit", unit}, {"value", std::string("<action ") + action->serializedName + ">"}}.dump();
    }

    throw std::runtime_error("hover_event: 无法识别的单元：" + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE("hover_event", &writeList, &runLine)

} // namespace mc::diff
