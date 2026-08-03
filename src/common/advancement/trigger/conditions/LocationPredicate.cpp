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

#include "LocationPredicate.hpp"
#include "common/advancement/MinMaxBounds.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/dimension/MapDimensionId.hpp"
#include <cmath>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <nlohmann/json_fwd.hpp>

namespace mc::advancement {

namespace {

/**
 * @brief 检查维度名称是否匹配
 *
 * 将维度ID与ResourceLocation格式的维度名称进行比较。
 * 支持 namespaced 格式（minecraft:overworld）、短格式（overworld）、数字格式（0、-1、1）。
 *
 * @param dimensionId 维度ID
 * @param expected 期望的维度ResourceLocation
 * @return 是否匹配
 */
bool matchesDimension(DimensionId dimensionId, const ResourceLocation& expected)
{
    // 使用集中式维度名称工具进行比较
    // 先尝试直接匹配完整字符串（如 "minecraft:overworld"）
    std::string_view canonicalName = dimensionIdToString(dimensionId);
    if (expected.toString() == canonicalName) {
        return true;
    }

    // 尝试匹配短格式（不带命名空间前缀，如 "overworld"）
    const std::string& expectedPath = expected.path();
    auto colonPos = canonicalName.find(':');
    if (colonPos != std::string_view::npos) {
        std::string_view canonicalPath = canonicalName.substr(colonPos + 1);
        if (expectedPath == canonicalPath) {
            return true;
        }
    }

    // 支持数字格式的维度名称（如 "minecraft:0"）
    if (expected.namespace_() == "minecraft") {
        try {
            DimensionId id = static_cast<DimensionId>(std::stoi(expectedPath));
            return dimensionId == id;
        }
        catch (...) {
            return false;
        }
    }

    return false;
}

} // namespace

bool LocationPredicate::test(const IWorld& world, f64 x, f64 y, f64 z) const
{
    if (m_isAny) {
        return true;
    }

    // 检查坐标范围
    if (!m_x.test(x)) return false;
    if (!m_y.test(y)) return false;
    if (!m_z.test(z)) return false;

    // 检查维度
    if (m_dimension.has_value()) {
        if (!matchesDimension(world.dimension(), m_dimension.value())) {
            return false;
        }
    }

    // 检查生物群系
    if (m_biome.has_value()) {
        i32 blockX = math::floorTo<i32>(x);
        i32 blockY = math::floorTo<i32>(y);
        i32 blockZ = math::floorTo<i32>(z);

        // 获取区块坐标
        ChunkCoord chunkX = math::toChunkCoord(blockX);
        ChunkCoord chunkZ = math::toChunkCoord(blockZ);

        // 获取区块
        const ChunkData* chunk = world.getChunk(chunkX, chunkZ);
        if (chunk == nullptr) {
            // 区块未加载，无法判断
            return false;
        }

        // 获取本地坐标
        i32 localX = math::toLocalCoord(blockX);
        i32 localZ = math::toLocalCoord(blockZ);

        // 获取生物群系ID
        BiomeId biomeId = chunk->getBiomeAtBlock(localX, blockY, localZ);

        // 获取生物群系定义
        const Biome& biome = BiomeRegistry::instance().get(biomeId);

        // 比较生物群系名称
        // ResourceLocation 格式：minecraft:plains
        // Biome::name() 返回：plains
        std::string_view expectedPath = m_biome.value().path();
        if (biome.name() != expectedPath) {
            return false;
        }
    }

    return true;
}

bool LocationPredicate::test(const IWorld& world, const BlockPos& pos) const
{
    return test(world, static_cast<f64>(pos.x) + 0.5, static_cast<f64>(pos.y) + 0.5, static_cast<f64>(pos.z) + 0.5);
}

Result<LocationPredicate> LocationPredicate::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return LocationPredicate{};
    }

    std::optional<ResourceLocation> biome;
    std::optional<ResourceLocation> dimension;
    DoubleBounds x, y, z;

    if (json.contains("biome")) {
        biome = ResourceLocation(json["biome"].get<std::string>());
    }

    if (json.contains("dimension")) {
        dimension = ResourceLocation(json["dimension"].get<std::string>());
    }

    if (json.contains("position")) {
        const auto& pos = json["position"];
        if (pos.contains("x")) {
            x = DoubleBounds::fromJson(pos["x"]);
        }
        if (pos.contains("y")) {
            y = DoubleBounds::fromJson(pos["y"]);
        }
        if (pos.contains("z")) {
            z = DoubleBounds::fromJson(pos["z"]);
        }
    }

    // 直接的x/y/z字段
    if (json.contains("x")) {
        x = DoubleBounds::fromJson(json["x"]);
    }
    if (json.contains("y")) {
        y = DoubleBounds::fromJson(json["y"]);
    }
    if (json.contains("z")) {
        z = DoubleBounds::fromJson(json["z"]);
    }

    LocationPredicate predicate;
    predicate.m_biome = std::move(biome);
    predicate.m_dimension = std::move(dimension);
    predicate.m_x = std::move(x);
    predicate.m_y = std::move(y);
    predicate.m_z = std::move(z);
    predicate.m_isAny = !predicate.m_biome.has_value() && !predicate.m_dimension.has_value() &&
        predicate.m_x.isUnbounded() && predicate.m_y.isUnbounded() && predicate.m_z.isUnbounded();
    return predicate;
}

nlohmann::json LocationPredicate::toJson() const
{
    if (m_isAny) {
        return nullptr;
    }

    nlohmann::json json;

    if (m_biome.has_value()) {
        json["biome"] = m_biome.value().toString();
    }
    if (m_dimension.has_value()) {
        json["dimension"] = m_dimension.value().toString();
    }

    if (!m_x.isUnbounded() || !m_y.isUnbounded() || !m_z.isUnbounded()) {
        nlohmann::json pos;
        if (!m_x.isUnbounded()) {
            pos["x"] = m_x.toJson();
        }
        if (!m_y.isUnbounded()) {
            pos["y"] = m_y.toJson();
        }
        if (!m_z.isUnbounded()) {
            pos["z"] = m_z.toJson();
        }
        json["position"] = std::move(pos);
    }

    return json;
}

// ========== DistancePredicate ==========

bool DistancePredicate::test(f64 x1, f64 y1, f64 z1, f64 x2, f64 y2, f64 z2) const
{
    if (m_isAny) {
        return true;
    }

    f64 dx = x2 - x1;
    f64 dy = y2 - y1;
    f64 dz = z2 - z1;
    f64 distanceSq = dx * dx + dy * dy + dz * dz;

    return testSquared(distanceSq);
}

bool DistancePredicate::testSquared(f64 distanceSq) const
{
    if (m_isAny) {
        return true;
    }

    f64 distance = std::sqrt(distanceSq);
    return m_range.test(distance);
}

Result<DistancePredicate> DistancePredicate::fromJson(const nlohmann::json& json)
{
    if (json.is_null()) {
        return DistancePredicate{};
    }

    return DistancePredicate(DoubleBounds::fromJson(json));
}

nlohmann::json DistancePredicate::toJson() const
{
    if (m_isAny) {
        return nullptr;
    }
    return m_range.toJson();
}

} // namespace mc::advancement
