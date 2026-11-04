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
 * @brief 差分能力：实体类型的**刷怪分类**、**网络追踪参数**与**环境免疫**
 *        （原版 `EntityType` 的 `MobCategory` / `clientTrackingRange` /
 *        `updateInterval` / `fireImmune`）。
 *
 * `list`：全部已注册实体类型
 * `run` ：一行 -> { classification, trackingRange, updateInterval, fireImmune }
 *
 * ## 五项各自决定什么
 *
 * | 项 | 作用 | 写错的后果 |
 * |---|---|---|
 * | `classification` | 刷怪类别：影响生物上限、刷怪权重、是否进史莱姆区块统计 | 怪物按生物上限算、动物无上限，或反之 |
 * | `trackingRange` | 客户端多远处还同步这个实体 | 太小题材：远处实体突然消失/卡住；太大浪费带宽 |
 * | `updateInterval` | 位置同步间隔（刻） | 太大会拖影，太小浪费带宽 |
 * | `fireImmune` | 火焰伤害是否免疫 | 烈焰人能被烧死，或普通生物掉进火里不死 |
 *
 * ## 分类是重灾区
 *
 * 原版把实体分成 8 类，每类的**生物上限**完全不同：
 *
 * ```
 * monster 70 / creature 10 / ambient 15 / axolotls 5
 * underground_water_creature 5 / water_creature 5 / water_ambient 20 / misc -1
 * ```
 *
 * 把美西螈错归进水生生物（5 上限）而不是独立分类，会让美西螈和鱿鱼抢名额；
 * 把发光鱿鱼错归进水生生物，会挤占海豚的刷新。这些在单人世界里表现为
 * 「某种生物怎么都不刷」。
 */

#include <algorithm>
#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/entity/core/EntityClassification.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/entity/registry/VanillaEntities.hpp"

#include "probe.hpp"
#include "vanilla_datapack.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

void ensureInitialized()
{
    static const bool s_initialized = []() {
        datapack::initializeRegistries();
        entity::VanillaEntities::registerAll();
        return true;
    }();
    (void)s_initialized;
}

std::vector<std::string> typeNames()
{
    std::vector<std::string> names;
    for (const entity::EntityType* type : entity::EntityRegistry::instance().getAllTypes()) {
        names.push_back(type->name());
    }
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    return names;
}

void writeList(FILE* out)
{
    ensureInitialized();
    for (const std::string& name : typeNames()) {
        std::fprintf(out, "%s\n", name.c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    for (const entity::EntityType* type : entity::EntityRegistry::instance().getAllTypes()) {
        if (type->name() != unit) {
            continue;
        }
        const entity::EntityClassificationInfo info = entity::EntityClassificationInfo::get(type->classification());

        json result{{"unit", unit}};
        result["classification"] = info.name;
        result["trackingRange"] = type->trackingRange();
        result["updateInterval"] = type->updateInterval();
        result["fireImmune"] = type->immuneToFire();
        return result.dump();
    }

    return json{{"unit", unit}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(entity_type_flags, &writeList, &runLine)

} // namespace mc::diff
