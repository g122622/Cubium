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
 * @file stack_codec.cpp
 * @brief 差分能力：物品堆**整体编解码**（原版 `ItemStack.CODEC`）。
 *
 * `list`：全部物品 ID × 两个变体（base / suite）
 * `run` ：{ "unit": "minecraft:diamond_sword#suite",
 *          "stack": { "id": ..., "count": ..., "components": {...} } }
 *         → { "ok": true, "decoded": {...}, "reencode": {...} } 或 { "ok": false }
 *
 * `item_component_nbt` 比的是组件载荷本身；这一层比的是把组件放进物品堆之后**整个物品堆**
 * 的编解码：`count` 的取值区间（原版 1..99）、空堆怎么表示、`components` 段什么时候省略、
 * 以及解码应当失败的场合。这些规则写错不会在运行时立刻报错，而是「存档能读、物品不对」
 * ——数量被截断、坏数据被静默接受。
 *
 * 输出的两段互相印证：
 *   - `decoded`：解码之后物品堆的实际状态（id / count / 是否空 / 组件补丁逐条 SNBT）；
 *   - `reencode`：把这个物品堆再编码回 NBT 得到的文本。
 * 只比其中一个方向时，「解码丢字段、编码又凭空补上」这类错误会互相抵消。
 */
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Result.hpp"
#include "common/item/Items.hpp"
#include "common/item/component/DataComponentMap.hpp"
#include "common/item/component/DataComponentPatchNbt.hpp"
#include "common/item/component/DataComponentPayloadCodec.hpp"
#include "common/item/component/DataComponentType.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/instrument/InstrumentRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/nbt/Nbt.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;
using namespace item::component;

/// 与 StackCodecOracle 的变体名必须一字不差（单元键的一部分）。
constexpr const char* VARIANT_BASE = "base";
constexpr const char* VARIANT_SUITE = "suite";

/**
 * 固定场景的单元名，与 StackCodecOracle.SCENARIO_UNITS 逐字一致。
 *
 * 两份清单靠差分本身保真：任何一边新增场景而另一边没有，单元集合立刻对不上，报告里
 * 会点名多出来的那个。场景的**取值**只有一份（harness/harnesses/stack_codec.ts），
 * 不在这里重复。
 */
constexpr const char* SCENARIO_UNITS[] = {
    "count#min",
    "count#max",
    "count#zero",
    "count#over",
    "count#negative",
    "count#absent",
    "shell#extra-key",
    "shell#air",
    "shell#unknown-item",
    "shell#empty-components",
    "shell#damageable-stacked",
};

void ensureItems()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        Items::initialize();
        // suite 变体里含 instrument 组件；它的持久形态是 Holder 的完整定义，编码要查注册表。
        item::instrument::InstrumentRegistry::instance().initialize();
        s_initialized = true;
    }
}

void writeList(FILE* out)
{
    ensureItems();
    std::vector<std::string> names;
    ItemRegistry::instance().forEachItem([&](Item& item) {
        names.push_back(item.itemLocation().toString());
    });
    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s#%s\n", name.c_str(), VARIANT_BASE);
        std::fprintf(out, "%s#%s\n", name.c_str(), VARIANT_SUITE);
    }
    // 固定场景（数量区间、坏数据）与具体物品无关，单独列出。
    for (const char* scenario : SCENARIO_UNITS) {
        std::fprintf(out, "%s\n", scenario);
    }
}

/**
 * @brief 把标签写成 SNBT 文本；标量与列表去掉外层 compound 的包裹。
 *
 * 少数组件的顶层不是 compound（附魔是平铺 map、can_break 是列表、damage 是整数）。
 * mojangson 写出器只接受 compound 作根，因此先包一层只含单键的 compound，再按最外层的
 * 一对花括号剥掉——只能剥最外层那一对，文本内部还有大量花括号。
 */
std::string toSnbt(const nbt::tags::tag& tag)
{
    if (tag.id() == nbt::TagId::Compound) {
        std::ostringstream output;
        output << nbt::contexts::mojangson;
        nbt::operator<<(output, dynamic_cast<const nbt::tags::compound_tag&>(tag));
        return output.str();
    }
    nbt::tags::compound_tag wrapper;
    wrapper.value.emplace("v", tag.copy());
    std::ostringstream output;
    output << nbt::contexts::mojangson;
    nbt::operator<<(output, wrapper);
    const std::string text = output.str();
    const auto begin = text.find('{');
    const auto end = text.rfind('}');
    if (begin == std::string::npos || end == std::string::npos || end <= begin) {
        return text;
    }
    // 花括号之间是 `v:<value>`（或 `'v':<value>`，取决于引号策略），按第一个冒号切开。
    const std::string inner = text.substr(begin + 1, end - begin - 1);
    const auto colon = inner.find(':');
    return colon == std::string::npos ? inner : inner.substr(colon + 1);
}

/// 把物品堆的组件补丁写成「组件名 → SNBT」对象（键排序，与 oracle 同口径）。
json encodePatch(const ItemStack& stack)
{
    json result = json::object();
    const DataComponentPatch patch = stack.toComponentPatch();
    for (const auto& entry : patch.added()) {
        auto type = componentTypeById(entry.typeId);
        if (!type.has_value()) {
            continue;
        }
        auto name = componentName(*type);
        if (!name.has_value()) {
            continue;
        }
        auto tag = detail::payloadToNbt(*type, entry.value);
        if (tag == nullptr) {
            continue;
        }
        result[std::string(*name)] = toSnbt(*tag);
    }
    for (i32 typeId : patch.removed()) {
        auto type = componentTypeById(typeId);
        if (!type.has_value()) {
            continue;
        }
        auto name = componentName(*type);
        if (name.has_value()) {
            result["!" + std::string(*name)] = "";
        }
    }
    return result;
}

