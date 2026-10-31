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
 * @brief 差分能力：附魔提供器（数据包 `data/<ns>/enchantment_provider/`，原版 7 项）。
 *
 * `list`：全部提供器 ID（排序）
 * `run` ：{ "provider": "minecraft:mob_spawn_equipment", "specialMultiplier": "0.75" }
 *         → { "provider": "minecraft:mob_spawn_equipment",
 *             "type": "minecraft:by_cost_with_difficulty",
 *             "enchantments": "minecraft:on_mob_spawn_equipment",
 *             "minCost": 5, "maxCostSpan": 17,
 *             "costMin": 5, "costMax": 17 }
 *
 * 对应原版 `interface EnchantmentProvider` 与它的三个实现：
 * - `single`                     → `record SingleEnchantment(Holder<Enchantment>, IntProvider level)`
 * - `by_cost`                    → `record EnchantmentsByCost(HolderSet<Enchantment>, IntProvider cost)`
 * - `by_cost_with_difficulty`    → `record EnchantmentsByCostWithDifficulty(HolderSet, int minCost, int maxCostSpan)`
 *
 * ## 为什么只比到「代价区间」为止
 *
 * 原版 `enchant()` 的后半段是 `EnchantmentHelper.selectEnchantment(rng, stack, cost, candidates)`，
 * 那部分属于附魔选择算法，已由 `enchantment_applicability` 与 `enchantment_cost` 两个能力
 * 从别的角度覆盖。本能力聚焦提供器**自身**的那层逻辑，也就是：
 *
 * - JSON 的 `type` 分派到哪个实现类；
 * - `by_cost_with_difficulty` 的代价上界 `minCost + (int)(specialMultiplier * maxCostSpan)`。
 *
 * 最后一项尤其值得单独比：`(int)` 在原版是**截断而非四舍五入**，且 `MAX_ALLOWED_VALUE_PART = 10000`
 * 给 `max_cost_span` 设了上界。这两点写错都会让生成的代价区间整体偏移。
 *
 * TODO: Cubium 目前完全没有实现 `enchantment_provider`（数据包 7 项一项未加载）。
 * 因此本探针直接解析数据包 JSON，不依赖任何 Cubium 注册表——比对的是「数据包内容 +
 * 代价区间算法」，能锁住基线，但覆盖不到「注册表装载」。待 Cubium 实现后应改为读注册表。
 */
#include <algorithm>
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

constexpr std::string_view ENCHANTMENT_PROVIDER_DIRECTORY = "enchantment_provider";

/**
 * @brief 把浮点数格式化成「6 位有效数字，去掉尾随零」的字符串。
 *
 * 两侧必须用同一规则：C++ 的 `%.6g` 与 Java 的 `%g` 行为不同，先统一成 6 位有效数字
 * 再由双方各自剥掉尾随零。
 */
