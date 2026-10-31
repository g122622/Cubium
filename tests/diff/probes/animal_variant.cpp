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
 * @brief 差分能力：动物变体及其出生条件
 *        （`wolf_variant` / `cat_variant` / `frog_variant` / `cow_variant` /
 *         `chicken_variant` / `pig_variant`）。
 *
 * `list`：全部单元，形如 `wolf_variant/minecraft:pale`（排序）
 * `run` ：{ "unit": "cat_variant/minecraft:all_black", "kind": "cat_variant",
 *           "variant": "minecraft:all_black" }
 *         → { "assets": [{ "name": "", "id": "minecraft:entity/cat/all_black" }],
 *             "model": "",
 *             "selectors": [{ "condition": { "type": "structure",
 *                                            "entries": ["#minecraft:cats_spawn_as_black"] },
 *                             "priority": 1 },
 *                           { "condition": { "type": "moon_brightness", "entries": [],
 *                                            "rangeMin": 0.9 },
 *                             "priority": 0 }] }
 *
 * 对应原版 `SpawnPrioritySelectors(List<Selector<SpawnContext, SpawnCondition>>)`，
 * 每个 `Selector` 是 `(Optional<SpawnCondition> condition, int priority)`；
 * 条件实现为 `BiomeCheck(HolderSet<Biome>)`、`MoonBrightnessCheck(MinMaxBounds.Doubles)`、
 * `StructureCheck(HolderSet<Structure>)`。
 *
 * 三处刻意的口径：
 * - `entries` 保留 `#` 前缀：原版 `HolderSet` 的标签引用与内联元素语义不等价，
 *   比对必须能区分。
 * - 狼的 `assets` 按原版 `AssetInfo(wild, tame, angry)` 的**字段顺序**输出，
 *   而不是按 JSON 对象的遍历顺序。
 * - `rangeMin` / `rangeMax` 缺省时不输出该字段（`MinMaxBounds.Doubles` 两端都是可选）。
 */
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/entity/variant/AnimalVariant.hpp"
#include "common/entity/variant/AnimalVariants.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/// 本能力覆盖的全部变体类别（数据包目录名）。
constexpr const char* VARIANT_KINDS[] = {
    "wolf_variant",
    "cat_variant",
    "frog_variant",
    "cow_variant",
    "chicken_variant",
    "pig_variant",
};

/**
 * @brief 确保全部变体类别已从数据包加载。
 */
void ensureVariantsLoaded()
{
    static bool s_initialized = false;
    if (s_initialized) {
        return;
    }

    const char* root = std::getenv("MC_DIFF_VANILLA_DATAPACK");
    if (root == nullptr || *root == '\0') {
        throw std::runtime_error("MC_DIFF_VANILLA_DATAPACK 未设置：需要指向原版数据包根目录（含 data/ 的那一层）");
    }
    const std::filesystem::path packRoot(root);
    auto repository = std::make_unique<resource::DataPackRepository>();
    const auto scanResult = repository->scanDirectory(packRoot.parent_path());
    if (!scanResult.success() || scanResult.value() == 0) {
        throw std::runtime_error("animal_variant: 数据包扫描失败：" + packRoot.string());
    }

    for (const char* kind : VARIANT_KINDS) {
        const auto loadResult = AnimalVariants::loadFromDataPackRepository(*repository, kind);
        if (loadResult.failed()) {
            throw std::runtime_error(
                std::string("animal_variant: ") + kind + " 加载失败：" + loadResult.error().toString());
        }
    }
    s_initialized = true;
}

void writeList(FILE* out)
{
    ensureVariantsLoaded();
    std::vector<std::string> units;
    for (const char* kind : VARIANT_KINDS) {
        const auto& table = AnimalVariants::table(kind);
        for (const auto& [id, variant] : table) {
            if (variant) {
                units.push_back(std::string(kind) + "/" + id.toString());
            }
        }
    }
    // 注册表内部无序，排序后再输出，两侧才可比。
    std::sort(units.begin(), units.end());
    for (const auto& unit : units) {
        std::fprintf(out, "%s\n", unit.c_str());
    }
}

json conditionToJson(const SpawnCondition& condition)
{
    json entries = json::array();
    for (const auto& entry : condition.entries) {
        entries.push_back(entry);
    }
    json result{{"type", condition.type}, {"entries", entries}};
    if (condition.rangeMin.has_value()) {
        result["rangeMin"] = *condition.rangeMin;
    }
    if (condition.rangeMax.has_value()) {
        result["rangeMax"] = *condition.rangeMax;
    }
    return result;
}

std::string runLine(std::string_view inputLine)
{
    ensureVariantsLoaded();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const std::string kind = input.at("kind").get<std::string>();
    const std::string requested = input.at("variant").get<std::string>();

    const auto& table = AnimalVariants::table(kind);
    const auto it = table.find(ResourceLocation(requested));
    if (it == table.end() || !it->second) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }
    const AnimalVariant& variant = *it->second;

    json assets = json::array();
    for (const auto& asset : variant.getAssets()) {
        // 原版 `ClientAsset.ResourceTexture` 的私有构造器把数据文件里的 `asset_id`
        // 变换成完整纹理路径：`textures/` + path + `.png`。这里做同样的变换，
        // 否则两侧比的根本不是同一个东西（数据文件里写的是 `entity/cat/all_black`，
        // 原版暴露的是 `textures/entity/cat/all_black.png`）。
        const std::string texturePath = "textures/" + asset.id.path() + ".png";
        assets.push_back(json{
            {"name", asset.name},
            {"id", asset.id.namespace_() + ":" + texturePath},
        });
    }

    // 选择器的顺序在数组里有语义（优先级链的书写顺序），必须原样保留。
    json selectors = json::array();
    for (const auto& selector : variant.getSelectors()) {
        json item{{"priority", selector.priority}};
        if (selector.condition.has_value()) {
            item["condition"] = conditionToJson(*selector.condition);
        } else {
            // 原版 Selector 的 condition 是 Optional：空表示无条件命中。
            item["condition"] = nullptr;
        }
        selectors.push_back(std::move(item));
    }

    return json{{"unit", unit}, {"assets", assets}, {"model", variant.getModel()}, {"selectors", selectors}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("animal_variant", &writeList, &runLine)

} // namespace mc::diff