/**
 * @brief 由 JSON 描述构造待解码的 NBT 复合标签。
 *
 * 与 oracle 同口径：整数落 Int（超出 i32 落 Long）、浮点落 Double、布尔落 Byte、
 * JSON 对象落 compound、JSON 数组落同构列表。
 *
 * 键名全部按原样透传——`id` / `count` / `components` 之外的多余键也会写进去，用来验证
 * 两侧对「未知键」的态度是否一致。
 */
std::unique_ptr<nbt::tags::tag> jsonToTag(const json& value)
{
    if (value.is_null()) {
        return std::make_unique<nbt::tags::compound_tag>();
    }
    if (value.is_boolean()) {
        return std::make_unique<nbt::tags::byte_tag>(static_cast<std::int8_t>(value.get<bool>() ? 1 : 0));
    }
    if (value.is_number_unsigned()) {
        const auto number = value.get<std::uint64_t>();
        if (number > static_cast<std::uint64_t>(std::numeric_limits<i32>::max())) {
            return std::make_unique<nbt::tags::long_tag>(static_cast<i64>(number));
        }
        return std::make_unique<nbt::tags::int_tag>(static_cast<i32>(number));
    }
    if (value.is_number_integer()) {
        const auto number = value.get<i64>();
        if (number > std::numeric_limits<i32>::max() || number < std::numeric_limits<i32>::min()) {
            return std::make_unique<nbt::tags::long_tag>(number);
        }
        return std::make_unique<nbt::tags::int_tag>(static_cast<i32>(number));
    }
    if (value.is_number_float()) {
        return std::make_unique<nbt::tags::double_tag>(value.get<f64>());
    }
    if (value.is_string()) {
        return std::make_unique<nbt::tags::string_tag>(value.get<std::string>());
    }
    if (value.is_array()) {
        // NBT 列表的元素类型必须同构。这里按「第一个非空元素」挑容器：
        //   - 全是对象 → compound 列表；
        //   - 空数组 / 首元素是 null → 空列表（元素类型未知，按原版的 End 处理）；
        //   - 其余 → 通用列表，元素类型随第一个元素。
        if (value.empty()) {
            return std::make_unique<nbt::tags::end_list_tag>();
        }
        const bool allCompound = std::all_of(value.begin(), value.end(), [](const json& element) {
            return element.is_object() || element.is_null();
        });
        if (allCompound) {
            auto list = std::make_unique<nbt::tags::compound_list_tag>();
            for (const auto& element : value) {
                const auto child = jsonToTag(element);
                const auto* compound = dynamic_cast<const nbt::tags::compound_tag*>(child.get());
                if (compound != nullptr) {
                    list->value.push_back(*compound);
                }
            }
            return list;
        }
        auto list = std::make_unique<nbt::tags::tag_list_tag>();
        for (const auto& element : value) {
            list->value.push_back(jsonToTag(element));
        }
        return list;
    }
    auto compound = std::make_unique<nbt::tags::compound_tag>();
    for (const auto& [key, element] : value.items()) {
        compound->value.emplace(key, jsonToTag(element));
    }
    return compound;
}

std::string runLine(std::string_view inputLine)
{
    ensureItems();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    json result{{"unit", unit}};
    if (!input.contains("stack")) {
        result["missing"] = true;
        return result.dump();
    }
    const auto& spec = input.at("stack");
    // 注意：**不**把「物品不存在」翻译成 missing。原版 `Item.CODEC` 查不到注册项就是解码
    // 失败，这里也必须失败——「坏存档被静默接受」正是这个能力要抓的错误。
    // 单元集合的缺口（Cubium 没注册某个物品）由编排层按两侧 list 的差异归口，
    // 与「这一行解码成不成功」是两件事。
    const auto tag = jsonToTag(spec);
    const auto* compound = dynamic_cast<const nbt::tags::compound_tag*>(tag.get());
    if (compound == nullptr) {
        result["ok"] = false;
        return result.dump();
    }

    // 解码失败与「成功」是同一个字段的两个取值：探针失败时也必须带上 key，
    // 否则编排层认不出这一行属于哪个单元（原版侧同理）。
    Result<ItemStack> decoded = ItemStack::fromNbt(*compound);
    if (!decoded.success()) {
        result["ok"] = false;
        return result.dump();
    }
    const ItemStack stack = decoded.value();
    result["ok"] = true;

    // id 取「物品类型」，不随数量清零而变：原版 `getItem()` 对数量为 0 的堆同样返回物品本身，
    // 只有从未设置过物品的默认堆才是空气。
    json describe = json::object();
    describe["id"] =
        stack.getItem() == nullptr ? std::string("minecraft:air") : stack.getItem()->itemLocation().toString();
    describe["count"] = stack.getCount();
    describe["empty"] = stack.isEmpty();
    describe["components"] = encodePatch(stack);
    result["decoded"] = std::move(describe);

    json reencode = json::object();
    reencode["ok"] = true;
    nbt::tags::compound_tag encoded;
    stack.toNbt(encoded);
    std::ostringstream output;
    output << nbt::contexts::mojangson;
    nbt::operator<<(output, encoded);
    reencode["nbt"] = output.str();
    result["reencode"] = std::move(reencode);
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("stack_codec", &writeList, &runLine)

} // namespace mc::diff
