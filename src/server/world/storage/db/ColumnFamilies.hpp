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

#pragma once

#include "common/core/Types.hpp"
#include <string>
#include <vector>

namespace mc::world::storage::cf {

// ============================================================================
// 列族名称常量
// ============================================================================

/// 元数据列族（RocksDB默认列族）
constexpr const char* META = "default";

/// 主世界Section数据
constexpr const char* SECTIONS_OVERWORLD = "sections_overworld";

/// 下界Section数据
constexpr const char* SECTIONS_NETHER = "sections_nether";

/// 末地Section数据
constexpr const char* SECTIONS_THE_END = "sections_the_end";

/// 主世界实体数据
constexpr const char* ENTITIES_OVERWORLD = "entities_overworld";

/// 下界实体数据
constexpr const char* ENTITIES_NETHER = "entities_nether";

/// 末地实体数据
constexpr const char* ENTITIES_THE_END = "entities_the_end";

/// 主世界方块实体数据
constexpr const char* BLOCK_ENTITIES_OVERWORLD = "block_entities_overworld";

/// 下界方块实体数据
constexpr const char* BLOCK_ENTITIES_NETHER = "block_entities_nether";

/// 末地方块实体数据
constexpr const char* BLOCK_ENTITIES_THE_END = "block_entities_the_end";

/// 主世界POI（兴趣点）数据
constexpr const char* POI_OVERWORLD = "poi_overworld";

/// 下界POI数据
constexpr const char* POI_NETHER = "poi_nether";

/// 末地POI数据
constexpr const char* POI_THE_END = "poi_the_end";

/// 快照元数据
constexpr const char* SNAPSHOTS = "snapshots";

/// 玩家数据
constexpr const char* PLAYERS = "players";

/// 记分板数据（目标、分数、队伍）
constexpr const char* SCOREBOARD = "scoreboard";

// ============================================================================
// 列族名称数组
// ============================================================================

/// 所有列族名称
inline const std::vector<std::string> ALL_COLUMN_FAMILIES = {META,
    SECTIONS_OVERWORLD,
    SECTIONS_NETHER,
    SECTIONS_THE_END,
    ENTITIES_OVERWORLD,
    ENTITIES_NETHER,
    ENTITIES_THE_END,
    BLOCK_ENTITIES_OVERWORLD,
    BLOCK_ENTITIES_NETHER,
    BLOCK_ENTITIES_THE_END,
    POI_OVERWORLD,
    POI_NETHER,
    POI_THE_END,
    SNAPSHOTS,
    PLAYERS,
    SCOREBOARD};

// ============================================================================
// 工具函数
// ============================================================================

/**
 * @brief 根据维度ID获取Section列族名
 * @param dim 维度ID (主世界=0, 下界=-1, 末地=1)
 * @return 列族名
 */
inline const char* getSectionCF(DimensionId dim) noexcept
{
    switch (dim) {
        case 0:
            return SECTIONS_OVERWORLD;
        case -1:
            return SECTIONS_NETHER;
        case 1:
            return SECTIONS_THE_END;
        default:
            return SECTIONS_OVERWORLD;
    }
}

/**
 * @brief 根据维度ID获取实体列族名
 * @param dim 维度ID (主世界=0, 下界=-1, 末地=1)
 * @return 列族名
 */
inline const char* getEntityCF(DimensionId dim) noexcept
{
    switch (dim) {
        case 0:
            return ENTITIES_OVERWORLD;
        case -1:
            return ENTITIES_NETHER;
        case 1:
            return ENTITIES_THE_END;
        default:
            return ENTITIES_OVERWORLD;
    }
}

/**
 * @brief 根据维度ID获取POI列族名
 * @param dim 维度ID (主世界=0, 下界=-1, 末地=1)
 * @return 列族名
 */
inline const char* getPoiCF(DimensionId dim) noexcept
{
    switch (dim) {
        case 0:
            return POI_OVERWORLD;
        case -1:
            return POI_NETHER;
        case 1:
            return POI_THE_END;
        default:
            return POI_OVERWORLD;
    }
}

/**
 * @brief 根据维度ID获取方块实体列族名
 * @param dim 维度ID (主世界=0, 下界=-1, 末地=1)
 * @return 列族名
 */
inline const char* getBlockEntityCF(DimensionId dim) noexcept
{
    switch (dim) {
        case 0:
            return BLOCK_ENTITIES_OVERWORLD;
        case -1:
            return BLOCK_ENTITIES_NETHER;
        case 1:
            return BLOCK_ENTITIES_THE_END;
        default:
            return BLOCK_ENTITIES_OVERWORLD;
    }
}

} // namespace mc::world::storage::cf
