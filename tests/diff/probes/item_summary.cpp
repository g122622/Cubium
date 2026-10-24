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
 * @file item_summary.cpp
 * @brief 差分能力：物品的注册表与食物属性。
 *
 * `list`   ：全部物品 ID
 * `run`    ：{ "item": "minecraft:apple" } → 堆叠上限、是否为食物、饥饿值、饱和度修正值
 *
 * 饱和度按 vanilla 的公式 `nutrition * saturationModifier * 2` 计算后**按位**输出
 * （十六进制），避免十进制打印的舍入歧义——这正是差分最容易出问题的地方。
 */
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemRegistry.hpp"
#include "common/item/food/Food.hpp"
#include "common/resource/ResourceLocation.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

void ensureItems()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        Items::initialize();
        s_initialized = true;
    }
}

/** 浮点按位输出：与 Java 的 Float.floatToIntBits 一样，屏蔽打印格式差异。 */
std::string floatBits(f32 value)
{
    u32 bits = 0;
    static_assert(sizeof(bits) == sizeof(value), "f32 必须是 4 字节");
    std::memcpy(&bits, &value, sizeof(bits));
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "0x%08x", bits);
    return buffer;
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
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureItems();
    const json input = json::parse(inputLine);
    const std::string requested = input.at("item").get<std::string>();

    Item* item = ItemRegistry::instance().getItem(ResourceLocation(requested));
    if (item == nullptr) {
        return json{{"item", requested}, {"missing", true}}.dump();
    }

    json result{{"item", requested}, {"maxStackSize", item->maxStackSize()}, {"isFood", item->isFood()}};

    // 食物信息挂在 ItemProperties 上，由 Item::foodProperties() 暴露（与 isFood() 同源）。
    // 不能用 Item::getFood()——那是给子类覆盖用的虚函数，默认返回 nullptr，
    // 普通物品即使注册了 Food 也拿不到。
    const auto* food = item->foodProperties();
    if (food != nullptr) {
        const f32 saturation = static_cast<f32>(food->getHunger()) * food->getSaturationModifier() * 2.0F;
        result["hunger"] = food->getHunger();
        result["saturationModifier"] = floatBits(food->getSaturationModifier());
        result["saturation"] = floatBits(saturation);
    }
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("item_summary", &writeList, &runLine)

} // namespace mc::diff