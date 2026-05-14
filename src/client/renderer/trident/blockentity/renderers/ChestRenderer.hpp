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
#include "client/renderer/trident/blockentity/model/ChestModel.hpp"
#include <memory>

namespace mc {

namespace blockentity {
class ChestEntity;
}

namespace client::renderer::trident::blockentity {

// 使用模型命名空间的别名
namespace model = mc::client::renderer::blockentity::model;

/**
 * @brief 箱子方块实体渲染器
 *
 * 渲染箱子和盖子的开合动画。
 * 参考 MC 1.16.5 ChestTileEntityRenderer
 *
 * 箱子动画特点：
 * - 盖子角度从0.0到1.0
 * - 使用非线性缓动：angle = 1.0 - (1.0 - angle)^3
 * - 支持单箱和双箱（LEFT/RIGHT/SINGLE 类型）
 * - 12月24-26日使用圣诞节纹理（彩蛋）
 */
class ChestRenderer : public BlockEntityRenderer<mc::blockentity::ChestEntity> {
public:
    ChestRenderer();
    ~ChestRenderer() override = default;

    // 禁止拷贝
    ChestRenderer(const ChestRenderer&) = delete;
    ChestRenderer& operator=(const ChestRenderer&) = delete;

    // 允许移动
    ChestRenderer(ChestRenderer&&) noexcept = default;
    ChestRenderer& operator=(ChestRenderer&&) noexcept = default;

    /**
     * @brief 渲染箱子方块实体
     *
     * @param entity 箱子方块实体
     * @param partialTick 部分tick（0.0-1.0）
     * @param light 组合光照值
     */
    void render(const mc::blockentity::ChestEntity& entity, f32 partialTick, u32 light) override;

    /**
     * @brief 箱子不是全局渲染器
     */
    [[nodiscard]] bool isGlobalRenderer() const override { return false; }

    /**
     * @brief 箱子最大渲染距离（8格）
     */
    [[nodiscard]] f64 getMaxRenderDistanceSquared() const override { return 64.0; }

    /**
     * @brief 检查是否是圣诞节（12月24-26日）
     * @return 如果是圣诞节期间返回true
     */
    [[nodiscard]] static bool isChristmas();

private:
    mc::client::renderer::blockentity::model::ChestModel m_model; ///< 箱子模型

    /**
     * @brief 根据方块状态获取箱子类型
     * @param entity 箱子实体
     * @return 箱子类型
     */
    [[nodiscard]] mc::client::renderer::blockentity::model::ChestModel::ChestType determineChestType(
        const mc::blockentity::ChestEntity& entity) const;
};

} // namespace client::renderer::trident::blockentity
} // namespace mc
