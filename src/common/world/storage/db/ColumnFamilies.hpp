#pragma once

#include "../../../core/Types.hpp"
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
inline const std::vector<std::string> ALL_COLUMN_FAMILIES = {
    META,
    SECTIONS_OVERWORLD,
    SECTIONS_NETHER,
    SECTIONS_THE_END,
    ENTITIES_OVERWORLD,
    ENTITIES_NETHER,
    ENTITIES_THE_END,
    POI_OVERWORLD,
    POI_NETHER,
    POI_THE_END,
    SNAPSHOTS,
    PLAYERS,
    SCOREBOARD
};

// ============================================================================
// 工具函数
// ============================================================================

/**
 * @brief 根据维度ID获取Section列族名
 * @param dim 维度ID (MC 1.16.5: 主世界=0, 下界=-1, 末地=1)
 * @return 列族名
 */
inline const char* getSectionCF(DimensionId dim) {
    // MC 1.16.5 标准：主世界=0，下界=-1，末地=1
    switch (dim) {
        case 0: return SECTIONS_OVERWORLD;
        case -1: return SECTIONS_NETHER;
        case 1: return SECTIONS_THE_END;
        default: return SECTIONS_OVERWORLD;
    }
}

/**
 * @brief 根据维度ID获取实体列族名
 * @param dim 维度ID (MC 1.16.5: 主世界=0, 下界=-1, 末地=1)
 * @return 列族名
 */
inline const char* getEntityCF(DimensionId dim) {
    // MC 1.16.5 标准：主世界=0，下界=-1，末地=1
    switch (dim) {
        case 0: return ENTITIES_OVERWORLD;
        case -1: return ENTITIES_NETHER;
        case 1: return ENTITIES_THE_END;
        default: return ENTITIES_OVERWORLD;
    }
}

/**
 * @brief 根据维度ID获取POI列族名
 * @param dim 维度ID (MC 1.16.5: 主世界=0, 下界=-1, 末地=1)
 * @return 列族名
 */
inline const char* getPoiCF(DimensionId dim) {
    // MC 1.16.5 标准：主世界=0，下界=-1，末地=1
    switch (dim) {
        case 0: return POI_OVERWORLD;
        case -1: return POI_NETHER;
        case 1: return POI_THE_END;
        default: return POI_OVERWORLD;
    }
}

} // namespace mc::world::storage::cf
