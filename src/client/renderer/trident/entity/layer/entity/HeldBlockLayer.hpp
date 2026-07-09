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
#include "common/core/Types.hpp"
#include "common/util/math/Vector4.hpp"
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

namespace mc {
class LivingEntity;
class BlockState;
} // namespace mc

namespace mc::client {
struct ChunkTextureAtlas;
} // namespace mc::client

namespace mc::client::renderer::entity::pipeline {
class EntityPipeline;
struct EntityMesh;
} // namespace mc::client::renderer::entity::pipeline

namespace mc::client::renderer::entity::layer::entity {

/**
 * @brief 方块持有层渲染器
 *
 * 渲染末影人手持的方块。对应 MC 1.21.11 CarriedBlockLayer（1.16.5 为 HeldBlockLayer）。
 *
 * 类型安全的实现：
 * - 使用 `if constexpr` + `std::is_base_of_v` 进行编译时类型检查
 * - 只有 `EndermanEntity` 有手持方块功能
 * - 从 `EndermanEntity::getHeldBlockState()` 获取方块状态
 * - 从 `EndermanEntity::isHoldingBlock()` 判断是否渲染
 *
 * 网格构建：
 * - 从 `BlockModelCache::getBlockAppearance()` 获取方块外观，遍历 `elements` 构建真实方块网格
 * - 复用 `ElementRotation.hpp` 中的旋转矩阵和 UV 旋转工具
 * - 按 `BlockState*` 缓存网格，避免重复构建
 *
 * 纹理图集切换：
 * - 方块纹理 UV 基于方块纹理图集（ChunkTextureAtlas），而非实体纹理图集
 * - 渲染前通过 `EntityPipeline::setTextureAtlas` 切换到方块纹理图集，渲染后切回
 * - `ChunkTextureAtlas` 引用通过 `setChunkTextureAtlas` 注入
 *
 * @tparam TEntity 实体类型
 */
template <typename TEntity>
class HeldBlockLayer : public core::LayerRenderer<TEntity> {
public:
    HeldBlockLayer() = default;
    ~HeldBlockLayer() override = default;

    /**
     * @brief 渲染方块持有层（GPU管线路径）
     */
    void renderPipeline(TEntity& entity,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline) override;

    /**
     * @brief 检查是否应该渲染持有的方块
     */
    [[nodiscard]] bool shouldRender(const TEntity& entity) const override;

    /**
     * @brief 设置方块纹理图集引用
     *
     * 用于在渲染时切换 EntityPipeline 的纹理图集到方块纹理图集，
     * 以便正确采样方块纹理 UV。
     *
     * @param atlas 方块纹理图集指针（可为 nullptr 表示未注入，此时仅使用默认图集）
     */
    void setChunkTextureAtlas(const ::mc::client::ChunkTextureAtlas* atlas) { m_chunkTextureAtlas = atlas; }

private:
    /**
     * @brief 获取实体持有的方块状态
     */
    [[nodiscard]] const ::mc::BlockState* _getHeldBlock(const TEntity& entity) const;

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
    void _renderBlockPipeline(TEntity& entity,
        const ::mc::BlockState& blockState,
        f32 x,
        f32 y,
        f32 z,
        VkCommandBuffer cmd,
        const mc::client::renderer::entity::core::AnimationContext& context,
        pipeline::EntityPipeline& pipeline);

    /**
     * @brief 从方块模型构建网格
     *
     * 遍历 BlockAppearance.elements 中的每个 ModelElement，对每个面生成
     * 4 顶点 + 6 索引。顶点坐标基于 element.from/to（0-16 范围）乘以 1/16 转换为世界单位。
     * UV 使用方块纹理图集中的 TextureRegion。支持元素旋转和 UV 旋转。
     *
     * 若无法获取方块外观，回退到简单立方体网格。
     *
     * @param blockState 方块状态
     * @param vertices 输出顶点
     * @param indices 输出索引
     */
    void _buildBlockMesh(
        const ::mc::BlockState& blockState, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);

    /**
     * @brief 构建回退用的简单立方体网格
     *
     * 当无法从 BlockModelCache 获取方块外观时使用。
     */
    void _buildFallbackCubeMesh(std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);

    /**
     * @brief 获取或创建方块网格
     *
     * 按 BlockState 指针缓存网格，避免重复构建。方块状态指针在项目中是稳定的
     * （来自 BlockRegistry），可作为缓存 key。
     *
     * @param pipeline 实体管线（用于创建 GPU 缓冲区）
     * @param blockState 方块状态
     * @return 网格指针，失败返回 nullptr
     */
    [[nodiscard]] pipeline::EntityMesh* _getOrCreateBlockMesh(
        pipeline::EntityPipeline& pipeline, const ::mc::BlockState& blockState);

    /**
     * @brief 销毁所有缓存的网格
     *
     * 在管线销毁前调用，释放 GPU 资源。
     */
    void _destroyCachedMeshes(pipeline::EntityPipeline& pipeline);

    // 方块纹理图集引用（弱引用，由外部 EntityRendererManager 注入）
    const ::mc::client::ChunkTextureAtlas* m_chunkTextureAtlas = nullptr;

    // 按 BlockState 指针缓存的网格
    // key 为 BlockState*（项目中方块状态指针稳定，来自 BlockRegistry）
    std::unordered_map<const ::mc::BlockState*, std::unique_ptr<pipeline::EntityMesh>> m_blockMeshCache;
};

} // namespace mc::client::renderer::entity::layer::entity
