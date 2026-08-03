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
#include "common/core/Types.hpp"
#include "common/world/blockentity/interactive/PistonBlockEntity.hpp"

namespace mc {

class MatrixStack;

namespace client::renderer::trident::blockentity {

/**
 * @brief 活塞方块实体渲染器
 *
 * 渲染活塞移动过程中的动画效果。
 *
 * 活塞动画特点：
 * - 动画持续2个游戏tick（约0.1秒）
 * - 每tick进度增加0.5
 * - 使用partialTick进行帧间插值
 * - 需要渲染被移动的方块和活塞臂
 * - 收回时需要额外渲染活塞头
 */
class PistonRenderer : public BlockEntityRenderer<mc::blockentity::PistonBlockEntity> {
public:
    PistonRenderer();
    ~PistonRenderer() override = default;

    // 禁止拷贝
    PistonRenderer(const PistonRenderer&) = delete;
    PistonRenderer& operator=(const PistonRenderer&) = delete;

    // 允许移动
    PistonRenderer(PistonRenderer&&) noexcept = default;
    PistonRenderer& operator=(PistonRenderer&&) noexcept = default;

    /**
     * @brief 渲染活塞方块实体
     *
     * @param entity 活塞方块实体
     * @param partialTick 部分tick（0.0-1.0）
     * @param light 组合光照值
     * @param gameTime 游戏时间（总 tick 数），暂未使用，保留供未来动画需求
     */
    void render(const mc::blockentity::PistonBlockEntity& entity, f32 partialTick, u32 light, i64 gameTime) override;

    /**
     * @brief 活塞是全局渲染器
     *
     * 活塞可能推动方块到相邻区块，需要跨区块可见。
     */
    [[nodiscard]] bool isGlobalRenderer() const override { return true; }

    /**
     * @brief 活塞最大渲染距离
     *
     * 活塞通常在视距内，返回默认值。
     */
    [[nodiscard]] f64 getMaxRenderDistanceSquared() const override { return 64.0; }

private:
    BlockEntityRendererHelper m_helper; ///< 渲染辅助工具

    /**
     * @brief 渲染活塞头
     *
     * 收回时渲染活塞头方块。
     *
     * @param entity 活塞方块实体
     * @param progress 插值后的进度
     * @param light 组合光照
     */
    void _renderPistonHead(const mc::blockentity::PistonBlockEntity& entity, f32 progress, u32 light);

    /**
     * @brief 渲染被移动的方块
     *
     * @param entity 活塞方块实体
     * @param offsetX X方向偏移
     * @param offsetY Y方向偏移
     * @param offsetZ Z方向偏移
     * @param light 组合光照
     */
    void _renderMovingBlock(
        const mc::blockentity::PistonBlockEntity& entity, f32 offsetX, f32 offsetY, f32 offsetZ, u32 light);
};

} // namespace client::renderer::trident::blockentity
} // namespace mc
