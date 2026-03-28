#pragma once

#include "../core/Types.hpp"
#include <string>

namespace mc {

/**
 * @brief 世界类型枚举
 *
 * 定义可用的世界生成类型。
 *
 * 参考 MC 1.16.5 WorldType / BiomeGeneratorTypeScreens
 */
enum class WorldType : u8 {
    Default,        ///< 默认世界（噪声地形生成）
    Flat,           ///< 超平坦世界
    LargeBiomes,    ///< 大型生物群系
    Amplified,      ///< 放大化地形
    Debug           ///< 调试模式（展示所有方块状态）
};

/**
 * @brief 获取世界类型的显示名称
 * @param type 世界类型
 * @return 显示名称
 */
[[nodiscard]] inline String worldTypeName(WorldType type) {
    switch (type) {
        case WorldType::Default:      return "default";
        case WorldType::Flat:         return "flat";
        case WorldType::LargeBiomes:  return "largeBiomes";
        case WorldType::Amplified:    return "amplified";
        case WorldType::Debug:        return "debug_all_block_states";
        default:                      return "unknown";
    }
}

/**
 * @brief 从字符串解析世界类型
 * @param name 世界类型名称
 * @return 世界类型枚举值
 */
[[nodiscard]] inline WorldType parseWorldType(const String& name) {
    if (name == "flat") {
        return WorldType::Flat;
    } else if (name == "largeBiomes" || name == "large_biomes") {
        return WorldType::LargeBiomes;
    } else if (name == "amplified") {
        return WorldType::Amplified;
    } else if (name == "debug_all_block_states" || name == "debug") {
        return WorldType::Debug;
    } else {
        return WorldType::Default;
    }
}

/**
 * @brief 世界配置结构
 *
 * 包含世界生成的所有配置参数。
 */
struct WorldConfig {
    /// 世界种子
    u64 seed = 0;

    /// 世界类型
    WorldType worldType = WorldType::Default;  // 默认使用普通世界

    /// 视距
    i32 viewDistance = 10;

    /// 维度ID（0=主世界，1=下界，2=末地）
    DimensionId dimension = 0;

    /// 是否启用作弊
    bool enableCheats = true;

    /**
     * @brief 是否为调试世界
     */
    [[nodiscard]] bool isDebugWorld() const {
        return worldType == WorldType::Debug;
    }
};

} // namespace mc
