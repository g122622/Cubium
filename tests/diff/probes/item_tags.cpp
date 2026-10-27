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
 * @file item_tags.cpp
 * @brief 差分能力：物品标签的展开成员（`data/minecraft/tags/item`）。
 *
 * `list`：已注册的全部物品标签（`minecraft:<path>`）
 * `run` ：{ "tag": "minecraft:planks" }
 *         → 展开全部 `#tag` 引用后、属于该标签的物品（排序）
 *
 * 物品标签决定合成/熔炼配方接受哪些材料、村民能否捡起、刷怪蛋归类等。原版大量标签
 * 靠嵌套引用组织，所以比的是展开后的集合。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/tag/ItemTags.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

using item::tag::ItemTag;
using item::tag::ItemTags;

void ensureTags()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        // 物品标签按物品对象存储，必须先注册物品。
        Items::initialize();
        ItemTags::initialize();
        s_initialized = true;
    }
}

std::string qualifiedTagName(const ItemTag& tag)
{
    const std::string name = tag.getId().toString();
    return name.starts_with("minecraft:") ? name : "minecraft:" + name;
}

void writeList(FILE* out)
{
    ensureTags();
    std::vector<std::string> names;
    ItemTags::forEachTag([&names](ItemTag& tag) {
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

    ItemTag* tag = ItemTags::getTag(ResourceLocation(requested));
    if (tag == nullptr) {
        return json{{"tag", requested}, {"missing", true}}.dump();
    }

    json members = json::array();
    for (const Item* item : tag->getItems()) {
        members.push_back(item->itemLocation().toString());
    }
    std::sort(members.begin(), members.end());
    return json{{"tag", requested}, {"values", std::move(members)}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("item_tags", &writeList, &runLine)

} // namespace mc::diff
