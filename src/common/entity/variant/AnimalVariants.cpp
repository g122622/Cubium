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

#include "AnimalVariants.hpp"
#include "common/resource/repository/DataPackRepository.hpp"
#include <optional>
#include <utility>
#include <nlohmann/json.hpp>

namespace mc {

namespace {

using nlohmann::json;

std::unordered_map<std::string, AnimalVariants::Table>& _getAllTables()
{
    static std::unordered_map<std::string, AnimalVariants::Table> tables;
    return tables;
}

std::optional<ResourceLocation> _variantIdFromPath(const std::string& resourcePath)
{
    auto slash = resourcePath.rfind('/');
    const std::string fileName = slash == std::string::npos ? resourcePath : resourcePath.substr(slash + 1);
    if (fileName.size() <= 5 || fileName.substr(fileName.size() - 5) != ".json") {
        return std::nullopt;
    }
    return ResourceLocation(fileName.substr(0, fileName.size() - 5));
}

/**
 * @brief 解析一个 `HolderSet`（标签引用或内联元素列表）
 *
 * 数据文件里两种写法：
 * - 字符串 `"#minecraft:spawns_cold_variant_frogs"` —— 标签引用
 * - 字符串 `"minecraft:plains"` —— 内联单元素
 * - 数组 `["minecraft:plains", "minecraft:desert"]` —— 内联多元素
 *
 * 标签引用保留 `#` 前缀，理由是原版 `HolderSet` 的标签与内联语义不等价。
 */
std::vector<std::string> _parseHolderSet(const json& value)
{
    std::vector<std::string> entries;
    if (value.is_string()) {
        entries.push_back(value.get<std::string>());
        return entries;
    }
    if (value.is_array()) {
        for (const auto& item : value) {
            if (item.is_string()) {
                entries.push_back(item.get<std::string>());
            }
        }
    }
    return entries;
}

/**
 * @brief 解析一条出生条件
 */
SpawnCondition _parseCondition(const json& value)
{
    SpawnCondition condition;
    // 数据文件里 `type` 带命名空间前缀（`minecraft:structure`），而原版注册的是
    // 不带前缀的 `structure` / `moon_brightness` / `biome`。这里统一剥掉前缀再比较，
    // 否则下面三个分支全都不会命中（表现为 entries 为空、range 丢失）。
    condition.type = ResourceLocation(value.value("type", std::string{})).path();

    if (condition.type == "biome") {
        const auto it = value.find("biomes");
        if (it != value.end()) {
            condition.entries = _parseHolderSet(*it);
        }
    } else if (condition.type == "structure") {
        const auto it = value.find("structures");
        if (it != value.end()) {
            condition.entries = _parseHolderSet(*it);
        }
    } else if (condition.type == "moon_brightness") {
        // 原版是 `MinMaxBounds.Doubles`，min / max 都是可选字段。
        const auto rangeIt = value.find("range");
        if (rangeIt != value.end() && rangeIt->is_object()) {
            const auto minIt = rangeIt->find("min");
            if (minIt != rangeIt->end() && minIt->is_number()) {
                condition.rangeMin = minIt->get<double>();
            }
            const auto maxIt = rangeIt->find("max");
            if (maxIt != rangeIt->end() && maxIt->is_number()) {
                condition.rangeMax = maxIt->get<double>();
            }
        }
    }
    return condition;
}

/**
 * @brief 解析贴图字段
 *
 * 两种数据形状：
 * - `"asset_id": "minecraft:entity/cat/all_black"` —— 单贴图（猫 / 蛙 / 牛 / 鸡 / 猪）
 * - `"assets": { "wild": ..., "tame": ..., "angry": ... }` —— 多贴图（狼）
 */
std::vector<AnimalVariant::AssetField> _parseAssets(const json& value)
{
    std::vector<AnimalVariant::AssetField> assets;

    const auto assetIdIt = value.find("asset_id");
    if (assetIdIt != value.end() && assetIdIt->is_string()) {
        assets.push_back({std::string{}, ResourceLocation(assetIdIt->get<std::string>())});
        return assets;
    }

    const auto assetsIt = value.find("assets");
    if (assetsIt != value.end() && assetsIt->is_object()) {
        // 原版 `WolfVariant.AssetInfo` 的字段顺序是 wild / tame / angry，这里按该顺序
        // 输出而不是按 JSON 对象遍历顺序，两侧才可比。
        for (const char* name : {"wild", "tame", "angry"}) {
            const auto it = assetsIt->find(name);
            if (it != assetsIt->end() && it->is_string()) {
                assets.push_back({std::string(name), ResourceLocation(it->get<std::string>())});
            }
        }
    }
    return assets;
}

} // namespace

AnimalVariants::Table& AnimalVariants::table(const std::string& kind)
{
    return _getAllTables()[kind];
}

void AnimalVariants::clear(const std::string& kind)
{
    _getAllTables()[kind].clear();
}

Result<std::size_t> AnimalVariants::loadFromDataPackRepository(
    const resource::DataPackRepository& dataPacks, const std::string& kind)
{
    auto namespacesResult = dataPacks.getResourceNamespaces();
    if (!namespacesResult.success()) {
        return Error(
            ErrorCode::FileReadFailed, kind + ": 无法列出数据包命名空间：" + namespacesResult.error().toString());
    }

    auto& variants = _getAllTables()[kind];
    variants.clear();
    std::size_t loaded = 0;

    for (const auto& ns : namespacesResult.value()) {
        const std::string directory = ns + "/" + kind;
        auto stacksResult = dataPacks.listResourceStacks(directory, ".json");
        if (!stacksResult.success()) {
            continue;
        }

        for (const auto& [resourcePath, versions] : stacksResult.value()) {
            if (versions.empty()) {
                continue;
            }
            const auto& content = versions.back();

            json value;
            try {
                value = json::parse(content.content);
            }
            catch (const std::exception& e) {
                return Error(
                    ErrorCode::InvalidData, kind + ": 解析失败 " + resourcePath + "：" + std::string(e.what()));
            }

            const auto id = _variantIdFromPath(resourcePath);
            if (!id.has_value()) {
                continue;
            }
            if (!value.is_object()) {
                return Error(ErrorCode::InvalidData, kind + ": 顶层不是对象 " + resourcePath);
            }

            auto assets = _parseAssets(value);
            if (assets.empty()) {
                return Error(ErrorCode::InvalidData, kind + ": 缺少贴图字段 " + resourcePath);
            }

            // 原版 `ModelAndTexture` 的 `model` 是 `optionalFieldOf("model", ModelType.NORMAL)`：
            // 字段缺省时取默认值 `normal`，而不是空串。数据文件里 temperate 变体就没写这个字段。
            //
            // 只有牛 / 鸡 / 猪三类用 `ModelAndTexture`（有 model 概念）；狼 / 猫 / 蛙的原版记录
            // 里根本没有 model 字段，它们的 model 必须保持空串。
            const bool hasModelConcept = kind == "cow_variant" || kind == "chicken_variant" || kind == "pig_variant";
            const auto model = value.value("model", hasModelConcept ? std::string("normal") : std::string{});

            std::vector<SpawnPrioritySelector> selectors;
            const auto selectorsIt = value.find("spawn_conditions");
            if (selectorsIt != value.end() && selectorsIt->is_array()) {
                for (const auto& item : *selectorsIt) {
                    if (!item.is_object()) {
                        continue;
                    }
                    SpawnPrioritySelector selector;
                    selector.priority = item.value("priority", 0);
                    const auto conditionIt = item.find("condition");
                    // 原版 `Selector` 的 condition 是 Optional：缺省表示无条件命中。
                    if (conditionIt != item.end() && conditionIt->is_object()) {
                        selector.condition = _parseCondition(*conditionIt);
                    }
                    selectors.push_back(std::move(selector));
                }
            }

            variants[*id] = std::make_unique<AnimalVariant>(std::move(assets), model, std::move(selectors));
            ++loaded;
        }
    }

    return loaded;
}

} // namespace mc
