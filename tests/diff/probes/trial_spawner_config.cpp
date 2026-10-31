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
 * @brief 差分能力：试炼刷怪笼配置（数据包 `data/<ns>/trial_spawner/`，原版 28 项）。
 *
 * `list`：全部配置 ID（排序）
 * `run` ：{ "config": "minecraft:trial_chamber/breeze/normal", "playerCount": 2 }
 *         → { "config": "...", "spawnRange": 4,
 *             "totalMobs": "2", "simultaneousMobs": "1",
 *             "totalMobsAddedPerPlayer": "1", "simultaneousMobsAddedPerPlayer": "0.5",
 *             "ticksBetweenSpawn": 20,
 *             "targetTotalMobs": 4, "targetSimultaneousMobs": 2, ... }
 *
 * 对应原版 `record TrialSpawnerConfig(int spawnRange, float totalMobs, float simultaneousMobs,
 * float totalMobsAddedPerPlayer, float simultaneousMobsAddedPerPlayer, int ticksBetweenSpawn, ...)`。
 *
 * ## 为什么这四个字段必须是浮点
 *
 * Cubium 当前把 `totalMobs` / `simultaneousMobs` / `*AddedPerPlayer` 全声明成 `i32`
 * （见 `TrialSpawnerBlockEntity::Config`），而原版是 `f32`。数据包里
 * `simultaneous_mobs_added_per_player` 全部 28 项都写的是 **`0.5`** —— 整数字段根本
 * 装不下这个值。
 *
 * 这个偏差会直接改变刷怪数量，因为原版的派生方法是：
 *
 *   calculateTargetTotalMobs(n)        = (int) Math.floor(totalMobs + totalMobsAddedPerPlayer * n)
 *   calculateTargetSimultaneousMobs(n) = (int) Math.floor(simultaneousMobs + simultaneousMobsAddedPerPlayer * n)
 *
 * 乘数取 0.5 时，`1 + 0.5 * 3 = 2.5` → floor 得 2；若按整数加法先算成 `1 + 0 = 1`，
 * 结果就少一只。因此本能力**同时比字段原值与派生结果**，且在多个玩家数下各算一遍。
 *
 * ## 默认值
 *
 * 两个缺省字段都**不是空**，回退错了会静默少东西：
 *
 * - `spawn_range` 在全部 28 项里都没有出现，回退到 `Builder` 的默认值 **4**；
 * - `loot_tables_to_eject` 在 14 项里缺失，回退到 `DEFAULT.lootTablesToEject`，
 *   而那本身是个**含 2 项的 WeightedList**（`spawner_trial_chamber_consumables`
 *   与 `spawner_trial_chamber_key`），不是空列表。
 *
 * 后者是本能力第一次跑出来就抓到的一处真实差异：探针原先按空列表回退，
 * 与 oracle 的 `lootTableCount: 2` 对不上。
 *
 * TODO: Cubium 目前用四个硬编码的 `Config` 结构体（`getBreezeConfig` /
 * `getMeleeConfig` / `getSmallMeleeConfig`），没有从数据包加载，因此本探针直接解析
 * JSON。待 Cubium 改为数据包驱动后，应改为读注册表。
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

constexpr std::string_view TRIAL_SPAWNER_DIRECTORY = "trial_spawner";

/**
 * @brief 把浮点数格式化成「6 位有效数字，去掉尾随零」的字符串。
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

std::optional<ResourceLocation> configIdFromPath(const std::string& resourcePath)
{
    const std::string marker = "/" + std::string(TRIAL_SPAWNER_DIRECTORY) + "/";
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
 * @brief 解析后的试炼刷怪笼配置（与原版 record 同构，数值字段一律 f32）。
 */
struct TrialSpawnerConfigData {
    i32 spawnRange = 4;
    f32 totalMobs = 6.0f;
    f32 simultaneousMobs = 2.0f;
    f32 totalMobsAddedPerPlayer = 2.0f;
    f32 simultaneousMobsAddedPerPlayer = 1.0f;
    i32 ticksBetweenSpawn = 40;
    /// 生成潜力条目数（权重之和与条目数本身都比，避免把结构差异藏起来）
    std::size_t spawnPotentialCount = 0;
    /// 战利品表条目数（缺省时回退为 2，见下方说明）
    std::size_t lootTableCount = 2;
};

/**
 * @brief 从数据包读取全部配置，返回 ID → 解析结果（保持 ID 排序）。
 */
