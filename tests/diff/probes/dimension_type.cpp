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
 * @brief 差分能力：维度类型（数据包 `data/<ns>/dimension_type/`，原版 4 项）。
 *
 * `list`：全部维度类型 ID（排序）
 * `run` ：{ "dimensionType": "minecraft:the_nether" }
 *         → { "dimensionType": "minecraft:the_nether", "hasSkylight": false, ... }
 *
 * 对应原版 `record DimensionType(...)`。
 *
 * ## 为什么这个能力重要
 *
 * 1.21.11 把一大批「维度游戏规则」从独立布尔字段改成了 `attributes` 映射，例如
 * `gameplay/bed_rule`、`gameplay/respawn_anchor_works`、`gameplay/piglins_zombify`。
 * Cubium 的 `DimensionType` 仍是旧模型（`ultraWarm` / `natural` / `bedWorks` /
 * `piglinSafe` 等独立字段），且**没有任何代码读取 `dimension_type` 数据包**——
 * 只有一份硬编码实现。本能力直接解析数据包，把两侧的口径固定下来。
 *
 * ## 三类可比的量
 *
 * - **原值**：`height` / `min_y` / `logical_height` / `coordinate_scale` / `ambient_light`
 *   等。其中 `coordinate_scale` 在原版是 **`double`**，Cubium 是 `f32`——下界 1e-5，
 *   主世界/nether/末地的 1.0 / 8.0 都能精确表示，但类型本身不同。
 * - **缺省回退**：`skybox` 默认 `OVERWORLD`、`cardinal_light` 默认 `DEFAULT`、
 *   `has_fixed_time` 默认 `false`。
 * - **派生值**：`hasEndFlashes` 就是 `skybox == END`；`monsterSpawnLightLevel` 可能是
 *   整数也可能是 IntProvider，两种形态都要正确表达。
 *
 * 另外把原版 `createDirectCodec` 里的三条校验也算成输出字段，让「数据是否合法」
 * 本身成为可比量：`height % 16 == 0`、`min_y % 16 == 0`、`logical_height <= height`。
 *
 * TODO: Cubium 目前没有 `dimension_type` 数据包加载器，本探针直接解析 JSON。
 * 待其改为数据包驱动后应改为读注册表。
 */
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/repository/DataPackRepository.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

constexpr std::string_view DIMENSION_TYPE_DIRECTORY = "dimension_type";

/**
 * @brief 把浮点数格式化成「6 位有效数字，去掉尾随零」的字符串。
 */
std::string formatFloat(double value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.6g", value);
    std::string text(buffer);
    if (text.find('.') != std::string::npos && text.find('e') == std::string::npos &&
        text.find('E') == std::string::npos) {
        while (!text.empty() && text.back() == '0') {
            text.pop_back();
        }
        if (!text.empty() && text.back() == '.') {
            text.pop_back();
        }
    }
    return text;
}

std::optional<ResourceLocation> dimensionTypeIdFromPath(const std::string& resourcePath)
{
    const std::string marker = "/" + std::string(DIMENSION_TYPE_DIRECTORY) + "/";
    const auto markerPos = resourcePath.find(marker);
    if (markerPos == std::string::npos) {
        return std::nullopt;
    }
    const std::string ns = resourcePath.substr(0, markerPos);
    std::string path = resourcePath.substr(markerPos + marker.size());
    const auto dotPos = path.rfind('.');
    if (dotPos != std::string::npos) {
        path = path.substr(0, dotPos);
    }
    return ResourceLocation(ns + ":" + path);
}

std::vector<std::pair<ResourceLocation, json>> loadDimensionTypes()
{
    const char* root = std::getenv("MC_DIFF_VANILLA_DATAPACK");
    if (root == nullptr || *root == '\0') {
        throw std::runtime_error("MC_DIFF_VANILLA_DATAPACK 未设置：需要指向原版数据包根目录（含 data/ 的那一层）");
    }
    const std::filesystem::path packRoot(root);
    auto repository = std::make_unique<resource::DataPackRepository>();
    const auto scanResult = repository->scanDirectory(packRoot.parent_path());
    if (!scanResult.success() || scanResult.value() == 0) {
        throw std::runtime_error("dimension_type: 数据包扫描失败：" + packRoot.string());
    }

    const auto namespacesResult = repository->getResourceNamespaces();
    if (!namespacesResult.success()) {
        throw std::runtime_error("dimension_type: 无法列出数据包命名空间：" + namespacesResult.error().toString());
    }

    std::vector<std::pair<ResourceLocation, json>> entries;
    for (const auto& ns : namespacesResult.value()) {
        const std::string directory = ns + "/" + std::string(DIMENSION_TYPE_DIRECTORY);
        const auto stacksResult = repository->listResourceStacks(directory, ".json");
        if (!stacksResult.success()) {
            continue;
        }
        for (const auto& [resourcePath, versions] : stacksResult.value()) {
            if (versions.empty()) {
                continue;
            }
            const auto& content = versions.back();
            const auto id = dimensionTypeIdFromPath(resourcePath);
            if (!id.has_value()) {
                continue;
            }
            json parsed;
            try {
                parsed = json::parse(content.content);
            }
            catch (const std::exception& e) {
                throw std::runtime_error("dimension_type: 解析失败 " + resourcePath + "：" + std::string(e.what()));
            }

            bool replaced = false;
            for (auto& [existingId, existingJson] : entries) {
                if (existingId.toString() == id->toString()) {
                    existingJson = parsed;
                    replaced = true;
                    break;
                }
            }
            if (!replaced) {
                entries.emplace_back(*id, parsed);
            }
        }
    }

    std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first.toString() < rhs.first.toString();
    });
    return entries;
}

