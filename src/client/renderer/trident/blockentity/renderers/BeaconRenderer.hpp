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

#include "client/renderer/trident/blockentity/BlockEntityRenderer.hpp"
#include "client/renderer/trident/blockentity/IBlockEntityRenderer.hpp"
#include "client/renderer/trident/blockentity/model/BeaconBeamModel.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>
#include <vector>

namespace mc {

namespace blockentity {
class BeaconEntity;
struct BeaconBeamSegment;
} // namespace blockentity

namespace client::renderer::trident::blockentity {

/**
 * @brief 信标方块实体渲染器
 *
 * 渲染信标光束效果。
 *
 * 信标光束特点：
 * - 全局渲染器（可见距离256格）
 * - 光束使用 gameTime 驱动旋转
 * - 光束颜色由金字塔顶部的玻璃决定
 * - 激活时有垂直光束，未激活时无光束
 * - 双层渲染：内层光束(radius=0.2) + 外层光晕(radius=0.25)
 */
class BeaconRenderer : public BlockEntityRenderer<mc::blockentity::BeaconEntity> {
public:
    BeaconRenderer();
    ~BeaconRenderer() override = default;

    // 禁止拷贝
    BeaconRenderer(const BeaconRenderer&) = delete;
    BeaconRenderer& operator=(const BeaconRenderer&) = delete;

    // 允许移动
    BeaconRenderer(BeaconRenderer&&) noexcept = default;
    BeaconRenderer& operator=(BeaconRenderer&&) noexcept = default;

    /**
     * @brief 渲染信标方块实体
     *
     * @param entity 信标方块实体
     * @param partialTick 部分tick（0.0-1.0）
     * @param light 组合光照值
     * @param gameTime 游戏时间（总 tick 数），用于驱动光束旋转动画
     */
    void render(const mc::blockentity::BeaconEntity& entity, f32 partialTick, u32 light, i64 gameTime) override;

    /**
     * @brief 信标是全局渲染器
     *
     * 光束需要远距离可见。
     */
    [[nodiscard]] bool isGlobalRenderer() const override { return true; }

    /**
     * @brief 信标最大渲染距离
     *
     * 光束可以在256格（16个区块）外看到。
     */
    [[nodiscard]] f64 getMaxRenderDistanceSquared() const override { return 65536.0; }

private:
    mc::client::renderer::blockentity::model::BeaconBeamModel m_beamModel; ///< 光束模型
    BlockEntityRendererHelper m_helper;                                    ///< 渲染辅助工具

    /**
     * @brief 渲染信标基座
     *
     * @param pos 方块位置
     * @param light 组合光照
     */
    void _renderBeaconBase(const BlockPos& pos, u32 light);

    /**
     * @brief 渲染光束
     *
     * @param pos 方块位置
     * @param segments 光束段列表
     * @param gameTime 游戏时间
     * @param partialTick 部分tick
     * @param light 组合光照
     */
    void _renderBeam(const BlockPos& pos,
        const std::vector<mc::blockentity::BeaconBeamSegment>& segments,
        i64 gameTime,
        f32 partialTick,
        u32 light);
};

} // namespace client::renderer::trident::blockentity
} // namespace mc
