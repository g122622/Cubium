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

#include "BlockMeshBuilder.hpp"
#include "client/renderer/trident/chunk/ChunkMesher.hpp"
#include "client/renderer/trident/item/ElementRotation.hpp"
#include "client/resource/BlockModelCache.hpp"
#include "client/resource/ResourceManager.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/Block.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity::util {

// ---------------------------------------------------------------------------
// 辅助：方向 -> 面名（与 BlockAppearance::faceTextures 的键一致）
// ---------------------------------------------------------------------------
static const char* directionToFaceName(Direction dir)
{
    switch (dir) {
        case Direction::Down:
            return "down";
        case Direction::Up:
            return "up";
        case Direction::North:
            return "north";
        case Direction::South:
            return "south";
        case Direction::West:
            return "west";
        case Direction::East:
            return "east";
        default:
            return nullptr;
    }
}

// ---------------------------------------------------------------------------
// 辅助：从 BlockAppearance 查找指定方向的纹理区域（带 side/all 回退）
// 复刻 ChunkMesher::collectFaceLayers 的查找逻辑
// ---------------------------------------------------------------------------
static const TextureRegion* resolveFaceTexture(const BlockAppearance& appearance, Direction dir)
{
    const char* name = directionToFaceName(dir);
    if (name == nullptr) {
        return nullptr;
    }

    auto it = appearance.faceTextures.find(name);
    if (it != appearance.faceTextures.end()) {
        return &it->second;
    }
    it = appearance.faceTextures.find("side");
    if (it != appearance.faceTextures.end()) {
        return &it->second;
    }
    it = appearance.faceTextures.find("all");
    if (it != appearance.faceTextures.end()) {
        return &it->second;
    }
    return nullptr;
}

