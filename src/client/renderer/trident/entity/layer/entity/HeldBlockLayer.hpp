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

#include "client/renderer/trident/entity/layer/core/LayerRenderer.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector4.hpp"
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

namespace mc {
class BlockState;
} // namespace mc

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
class EntityTextureAtlas;
struct EntityMesh;
} // namespace mc::client::renderer::entity::pipeline

namespace mc::client::renderer::entity::layer::entity {

/**
 * @brief 方块持有层渲染器
 *
 * 渲染末影人手持的方块。对应 MC 1.21.11 CarriedBlockLayer（1.16.5 为 HeldBlockLayer）。
 *
 * 类型安全的实现：
 * - 模板参数为 `ClientEntity`，直接消费客户端实体的元数据镜像字段
 * - 从 `ClientEntity::endermanHeldBlockState()` 获取方块状态
 * - 当 `endermanHeldBlockState()` 返回非空时渲染手持方块
 *
 * 数据来源：
 * - 服务端 `EndermanEntity::DATA_CARRIED_BLOCK_STATE_ID_PARAM`（i32 stateId）
 * - `EntityTracker` 自动广播 `EntityMetadataPacket`
 * - 客户端 `ClientEntity::syncMetadataFromDataManager` 读取 stateId，
 *   通过 `BlockRegistry::getBlockState(stateId)` 解析为 `BlockState*`，
 *   缓存到 `m_endermanHeldBlockState` 镜像字段
 *
 * 网格构建：
 * - 从 `BlockModelCache::getBlockAppearance()` 获取方块外观，遍历 `elements` 构建真实方块网格
 * - 复用 `ElementRotation.hpp` 中的旋转矩阵和 UV 旋转工具
 * - 按 `BlockState*` 缓存网格，避免重复构建
 *
 * 纹理图集切换：
 * - 方块纹理 UV 基于 blocks atlas，而非实体纹理图集
 * - 渲染前通过 `EntityPipeline::setTextureAtlas` 切换到 blocks atlas
 * - 渲染后通过 `EntityPipeline::setTextureAtlas` 恢复为实体纹理图集
 * - blocks atlas 句柄通过 `setBlockAtlas` 注入（来自 AtlasManager 的 blocks atlas）
 * - 实体纹理图集引用通过 `setEntityTextureAtlas` 注入（来自 `EntityRendererManager::textureAtlas()`）
 *
 * 调用链：
 * `EntityRendererManager::renderWithPipeline`
 *   → `EndermanRenderer::renderLayersPipelineClient(ClientEntity&, cmd, context, pipeline)`
 *     → `HeldBlockLayer::shouldRender(ClientEntity&)` 检查 endermanHeldBlockState() != nullptr
 *     → `HeldBlockLayer::renderPipeline(ClientEntity&, cmd, context, pipeline)`
 *       → `_getHeldBlock(entity)` 读取 endermanHeldBlockState()
 *       → `_getOrCreateBlockMesh(pipeline, blockState)` 获取/构建方块网格
 *       → `_renderBlockPipeline(...)` 应用 MC 1.21.11 CarriedBlockLayer 变换链并绘制
 */
class HeldBlockLayer : public core::LayerRenderer<::mc::client::ClientEntity> {
public:
    HeldBlockLayer() = default;
    ~HeldBlockLayer() override = default;

    /**
     * @brief 渲染方块持有层（GPU管线路径）
     */
    void renderPipeline(::mc::client::ClientEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

    /**
     * @brief 检查是否应该渲染持有的方块
     */
    [[nodiscard]] bool shouldRender(const ::mc::client::ClientEntity& entity) const override;

    /**
     * @brief 设置方块图集（blocks atlas 的 GPU 句柄）引用
     *
     * 用于在渲染时切换 EntityPipeline 的纹理图集到 blocks atlas，
     * 以便正确采样方块纹理 UV。
     *
     * @param imageView blocks atlas 的图像视图（VK_NULL_HANDLE 表示未注入，此时跳过渲染）
     * @param sampler   blocks atlas 的采样器
     */
    void setBlockAtlas(VkImageView imageView, VkSampler sampler)
    {
        m_blockImageView = imageView;
        m_blockSampler = sampler;
    }

    /**
     * @brief 设置实体纹理图集引用
     *
     * 用于在渲染方块层后恢复 EntityPipeline 的纹理图集到实体纹理图集，
     * 避免污染后续实体渲染。
     *
     * @param atlas 实体纹理图集指针（可为 nullptr 表示未注入，此时不恢复）
     */
    void setEntityTextureAtlas(const pipeline::EntityTextureAtlas* atlas) { m_entityTextureAtlas = atlas; }

private:
    /**
     * @brief 获取实体持有的方块状态
     */
    [[nodiscard]] static const ::mc::BlockState* _getHeldBlock(const ::mc::client::ClientEntity& entity);

    /**
     * @brief 渲染持有的方块（GPU管线路径）
     *
     * 复刻 MC 1.21.11 CarriedBlockLayer.submit() 的完整变换链：
     *   translate(0, 0.6875, -0.75)
     *   rotateX(20°)
     *   rotateY(45°)
     *   translate(0.25, 0.1875, 0.25)
     *   scale(-0.5, -0.5, 0.5)
     *   rotateY(90°)
     *
     * 实体位置从 entity.x()/y()/z() 获取，作为 drawMesh 的 position 参数。
     *
     * @param entity 实体（用于获取位置）
     * @param blockState 方块状态
     * @param x 方块相对实体的 X 偏移（MC 原版为 0）
     * @param y 方块相对实体的 Y 偏移（MC 原版为 0.6875）
     * @param z 方块相对实体的 Z 偏移（MC 原版为 -0.75）
     * @param cmd 命令缓冲区
     * @param context 动画上下文
     * @param pipeline 实体管线
     */
    void _renderBlockPipeline(::mc::client::ClientEntity& entity,
        const ::mc::BlockState& blockState,
        f32 x,
        f32 y,
        f32 z,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline);

    /**
     * @brief 获取或创建方块网格
     *
     * 按 BlockState 指针缓存网格，避免重复构建。方块状态指针在项目中是稳定的
     * （来自 BlockRegistry），可作为缓存 key。网格构建委托给
     * util::BlockMeshBuilder::buildBlockMesh。
     *
     * @param pipeline 实体管线（用于创建 GPU 缓冲区）
     * @param blockState 方块状态
     * @return 网格指针，失败返回 nullptr
     */
    [[nodiscard]] pipeline::EntityMesh* _getOrCreateBlockMesh(
        pipeline::EntityPipeline& pipeline, const ::mc::BlockState& blockState);

    // blocks atlas 的 GPU 句柄（弱引用，由 EndermanRenderer 通过 setBlockAtlas 注入）
    VkImageView m_blockImageView = VK_NULL_HANDLE;
    VkSampler m_blockSampler = VK_NULL_HANDLE;

    // 实体纹理图集引用（弱引用，由 EndermanRenderer 通过 setEntityTextureAtlas 注入）
    // 渲染方块后用于恢复 EntityPipeline 的纹理图集到实体纹理图集
    const pipeline::EntityTextureAtlas* m_entityTextureAtlas = nullptr;

    // 按 BlockState 指针缓存的网格
    // key 为 BlockState*（项目中方块状态指针稳定，来自 BlockRegistry）
    std::unordered_map<const ::mc::BlockState*, std::unique_ptr<pipeline::EntityMesh>> m_blockMeshCache;
};

} // namespace mc::client::renderer::entity::layer::entity
