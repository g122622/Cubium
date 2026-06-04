/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software", to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", ANY KIND, EXPRESS OR
 * IMPLIED, WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABILITY FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "client/renderer/trident/blockentity/BlockEntityRenderer.hpp"
#include "client/renderer/trident/blockentity/IBlockEntityRenderer.hpp"
#include "client/renderer/trident/blockentity/model/BannerModel.hpp"
#include <memory>

namespace mc {

namespace blockentity {
class BannerEntity;
}

namespace client::renderer::trident::blockentity {

namespace model = mc::client::renderer::blockentity::model;

/**
 * @brief 旗帜方块实体渲染器
 *
 * 渲染旗帜方块实体，包括旗杆和旗帜面。
 * 支持站立旗帜和墙壁旗帜两种形态，以及风吹飘动动画。
 *
 * 参考: net.minecraft.client.renderer.tileentity.BannerTileEntityRenderer
 */
class BannerRenderer : public BlockEntityRenderer<mc::blockentity::BannerEntity> {
public:
    BannerRenderer();
    ~BannerRenderer() override = default;

    // 禁止拷贝
    BannerRenderer(const BannerRenderer&) = delete;
    BannerRenderer& operator=(const BannerRenderer&) = delete;

    // 允许移动
    BannerRenderer(BannerRenderer&&) noexcept = default;
    BannerRenderer& operator=(BannerRenderer&&) noexcept = default;

    /**
     * @brief 渲染旗帜方块实体
     *
     * @param entity 旗帜方块实体
     * @param partialTick 部分tick（0.0-1.0）
     * @param light 组合光照值
     */
    void render(const mc::blockentity::BannerEntity& entity, f32 partialTick, u32 light) override;

    /**
     * @brief 旗帜不是全局渲染器
     */
    [[nodiscard]] bool isGlobalRenderer() const override { return false; }

    /**
     * @brief 旗帜最大渲染距离（8格）
     */
    [[nodiscard]] f64 getMaxRenderDistanceSquared() const override { return 64.0; }

private:
    model::BannerModel m_model; ///< 旗帜模型

    /**
     * @brief 根据方块状态判断旗帜类型
     * @param entity 旗帜实体
     * @return 站立或墙壁旗帜类型
     */
    [[nodiscard]] model::BannerModel::BannerType _determineBannerType(
        const mc::blockentity::BannerEntity& entity) const;

    /**
     * @brief 从方块状态获取旗帜旋转角度
     * @param entity 旗帜实体
     * @return Y轴旋转角度（弧度）
     */
    [[nodiscard]] f32 _getRotation(const mc::blockentity::BannerEntity& entity) const;
};

} // namespace client::renderer::trident::blockentity
} // namespace mc
