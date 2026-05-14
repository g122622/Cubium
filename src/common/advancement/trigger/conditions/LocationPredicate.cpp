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
#include "common/util/assert/AssertAll.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/block/BlockPos.hpp"

namespace mc::advancement {

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
        // [TODO 阶段3+4：触发器完善] 需要维度系统支持获取维度ID比较
        // if (world.getDimensionId() != m_dimension.value()) return false;
        MC_UNUSED(world);
    }

    // 检查生物群系
    if (m_biome.has_value()) {
        // [TODO 阶段3+4：触发器完善] 需要生物群系系统支持获取生物群系
        // const Biome* biome = world.getBiomeAtBlock(
        //     math::floorTo<i32>(x),
        //     math::floorTo<i32>(y),
        //     math::floorTo<i32>(z)
        // );
        // if (biome == nullptr) return false;
        // if (biome->getId() != m_biome.value()) return false;
    }

    // [TODO 阶段3+4：触发器完善] 检查特征、流体、方块、光源等

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
