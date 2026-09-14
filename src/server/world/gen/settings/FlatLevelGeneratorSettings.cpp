/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software by
 * furnished to do so, subject to the following conditions:
 *
 * THE ABOVE copyright notice and this permission notice shall be included in all
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

#include "FlatLevelGeneratorSettings.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/biome/BiomeIds.hpp"
#include "common/world/biome/BiomeLoader.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <nlohmann/json_fwd.hpp>

namespace mc {

void FlatLevelGeneratorSettings::updateLayers()
{
    m_layers.clear();
    m_fillLayerEntries.clear();

    // 预计算总高度以避免重复分配
    i32 totalHeight = 0;
    for (const auto& layerInfo : m_layersInfo) {
        totalHeight += layerInfo.height();
    }
    m_layers.reserve(static_cast<size_t>(totalHeight));

    i32 currentY = 0;
    for (const auto& layerInfo : m_layersInfo) {
        const BlockState* state = layerInfo.blockState();
        const i32 height = layerInfo.height();

        for (i32 i = 0; i < height; ++i) {
            // 不阻挡运动的方块（如水）替换为 null，由特性系统放置
            // 判断标准: isSolid() || isLiquid() → motion-blocking → 保留
            // 否则（非固体、非液体的非空气方块如草径）→ 设为 null
            if (state != nullptr && !state->isAir() && !state->owner().isSolid(*state) && !state->isLiquid()) {
                // 非运动阻挡方块：由特性系统放置，记录填充层信息
                m_layers.push_back(nullptr);
                m_fillLayerEntries.push_back({currentY, state});
            } else {
                m_layers.push_back(state);
            }
            ++currentY;
        }
    }

    // voidGen 判断：仅当所有展开后的方块都是空气时才为 void
    m_voidGen =
        std::all_of(m_layers.begin(), m_layers.end(), [](const BlockState* s) { return s == nullptr || s->isAir(); });
}

FlatLevelGeneratorSettings FlatLevelGeneratorSettings::createDefault()
{
    FlatLevelGeneratorSettings settings(Biomes::Plains);

    // 默认平坦世界配置：1x Bedrock + 2x Dirt + 1x Grass Block
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::BEDROCK));
    settings.layersInfo().emplace_back(2, VanillaBlocks::getState(VanillaBlocks::DIRT));
    settings.layersInfo().emplace_back(1, VanillaBlocks::getState(VanillaBlocks::GRASS_BLOCK));

    // MC 1.21.11: 默认超平坦世界启用村庄和要塞结构集
    // 参考: FlatLevelGeneratorPresets.getClassicFlat() -> structureOverrides = [VILLAGES, STRONGHOLDS]
    settings.m_structureOverrides = {
        ResourceLocation::parse("minecraft:villages"),
        ResourceLocation::parse("minecraft:strongholds"),
    };

    settings.updateLayers();
    return settings;
}

namespace {

using json = nlohmann::json;

/// 读取必填 bool 字段
Result<bool> readBool(const json& j, std::string_view field)
{
    if (!j.contains(field) || !j[field].is_boolean()) {
        return Error(ErrorCode::InvalidData, "flat preset: missing boolean field '" + std::string(field) + "'");
    }
    return j[field].get<bool>();
}

/// 读取必填整数字段
Result<i32> readInt(const json& j, std::string_view field)
{
    if (!j.contains(field) || !j[field].is_number_integer()) {
        return Error(ErrorCode::InvalidData, "flat preset: missing integer field '" + std::string(field) + "'");
    }
    return j[field].get<i32>();
}

/// structure_overrides 三态解析：string | array<string> | 缺省(空列表)
/// 单字符串 → 一个 RL；数组 → 多个 RL；空数组 → 空列表（不生成结构）。
std::vector<ResourceLocation> parseStructureOverrides(const json& settingsObj)
{
    std::vector<ResourceLocation> overrides;
    if (!settingsObj.contains("structure_overrides")) {
        return overrides;
    }
    const auto& so = settingsObj["structure_overrides"];
    if (so.is_string()) {
        overrides.push_back(ResourceLocation::parse(so.get<std::string>()));
    } else if (so.is_array()) {
        for (const auto& entry : so) {
            if (entry.is_string()) {
                overrides.push_back(ResourceLocation::parse(entry.get<std::string>()));
            }
        }
    }
    return overrides;
}

} // namespace

