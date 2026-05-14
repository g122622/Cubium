#pragma once

#include "../../core/Types.hpp"
#include <cmath>
#include <limits>

namespace mc::world {

/**
 * @brief 雾类型
 *
 * 参考 MC 1.16.5 DimensionRenderInfo.FogType
 */
enum class FogType : u8 {
    None = 0,   ///< 无雾
    Normal = 1, ///< 普通雾
    End = 2     ///< 末地雾
};

/**
 * @brief 维度渲染设置
 *
 * 定义各维度特有的渲染参数。
 * 参考 MC 1.16.5 DimensionRenderInfo。
 *
 * 使用示例:
 * @code
 * auto settings = DimensionRenderSettings::overworld();
 * float cloudHeight = settings.cloudHeight;
 * if (!std::isnan(cloudHeight)) {
 *     // 渲染云
 * }
 * @endcode
 */
struct DimensionRenderSettings {
    /// 云高度 (NaN 表示该维度无云)
    /// 主世界: 192.0f, 下界/末地: NaN
    f32 cloudHeight;

    /// 是否有天空
    bool hasSky;

    /// 是否有天花板 (下界为 true)
    bool hasCeiling;

    /// 雾类型
    FogType fogType;

    /// 是否有自然光照
    bool hasNaturalLight;

    /// 维度名称 (用于调试)
    const char* name;

    /**
     * @brief 获取主世界渲染设置
     */
    static DimensionRenderSettings overworld()
    {
        DimensionRenderSettings settings;
        settings.cloudHeight = 192.0f;
        settings.hasSky = true;
        settings.hasCeiling = false;
        settings.fogType = FogType::Normal;
        settings.hasNaturalLight = true;
        settings.name = "overworld";
        return settings;
    }

    /**
     * @brief 获取下界渲染设置
     */
    static DimensionRenderSettings nether()
    {
        DimensionRenderSettings settings;
        settings.cloudHeight = std::numeric_limits<f32>::quiet_NaN();
        settings.hasSky = false;
        settings.hasCeiling = true;
        settings.fogType = FogType::None;
        settings.hasNaturalLight = false;
        settings.name = "nether";
        return settings;
    }

    /**
     * @brief 获取末地渲染设置
     */
    static DimensionRenderSettings end()
    {
        DimensionRenderSettings settings;
        settings.cloudHeight = std::numeric_limits<f32>::quiet_NaN();
        settings.hasSky = false;
        settings.hasCeiling = false;
        settings.fogType = FogType::End;
        settings.hasNaturalLight = false;
        settings.name = "end";
        return settings;
    }

    /**
     * @brief 检查该维度是否有云
     */
    [[nodiscard]] bool hasClouds() const { return !std::isnan(cloudHeight); }

    /**
     * @brief 获取默认维度设置 (主世界)
     */
    static DimensionRenderSettings getDefault() { return overworld(); }
};

} // namespace mc::world
