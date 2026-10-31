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
 * @brief 差分能力：盔甲纹饰（`trim_pattern` 与 `trim_material` 两类数据包注册表）。
 *
 * `list`：全部单元，形如 `pattern/minecraft:coast`、`material/minecraft:gold`（排序）
 * `run` ：
 *   - { "unit": "pattern/minecraft:coast", "pattern": "minecraft:coast" }
 *       → { "assetId": "minecraft:coast", "descriptionKey": "trim_pattern.minecraft.coast",
 *           "decal": false }
 *   - { "unit": "material/minecraft:gold", "material": "minecraft:gold" }
 *       → { "assetName": "gold", "descriptionKey": "trim_material.minecraft.gold",
 *           "descriptionColor": "#DEB12D",
 *           "overrides": [{ "asset": "minecraft:gold", "suffix": "gold_darker" }] }
 *
 * 对应原版
 * `record TrimPattern(Identifier assetId, Component description, boolean decal)` 与
 * `record TrimMaterial(MaterialAssetGroup assets, Component description)`，
 * 其中 `MaterialAssetGroup` = `(AssetInfo base, Map<ResourceKey<EquipmentAsset>,
 * AssetInfo> overrides)`、`AssetInfo` = `(String suffix)`。
 *
 * `description` 比的是**翻译键**而不是渲染后的文本（唱片机歌曲能力里踩过
 * `getString()` 的坑），另外单独比 `color` 字段。
 *
 * `overrides` 在两侧都是无序结构，故按装备材质键排序后输出，并由 harness 声明
 * `unorderedArrays`。
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
#include "common/item/armortrim/TrimMaterial.hpp"
#include "common/item/armortrim/TrimMaterials.hpp"
#include "common/item/armortrim/TrimPattern.hpp"
#include "common/item/armortrim/TrimPatterns.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

/**
 * @brief 确保纹饰注册表已从数据包加载。
 *
 * 两个注册表都是纯数据包注册表，没有任何硬编码默认值——不加载数据包就一个单元都没有。
 */
void ensureTrimLoaded()
{
    static bool s_initialized = false;
    if (s_initialized) {
        return;
    }

    const char* root = std::getenv("MC_DIFF_VANILLA_DATAPACK");
    if (root == nullptr || *root == '\0') {
        throw std::runtime_error("MC_DIFF_VANILLA_DATAPACK 未设置：需要指向原版数据包根目录（含 data/ 的那一层）");
    }
    // 仓库扫描的是「包含若干数据包的目录」，环境变量给的是数据包本身那一层，故取上一级。
    const std::filesystem::path packRoot(root);
    auto repository = std::make_unique<resource::DataPackRepository>();
    const auto scanResult = repository->scanDirectory(packRoot.parent_path());
    if (!scanResult.success() || scanResult.value() == 0) {
        throw std::runtime_error("armor_trim: 数据包扫描失败：" + packRoot.string());
    }

    const auto patternResult = TrimPatterns::loadFromDataPackRepository(*repository);
    if (patternResult.failed()) {
        throw std::runtime_error("armor_trim: 纹饰图案加载失败：" + patternResult.error().toString());
    }
    const auto materialResult = TrimMaterials::loadFromDataPackRepository(*repository);
    if (materialResult.failed()) {
        throw std::runtime_error("armor_trim: 纹饰材料加载失败：" + materialResult.error().toString());
    }
    s_initialized = true;
}

void writeList(FILE* out)
{
    ensureTrimLoaded();
    std::vector<std::string> units;
    TrimPatterns::forEachPattern([&](const ResourceLocation& id, const TrimPattern&) {
        units.push_back("pattern/" + id.toString());
    });
    TrimMaterials::forEachMaterial([&](const ResourceLocation& id, const TrimMaterial&) {
        units.push_back("material/" + id.toString());
    });
    // 注册表内部无序，排序后再输出，两侧才可比。
    std::sort(units.begin(), units.end());
    for (const auto& unit : units) {
        std::fprintf(out, "%s\n", unit.c_str());
    }
}

std::string runPattern(const json& input)
{
    const std::string requested = input.at("pattern").get<std::string>();
    const TrimPattern* pattern = TrimPatterns::get(ResourceLocation(requested));
    if (pattern == nullptr) {
        return json{{"unit", input.value("unit", std::string{})}, {"missing", true}}.dump();
    }
    return json{{"unit", input.value("unit", std::string{})},
        {"assetId", pattern->getAssetId().toString()},
        {"descriptionKey", pattern->getDescriptionKey()},
        {"decal", pattern->isDecal()}}
        .dump();
}

std::string runMaterial(const json& input)
{
    const std::string requested = input.at("material").get<std::string>();
    const TrimMaterial* material = TrimMaterials::get(ResourceLocation(requested));
    if (material == nullptr) {
        return json{{"unit", input.value("unit", std::string{})}, {"missing", true}}.dump();
    }

    // 覆盖表在两侧都是无序结构：按装备材质键排序后输出，保证逐字节可比。
    std::vector<std::pair<std::string, std::string>> overrides;
    overrides.reserve(material->getOverrides().size());
    for (const auto& [asset, suffix] : material->getOverrides()) {
        overrides.emplace_back(asset.toString(), suffix);
    }
    std::sort(overrides.begin(), overrides.end());

    json overrideJson = json::array();
    for (const auto& [asset, suffix] : overrides) {
        overrideJson.push_back(json{{"asset", asset}, {"suffix", suffix}});
    }

    return json{{"unit", input.value("unit", std::string{})},
        {"assetName", material->getAssetName()},
        {"descriptionKey", material->getDescriptionKey()},
        {"descriptionColor", material->getDescriptionColor()},
        {"overrides", overrideJson}}
        .dump();
}

std::string runLine(std::string_view inputLine)
{
    ensureTrimLoaded();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    if (unit.rfind("pattern/", 0) == 0) {
        return runPattern(input);
    }
    if (unit.rfind("material/", 0) == 0) {
        return runMaterial(input);
    }
    return json{{"unit", unit}, {"error", "未知单元前缀"}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("armor_trim", &writeList, &runLine)

} // namespace mc::diff
