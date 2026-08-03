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

#include "ChestRenderer.hpp"
#include "client/renderer/trident/blockentity/IBlockEntityRenderer.hpp"
#include "client/renderer/trident/blockentity/model/ChestModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/util/SpecialDates.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/world/blockentity/storage/ChestEntity.hpp"
#include <vector>

namespace mc::client::renderer::trident::blockentity {

ChestRenderer::ChestRenderer()
    : BlockEntityRenderer<mc::blockentity::ChestEntity>()
    , m_model()
{}

bool ChestRenderer::isChristmas()
{
    return util::SpecialDates::isExtendedChristmas();
}

mc::client::renderer::blockentity::model::ChestModel::ChestType ChestRenderer::_determineChestType(
    const mc::blockentity::ChestEntity& entity) const
{
    // 需要 entity.getBlockState() 获取 TYPE 属性
    // ChestType: SINGLE, LEFT, RIGHT
    // 当前默认返回单箱，完整实现需要 BlockState 访问
    (void)entity;
    return mc::client::renderer::blockentity::model::ChestModel::ChestType::Single;
}

void ChestRenderer::render(const mc::blockentity::ChestEntity& entity, f32 partialTick, u32 light, i64 gameTime)
{
    MC_UNUSED(gameTime);

    // 获取插值后的盖子角度
    const f32 lidAngle = entity.getInterpolatedLidAngle(partialTick);

    // 如果盖子关闭且没有打开计数，跳过渲染
    if (lidAngle <= 0.001f && entity.getOpenCount() == 0) {
        // 仍然需要渲染箱体
        m_model.setLidAngle(0.0f);
    } else {
        // 应用缓动函数
        m_model.setLidAngle(lidAngle);
    }

    // 设置箱子类型（单箱/双箱左/右）
    const auto chestType = _determineChestType(entity);
    m_model.setChestType(chestType);

    // 生成网格数据
    std::vector<entity::model::ModelVertex> vertices;
    std::vector<u32> indices;
    m_model.generateMesh(vertices, indices);

    // 渲染管线集成说明：
    // 该渲染器生成模型顶点数据后，由上层渲染系统（BlockEntityRenderDispatcher）
    // 负责将顶点数据提交到 GPU。完整的渲染需要：
    // 1. 从方块状态获取 FACING 属性确定朝向
    // 2. 根据 isChristmas() 选择纹理变体
    // 3. 计算双箱时的光照合并
    // TODO 这些功能依赖外部系统（方块状态、纹理系统）的实现完善后集成。
    (void)light;
    (void)vertices;
    (void)indices;
}

} // namespace mc::client::renderer::trident::blockentity