Result<FlatLevelGeneratorSettings> FlatLevelGeneratorSettings::fromJson(const json& root, const ResourceLocation& id)
{
    if (!root.is_object()) {
        return Error(ErrorCode::InvalidData, "flat preset '" + id.toString() + "' root must be an object");
    }
    // 顶层 { display, settings }；仅解析 settings 子对象（display 为图标，生成期不用）。
    if (!root.contains("settings") || !root["settings"].is_object()) {
        return Error(ErrorCode::InvalidData, "flat preset '" + id.toString() + "' missing 'settings' object");
    }
    return fromSettingsObject(root["settings"], id);
}

Result<FlatLevelGeneratorSettings> FlatLevelGeneratorSettings::fromSettingsObject(
    const json& settingsObj, const ResourceLocation& id)
{
    if (!settingsObj.is_object()) {
        return Error(ErrorCode::InvalidData, "flat settings '" + id.toString() + "' must be an object");
    }

    // biome（RL → BiomeId，经 BiomeLoader 共享映射表）
    if (!settingsObj.contains("biome") || !settingsObj["biome"].is_string()) {
        return Error(ErrorCode::InvalidData, "flat preset '" + id.toString() + "' missing 'biome' string");
    }
    const ResourceLocation biomeRl = ResourceLocation::parse(settingsObj["biome"].get<std::string>());
    auto biomeId = world::biome::BiomeLoader::biomeIdByName(biomeRl);
    if (!biomeId.has_value()) {
        return Error(ErrorCode::InvalidData,
            "flat preset '" + id.toString() + "': biome '" + biomeRl.toString() + "' has no BiomeId mapping");
    }

    FlatLevelGeneratorSettings settings(biomeId.value());

    // features / lakes（bool，缺省 false 对齐原版默认）
    if (settingsObj.contains("features")) {
        auto features = readBool(settingsObj, "features");
        if (features.failed()) return features.error();
        settings.setDecoration(features.value());
    }
    if (settingsObj.contains("lakes")) {
        auto lakes = readBool(settingsObj, "lakes");
        if (lakes.failed()) return lakes.error();
        settings.setLakes(lakes.value());
    }

    // layers（每层 {block:RL, height:int}；block 经 BlockRegistry 取默认 BlockState）
    if (!settingsObj.contains("layers") || !settingsObj["layers"].is_array()) {
        return Error(ErrorCode::InvalidData, "flat preset '" + id.toString() + "' missing 'layers' array");
    }
    for (const auto& layerJson : settingsObj["layers"]) {
        if (!layerJson.is_object() || !layerJson.contains("block") || !layerJson["block"].is_string()) {
            return Error(ErrorCode::InvalidData, "flat preset '" + id.toString() + "': layer missing 'block' string");
        }
        const ResourceLocation blockRl = ResourceLocation::parse(layerJson["block"].get<std::string>());
        // 用 getBlock 区分未知方块（返回 nullptr）与已注册方块；air 显式合法。
        Block* block = BlockRegistry::instance().getBlock(blockRl);
        if (block == nullptr) {
            return Error(ErrorCode::InvalidData,
                "flat preset '" + id.toString() + "': unknown block '" + blockRl.toString() + "'");
        }
        const BlockState* blockState = &block->defaultState();
        auto height = readInt(layerJson, "height");
        if (height.failed()) return height.error();
        settings.layersInfo().emplace_back(height.value(), blockState);
    }

    // structure_overrides（三态）
    settings.setStructureOverrides(parseStructureOverrides(settingsObj));

    settings.updateLayers();
    return settings;
}

} // namespace mc
