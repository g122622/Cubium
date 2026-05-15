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

#include "../../core/Types.hpp"

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
 * 注意：由于项目使用 -ffast-math 编译选项，NaN 检测不可靠，
 * 因此使用显式的布尔字段来表示云的存在性。
 *
 * 使用示例:
 * @code
 * auto settings = DimensionRenderSettings::overworld();
 * float cloudHeight = settings.cloudHeight;
 * if (settings.hasClouds) {
 *     // 渲染云
 * }
 * @endcode
 */
struct DimensionRenderSettings {
    /// 云高度 (仅当 hasClouds 为 true 时有效)
    /// 主世界: 192.0f
    f32 cloudHeight;

    /// 是否有云 (下界和末地为 false)
    /// 注意：必须使用此字段而非 std::isnan(cloudHeight)，
    /// 因为 -ffast-math 会破坏 NaN 检测
    bool hasClouds;

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
        settings.hasClouds = true;
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
        settings.cloudHeight = 0.0f; // 无云时不使用此值
        settings.hasClouds = false;
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
        settings.cloudHeight = 0.0f; // 无云时不使用此值
        settings.hasClouds = false;
        settings.hasSky = false;
        settings.hasCeiling = false;
        settings.fogType = FogType::End;
        settings.hasNaturalLight = false;
        settings.name = "end";
        return settings;
    }

    /**
     * @brief 获取默认维度设置 (主世界)
     */
    static DimensionRenderSettings getDefault() { return overworld(); }
};

} // namespace mc::world