std::vector<std::pair<ResourceLocation, TrialSpawnerConfigData>> loadConfigs()
{
    const char* root = std::getenv("MC_DIFF_VANILLA_DATAPACK");
    if (root == nullptr || *root == '\0') {
        throw std::runtime_error("MC_DIFF_VANILLA_DATAPACK 未设置：需要指向原版数据包根目录（含 data/ 的那一层）");
    }
    const std::filesystem::path packRoot(root);
    auto repository = std::make_unique<resource::DataPackRepository>();
    const auto scanResult = repository->scanDirectory(packRoot.parent_path());
    if (!scanResult.success() || scanResult.value() == 0) {
        throw std::runtime_error("trial_spawner: 数据包扫描失败：" + packRoot.string());
    }

    const auto namespacesResult = repository->getResourceNamespaces();
    if (!namespacesResult.success()) {
        throw std::runtime_error("trial_spawner: 无法列出数据包命名空间：" + namespacesResult.error().toString());
    }

    std::vector<std::pair<ResourceLocation, TrialSpawnerConfigData>> entries;
    for (const auto& ns : namespacesResult.value()) {
        const std::string directory = ns + "/" + std::string(TRIAL_SPAWNER_DIRECTORY);
        const auto stacksResult = repository->listResourceStacks(directory, ".json");
        if (!stacksResult.success()) {
            continue;
        }
        for (const auto& [resourcePath, versions] : stacksResult.value()) {
            if (versions.empty()) {
                continue;
            }
            const auto& content = versions.back();
            const auto id = configIdFromPath(resourcePath);
            if (!id.has_value()) {
                continue;
            }

            json parsed;
            try {
                parsed = json::parse(content.content);
            }
            catch (const std::exception& e) {
                throw std::runtime_error("trial_spawner: 解析失败 " + resourcePath + "：" + std::string(e.what()));
            }

            // 全部字段都是可选的，缺失时回退到与原版 `Builder` 一致的默认值。
            TrialSpawnerConfigData data;
            if (parsed.contains("spawn_range")) {
                data.spawnRange = parsed["spawn_range"].get<i32>();
            }
            if (parsed.contains("total_mobs")) {
                data.totalMobs = parsed["total_mobs"].get<f32>();
            }
            if (parsed.contains("simultaneous_mobs")) {
                data.simultaneousMobs = parsed["simultaneous_mobs"].get<f32>();
            }
            if (parsed.contains("total_mobs_added_per_player")) {
                data.totalMobsAddedPerPlayer = parsed["total_mobs_added_per_player"].get<f32>();
            }
            if (parsed.contains("simultaneous_mobs_added_per_player")) {
                data.simultaneousMobsAddedPerPlayer = parsed["simultaneous_mobs_added_per_player"].get<f32>();
            }
            if (parsed.contains("ticks_between_spawn")) {
                data.ticksBetweenSpawn = parsed["ticks_between_spawn"].get<i32>();
            }
            if (parsed.contains("spawn_potentials") && parsed["spawn_potentials"].is_array()) {
                data.spawnPotentialCount = parsed["spawn_potentials"].size();
            }
            if (parsed.contains("loot_tables_to_eject") && parsed["loot_tables_to_eject"].is_array()) {
                data.lootTableCount = parsed["loot_tables_to_eject"].size();
            }

            bool replaced = false;
            for (auto& [existingId, existingData] : entries) {
                if (existingId.toString() == id->toString()) {
                    existingData = data;
                    replaced = true;
                    break;
                }
            }
            if (!replaced) {
                entries.emplace_back(*id, data);
            }
        }
    }

    std::sort(entries.begin(), entries.end(), [](const auto& lhs, const auto& rhs) {
        return lhs.first.toString() < rhs.first.toString();
    });
    return entries;
}

void writeList(FILE* out)
{
    for (const auto& [id, data] : loadConfigs()) {
        std::fprintf(out, "%s\n", id.toString().c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("config").get<std::string>();
    const i32 playerCount = input.at("playerCount").get<i32>();

    for (const auto& [id, data] : loadConfigs()) {
        if (id.toString() != requested) {
            continue;
        }

        // 原版：
        //   (int) Math.floor(totalMobs + totalMobsAddedPerPlayer * playerCount)
        //   (int) Math.floor(simultaneousMobs + simultaneousMobsAddedPerPlayer * playerCount)
        // 全程用 f32 计算，再 floor 到整数。
        const f32 totalSum = data.totalMobs + data.totalMobsAddedPerPlayer * static_cast<f32>(playerCount);
        const f32 simultaneousSum =
            data.simultaneousMobs + data.simultaneousMobsAddedPerPlayer * static_cast<f32>(playerCount);

        return json{{"config", requested},
            {"playerCount", playerCount},
            {"spawnRange", data.spawnRange},
            {"totalMobs", formatFloat(data.totalMobs)},
            {"simultaneousMobs", formatFloat(data.simultaneousMobs)},
            {"totalMobsAddedPerPlayer", formatFloat(data.totalMobsAddedPerPlayer)},
            {"simultaneousMobsAddedPerPlayer", formatFloat(data.simultaneousMobsAddedPerPlayer)},
            {"ticksBetweenSpawn", data.ticksBetweenSpawn},
            {"spawnPotentialCount", data.spawnPotentialCount},
            {"lootTableCount", data.lootTableCount},
            {"targetTotalMobs", static_cast<i32>(std::floor(totalSum))},
            {"targetSimultaneousMobs", static_cast<i32>(std::floor(simultaneousSum))}}
            .dump();
    }
    return json{{"config", requested}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("trial_spawner_config", &writeList, &runLine)

} // namespace mc::diff
