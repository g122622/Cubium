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
 */

#include "client/resource/atlas/AtlasSourceParser.hpp"

#include "client/resource/atlas/AtlasSource.hpp"
#include "client/resource/atlas/IdentifierPattern.hpp"
#include "client/resource/atlas/Sources.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <exception>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <nlohmann/json_fwd.hpp>
#include <spdlog/spdlog.h>

namespace mc::client::resource::atlas {

namespace {

// atlas source type 标识符（对齐原版 SpriteSources.bootstrap 注册表）
constexpr std::string_view TYPE_SINGLE = "minecraft:single";
constexpr std::string_view TYPE_DIRECTORY = "minecraft:directory";
constexpr std::string_view TYPE_FILTER = "minecraft:filter";
constexpr std::string_view TYPE_UNSTITCH = "minecraft:unstitch";
constexpr std::string_view TYPE_PALETTED = "minecraft:paletted_permutations";

[[nodiscard]] std::string getTypeString(const nlohmann::json& j)
{
    if (!j.contains("type") || !j["type"].is_string()) {
        return {};
    }
    return j["type"].get<std::string>();
}

[[nodiscard]] Result<ResourceLocation> parseIdentifier(const nlohmann::json& j, std::string_view field)
{
    if (!j.contains(field)) {
        return Error(
            ErrorCode::ResourceParseError, std::string("atlas source missing required field: ") + std::string(field));
    }
    const auto& v = j[field];
    if (!v.is_string()) {
        return Error(
            ErrorCode::ResourceParseError, std::string("atlas source field must be string: ") + std::string(field));
    }
    return ResourceLocation::parse(v.get<std::string>());
}

[[nodiscard]] Result<std::unique_ptr<AtlasSource>> parseSingle(const nlohmann::json& j)
{
    auto resource = parseIdentifier(j, "resource");
    if (resource.failed()) {
        return resource.error();
    }
    // sprite 可选，缺省 = resource
    ResourceLocation sprite = resource.value();
    if (j.contains("sprite")) {
        const auto& s = j["sprite"];
        if (s.is_string()) {
            sprite = ResourceLocation::parse(s.get<std::string>());
        }
    }
    return std::unique_ptr<AtlasSource>(std::make_unique<SingleFileSource>(resource.value(), std::move(sprite)));
}

[[nodiscard]] Result<std::unique_ptr<AtlasSource>> parseDirectory(const nlohmann::json& j)
{
    if (!j.contains("source") || !j["source"].is_string()) {
        return Error(ErrorCode::ResourceParseError, "directory source missing required string field: source");
    }
    if (!j.contains("prefix") || !j["prefix"].is_string()) {
        return Error(ErrorCode::ResourceParseError, "directory source missing required string field: prefix");
    }
    return std::unique_ptr<AtlasSource>(
        std::make_unique<DirectoryListerSource>(j["source"].get<std::string>(), j["prefix"].get<std::string>()));
}

[[nodiscard]] Result<std::unique_ptr<AtlasSource>> parseFilter(const nlohmann::json& j)
{
    if (!j.contains("pattern") || !j["pattern"].is_object()) {
        return Error(ErrorCode::ResourceParseError, "filter source missing required object field: pattern");
    }
    auto pattern = IdentifierPattern::parse(j["pattern"]);
    if (pattern.failed()) {
        return pattern.error();
    }
    return std::unique_ptr<AtlasSource>(std::make_unique<FilterSource>(pattern.value()));
}

[[nodiscard]] Result<std::unique_ptr<AtlasSource>> parseUnstitch(const nlohmann::json& j)
{
    auto resource = parseIdentifier(j, "resource");
    if (resource.failed()) {
        return resource.error();
    }
    if (!j.contains("regions") || !j["regions"].is_array() || j["regions"].empty()) {
        return Error(ErrorCode::ResourceParseError, "unstitch source requires non-empty regions array");
    }

    std::vector<UnstitchRegion> regions;
    regions.reserve(j["regions"].size());
    for (const auto& rj : j["regions"]) {
        auto sprite = parseIdentifier(rj, "sprite");
        if (sprite.failed()) {
            return sprite.error();
        }
        if (!rj.contains("x") || !rj.contains("y") || !rj.contains("width") || !rj.contains("height")) {
            return Error(ErrorCode::ResourceParseError, "unstitch region missing x/y/width/height");
        }
        UnstitchRegion region;
        region.sprite = sprite.value();
        region.x = rj["x"].get<double>();
        region.y = rj["y"].get<double>();
        region.width = rj["width"].get<double>();
        region.height = rj["height"].get<double>();
        regions.push_back(std::move(region));
    }

    double divisorX = 1.0;
    double divisorY = 1.0;
    if (j.contains("divisor_x")) {
        divisorX = j["divisor_x"].get<double>();
    }
    if (j.contains("divisor_y")) {
        divisorY = j["divisor_y"].get<double>();
    }
    return std::unique_ptr<AtlasSource>(
        std::make_unique<UnstitcherSource>(resource.value(), std::move(regions), divisorX, divisorY));
}

[[nodiscard]] Result<std::unique_ptr<AtlasSource>> parsePaletted(const nlohmann::json& j)
{
    auto paletteKey = parseIdentifier(j, "palette_key");
    if (paletteKey.failed()) {
        return paletteKey.error();
    }
    if (!j.contains("textures") || !j["textures"].is_array()) {
        return Error(ErrorCode::ResourceParseError, "paletted_permutations source missing textures array");
    }
    if (!j.contains("permutations") || !j["permutations"].is_object()) {
        return Error(ErrorCode::ResourceParseError, "paletted_permutations source missing permutations object");
    }

    std::vector<ResourceLocation> textures;
    textures.reserve(j["textures"].size());
    for (const auto& tj : j["textures"]) {
        if (!tj.is_string()) {
            return Error(ErrorCode::ResourceParseError, "paletted_permutations textures must be strings");
        }
        textures.push_back(ResourceLocation::parse(tj.get<std::string>()));
    }

    std::map<std::string, ResourceLocation> permutations;
    for (auto it = j["permutations"].begin(); it != j["permutations"].end(); ++it) {
        if (!it.value().is_string()) {
            return Error(ErrorCode::ResourceParseError, "paletted_permutations permutation values must be strings");
        }
        permutations[it.key()] = ResourceLocation::parse(it.value().get<std::string>());
    }

    std::string separator = "_";
    if (j.contains("separator") && j["separator"].is_string()) {
        separator = j["separator"].get<std::string>();
    }

    return std::unique_ptr<AtlasSource>(std::make_unique<PalettedPermutationsSource>(
        std::move(textures), paletteKey.value(), std::move(permutations), std::move(separator)));
}

} // namespace

Result<std::unique_ptr<AtlasSource>> AtlasSourceParser::parseSource(const nlohmann::json& j)
{
    if (!j.is_object()) {
        return Error(ErrorCode::ResourceParseError, "atlas source must be a JSON object");
    }
    const std::string type = getTypeString(j);
    if (type.empty()) {
        return Error(ErrorCode::ResourceParseError, "atlas source missing required string field: type");
    }

    if (type == TYPE_SINGLE) {
        return parseSingle(j);
    }
    if (type == TYPE_DIRECTORY) {
        return parseDirectory(j);
    }
    if (type == TYPE_FILTER) {
        return parseFilter(j);
    }
    if (type == TYPE_UNSTITCH) {
        return parseUnstitch(j);
    }
    if (type == TYPE_PALETTED) {
        return parsePaletted(j);
    }
    return Error(ErrorCode::ResourceParseError, "Unknown atlas source type: " + type);
}

Result<std::vector<std::unique_ptr<AtlasSource>>> AtlasSourceParser::parseAtlasJson(const nlohmann::json& j)
{
    if (!j.is_object() || !j.contains("sources") || !j["sources"].is_array()) {
        return Error(ErrorCode::ResourceParseError, "atlas JSON must be an object with \"sources\" array");
    }

    std::vector<std::unique_ptr<AtlasSource>> sources;
    sources.reserve(j["sources"].size());
    for (const auto& sj : j["sources"]) {
        auto result = parseSource(sj);
        if (result.failed()) {
            // 单个 source 解析失败只记日志不中断（对齐原版 SpriteSourceList.load 容错）
            spdlog::warn("atlas source parse failed, skipping: {}", result.error().message());
            continue;
        }
        sources.push_back(std::move(result).value());
    }
    return sources;
}

Result<std::vector<std::unique_ptr<AtlasSource>>> AtlasSourceParser::parseAtlasText(std::string_view jsonText)
{
    try {
        auto json = nlohmann::json::parse(jsonText);
        return parseAtlasJson(json);
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::ResourceParseError, std::string("Failed to parse atlas JSON: ") + e.what());
    }
}

} // namespace mc::client::resource::atlas