/**
 * @brief 把 `monster_spawn_light_level` 归一成字符串。
 *
 * 原版这里是 `IntProvider.codec(0, 15)`：既可以写整数（等价 ConstantInt），也可以写
 * 带 `type` 的对象（uniform / biased_to_bottom 等）。这里整数直接输出数字，对象输出
 * 排序后的紧凑 JSON，两侧规则一致。
 */
std::string intProviderToString(const json& value)
{
    if (value.is_number_integer()) {
        return std::to_string(value.get<i64>());
    }
    // 非恒定形态按「类型[下界,上界]」拼成固定格式，不直接 dump JSON：
    // nlohmann 与 Gson 的键顺序不同，直接比原始 JSON 会因字段顺序而误报。
    if (value.is_object()) {
        const std::string type = value.value("type", "");
        if (value.contains("min_inclusive") && value.contains("max_inclusive")) {
            return type + "[" + std::to_string(value["min_inclusive"].get<i64>()) + "," +
                std::to_string(value["max_inclusive"].get<i64>()) + "]";
        }
        return type;
    }
    return value.dump();
}

void writeList(FILE* out)
{
    for (const auto& [id, definition] : loadDimensionTypes()) {
        std::fprintf(out, "%s\n", id.toString().c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("dimensionType").get<std::string>();

    for (const auto& [id, definition] : loadDimensionTypes()) {
        if (id.toString() != requested) {
            continue;
        }

        const i32 minY = definition.at("min_y").get<i32>();
        const i32 height = definition.at("height").get<i32>();
        const i32 logicalHeight = definition.at("logical_height").get<i32>();
        const double coordinateScale = definition.at("coordinate_scale").get<double>();

        // 缺省字段按原版 codec 的默认值回退。
        const std::string skybox = definition.value("skybox", "overworld");
        const std::string cardinalLight = definition.value("cardinal_light", "default");
        const bool hasFixedTime = definition.value("has_fixed_time", false);

        // 原版 createDirectCodec 的三条校验。
        const bool validHeightMultiple = (height % 16) == 0;
        const bool validMinYMultiple = (minY % 16) == 0;
        const bool validLogicalHeight = logicalHeight <= height;

        json attributes = json::array();
        if (definition.contains("attributes") && definition["attributes"].is_object()) {
            for (const auto& [key, value] : definition["attributes"].items()) {
                attributes.push_back(key);
            }
            std::sort(attributes.begin(), attributes.end());
        }

        json timelines = json::array();
        if (definition.contains("timelines")) {
            const auto& raw = definition["timelines"];
            if (raw.is_string()) {
                timelines.push_back(raw.get<std::string>());
            } else if (raw.is_array()) {
                for (const auto& item : raw) {
                    timelines.push_back(item);
                }
                std::sort(timelines.begin(), timelines.end());
            }
        }

        // `infiniburn` 在 JSON 里写作 `"#minecraft:infiniburn_overworld"`，但原版
        // `DimensionType.infiniburn()` 返回 `TagKey<Block>`，它的 `location()` **不带**
        // `#` 前缀。这里剥掉前缀再输出，否则每个维度都会差一个字符。
        std::string infiniburn = definition.at("infiniburn").get<std::string>();
        if (!infiniburn.empty() && infiniburn.front() == '#') {
            infiniburn.erase(infiniburn.begin());
        }

        return json{{"dimensionType", requested},
            {"minY", minY},
            {"height", height},
            {"logicalHeight", logicalHeight},
            {"coordinateScale", formatFloat(coordinateScale)},
            {"ambientLight", formatFloat(definition.at("ambient_light").get<double>())},
            {"hasSkylight", definition.at("has_skylight").get<bool>()},
            {"hasCeiling", definition.at("has_ceiling").get<bool>()},
            {"hasFixedTime", hasFixedTime},
            {"skybox", skybox},
            {"cardinalLight", cardinalLight},
            {"infiniburn", infiniburn},
            {"monsterSpawnLightLevel", intProviderToString(definition.at("monster_spawn_light_level"))},
            {"monsterSpawnBlockLightLimit", definition.at("monster_spawn_block_light_limit").get<i32>()},
            {"attributeCount", attributes.size()},
            {"timelineCount", timelines.size()},
            {"validHeightMultiple", validHeightMultiple},
            {"validMinYMultiple", validMinYMultiple},
            {"validLogicalHeight", validLogicalHeight}}
            .dump();
    }
    return json{{"dimensionType", requested}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(dimension_type, &writeList, &runLine)

} // namespace mc::diff