std::string formatFloat(float value)
{
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%.6g", static_cast<double>(value));
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

std::optional<ResourceLocation> providerIdFromPath(const std::string& resourcePath)
{
    const std::string marker = "/" + std::string(ENCHANTMENT_PROVIDER_DIRECTORY) + "/";
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

/**
 * @brief 从数据包读取全部提供器定义，返回 ID → 原始 JSON。
 *
 * 每次调用都重新扫描：差分工具会重复调用，缓存反而会让两侧看到不同数据。
 */
std::vector<std::pair<ResourceLocation, json>> loadProviders()
{
    const char* root = std::getenv("MC_DIFF_VANILLA_DATAPACK");
    if (root == nullptr || *root == '\0') {
        throw std::runtime_error("MC_DIFF_VANILLA_DATAPACK 未设置：需要指向原版数据包根目录（含 data/ 的那一层）");
    }
    const std::filesystem::path packRoot(root);
    auto repository = std::make_unique<resource::DataPackRepository>();
    const auto scanResult = repository->scanDirectory(packRoot.parent_path());
    if (!scanResult.success() || scanResult.value() == 0) {
        throw std::runtime_error("enchantment_provider: 数据包扫描失败：" + packRoot.string());
    }

    const auto namespacesResult = repository->getResourceNamespaces();
    if (!namespacesResult.success()) {
        throw std::runtime_error(
            "enchantment_provider: 无法列出数据包命名空间：" + namespacesResult.error().toString());
    }

    std::vector<std::pair<ResourceLocation, json>> entries;
    for (const auto& ns : namespacesResult.value()) {
        const std::string directory = ns + "/" + std::string(ENCHANTMENT_PROVIDER_DIRECTORY);
        const auto stacksResult = repository->listResourceStacks(directory, ".json");
        if (!stacksResult.success()) {
            continue;
        }
        for (const auto& [resourcePath, versions] : stacksResult.value()) {
            if (versions.empty()) {
                continue;
            }
            // 优先级最高（最后一个）的版本生效。
            const auto& content = versions.back();
            const auto id = providerIdFromPath(resourcePath);
            if (!id.has_value()) {
                continue;
            }
            json parsed;
            try {
                parsed = json::parse(content.content);
            }
            catch (const std::exception& e) {
                throw std::runtime_error(
                    "enchantment_provider: 解析失败 " + resourcePath + "：" + std::string(e.what()));
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
 * @brief 把 `enchantments` 字段（标签引用或单个 ID）转成输出用的字符串。
 *
 * 原版这里是 `HolderSet<Enchantment>`，可以是 `"#minecraft:xxx"`（标签）或
 * `"minecraft:xxx"`（单项）或数组。直接原样输出，让两侧对同一份输入做同一件事。
 */
std::string enchantmentsToString(const json& value)
{
    if (value.is_string()) {
        return value.get<std::string>();
    }
    if (value.is_array()) {
        std::vector<std::string> parts;
        parts.reserve(value.size());
        for (const auto& item : value) {
            parts.push_back(item.is_string() ? item.get<std::string>() : item.dump());
        }
        std::sort(parts.begin(), parts.end());
        std::string joined;
        for (const auto& part : parts) {
            if (!joined.empty()) {
                joined += ",";
            }
            joined += part;
        }
        return joined;
    }
    return value.dump();
}

void writeList(FILE* out)
{
    for (const auto& [id, definition] : loadProviders()) {
        std::fprintf(out, "%s\n", id.toString().c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("provider").get<std::string>();
    // 难度乘数由调用方给定，两侧用同一个值，避免依赖各自的难度计算链路。
    // 输入按字符串传递，再解析成 f32：0.25 / 0.75 这类值在 JSON 数字里有多种写法，
    // 统一走字符串可以保证两侧解析出**同一个** f32 位模式。
    const auto& multiplierJson = input.at("specialMultiplier");
    const std::string multiplierText =
        multiplierJson.is_string() ? multiplierJson.get<std::string>() : multiplierJson.dump();
    const f32 specialMultiplier = static_cast<f32>(std::stod(multiplierText));

    for (const auto& [id, definition] : loadProviders()) {
        if (id.toString() != requested) {
            continue;
        }

        json result{{"provider", requested}};
        const std::string type = definition.value("type", "");
        result["type"] = type;

        if (type == "minecraft:single") {
            // record SingleEnchantment(Holder<Enchantment> enchantment, IntProvider level)
            result["enchantment"] = definition.value("enchantment", "");
            const auto& level = definition.at("level");
            result["level"] = level.is_number_integer() ? std::to_string(level.get<i64>()) : level.dump();
        } else if (type == "minecraft:by_cost") {
            // record EnchantmentsByCost(HolderSet<Enchantment> enchantments, IntProvider cost)
            result["enchantments"] = enchantmentsToString(definition.at("enchantments"));
            const auto& cost = definition.at("cost");
            result["cost"] = cost.is_number_integer() ? std::to_string(cost.get<i64>()) : cost.dump();
        } else if (type == "minecraft:by_cost_with_difficulty") {
            // record EnchantmentsByCostWithDifficulty(HolderSet, int minCost, int maxCostSpan)
            result["enchantments"] = enchantmentsToString(definition.at("enchantments"));
            const i32 minCost = definition.at("min_cost").get<i32>();
            const i32 maxCostSpan = definition.at("max_cost_span").get<i32>();
            result["minCost"] = minCost;
            result["maxCostSpan"] = maxCostSpan;
            // 原版：Mth.randomBetweenInclusive(rng, minCost, minCost + (int)(f * maxCostSpan))
            // 注意 `(int)` 是**截断**，且下界不随难度变化。
            const i32 upper = minCost + static_cast<i32>(specialMultiplier * static_cast<f32>(maxCostSpan));
            result["costMin"] = minCost;
            result["costMax"] = upper;
        } else {
            result["unknownType"] = true;
        }

        result["specialMultiplier"] = formatFloat(specialMultiplier);
        return result.dump();
    }
    return json{{"provider", requested}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("enchantment_provider", &writeList, &runLine)

} // namespace mc::diff
