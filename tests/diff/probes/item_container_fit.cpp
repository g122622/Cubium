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
 * @brief 差分能力：物品能否放进容器类物品（原版 `Item#canFitInsideContainerItems`）。
 *
 * `list`：全部已注册物品
 * `run` ：一行 -> { "fits": bool }
 *
 * ## 规则
 *
 * - `Item` 默认返回 **true**
 * - `BlockItem` 覆写：**潜影盒返回 false**，其余仍为 true
 *
 * 也就是 37 个潜影盒（含未染色与 16 色、以及铜质变体如果是容器的话）
 * 是唯一「不能放进去」的物品。
 *
 * ## 为什么这条重要
 *
 * 它防的是**递归存储**：潜影盒能装物品，如果潜影盒也能装潜影盒，
 * 就能做出无限嵌套的容器，一个物品格子承载无限内容 —— 既是无限容量漏洞，
 * 也是存档体积炸弹。
 *
 * 判据挂在 `BlockItem` 上而不是潜影盒物品类上，所以任何新增的潜影盒变体
 * （不同颜色、不同材质）都会自动被覆盖。反过来，如果按物品 id 白名单判断，
 * 新增颜色就会漏掉。
 */

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"

#include "probe.hpp"
#include "vanilla_datapack.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

void ensureInitialized()
{
    static const bool s_initialized = []() {
        datapack::initializeRegistries();
        Items::initialize();
        return true;
    }();
    (void)s_initialized;
}

struct ItemEntry {
    std::string name;
    const Item* item;
};

const std::vector<ItemEntry>& items()
{
    static const std::vector<ItemEntry> s_items = []() {
        std::vector<ItemEntry> all;
        ItemRegistry::instance().forEachItem([&all](Item& item) {
            all.push_back({item.itemLocation().toString(), &item});
        });
        std::sort(all.begin(), all.end(), [](const ItemEntry& a, const ItemEntry& b) {
            return a.name < b.name;
        });
        return all;
    }();
    return s_items;
}

void writeList(FILE* out)
{
    ensureInitialized();
    for (const ItemEntry& entry : items()) {
        std::fprintf(out, "%s\n", entry.name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    for (const ItemEntry& entry : items()) {
        if (entry.name != unit) {
            continue;
        }
        json result{{"unit", unit}};
        result["fits"] = entry.item->canFitInsideContainerItems();
        return result.dump();
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(item_container_fit, &writeList, &runLine)

} // namespace mc::diff
