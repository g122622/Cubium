/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, the subject to the conditions:
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

#include "client/renderer/trident/entity/core/EntityRenderer.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <vector>

namespace mc::client {
class ClientEntity;
}

namespace mc::client::renderer::entity::renderer::projectile {

/**
 * @brief 钓鱼浮标渲染器
 *
 * 渲染钓鱼浮标和钓线。
 * 浮标使用 billboard 四边形渲染，钓线使用 LINE_LIST 拓扑渲染。
 * 参考 MC 1.16.5 FishingBobberEntity / FishRenderer
 */
class FishingBobberRenderer : public core::EntityRenderer, public core::PipelineMeshProvider {
public:
    FishingBobberRenderer();
    ~FishingBobberRenderer() override = default;

    void render(Entity& entity, f64 partialTicks) override;

    // PipelineMeshProvider 接口
    [[nodiscard]] bool generateMesh(
        ::mc::client::ClientEntity& entity, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices) override;

    [[nodiscard]] bool needsMeshUpdate(::mc::client::ClientEntity& entity) const override;

    [[nodiscard]] VkPrimitiveTopology getTopology() const override
    {
        return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    }

private:
    /**
     * @brief 生成浮标四边形顶点
     * @param vertices 输出顶点数组
     * @param indices 输出索引数组
     */
    void _generateBobberQuad(std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices);

    /**
     * @brief 生成钓鱼线顶点
     *
     * 从浮标位置到玩家手持位置生成16段抛物线线段。
     * 参考 MC 1.16.5 FishRenderer 的线段算法。
     *
     * @param bobberPos 浮标世界位置
     * @param playerHandPos 玩家手持位置
     * @param vertices 输出顶点数组
     * @param indices 输出索引数组
     */
    void _generateFishingLine(const Vector3f& bobberPos,
        const Vector3f& playerHandPos,
        std::vector<model::ModelVertex>& vertices,
        std::vector<u32>& indices);
};

} // namespace mc::client::renderer::entity::renderer::projectile
