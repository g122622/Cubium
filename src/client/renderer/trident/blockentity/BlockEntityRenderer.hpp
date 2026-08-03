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

#include "IBlockEntityRenderer.hpp"
#include "client/renderer/api/texture/TextureRegion.hpp"
#include "common/core/Types.hpp"
#include "common/world/block/BlockPos.hpp"
#include <memory>

namespace mc {

class BlockState;
class IWorld;
class MatrixStack;
class VertexBuffer;
class BlockModelCache;

namespace client::renderer::trident {

class TridentTextureAtlas;

namespace blockentity {

/**
 * @brief 方块实体渲染器辅助类
 *
 * 提供方块实体渲染的通用功能，如方块模型渲染、光照获取等。
 * 具体渲染器应继承 BlockEntityRenderer<TEntity> 并使用此类的辅助方法。
 */
class BlockEntityRendererHelper {
public:
    BlockEntityRendererHelper();
    ~BlockEntityRendererHelper();

    // 禁止拷贝
    BlockEntityRendererHelper(const BlockEntityRendererHelper&) = delete;
    BlockEntityRendererHelper& operator=(const BlockEntityRendererHelper&) = delete;

    // 允许移动
    BlockEntityRendererHelper(BlockEntityRendererHelper&&) noexcept = default;
    BlockEntityRendererHelper& operator=(BlockEntityRendererHelper&&) noexcept = default;

    /**
     * @brief 渲染方块模型
     *
     * 在指定位置渲染方块的默认模型。
     *
     * @param state 方块状态
     * @param pos 方块位置
     * @param light 组合光照
     * @return 是否渲染成功
     */
    [[nodiscard]] bool renderBlock(const BlockState& state, const BlockPos& pos, u32 light);

    /**
     * @brief 渲染方块模型（带变换）
     *
     * @param state 方块状态
     * @param pos 方块位置
     * @param offsetX X偏移
     * @param offsetY Y偏移
     * @param offsetZ Z偏移
     * @param light 组合光照
     * @return 是否渲染成功
     */
    [[nodiscard]] bool renderBlockWithOffset(
        const BlockState& state, const BlockPos& pos, f32 offsetX, f32 offsetY, f32 offsetZ, u32 light);

    /**
     * @brief 获取方块在指定位置的光照值
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @return 组合光照值（天空光 << 20 | 方块光 << 4）
     */
    [[nodiscard]] static u32 getLightAt(IWorld& world, const BlockPos& pos);

    /**
     * @brief 获取方块外观缓存
     */
    [[nodiscard]] BlockModelCache* modelCache() const { return m_modelCache; }

    /**
     * @brief 获取纹理图集
     */
    [[nodiscard]] TridentTextureAtlas* textureAtlas() const { return m_textureAtlas; }

    /**
     * @brief 设置模型缓存
     */
    void setModelCache(BlockModelCache* cache) { m_modelCache = cache; }

    /**
     * @brief 设置纹理图集
     */
    void setTextureAtlas(TridentTextureAtlas* atlas) { m_textureAtlas = atlas; }

private:
    BlockModelCache* m_modelCache = nullptr;
    TridentTextureAtlas* m_textureAtlas = nullptr;
};

} // namespace blockentity
} // namespace client::renderer::trident
} // namespace mc
