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
 * @file light_occlusion.cpp
 * @brief 差分能力：方块状态的光照阻挡属性（每个状态各一组）。
 *
 * `list`：全部方块 ID（单元是方块，一次返回它全部状态的值）
 * `run` ：{ "block": "minecraft:oak_leaves" }
 *         → { "values": { "minecraft:oak_leaves[distance=1,...]": {
 *               "lightBlock": 1, "propagatesSkylightDown": false,
 *               "useShapeForLightOcclusion": false, "canOcclude": true }, ... } }
 *
 * ## 为什么值得比
 *
 * 光照引擎按这四个值决定「一束光走过这一格之后还剩多少」：`lightBlock` 是每格的衰减量，
 * 天空光另有 `propagatesSkylightDown` 的直通快路，而 `canOcclude` 与
 * `useShapeForLightOcclusion` 决定要不要按形状做面遮挡。树叶、台阶、耕地、海带这类
 * 「看着像实心、其实是特殊方块」的东西全在这里分流。
 *
 * 它们都是纯数据，抄错不会有编译期提示：多衰减一级，屋子里就永远差一格看不清；把
 * 树叶判成完全不透光，树冠下的地面就再也长不出草。只能靠与原版逐条比对发现。
 *
 * 与 `block_properties` 的分工：那个能力比的是方块级的硬度/亮度/滑度，且明确把
 * 「光照阻挡」排除在外（材质默认值与逐方块注册的差异见 README「未对齐项」）；本能力
 * 专比状态级的这四个光照属性，两者不重叠。
 */
#include <algorithm>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/resource/ResourceLocation.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

bool ensureBlocks()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        VanillaBlocks::initialize();
        s_initialized = true;
    }
    return true;
}

/** 原版 `BlockState#getSerializedName()` 的形式：`minecraft:oak_stairs[facing=east,half=bottom]`。 */
std::string stateName(const BlockState& state, std::string_view blockId)
{
    std::vector<std::pair<std::string, std::string>> pairs;
    for (const auto& entry : state.values()) {
        pairs.emplace_back(entry.property->name(), entry.property->valueToString(entry.valueIndex));
    }
    if (pairs.empty()) {
        return std::string(blockId);
    }
    std::sort(pairs.begin(), pairs.end());
    std::string result(blockId);
    result.push_back('[');
    for (size_t i = 0; i < pairs.size(); ++i) {
        if (i != 0) {
            result.push_back(',');
        }
        result += pairs[i].first;
        result.push_back('=');
        result += pairs[i].second;
    }
    result.push_back(']');
    return result;
}

void writeList(FILE* out)
{
    ensureBlocks();
    std::vector<std::string> names;
    BlockRegistry::instance().forEachBlock([&](Block& block) {
        names.push_back(block.blockLocation().toString());
    });
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureBlocks();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("block").get<std::string>();

    Block* block = BlockRegistry::instance().getBlock(ResourceLocation(requested));
    if (block == nullptr) {
        return json{{"block", requested}, {"missing", true}}.dump();
    }

    // 输出按状态名排序的 map：两侧的遍历顺序不同（注册顺序 vs 声明顺序），
    // 比的是「状态名 → 四个值」这组对应关系，与顺序无关。
    std::map<std::string, json> values;
    for (const auto& state : block->stateContainer().validStates()) {
        values.emplace(stateName(*state, requested),
            json{{"lightBlock", state->getOpacity()},
                {"propagatesSkylightDown", state->propagatesSkylightDown()},
                {"useShapeForLightOcclusion", state->useShapeForLightOcclusion()},
                {"canOcclude", state->canOcclude()}});
    }

    return json{{"block", requested}, {"values", values}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("light_occlusion", &writeList, &runLine)

} // namespace mc::diff
