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
 * @file attribute_range.cpp
 * @brief 差分能力：属性的默认值与取值上下限。
 *
 * `list`：项目注册的全部属性名
 * `run` ：{ "attribute": "minecraft:max_health" }
 *         → defaultValue / minValue / maxValue（位模式 + 十进制）
 *
 * 这三个数决定属性值会被钳到哪、修饰符算出来能到什么程度：
 * `max_health` 的上限写错，/attribute 命令与生命提升效果就会静默截断；
 * `movement_speed` 的下限写错，缓慢效果叠满后玩家会彻底动不了而不是留一点速度。
 * 这些数字都是手抄常量，错了不会有任何报错，只能逐个与原版比。
 *
 * `entity_attributes` 只比「每个生物类型的默认基础值」，这里比的是属性定义本身；
 * 名字（命名空间、旧版 `generic.` 前缀）也一并比对——名字错会让按名查找整体失效。
 */
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/entity/attribute/Attribute.hpp"
#include "common/entity/attribute/AttributeRegistry.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

using entity::attribute::Attribute;
using entity::attribute::AttributeRegistry;

/// f64 的位模式：属性的上下限里有 0.225 / 0.0625 这类二进制小数，
/// 十进制打印在 JSON 里会被 Gson 与 nlohmann 用不同精度写出来，直接比位最稳。
std::string doubleBits(f64 value)
{
    u64 bits = 0;
    static_assert(sizeof(bits) == sizeof(value), "f64 必须是 8 字节");
    std::memcpy(&bits, &value, sizeof(bits));
    char buffer[24];
    std::snprintf(buffer, sizeof(buffer), "0x%016llx", static_cast<unsigned long long>(bits));
    return buffer;
}

void writeList(FILE* out)
{
    for (const std::string& name : AttributeRegistry::instance().getAllNames()) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("attribute").get<std::string>();

    const Attribute* attribute = AttributeRegistry::instance().getAttribute(requested);
    if (attribute == nullptr) {
        return json{{"attribute", requested}, {"missing", true}}.dump();
    }

    return json{{"attribute", requested},
        {"defaultValue", doubleBits(attribute->defaultValue())},
        {"minValue", doubleBits(attribute->minValue())},
        {"maxValue", doubleBits(attribute->maxValue())}}
        .dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("attribute_range", &writeList, &runLine)

} // namespace mc::diff
