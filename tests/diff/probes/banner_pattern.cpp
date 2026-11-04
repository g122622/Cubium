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
 * @brief 差分能力：旗帜图案（数据包 `data/<ns>/banner_pattern/` 下的 JSON，原版 43 项）。
 *
 * `list`：全部旗帜图案 ID（排序）
 * `run` ：{ "pattern": "minecraft:base" }
 *         → { "pattern": "minecraft:base",
 *             "assetId": "minecraft:base",
 *             "translationKey": "block.minecraft.banner.base" }
 *
 * 对应原版 `record BannerPattern(Identifier assetId, String translationKey)`。
 *
 * 本能力刻意**绕开 Cubium 的注册表**：Cubium 目前把旗帜图案写死成
 * `blockentity::BannerPatternType` 枚举（43 个固定值），而原版 1.21.11 已经把它改成
 * 数据包驱动的 `Registries.BANNER_PATTERN`。若探针去读那个枚举，比对就退化成
 * 「枚举与自己比」，无法暴露「数据包改一个 asset_id 后 Cubium 不跟随」这类真正的偏差。
 *
 * 因此这里直接从数据包读 JSON，与原版 oracle 走的是同一份数据源：
 * 数据包内容一旦与原版不同，本能力立刻报出来。
 *
 * TODO: 待 Cubium 把 `BannerPatternType` 迁移成数据包驱动的注册表后，本探针应改为
 * 读取注册表而不是直接解析 JSON，届时才能同时覆盖「注册表装载顺序」这一类偏差。
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

constexpr std::string_view BANNER_PATTERN_DIRECTORY = "banner_pattern";

/**
 * @brief 从资源路径取出图案 ID。
 *
 * 资源路径形如 `<ns>/banner_pattern/base`，映射成 `minecraft:base`。
 */
std::optional<ResourceLocation> patternIdFromPath(const std::string& resourcePath)
{
    const std::string marker = "/" + std::string(BANNER_PATTERN_DIRECTORY) + "/";
    const auto markerPos = resourcePath.find(marker);
    if (markerPos == std::string::npos) {
        return std::nullopt;
    }
    const std::string ns = resourcePath.substr(0, markerPos);
    std::string path = resourcePath.substr(markerPos + marker.size());
    // 去掉扩展名（`base.json` → `base`）。
    const auto dotPos = path.rfind('.');
    if (dotPos != std::string::npos) {
        path = path.substr(0, dotPos);
    }
    return ResourceLocation(ns + ":" + path);
}

/**
 * @brief 一个旗帜图案的定义（与原版 `record BannerPattern` 同构）。
 */
struct BannerPatternEntry {
    std::string assetId;
    std::string translationKey;
};

/**
 * @brief 从数据包读取全部旗帜图案。
 *
 * 每次调用都重新扫描：数据包内容可能在同一进程里被换了（差分工具会重复调用），
 * 缓存反而会让两侧看到不同的数据。
 */
std::vector<std::pair<ResourceLocation, BannerPatternEntry>> loadBannerPatterns()
{
    const char* root = std::getenv("MC_DIFF_VANILLA_DATAPACK");
    if (root == nullptr || *root == '\0') {
        throw std::runtime_error("MC_DIFF_VANILLA_DATAPACK 未设置：需要指向原版数据包根目录（含 data/ 的那一层）");
    }
    const std::filesystem::path packRoot(root);
    auto repository = std::make_unique<resource::DataPackRepository>();
    const auto scanResult = repository->scanDirectory(packRoot.parent_path());
    if (!scanResult.success() || scanResult.value() == 0) {
        throw std::runtime_error("banner_pattern: 数据包扫描失败：" + packRoot.string());
    }

    const auto namespacesResult = repository->getResourceNamespaces();
    if (!namespacesResult.success()) {
        throw std::runtime_error("banner_pattern: 无法列出数据包命名空间：" + namespacesResult.error().toString());
    }

    // 以 ID 为键做覆盖：多数据包叠加时后者覆盖前者，与原版加载语义一致。
    std::vector<std::pair<ResourceLocation, BannerPatternEntry>> entries;
    auto findEntry = [&entries](const ResourceLocation& id) -> BannerPatternEntry* {
        for (auto& [existingId, entry] : entries) {
            if (existingId.toString() == id.toString()) {
                return &entry;
            }
        }
        return nullptr;
    };

    for (const auto& ns : namespacesResult.value()) {
        const std::string directory = ns + "/" + std::string(BANNER_PATTERN_DIRECTORY);
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

            const auto id = patternIdFromPath(resourcePath);
            if (!id.has_value()) {
                continue;
            }

            json parsed;
            try {
                parsed = json::parse(content.content);
            }
            catch (const std::exception& e) {
                throw std::runtime_error("banner_pattern: 解析失败 " + resourcePath + "：" + std::string(e.what()));
            }

            BannerPatternEntry entry;
            // 原版 `asset_id` 是 Identifier，缺省时整套加载会失败；这里要求必须存在。
            if (!parsed.contains("asset_id") || !parsed["asset_id"].is_string()) {
                throw std::runtime_error("banner_pattern: " + resourcePath + " 缺少字符串字段 asset_id");
            }
            entry.assetId = parsed["asset_id"].get<std::string>();
            // `translation_key` 同样是必填字段。
            if (!parsed.contains("translation_key") || !parsed["translation_key"].is_string()) {
                throw std::runtime_error("banner_pattern: " + resourcePath + " 缺少字符串字段 translation_key");
            }
            entry.translationKey = parsed["translation_key"].get<std::string>();

            if (auto* existing = findEntry(*id); existing != nullptr) {
                *existing = entry;
            } else {
                entries.emplace_back(*id, entry);
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
    for (const auto& [id, entry] : loadBannerPatterns()) {
        std::fprintf(out, "%s\n", id.toString().c_str());
    }
}

std::string runLine(std::string_view inputLine)
{
    const json input = json::parse(inputLine);
    const std::string requested = input.at("pattern").get<std::string>();

    for (const auto& [id, entry] : loadBannerPatterns()) {
        if (id.toString() == requested) {
            return json{{"pattern", requested}, {"assetId", entry.assetId}, {"translationKey", entry.translationKey}}
                .dump();
        }
    }
    return json{{"pattern", requested}, {"missing", true}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(banner_pattern, &writeList, &runLine)

} // namespace mc::diff