void BlockMeshBuilder::buildBlockMesh(
    const ::mc::BlockState& blockState, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    vertices.clear();
    indices.clear();

    // 从 BlockModelCache 获取方块外观
    BlockModelCache* modelCache = ChunkMesher::modelCache();
    if (modelCache == nullptr) {
        spdlog::warn("BlockMeshBuilder: BlockModelCache is null, falling back to cube mesh");
        buildFallbackCubeMesh(vertices, indices);
        return;
    }

    const BlockAppearance* appearance = modelCache->getBlockAppearance(&blockState);
    if (appearance == nullptr || appearance->elements.empty()) {
        buildFallbackCubeMesh(vertices, indices);
        return;
    }

    // 像素坐标 -> 世界单位的缩放因子（MC 模型坐标 0-16 对应方块 0-1）
    constexpr f64 SCALE = 1.0 / 16.0;

    for (const auto& element : appearance->elements) {
        const f64 x1 = static_cast<f64>(element.from.x) * SCALE;
        const f64 y1 = static_cast<f64>(element.from.y) * SCALE;
        const f64 z1 = static_cast<f64>(element.from.z) * SCALE;
        const f64 x2 = static_cast<f64>(element.to.x) * SCALE;
        const f64 y2 = static_cast<f64>(element.to.y) * SCALE;
        const f64 z2 = static_cast<f64>(element.to.z) * SCALE;

        // 构建元素旋转矩阵（若有旋转），参考 MC FaceBakery.bakeQuad()
        const bool hasRotation = !element.rotation.isIdentity();
        glm::mat4 rotationMatrix = glm::mat4(1.0f);
        if (hasRotation) {
            rotationMatrix = item::buildElementRotationMatrix(element.rotation, SCALE);
        }

        // 为每个面生成顶点
        for (const auto& [dir, face] : element.faces) {
            // 从 BlockAppearance 的 faceTextures 中查找该方向的纹理区域
            // （face.texture 是纹理变量名如 "#all"，需要解析为图集中的 TextureRegion）
            const TextureRegion* region = resolveFaceTexture(*appearance, dir);
            if (region == nullptr) {
                continue;
            }

            // 将图集的 TextureRegion（f64）转换为 0-1 UV 范围
            const f32 u0 = static_cast<f32>(region->u0);
            const f32 v0 = static_cast<f32>(region->v0);
            const f32 u1 = static_cast<f32>(region->u1);
            const f32 v1 = static_cast<f32>(region->v1);

            glm::vec3 normal;
            std::array<glm::vec3, 4> corners;

            switch (dir) {
                case Direction::North: // Z-
                    normal = glm::vec3(0.0f, 0.0f, -1.0f);
                    corners = {
                        glm::vec3(x1, y1, z1), glm::vec3(x2, y1, z1), glm::vec3(x2, y2, z1), glm::vec3(x1, y2, z1)};
                    break;
                case Direction::South: // Z+
                    normal = glm::vec3(0.0f, 0.0f, 1.0f);
                    corners = {
                        glm::vec3(x2, y1, z2), glm::vec3(x1, y1, z2), glm::vec3(x1, y2, z2), glm::vec3(x2, y2, z2)};
                    break;
                case Direction::West: // X-
                    normal = glm::vec3(-1.0f, 0.0f, 0.0f);
                    corners = {
                        glm::vec3(x1, y1, z2), glm::vec3(x1, y1, z1), glm::vec3(x1, y2, z1), glm::vec3(x1, y2, z2)};
                    break;
                case Direction::East: // X+
                    normal = glm::vec3(1.0f, 0.0f, 0.0f);
                    corners = {
                        glm::vec3(x2, y1, z1), glm::vec3(x2, y1, z2), glm::vec3(x2, y2, z2), glm::vec3(x2, y2, z1)};
                    break;
                case Direction::Down: // Y-
                    normal = glm::vec3(0.0f, -1.0f, 0.0f);
                    corners = {
                        glm::vec3(x1, y1, z2), glm::vec3(x2, y1, z2), glm::vec3(x2, y1, z1), glm::vec3(x1, y1, z1)};
                    break;
                case Direction::Up: // Y+
                    normal = glm::vec3(0.0f, 1.0f, 0.0f);
                    corners = {
                        glm::vec3(x1, y2, z1), glm::vec3(x2, y2, z1), glm::vec3(x2, y2, z2), glm::vec3(x1, y2, z2)};
                    break;
                default:
                    continue;
            }

            // 应用元素旋转到顶点位置
            if (hasRotation) {
                for (auto& corner : corners) {
                    glm::vec4 pos(corner, 1.0f);
                    pos = rotationMatrix * pos;
                    corner = glm::vec3(pos);
                }
            }

            // 旋转后重算法线（参考 MC FaceBakery.calculateFacing()）
            if (hasRotation) {
                const glm::vec3 edge1 = corners[1] - corners[0];
                const glm::vec3 edge2 = corners[2] - corners[0];
                normal = glm::normalize(glm::cross(edge1, edge2));
            }

            // 添加顶点，使用 UV 旋转排列
            const u32 faceBase = static_cast<u32>(vertices.size());
            for (int i = 0; i < 4; ++i) {
                auto [u, v] = item::getRotatedUV(i, face.uv.rotation, u0, v0, u1, v1);
                vertices.push_back(
                    model::ModelVertex(corners[i].x, corners[i].y, corners[i].z, u, v, normal.x, normal.y, normal.z));
            }

            // 添加索引（两个三角形）
            indices.push_back(faceBase + 0);
            indices.push_back(faceBase + 1);
            indices.push_back(faceBase + 2);
            indices.push_back(faceBase + 0);
            indices.push_back(faceBase + 2);
            indices.push_back(faceBase + 3);
        }
    }

    // 若构建失败（所有面都被跳过），回退到立方体
    if (vertices.empty() || indices.empty()) {
        buildFallbackCubeMesh(vertices, indices);
    }
}

