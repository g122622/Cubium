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
 */

/**
 * @file text_color.cpp
 * @brief 差分能力：TextColor 的构造、解析与序列化。
 *
 * 本探针调用生产代码 text::TextColor，不在探针里重写这些逻辑。
 *
 * ## 关键点
 *
 * 1. 十六进制必须恰好 6 位。#f00 与 #1000000 都不合法，不是「宽松地接受短写」。
 *
 * 2. 具名颜色必须与 16 个颜色名完全匹配，区分大小写；样式名（bold、reset）不是颜色。
 *
 * 3. 序列化时具名颜色写名字、自定义色写 #RRGGBB（大写），即使两者 RGB 值相同。
 *    这是「值相同、文本不同」的典型情形，逐字节比对必须区分。
 *
 * 4. fromRgb 只保留低 24 位，负值与超范围值都截断。
 */

#include <cstdio>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/util/text/TextStyle.hpp"

#include "probe.hpp"

namespace mc::diff {

namespace {

using nlohmann::json;
using text::TextColor;

void writeList(FILE* out)
{
    constexpr const char* UNITS[] = {
        // parse：# 分支
        "parse|#ff0000",
        "parse|#FF0000",
        "parse|#000000",
        "parse|#ffffff",
        // parse：# 分支的边界与失败
        "parse|#f00",
        "parse|#0",
        "parse|#1000000",
        "parse|#",
        "parse|#gggggg",
        "parse|#-1",
        "parse|#+ff",
        // parse：命名颜色
        "parse|red",
        "parse|black",
        "parse|white",
        "parse|light_purple",
        "parse|dark_gray",
        // parse：非法名字
        "parse|RED",
        "parse|bold",
        "parse|reset",
        "parse|",
        // fromRgb：截断
        "fromRgb|16711680",
        "fromRgb|-1",
        "fromRgb|16777216",
        "fromRgb|4294967295",
        // match：具名与十六进制值相同
        "match|red|#ff5555",
        "match|red|red",
        "match|red|#ff0000",
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

/// 渲染成 "<6 位大写十六进制>|<序列化形式>"；解析失败给出 "<error>"。
std::string renderColor(const std::optional<TextColor>& color)
{
    if (!color.has_value()) {
        return "<error>";
    }
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "%06X", static_cast<unsigned>(color->value()));
    return std::string(buffer) + "|" + color->serialize();
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() < 2) {
        throw std::runtime_error("text_color: unit needs at least two fields: " + unit);
    }

    const std::string mode = parts[0];

    if (mode == "parse" && parts.size() == 2) {
        return json{{"unit", unit}, {"value", renderColor(TextColor::parseColor(parts[1]))}}.dump();
    }
    if (mode == "fromRgb" && parts.size() == 2) {
        const i32 rgb = static_cast<i32>(std::stoll(parts[1]));
        return json{{"unit", unit}, {"value", renderColor(TextColor::fromRgb(rgb))}}.dump();
    }
    if (mode == "match" && parts.size() == 3) {
        const std::optional<TextColor> a = TextColor::parseColor(parts[1]);
        const std::optional<TextColor> b = TextColor::parseColor(parts[2]);
        // 原版比较的是解析后的值；任一解析失败都算不等
        const bool equal = a.has_value() && b.has_value() && a->value() == b->value();
        return json{{"unit", unit}, {"value", equal}}.dump();
    }

    throw std::runtime_error("text_color: unrecognized unit: " + unit);
}

} // namespace

MC_DIFF_DEFINE_PROBE(text_color, &writeList, &runLine)

} // namespace mc::diff
