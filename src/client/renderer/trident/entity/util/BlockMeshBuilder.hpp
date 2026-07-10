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

#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/core/Types.hpp"
#include <vector>

namespace mc {
class BlockState;
} // namespace mc

namespace mc::client::renderer::entity::util {

/**
 * @brief 方块网格构建工具
 *
 * 从 BlockState 构建 GPU 管线可用的方块网格（顶点 + 索引）。
 *
 * 提取自 HeldBlockLayer::_buildBlockMesh，供以下场景复用：
 * - HeldBlockLayer：末影人手持方块（MC 1.21.11 CarriedBlockLayer）
 * - FallingBlockRenderer：下落方块渲染（MC 1.21.11 FallingBlockRenderer）
 * - TNTRenderer：TNT 实体方块渲染（MC 1.21.11 TntRenderer）
 *
 * 网格构建流程：
 * 1. 从 BlockModelCache::getBlockAppearance() 获取方块外观
 * 2. 遍历 BlockAppearance::elements 中的每个 ModelElement
 * 3. 对每个面生成 4 顶点 + 6 索引（两个三角形）
 * 4. 顶点坐标基于 element.from/to（0-16 像素范围）乘以 1/16 转换为世界单位
 * 5. UV 使用方块纹理图集中的 TextureRegion
 * 6. 支持元素旋转（buildElementRotationMatrix）和 UV 旋转（getRotatedUV）
 *
 * 若无法获取方块外观，回退到简单立方体网格（单位 0-1 范围，UV 0-1 全图）。
 *
 * 对应 MC 1.21.11 中 ModelBlockRenderer.render / FaceBakery.bakeQuad 的网格构建逻辑。
 */
class BlockMeshBuilder {
public:
    /**
     * @brief 从方块状态构建网格
     *
     * @param blockState 方块状态
     * @param vertices 输出顶点数组
     * @param indices 输出索引数组
     */
    static void buildBlockMesh(
        const ::mc::BlockState& blockState, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);

    /**
     * @brief 构建回退用的简单立方体网格
     *
     * 当无法从 BlockModelCache 获取方块外观时使用。
     * 生成单位立方体（0-1 范围），UV 使用 0-1 全图。
     */
    static void buildFallbackCubeMesh(std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);
};

} // namespace mc::client::renderer::entity::util
