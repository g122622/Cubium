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
 * @file mob_category.cpp
 * @brief 差分能力：实体分类（原版 MobCategory）的生成参数。
 *
 * `list`：全部分类名（vanilla MobCategory 的枚举名，小写）
 * `run` ：{ "category": "monster" }
 *         → 每区块上限、是否友好、是否持久
 *
 * 这些数直接决定刷怪器每刻最大的刷怪量与消失节奏：上限差 1 就是明显的刷怪量差异，
 * 消失距离错一个区块会让生物在玩家眼前凭空消失。它们都是硬编码常量，只能靠比对发现。
 */
#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/entity/core/EntityClassification.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

using entity::EntityClassification;

constexpr EntityClassification ALL_CATEGORIES[] = {
    EntityClassification::Monster,
    EntityClassification::Creature,
    EntityClassification::Ambient,
    EntityClassification::Axolotls,
    EntityClassification::UndergroundWaterCreature,
    EntityClassification::WaterCreature,
    EntityClassification::WaterAmbient,
    EntityClassification::Misc,
};

std::string categoryName(EntityClassification category)
{
    return entity::EntityClassificationInfo::get(category).name;
}

void writeList(FILE* out)
{
    std::vector<std::string> names;
    for (const auto category : ALL_CATEGORIES) {
        names.push_back(categoryName(category));
    }
    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        std::fprintf(out, "minecraft:%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("category").get<std::string>();
    constexpr std::string_view NAMESPACE_PREFIX = "minecraft:";
    const std::string bare =
        requested.starts_with(NAMESPACE_PREFIX) ? requested.substr(NAMESPACE_PREFIX.size()) : requested;

    for (const auto category : ALL_CATEGORIES) {
        const auto info = entity::EntityClassificationInfo::get(category);
        if (info.name != bare) {
            continue;
        }
        // 原版只公开每区块上限、友好、持久三项；消失距离是 Cubium 自己维护的
        // 派生值，没有对应的原版字段，不纳入比较。
        return json{{"category", requested},
            {"maxCount", info.maxCount},
            {"peaceful", info.isPeaceful},
            {"persistent", entity::isPersistent(category)}}
            .dump();
    }
    return json{{"category", requested}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("mob_category", &writeList, &runLine)

} // namespace mc::diff
