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
 * @file item_component_nbt.cpp
 * @brief 差分能力：物品数据组件补丁的 NBT 表示。
 *
 * `list`：全部物品 ID × 三个变体（plain / suite / removed），每行「物品#变体」
 * `run` ：{ "unit": "minecraft:diamond_sword#suite", "stack": { "id": ..., "components": {...} } }
 *         → { "components": { "minecraft:damage": "7", ... } }
 *
 * 组件是 1.21 之后物品的全部可变状态：伤害、附魔、药水、工具规则、烟花……它们写进存档的
 * `components` 段、随网络同步、被潜影盒与箱子原样搬运。组件名拼错、值写成错误的 NBT 类型
 * （`4.2f` 与 `4.2d`、`[I]` 与 `[...L]`）、空值写成空 compound 而不是空列表——这些都不会
 * 在运行时立刻报错，而是表现为「存档能读、东西不对」。
 *
 * 两侧都从**同一份组件描述**（harness/harnesses/item_component_nbt.ts）构造物品堆：
 * 探针按它逐组件调 `ItemStack::setComponent`，oracle 按它喂 `ItemStack.CODEC`。因此比对的
 * 是组件编解码本身，而不是各自对「怎么造句」的理解。
 *
 * 输出一律转成 SNBT 文本（`{...}` / `[...]` / `4.2f`），与 oracle 的 `Tag.toString()` 同形；
 * 类型差异因此显式可见，不会被 JSON 数字吞掉。
 */
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

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

/// 与 ItemComponentNbtOracle 的变体名必须一字不差（单元键的一部分）。
constexpr const char* VARIANT_PLAIN = "plain";
constexpr const char* VARIANT_SUITE = "suite";
constexpr const char* VARIANT_REMOVED = "removed";

void ensureItems()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        Items::initialize();
        // 乐器注册表：instrument 组件的持久形态是 Holder<Instrument> 的完整定义
        // （sound_event / use_duration / range / description），编码时要查注册表才写得出来。
        // 没初始化时只能退回「资源位置字符串」，与原版不一致。
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
        for (const char* variant : {VARIANT_PLAIN, VARIANT_SUITE, VARIANT_REMOVED}) {
            std::fprintf(out, "%s#%s\n", name.c_str(), variant);
        }
    }
}

/**
 * @brief 把 NBT 标签写成 SNBT 文本。
 *
 * 先用二进制格式往返一次：`tag::write` 输出的是十进制/最短表示，与本探针手工拼字符串相比，
 * 走一趟官方写出路径能保证「同一个标签只有一种文本」，两侧比对才有意义。
 */
std::string toSnbt(const nbt::tags::compound_tag& tag)
{
    std::ostringstream output;
    output << nbt::contexts::mojangson;
    nbt::operator<<(output, tag);
    return output.str();
}

/**
 * @brief 把组件补丁里的每个组件单独编码成 SNBT。
 *
 * 与 oracle 的口径一致：键是 `minecraft:` 前缀的组件名，被移除的组件写成 `!name`。
 * `payloadToNbt` 是组件 NBT 编码的唯一入口（`ItemStack::toNbt` 的 components 段也走它）。
 */
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
        if (tag->id() == nbt::TagId::Compound) {
            result[std::string(*name)] = toSnbt(dynamic_cast<const nbt::tags::compound_tag&>(*tag));
        } else {
            // 少数组件的顶层就是列表/标量（附魔、can_break……）：包一层只含单键的
            // compound 再用 mojangson 写出，才是合法的 SNBT 根；去掉外层花括号。
            nbt::tags::compound_tag wrapper;
            wrapper.value.emplace(std::string(*name), tag->copy());
            const std::string text = toSnbt(wrapper);
            const auto colon = text.find(':');
            const auto begin = text.find(':', colon + 1);
            const auto end = text.rfind('}');
            result[std::string(*name)] = (begin == std::string::npos || end == std::string::npos)
                ? text
                : text.substr(begin + 1, end - begin - 1);
        }
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
 * @brief 按输入的 stack 描述构造物品堆。
 *
 * `components` 里的每个键都必须能被 Cubium 识别（未落地的组件名直接报错）：静默跳过会让
 * 「组件没实现」看起来像「组件正确」，这正是这个能力要抓的错误。Cubium 侧尚未落地的组件由
 * 框架层的 `knownDifferences` 登记缺口，不在这里吞掉。
 */
ItemStack buildStack(const json& spec)
{
    if (!spec.contains("id")) {
        throw std::runtime_error("stack 缺少 id");
    }
    // 单元集合来自 oracle 的物品表，Cubium 未注册的物品（方块物品尚未建、或原版有本项目没有）
    // 按「本侧没有这个单元」处理：输出 `missing`，由编排层按单元归口，而不是整行报错。
    const ResourceLocation location(spec.at("id").get<std::string>());
    Item* item = ItemRegistry::instance().getItem(location);
    if (item == nullptr) {
        return {};
    }
    // 原版 ItemStack.CODEC 显式拒绝空气（"Item must not be minecraft:air"），空气不是
    // 「一个物品堆」，而是「没有物品堆」。两侧都按本侧构造不出来处理。
    if (location == ResourceLocation("minecraft:air")) {
        return {};
    }
    ItemStack stack(item, 1);

    if (!spec.contains("components") || !spec.at("components").is_object()) {
        return stack;
    }
    const auto& components = spec.at("components");
    if (!components.is_object()) {
        throw std::runtime_error("stack.components 必须是对象");
    }
    // 先全部 remove 再全部 set：JSON 对象里 `!name` 与 `name` 可能同时出现（原版的补丁 map
    // 一项只有一个值，后写的覆盖先写的），逐项处理会让结果取决于 JSON 键序。
    // 这里的物品堆是本行独立构造的，先移除不会影响别的行。
    for (const auto& [key, value] : components.items()) {
        // 移除键写作 `!minecraft:name`，与存档 components 段的表示一致。
        const bool removed = key.rfind("!", 0) == 0;
        const std::string name = removed ? key.substr(1) : key;
        auto type = componentTypeByName(name);
        if (!type.has_value()) {
            throw std::runtime_error("Cubium 不认识组件：" + name);
        }
        if (removed) {
            stack.removeComponent(*type);
        }
    }
    for (const auto& [key, value] : components.items()) {
        if (key.rfind("!", 0) == 0) {
            continue;
        }
        auto type = componentTypeByName(key);
        if (!type.has_value()) {
            throw std::runtime_error("Cubium 不认识组件：" + key);
        }
        DataComponentPayload payload;
        if (!jsonToComponentPayload(*type, value, payload)) {
            throw std::runtime_error("组件载荷解析失败：" + key);
        }
        stack.setComponent(*type, std::move(payload));
    }
    return stack;
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
    const ItemStack stack = buildStack(input.at("stack"));
    // 只有「物品类型不存在」才算本侧缺失。`minecraft:air` 是**合法的空物品堆**（数量语义上
    // 归零），它的组件补丁是空对象，与 oracle 的 `components: {}` 对应——不能按 isEmpty()
    // 判缺失，否则唯一的真实差异会被掩盖。
    if (stack.getItem() == nullptr) {
        result["missing"] = true;
        return result.dump();
    }
    result["components"] = encodePatch(stack);
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("item_component_nbt", &writeList, &runLine)

} // namespace mc::diff
