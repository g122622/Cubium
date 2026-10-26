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
 * @file block_tags.cpp
 * @brief 差分能力：方块标签的展开成员（`data/minecraft/tags/block/*.json`）。
 *
 * `list`：已注册的全部方块标签（`minecraft:<path>`）
 * `run` ：{ "tag": "minecraft:logs" }
 *         → 展开全部 `#tag` 引用后、属于该标签的方块（排序）
 *
 * 标签决定「能不能用镐挖」「是不是原木」「会不会被水冲刷」这一类玩法判定，
 * 而原版大量标签靠嵌套引用组织。只比标签名/profile 没有意义，必须比**展开后的集合**：
 * 少展开一层就会让整族方块归类错位。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/world/block/BlockTags.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

using mc::BlockTag;
using mc::BlockTags;

void ensureTags()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        BlockTags::initialize();
        s_initialized = true;
    }
}

std::string qualifiedTagName(const BlockTag& tag)
{
    const std::string name = tag.getId().toString();
    return name.starts_with("minecraft:") ? name : "minecraft:" + name;
}

void writeList(FILE* out)
{
    ensureTags();
    std::vector<std::string> names;
    BlockTags::forEachTag([&names](BlockTag& tag) {
        names.push_back(qualifiedTagName(tag));
    });
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureTags();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("tag").get<std::string>();

    BlockTag* tag = BlockTags::getTag(ResourceLocation(requested));
    if (tag == nullptr) {
        return json{{"tag", requested}, {"missing", true}}.dump();
    }

    // 递归展开 #tag 引用；visited 防止循环引用无限展开。
    std::unordered_set<ResourceLocation> blocks;
    std::unordered_set<ResourceLocation> referenced;
    std::unordered_set<ResourceLocation> visited;
    tag->collectIncludingReferences(blocks, referenced, visited);

    json members = json::array();
    for (const auto& block : blocks) {
        members.push_back(block.toString());
    }
    std::sort(members.begin(), members.end());
    return json{{"tag", requested}, {"values", std::move(members)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("block_tags", &writeList, &runLine)

} // namespace mc::diff