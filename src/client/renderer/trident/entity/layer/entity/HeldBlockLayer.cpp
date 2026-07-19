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

#include "HeldBlockLayer.hpp"
#include "client/renderer/trident/chunk/ChunkMesher.hpp"
#include "client/renderer/trident/chunk/ChunkRenderer.hpp"
#include "client/renderer/trident/entity/core/AnimationContext.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/renderer/trident/entity/pipeline/EntityPipeline.hpp"
#include "client/renderer/trident/entity/pipeline/EntityTextureAtlas.hpp"
#include "client/renderer/trident/entity/util/BlockMeshBuilder.hpp"
#include "client/resource/ResourceManager.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/Block.hpp"
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::layer::entity {

// ---------------------------------------------------------------------------
// 辅助：将 4x4 矩阵（glm::mat4，列主序）转换为 drawMesh 所需的行主序 array<f64,16>
// ---------------------------------------------------------------------------
static std::array<f64, 16> toRowMajorArray(const glm::mat4& m)
{
    // glm::mat4 是列主序：m[col][row]
    // drawMesh 期望的 array<f64,16> 为行主序：
    //   [0..3]   = 第一行 (m00, m01, m02, m03)
    //   [4..7]   = 第二行 (m10, m11, m12, m13)
    //   [8..11]  = 第三行 (m20, m21, m22, m23)
    //   [12..15] = 第四行 (m30, m31, m32, m33)
    return {static_cast<f64>(m[0][0]),
        static_cast<f64>(m[1][0]),
        static_cast<f64>(m[2][0]),
        static_cast<f64>(m[3][0]),
        static_cast<f64>(m[0][1]),
        static_cast<f64>(m[1][1]),
        static_cast<f64>(m[2][1]),
        static_cast<f64>(m[3][1]),
        static_cast<f64>(m[0][2]),
        static_cast<f64>(m[1][2]),
        static_cast<f64>(m[2][2]),
        static_cast<f64>(m[3][2]),
        static_cast<f64>(m[0][3]),
        static_cast<f64>(m[1][3]),
        static_cast<f64>(m[2][3]),
        static_cast<f64>(m[3][3])};
}

void HeldBlockLayer::renderPipeline(::mc::client::ClientEntity& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    // 获取持有的方块
    const ::mc::BlockState* blockState = _getHeldBlock(entity);
    if (blockState == nullptr) {
        return;
    }

    // 末影人持有方块的位置（MC 原版偏移：y=0.6875, z=-0.75，这里 x/y/z 作为相对实体的偏移）
    _renderBlockPipeline(entity, *blockState, 0.0f, 0.6875f, -0.75f, cmd, context, pipeline);
}

bool HeldBlockLayer::shouldRender(const ::mc::client::ClientEntity& entity) const
{
    // 通过 ClientEntity 的元数据镜像字段判断是否持有方块
    // 对应 MC 1.21.11 CarriedBlockLayer.submit() 中的 carriedBlock != null 检查
    return entity.endermanHeldBlockState() != nullptr;
}

const ::mc::BlockState* HeldBlockLayer::_getHeldBlock(const ::mc::client::ClientEntity& entity)
{
    // 从 ClientEntity 镜像字段读取（由 syncMetadataFromDataManager 同步）
    return entity.endermanHeldBlockState();
}

void HeldBlockLayer::_renderBlockPipeline(::mc::client::ClientEntity& entity,
    const ::mc::BlockState& blockState,
    f32 x,
    f32 y,
    f32 z,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    // 获取或创建方块网格
    pipeline::EntityMesh* mesh = _getOrCreateBlockMesh(pipeline, blockState);
    if (mesh == nullptr || mesh->indexCount == 0) {
        return;
    }

    // ---- 复刻 MC 1.21.11 CarriedBlockLayer.submit() 的完整变换链 ----
    //   translate(0, 0.6875, -0.75)     // 已通过 x/y/z 参数传入
    //   rotateX(20°)
    //   rotateY(45°)
    //   translate(0.25, 0.1875, 0.25)
    //   scale(-0.5, -0.5, 0.5)
    //   rotateY(90°)
    //
    // 注意：glm 使用列主序，矩阵乘法从右向左应用（最右侧先作用于顶点）。
    // glm::rotate / glm::scale / glm::translate 返回的矩阵 M 使得
    //   v' = M * v
    // 组合时 M = T1 * R1 * T2 * S * R2，顶点依次被 R2、S、T2、R1、T1 变换。
    //
    // 顶点坐标已经在 _buildBlockMesh 中乘以 1/16 转换为世界单位（方块 0-1 范围），
    // 这里只应用 CarriedBlockLayer 的持方块变换。

    const f32 deg20 = glm::radians(20.0f);
    const f32 deg45 = glm::radians(45.0f);
    const f32 deg90 = glm::radians(90.0f);

    // 从最右侧（最先作用于顶点）向左构建
    glm::mat4 transform = glm::mat4(1.0f);
    transform = glm::translate(transform, glm::vec3(x, y, z));               // translate(0, 0.6875, -0.75)
    transform = glm::rotate(transform, deg20, glm::vec3(1.0f, 0.0f, 0.0f));  // rotateX(20°)
    transform = glm::rotate(transform, deg45, glm::vec3(0.0f, 1.0f, 0.0f));  // rotateY(45°)
    transform = glm::translate(transform, glm::vec3(0.25f, 0.1875f, 0.25f)); // translate(0.25, 0.1875, 0.25)
    transform = glm::scale(transform, glm::vec3(-0.5f, -0.5f, 0.5f));        // scale(-0.5, -0.5, 0.5)
    transform = glm::rotate(transform, deg90, glm::vec3(0.0f, 1.0f, 0.0f));  // rotateY(90°)

    std::array<f64, 16> blockTransform = toRowMajorArray(transform);

    // 实体位置（从实体本身获取，而非硬编码）
    Vector3f entityPos(static_cast<f32>(entity.x()), static_cast<f32>(entity.y()), static_cast<f32>(entity.z()));

    // 获取方块默认着色颜色（如草方块等需要 tint 的方块）
    const u32 tintColor = ChunkMesher::getDefaultBlockTintColor(&blockState);

    // 从打包颜色提取 RGBA 分量
    const f32 r = static_cast<f32>(tintColor & 0xFFu) / 255.0f;
    const f32 g = static_cast<f32>((tintColor >> 8) & 0xFFu) / 255.0f;
    const f32 b = static_cast<f32>((tintColor >> 16) & 0xFFu) / 255.0f;
    const f32 a = static_cast<f32>((tintColor >> 24) & 0xFFu) / 255.0f;
    Vector4f overlayColor(r, g, b, a);

    // ---- 切换到方块纹理图集（方块 UV 基于区块纹理图集，而非实体纹理图集）----
    // EntityPipeline::setTextureAtlas 是覆盖式写入，没有 getter 可以保存当前图集。
    // 因此通过外部注入的 m_entityTextureAtlas 指针来恢复。
    //
    // 流程：
    //   1. 渲染前：若 blocks atlas 可用，切换 EntityPipeline 到 blocks atlas
    //   2. 绘制方块网格
    //   3. 渲染后：若实体纹理图集可用，恢复 EntityPipeline 到实体纹理图集
    //              （避免污染后续实体渲染）
    const bool needAtlasSwitch = (m_blockImageView != VK_NULL_HANDLE);
    if (needAtlasSwitch) {
        pipeline.setTextureAtlas(m_blockImageView, m_blockSampler);
    }

    pipeline.drawMesh(cmd, *mesh, blockTransform, entityPos, 1.0, overlayColor, 0.0f, 0.0f);

    // 恢复实体纹理图集（避免后续实体渲染使用错误的纹理）
    if (needAtlasSwitch && m_entityTextureAtlas != nullptr) {
        pipeline.setTextureAtlas(m_entityTextureAtlas->imageView(), m_entityTextureAtlas->sampler());
    }

    (void)context;
}

pipeline::EntityMesh* HeldBlockLayer::_getOrCreateBlockMesh(
    pipeline::EntityPipeline& pipeline, const ::mc::BlockState& blockState)
{
    // 按 BlockState 指针缓存网格（项目中方块状态指针来自 BlockRegistry，是稳定的）
    const auto it = m_blockMeshCache.find(&blockState);
    if (it != m_blockMeshCache.end() && it->second && it->second->indexCount > 0) {
        return it->second.get();
    }

    // 构建方块网格（委托给公共工具 util::BlockMeshBuilder）
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;
    util::BlockMeshBuilder::buildBlockMesh(blockState, vertices, indices);

    if (vertices.empty() || indices.empty()) {
        return nullptr;
    }

    auto result = pipeline.createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::warn(
            "HeldBlockLayer: Failed to create block mesh for blockState {}", static_cast<const void*>(&blockState));
        return nullptr;
    }

    auto meshPtr = std::make_unique<pipeline::EntityMesh>(std::move(result.value()));
    pipeline::EntityMesh* rawPtr = meshPtr.get();
    m_blockMeshCache[&blockState] = std::move(meshPtr);
    return rawPtr;
}

} // namespace mc::client::renderer::entity::layer::entity
