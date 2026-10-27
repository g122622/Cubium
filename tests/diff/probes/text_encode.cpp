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
 * @file text_encode.cpp
 * @brief 差分能力：方块状态的文本表示。
 *
 * 覆盖 vanilla `BlockState#toString()` 的语义：
 *   minecraft:stone
 *   minecraft:oak_log[axis=y]
 *   minecraft:oak_slab[type=bottom,waterlogged=false]
 *
 * 两件事同时被验证：
 *   1. 状态集合——同一个 blockId 下有哪些状态、属性取值有哪些；
 *   2. 文本编码——属性顺序与值的拼写。
 *
 * vanilla 的属性按**名字字典序**排列（`StateHolder#toString` 遍历 `populateNeighbours`
 * 构建时按名字排序的 `propertyNeighbours`），而 Cubium 的 `StateHolder::toString()`
 * 按注册顺序输出。因此本 probe 不直接调用 `toString()`，而是自己按字典序拼装：
 * 差分测的是「状态集合 + 属性名值」，不是 Cubium 内部注册顺序。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/resource/ResourceLocation.hpp"
#include "common/util/property/IProperty.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/**
 * @brief 初始化方块注册表（幂等）。
 *
 * probe 进程只做一次注册，之后整个进程生命周期内只读。
 */
void ensureBlocks()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        VanillaBlocks::initialize();
        s_initialized = true;
    }
}

/**
 * @brief 按 vanilla 的格式拼装状态文本：name 或 name[k=v,...]，属性按名字字典序。
 *
 * `StateHolder::toString()` 用的是属性注册顺序，而 vanilla 的 `StateHolder#toString`
 * 遍历的是按名字排序的 `propertyNeighbours`。差分要比的是那个语义顺序，因此这里
 * 自己拼装，不直接调用 toString()。
 */
std::string stateName(const BlockState& state)
{
    // StateHolder::values() 是本状态已设置属性的公开视图（propertyLayouts() 为 protected）。
    const auto values = state.values();
    std::vector<std::pair<std::string, std::string>> entries;
    entries.reserve(values.size());
    for (const auto& entry : values) {
        if (entry.property == nullptr) {
            continue;
        }
        entries.emplace_back(entry.property->name(), entry.property->valueToString(entry.valueIndex));
    }
    std::sort(entries.begin(), entries.end());

    if (entries.empty()) {
        return state.owner().blockLocation().toString();
    }
    std::string result = state.owner().blockLocation().toString();
    result.push_back('[');
    for (size_t i = 0; i < entries.size(); ++i) {
        if (i != 0) {
            result.push_back(',');
        }
        result.append(entries[i].first);
        result.push_back('=');
        result.append(entries[i].second);
    }
    result.push_back(']');
    return result;
}

void writeList(FILE* out)
{
    ensureBlocks();
    auto& registry = BlockRegistry::instance();

    std::vector<std::string> names;
    names.reserve(registry.stateCount());
    registry.forEachBlockState([&](const BlockState& state) {
        names.push_back(stateName(state));
    });

    // 两侧都必须以同一顺序输出：按名字字典序，而不是依赖注册顺序或 stateId。
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());

    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

/**
 * @brief 方块名下每个属性的元数据，按属性名字典序。
 *
 * 输出形如 [{"name":"axis","type":"enum","values":["x","y","z"]}]。
 *
 * 特意带上 type 与全部允许值：vanilla 的 `axis` 是 EnumProperty、`level` 是
 * IntegerProperty、`lit` 是 BooleanProperty，三者在中文字符串表示上可能相同，
 * 但类型不同会导致后续 `BlockStateParser` 的接受范围不同。只比字符串会漏掉这类差异。
 */
json describeBlock(Block& block)
{
    const auto& registered = block.stateContainer().properties();
    std::vector<const IProperty*> ordered;
    ordered.reserve(registered.size());
    for (const auto& [name, prop] : registered) {
        ordered.push_back(prop);
    }
    std::sort(ordered.begin(), ordered.end(), [](const IProperty* lhs, const IProperty* rhs) {
        return lhs->name() < rhs->name();
    });

    json properties = json::array();
    for (const IProperty* prop : ordered) {
        json values = json::array();
        for (size_t i = 0; i < prop->valueCount(); ++i) {
            values.push_back(prop->valueToString(i));
        }
        properties.push_back({{"name", prop->name()}, {"type", prop->typeName()}, {"values", std::move(values)}});
    }

    return json{{"block", block.blockLocation().toString()},
        {"stateCount", block.stateContainer().stateCount()},
        {"properties", std::move(properties)}};
}

/**
 * @brief 执行一行输入。
 *
 * 输入：{ "block": "minecraft:oak_log" }
 * 输出：{ "block": "minecraft:oak_log", "stateCount": 3,
 *         "properties": [{"name":"axis","type":"enum","values":["x","y","z"]}] }
 *
 * 找不到方块时输出 { "block": ..., "missing": true }。
 *
 * 注意这里比对的是**每方块一次**的摘要，而不是 29 671 行逐状态文本：状态全集由
 * `list` 子命令覆盖，`run` 只需确认「这个方块有哪些属性、类型是什么、值域多大、状态数多少」。
 * 这样即使某一侧多了/少了一个方块，报告也能精确落在方块粒度上。
 */
std::string runLine(std::string_view inputLine)
{
    ensureBlocks();

    const json input = json::parse(inputLine);
    const std::string requested = input.at("block").get<std::string>();

    Block* block = BlockRegistry::instance().getBlock(ResourceLocation(requested));
    if (block == nullptr) {
        return json{{"block", requested}, {"missing", true}}.dump();
    }
    return describeBlock(*block).dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("text_encode", &writeList, &runLine)

} // namespace mc::diff