void BlockMeshBuilder::buildFallbackCubeMesh(std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    vertices.clear();
    indices.clear();

    // 单位立方体（0-1 范围），UV 使用 0-1 全图
    constexpr f32 x1 = 0.0f, y1 = 0.0f, z1 = 0.0f;
    constexpr f32 x2 = 1.0f, y2 = 1.0f, z2 = 1.0f;
    const f32 u0 = 0.0f, v0 = 0.0f, u1 = 1.0f, v1 = 1.0f;

    // 北面 (Z-) - 法线 (0, 0, -1)
    u32 base = static_cast<u32>(vertices.size());
    vertices.push_back(model::ModelVertex(x1, y1, z1, u0, v1, 0.0f, 0.0f, -1.0f));
    vertices.push_back(model::ModelVertex(x2, y1, z1, u1, v1, 0.0f, 0.0f, -1.0f));
    vertices.push_back(model::ModelVertex(x2, y2, z1, u1, v0, 0.0f, 0.0f, -1.0f));
    vertices.push_back(model::ModelVertex(x1, y2, z1, u0, v0, 0.0f, 0.0f, -1.0f));
    indices.push_back(base + 0);
    indices.push_back(base + 1);
    indices.push_back(base + 2);
    indices.push_back(base + 0);
    indices.push_back(base + 2);
    indices.push_back(base + 3);

    // 南面 (Z+) - 法线 (0, 0, 1)
    base = static_cast<u32>(vertices.size());
    vertices.push_back(model::ModelVertex(x2, y1, z2, u0, v1, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(x1, y1, z2, u1, v1, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(x1, y2, z2, u1, v0, 0.0f, 0.0f, 1.0f));
    vertices.push_back(model::ModelVertex(x2, y2, z2, u0, v0, 0.0f, 0.0f, 1.0f));
    indices.push_back(base + 0);
    indices.push_back(base + 1);
    indices.push_back(base + 2);
    indices.push_back(base + 0);
    indices.push_back(base + 2);
    indices.push_back(base + 3);

    // 西面 (X-) - 法线 (-1, 0, 0)
    base = static_cast<u32>(vertices.size());
    vertices.push_back(model::ModelVertex(x1, y1, z2, u0, v1, -1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(x1, y1, z1, u1, v1, -1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(x1, y2, z1, u1, v0, -1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(x1, y2, z2, u0, v0, -1.0f, 0.0f, 0.0f));
    indices.push_back(base + 0);
    indices.push_back(base + 1);
    indices.push_back(base + 2);
    indices.push_back(base + 0);
    indices.push_back(base + 2);
    indices.push_back(base + 3);

    // 东面 (X+) - 法线 (1, 0, 0)
    base = static_cast<u32>(vertices.size());
    vertices.push_back(model::ModelVertex(x2, y1, z1, u0, v1, 1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(x2, y1, z2, u1, v1, 1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(x2, y2, z2, u1, v0, 1.0f, 0.0f, 0.0f));
    vertices.push_back(model::ModelVertex(x2, y2, z1, u0, v0, 1.0f, 0.0f, 0.0f));
    indices.push_back(base + 0);
    indices.push_back(base + 1);
    indices.push_back(base + 2);
    indices.push_back(base + 0);
    indices.push_back(base + 2);
    indices.push_back(base + 3);

    // 底面 (Y-) - 法线 (0, -1, 0)
    base = static_cast<u32>(vertices.size());
    vertices.push_back(model::ModelVertex(x1, y1, z2, u0, v1, 0.0f, -1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(x2, y1, z2, u1, v1, 0.0f, -1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(x2, y1, z1, u1, v0, 0.0f, -1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(x1, y1, z1, u0, v0, 0.0f, -1.0f, 0.0f));
    indices.push_back(base + 0);
    indices.push_back(base + 1);
    indices.push_back(base + 2);
    indices.push_back(base + 0);
    indices.push_back(base + 2);
    indices.push_back(base + 3);

    // 顶面 (Y+) - 法线 (0, 1, 0)
    base = static_cast<u32>(vertices.size());
    vertices.push_back(model::ModelVertex(x1, y2, z1, u0, v1, 0.0f, 1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(x2, y2, z1, u1, v1, 0.0f, 1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(x2, y2, z2, u1, v0, 0.0f, 1.0f, 0.0f));
    vertices.push_back(model::ModelVertex(x1, y2, z2, u0, v0, 0.0f, 1.0f, 0.0f));
    indices.push_back(base + 0);
    indices.push_back(base + 1);
    indices.push_back(base + 2);
    indices.push_back(base + 0);
    indices.push_back(base + 2);
    indices.push_back(base + 3);
}

} // namespace mc::client::renderer::entity::util
