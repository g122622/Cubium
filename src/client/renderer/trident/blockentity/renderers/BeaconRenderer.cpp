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

#include "BeaconRenderer.hpp"
#include "client/renderer/trident/blockentity/IBlockEntityRenderer.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/processing/BeaconEntity.hpp"
#include <vector>

namespace mc::client::renderer::trident::blockentity {

BeaconRenderer::BeaconRenderer()
    : BlockEntityRenderer<mc::blockentity::BeaconEntity>()
    , m_beamModel()
    , m_helper()
{}

void BeaconRenderer::render(const mc::blockentity::BeaconEntity& entity, f32 partialTick, u32 light, i64 gameTime)
{
    const BlockPos& pos = entity.getPos();

    // 渲染信标基座（普通方块渲染）
    _renderBeaconBase(pos, light);

    // 如果未激活，不渲染光束
    if (!entity.isActive()) {
        return;
    }

    // 获取光束段数据
    const auto& segments = entity.getBeamSegments();
    if (segments.empty()) {
        return;
    }

    // 渲染光束
    _renderBeam(pos, segments, gameTime, partialTick, light);
}

void BeaconRenderer::_renderBeaconBase(const BlockPos& pos, u32 light)
{
    // 信标基座使用普通方块模型渲染
    // 由 BlockModelCache 和区块渲染器处理
    // 此处无需额外处理
    (void)pos;
    (void)light;
}

void BeaconRenderer::_renderBeam(const BlockPos& pos,
    const std::vector<mc::blockentity::BeaconBeamSegment>& segments,
    i64 gameTime,
    f32 partialTick,
    u32 light)
{
    // 1. 设置模型变换（平移到方块中心）
    // 2. 计算旋转角度
    // 3. 渲染每个光束段

    // 清除并设置光束段（BeaconBeamSegment 与 BeamSegment 是同一类型）
    m_beamModel.clearSegments();
    for (const auto& segment : segments) {
        m_beamModel.addSegment(segment);
    }

    // 生成网格数据
    std::vector<entity::model::ModelVertex> vertices;
    std::vector<u32> indices;
    m_beamModel.generateMesh(vertices, indices, gameTime, partialTick);

    // TODO 网格数据已生成，后续集成步骤：
    // 1. 获取 RenderType.beaconBeam(texture, true/false)
    // 2. 创建变换矩阵（平移到 pos + 0.5, 0.0, 0.5）
    // 3. 应用旋转（绕 Y 轴）
    // 4. 提交顶点数据到渲染管线
    // 注意：完整渲染管线集成需要在 EntityPipeline 或专用 BlockEntityPipeline 中实现

    (void)pos;
    (void)light;
}

} // namespace mc::client::renderer::trident::blockentity
