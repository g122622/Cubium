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

#include "common/world/attribute/EnvironmentAttributeValueJson.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <utility>

namespace mc {
namespace world {
namespace attribute {
namespace {

const nlohmann::json& _json(const void* json)
{
    return *static_cast<const nlohmann::json*>(json);
}

/**
 * @brief 读取十六进制颜色
 *
 * 与 vanilla `ExtraCodecs.RGB_COLOR_CODEC` 一致：接受 `#rrggbb`（6 位）与 `#aarrggbb`（8 位）
 * 两种写法，`#` 可省略。属性值以 ARGB 位模式保存（RGB 颜色类型只消费低 24 位）。
 */
bool _readColor(const nlohmann::json& json, i32& out)
{
    if (!json.is_string()) {
        return false;
    }
    std::string text = json.get<std::string>();
    if (!text.empty() && text.front() == '#') {
        text.erase(text.begin());
    }
    if (text.size() != 6 && text.size() != 8) {
        return false;
    }
    u32 value = 0;
    for (const char ch : text) {
        u32 digit = 0;
        if (ch >= '0' && ch <= '9') {
            digit = static_cast<u32>(ch - '0');
        } else if (ch >= 'a' && ch <= 'f') {
            digit = static_cast<u32>(ch - 'a' + 10);
        } else if (ch >= 'A' && ch <= 'F') {
            digit = static_cast<u32>(ch - 'A' + 10);
        } else {
            return false;
        }
        value = (value << 4) | digit;
    }
    // 6 位写法为不透明色：补 full alpha
    out = static_cast<i32>(text.size() == 6 ? (value | 0xFF000000u) : value);
    return true;
}

bool _readBool(const nlohmann::json& json, bool& out)
{
    if (!json.is_boolean()) {
        return false;
    }
    out = json.get<bool>();
    return true;
}

bool _readFloat(const nlohmann::json& json, f32& out)
{
    if (!json.is_number()) {
        return false;
    }
    out = json.get<f32>();
    return true;
}

bool _readInt(const nlohmann::json& json, i32& out)
{
    // 注意：颜色属性同样以 i32 承载，属性值写法的还原由调用方按属性类型区分。
    // 这里对字符串走颜色解析，对数字走整数；两者不会同时成立，故无需额外类型信息。
    if (json.is_string()) {
        return _readColor(json, out);
    }
    if (!json.is_number()) {
        return false;
    }
    out = json.get<i32>();
    return true;
}

bool _readTriState(const nlohmann::json& json, util::TriState& out)
{
    if (json.is_boolean()) {
        out = json.get<bool>() ? util::TriState::True : util::TriState::False;
        return true;
    }
    if (json.is_string() && json.get<std::string>() == "default") {
        out = util::TriState::Default;
        return true;
    }
    return false;
}

/// 月相蛇形名（与 vanilla MoonPhase.CODEC 的序列化名一致）
constexpr std::array<std::pair<std::string_view, MoonPhase>, MOON_PHASE_COUNT> MOON_PHASE_NAMES = {{
    {"full_moon", MoonPhase::FullMoon},
    {"waning_gibbous", MoonPhase::WaningGibbous},
    {"third_quarter", MoonPhase::ThirdQuarter},
    {"waning_crescent", MoonPhase::WaningCrescent},
    {"new_moon", MoonPhase::NewMoon},
    {"waxing_crescent", MoonPhase::WaxingCrescent},
    {"first_quarter", MoonPhase::FirstQuarter},
    {"waxing_gibbous", MoonPhase::WaxingGibbous},
}};

bool _readMoonPhase(const nlohmann::json& json, MoonPhase& out)
{
    if (!json.is_string()) {
        return false;
    }
    const std::string name = json.get<std::string>();
    for (const auto& [phaseName, phase] : MOON_PHASE_NAMES) {
        if (phaseName == name) {
            out = phase;
            return true;
        }
    }
    return false;
}

bool _readActivity(const nlohmann::json& json, entity::ai::brain::schedule::Activity& out)
{
    if (!json.is_string()) {
        return false;
    }
    out = entity::ai::brain::schedule::Activity(json.get<std::string>());
    return true;
}

} // namespace

template <>
std::function<bool(const void* json, bool& out)> valueJsonReader<bool>()
{
    return [](const void* json, bool& out) {
        return _readBool(_json(json), out);
    };
}

template <>
std::function<bool(const void* json, f32& out)> valueJsonReader<f32>()
{
    return [](const void* json, f32& out) {
        return _readFloat(_json(json), out);
    };
}

template <>
std::function<bool(const void* json, i32& out)> valueJsonReader<i32>()
{
    return [](const void* json, i32& out) {
        return _readInt(_json(json), out);
    };
}

template <>
std::function<bool(const void* json, util::TriState& out)> valueJsonReader<util::TriState>()
{
    return [](const void* json, util::TriState& out) {
        return _readTriState(_json(json), out);
    };
}

template <>
std::function<bool(const void* json, MoonPhase& out)> valueJsonReader<MoonPhase>()
{
    return [](const void* json, MoonPhase& out) {
        return _readMoonPhase(_json(json), out);
    };
}

template <>
std::function<bool(const void* json, entity::ai::brain::schedule::Activity& out)>
valueJsonReader<entity::ai::brain::schedule::Activity>()
{
    return [](const void* json, entity::ai::brain::schedule::Activity& out) {
        return _readActivity(_json(json), out);
    };
}

} // namespace attribute
} // namespace world
} // namespace mc
