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

#include "DimensionType.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/WorldConstants.hpp"
#include <optional>
#include <spdlog/spdlog.h>

namespace mc {

// ============================================================================
// 主世界
// ============================================================================

DimensionType DimensionType::overworld()
{
    DimensionType type(0, "minecraft:overworld");

    // 环境属性
    type.m_hasCeiling = false;
    type.m_hasSkyLight = true;
    type.m_ultraWarm = false;
    type.m_natural = true;
    type.m_hasEnderDragonFight = false;

    // 功能属性
    type.m_bedWorks = true;
    type.m_respawnAnchorWorks = false;
    type.m_hasRaids = true;
    type.m_piglinSafe = false;

    // 高度
    type.m_minHeight = world::MIN_BUILD_HEIGHT;
    type.m_maxHeight = world::MAX_BUILD_HEIGHT;
    type.m_logicalHeight = world::MAX_BUILD_HEIGHT;

    // 坐标转换
    type.m_coordinateScale = 1.0f;

    // 光照和时间
    type.m_ambientLight = 0.0f;
    type.m_fixedTime = std::nullopt; // 时间自然流逝

    // 方块标签
    type.m_infiniburn = "minecraft:infiniburn_overworld";

    return type;
}

// ============================================================================
// 下界
// ============================================================================

DimensionType DimensionType::nether()
{
    // 下界维度ID = -1 (存档目录 DIM-1)
    DimensionType type(-1, "minecraft:the_nether");

    // 环境属性
    type.m_hasCeiling = true;   // 有基岩天花板
    type.m_hasSkyLight = false; // 无天空光照
    type.m_ultraWarm = true;    // 水会蒸发
    type.m_natural = false;     // 非自然维度
    type.m_hasEnderDragonFight = false;

    // 功能属性
    type.m_bedWorks = false;          // 床会爆炸
    type.m_respawnAnchorWorks = true; // 重生锚可用
    type.m_hasRaids = false;          // 无袭击
    type.m_piglinSafe = true;         // 猪灵不会僵尸化

    // 高度
    type.m_minHeight = 0;
    type.m_maxHeight = 128; // 下界高度为 128
    type.m_logicalHeight = 128;

    // 坐标转换
    type.m_coordinateScale = 8.0f; // 主世界 1 格 = 下界 8 格

    // 光照和时间
    type.m_ambientLight = 0.1f; // 微弱环境光
    type.m_fixedTime = 18000;   // 固定为午夜

    // 方块标签
    type.m_infiniburn = "minecraft:infiniburn_nether";

    return type;
}

// ============================================================================
// 末地
// ============================================================================

DimensionType DimensionType::theEnd()
{
    // 末地维度ID = 1 (存档目录 DIM1)
    DimensionType type(1, "minecraft:the_end");

    // 环境属性
    type.m_hasCeiling = false;
    type.m_hasSkyLight = false; // 无天空光照
    type.m_ultraWarm = false;
    type.m_natural = false;            // 非自然维度
    type.m_hasEnderDragonFight = true; // 有末影龙战斗

    // 功能属性
    type.m_bedWorks = false;           // 床会爆炸
    type.m_respawnAnchorWorks = false; // 重生锚不可用
    type.m_hasRaids = false;           // 无袭击
    type.m_piglinSafe = false;

    // 高度
    type.m_minHeight = world::MIN_BUILD_HEIGHT;
    type.m_maxHeight = world::MAX_BUILD_HEIGHT;
    type.m_logicalHeight = world::MAX_BUILD_HEIGHT;

    // 坐标转换
    type.m_coordinateScale = 1.0f; // 与主世界 1:1

    // 光照和时间
    type.m_ambientLight = 0.0f;
    type.m_fixedTime = 6000; // 固定为正午

    // 方块标签
    type.m_infiniburn = "minecraft:infiniburn_end";

    return type;
}

DimensionType DimensionType::fromId(DimensionId id)
{
    // 维度ID: 主世界=0, 下界=-1, 末地=1
    switch (id) {
        case 0:
            return overworld();
        case -1:
            return nether();
        case 1:
            return theEnd();
        default:
            spdlog::warn("DimensionType: unknown dimension ID {}, falling back to overworld", id);
            return overworld();
    }
}

// ============================================================================
// 坐标转换
// ============================================================================

Vector3d DimensionType::scaleFromOverworld(const Vector3d& pos) const
{
    if (m_coordinateScale == 1.0f) {
        return pos;
    }
    // 主世界 -> 下界: 坐标除以 8
    // coordinateScale = 8.0 时，结果为 pos / 8
    return Vector3d(pos.x / m_coordinateScale, pos.y, pos.z / m_coordinateScale);
}

Vector3d DimensionType::scaleToOverworld(const Vector3d& pos) const
{
    if (m_coordinateScale == 1.0f) {
        return pos;
    }
    // 下界 -> 主世界: 坐标乘以 8
    // coordinateScale = 8.0 时，结果为 pos * 8
    return Vector3d(pos.x * m_coordinateScale, pos.y, pos.z * m_coordinateScale);
}

Vector3d DimensionType::transformPosition(const Vector3d& pos, const DimensionType& from, const DimensionType& to)
{
    // 如果任意一方是主世界，使用主世界作为中介
    if (from.isOverworld()) {
        // 主世界 -> 目标维度
        return to.scaleFromOverworld(pos);
    }

    if (to.isOverworld()) {
        // 源维度 -> 主世界
        return from.scaleToOverworld(pos);
    }

    // 非主世界之间的转换（如下界 <-> 末地）
    // 先转换到主世界，再转换到目标维度
    Vector3d overworldPos = from.scaleToOverworld(pos);
    return to.scaleFromOverworld(overworldPos);
}

} // namespace mc
