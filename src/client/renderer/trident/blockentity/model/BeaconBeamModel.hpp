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

#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/world/blockentity/processing/BeaconEntity.hpp"
#include <array>
#include <vector>

namespace mc::client::renderer::blockentity::model {

// 使用 common 模块中定义的信标光束段类型
using BeamSegment = mc::blockentity::BeaconBeamSegment;

/**
 * @brief 信标光束模型
 *
 * 渲染信标的垂直光束效果。
 * 光束特点：
 * - 使用 gameTime 驱动旋转动画
 * - 双层渲染：内层光束 + 外层光晕
 * - 每段光束有不同的颜色
 * - 最后一段高度固定为 1024 格
 *
 * 纹理：textures/entity/beacon_beam.png (64x64)
 */
class BeaconBeamModel {
public:
    BeaconBeamModel();
    ~BeaconBeamModel() = default;

    // 禁止拷贝
    BeaconBeamModel(const BeaconBeamModel&) = delete;
    BeaconBeamModel& operator=(const BeaconBeamModel&) = delete;

    // 允许移动
    BeaconBeamModel(BeaconBeamModel&&) noexcept = default;
    BeaconBeamModel& operator=(BeaconBeamModel&&) noexcept = default;

    // ========== 光束段管理 ==========

    /**
     * @brief 清除所有光束段
     */
    void clearSegments() { m_segments.clear(); }

    /**
     * @brief 添加光束段
     * @param segment 光束段
     */
    void addSegment(const BeamSegment& segment) { m_segments.push_back(segment); }

    /**
     * @brief 获取所有光束段
     */
    [[nodiscard]] const std::vector<BeamSegment>& getSegments() const { return m_segments; }

    // ========== 渲染 ==========

    /**
     * @brief 生成光束网格
     * @param vertices 顶点输出缓冲区
     * @param indices 索引输出缓冲区
     * @param gameTime 游戏时间（用于纹理动画）
     * @param partialTick 部分tick（用于插值）
     */
    void generateMesh(std::vector<entity::model::ModelVertex>& vertices,
        std::vector<u32>& indices,
        i64 gameTime,
        f32 partialTick) const;

    /**
     * @brief 计算光束旋转角度
     * @param gameTime 游戏时间
     * @param partialTick 部分tick
     * @return 旋转角度（度数）
     */
    [[nodiscard]] static f32 calculateBeamRotation(i64 gameTime, f32 partialTick);

    // ========== 常量 ==========

    /// 内层光束半径
    static constexpr f32 BEAM_RADIUS = 0.2f;

    /// 外层光晕半径
    static constexpr f32 GLOW_RADIUS = 0.25f;

    /// 光束旋转速度（度/tick）
    static constexpr f32 ROTATION_SPEED = 2.25f;

    /// 光束旋转偏移角度
    static constexpr f32 ROTATION_OFFSET = -45.0f;

    /// 旋转周期（ticks）
    static constexpr i64 ROTATION_PERIOD = 40L;

    /// 最后一段的默认高度
    static constexpr i32 MAX_BEAM_HEIGHT = 1024;

    /// 最大光照值
    static constexpr u32 MAX_LIGHT = 15728880;

private:
    /**
     * @brief 渲染单个光束段
     * @param vertices 顶点缓冲区
     * @param indices 索引缓冲区
     * @param yOffset Y偏移
     * @param height 段高度
     * @param colors 颜色
     * @param vOffset V纹理偏移
     * @param isGlow 是否为光晕层
     */
    void _renderSegment(std::vector<entity::model::ModelVertex>& vertices,
        std::vector<u32>& indices,
        i32 yOffset,
        i32 height,
        const std::array<f32, 3>& colors,
        f32 vOffset,
        bool isGlow) const;

    /**
     * @brief 添加一个四边形
     * @param vertices 顶点缓冲区
     * @param indices 索引缓冲区
     * @param yMin Y最小值
     * @param yMax Y最大值
     * @param x1 第一个X坐标
     * @param z1 第一个Z坐标
     * @param x2 第二个X坐标
     * @param z2 第二个Z坐标
     * @param u1 U纹理坐标1
     * @param u2 U纹理坐标2
     * @param v1 V纹理坐标1
     * @param v2 V纹理坐标2
     * @param r 红色
     * @param g 绿色
     * @param b 蓝色
     * @param alpha 透明度
     */
    void _addQuad(std::vector<entity::model::ModelVertex>& vertices,
        std::vector<u32>& indices,
        f32 yMin,
        f32 yMax,
        f32 x1,
        f32 z1,
        f32 x2,
        f32 z2,
        f32 u1,
        f32 u2,
        f32 v1,
        f32 v2,
        f32 r,
        f32 g,
        f32 b,
        f32 alpha) const;

    std::vector<BeamSegment> m_segments;
};

} // namespace mc::client::renderer::blockentity::model
