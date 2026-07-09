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
#include "client/renderer/trident/item/ElementRotation.hpp"
#include "client/resource/BlockModelCache.hpp"
#include "client/resource/ResourceManager.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/monster/end/EndermanEntity.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/Block.hpp"
#include <memory>
#include <type_traits>
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

template <typename TEntity>
void HeldBlockLayer<TEntity>::renderPipeline(TEntity& entity,
    VkCommandBuffer cmd,
    const mc::client::renderer::entity::core::AnimationContext& context,
    pipeline::EntityPipeline& pipeline)
{
    // TODO: 调用链集成问题——当前 HeldBlockLayer<LivingEntity> 实例虽在
    // EndermanRenderer::_setupLayers() 中注册，但 EntityRendererManager::renderWithPipeline
    // 调用的是 renderer->renderLayersPipelineClient(ClientEntity&, ...)，而
    // LivingRenderer 仅重写了 renderLayersPipeline(Entity&, ...)（服务端实体版本），
    // 未重写 renderLayersPipelineClient。因此本层目前不会被实际调用。
    //
    // 彻底解决需要以下之一：
    //   1) 让 LivingRenderer 重写 renderLayersPipelineClient，并从 ClientEntity 重建
    //      LivingEntity 视图（需要 ClientEntity 暴露 heldBlockState 等末影人字段，
    //      并在网络层同步该元数据）；
    //   2) 将 HeldBlockLayer 迁移为 ClientEntity 模板，直接消费 ClientEntity 的
    //      末影人持有方块元数据（需先在 ClientEntity 中加入对应字段与网络同步）。
    //
    // 在上述架构调整完成前，本层的内部实现（网格构建、变换链、纹理图集切换）已就绪，
    // 一旦调用链打通即可正确渲染末影人手持方块。

    // 获取持有的方块
    const ::mc::BlockState* blockState = _getHeldBlock(entity);
    if (!blockState) {
        return;
    }

    // 末影人持有方块的位置（MC 原版偏移：y=0.6875, z=-0.75，这里 x/y/z 作为相对实体的偏移）
    _renderBlockPipeline(entity, *blockState, 0.0f, 0.6875f, -0.75f, cmd, context, pipeline);
}

template <typename TEntity>
bool HeldBlockLayer<TEntity>::shouldRender(const TEntity& entity) const
{
    // 运行时类型检查：HeldBlockLayer<LivingEntity> 注册在 EndermanRenderer 上，
    // 运行时传入的实体实际上是 EndermanEntity（派生自 LivingEntity）。
    // 编译时 TEntity == LivingEntity，无法直接调用 EndermanEntity 接口，
    // 因此使用 dynamic_cast 进行安全的向下转型。
    if constexpr (std::is_base_of_v<::mc::EndermanEntity, TEntity>) {
        // 编译时即为 EndermanEntity（或其派生），直接调用
        return entity.isHoldingBlock();
    } else if constexpr (std::is_base_of_v<TEntity, ::mc::EndermanEntity>) {
        // TEntity 是 EndermanEntity 的基类（如 LivingEntity），运行时向下转型
        const auto* enderman = dynamic_cast<const ::mc::EndermanEntity*>(&entity);
        return enderman != nullptr && enderman->isHoldingBlock();
    }
    return false;
}

template <typename TEntity>
const ::mc::BlockState* HeldBlockLayer<TEntity>::_getHeldBlock(const TEntity& entity) const
{
    // 运行时类型检查（同 shouldRender 的逻辑）
    if constexpr (std::is_base_of_v<::mc::EndermanEntity, TEntity>) {
        return entity.getHeldBlockState();
    } else if constexpr (std::is_base_of_v<TEntity, ::mc::EndermanEntity>) {
        const auto* enderman = dynamic_cast<const ::mc::EndermanEntity*>(&entity);
        if (enderman != nullptr) {
            return enderman->getHeldBlockState();
        }
    }
    return nullptr;
}

template <typename TEntity>
void HeldBlockLayer<TEntity>::_renderBlockPipeline(TEntity& entity,
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
    if (!mesh || mesh->indexCount == 0) {
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

    // 切换到方块纹理图集（方块 UV 基于区块纹理图集，而非实体纹理图集）
    VkImageView savedImageView = VK_NULL_HANDLE;
    VkSampler savedSampler = VK_NULL_HANDLE;
    const bool needAtlasSwitch = (m_chunkTextureAtlas != nullptr && m_chunkTextureAtlas->isValid);
    if (needAtlasSwitch) {
        // 保存当前图集并在渲染后恢复（EntityPipeline 未提供 getter，此处依赖
        // 渲染调用者会在下一帧/下一次绑定前重置图集；为安全起见，我们在
        // 渲染完成后将图集切回实体纹理图集——通过传入空句柄让上层重绑）。
        // 实际上 EntityPipeline 的纹理描述符集是全局共享的，切换后需要恢复。
        // 这里采用"切换-渲染-恢复"模式，但因为没有 getter，我们只能切换。
        // 上层 EntityRendererManager 在每次 renderWithPipeline 开始时会重新
        // 绑定实体纹理图集，所以此处切换不会污染后续帧。
        pipeline.setTextureAtlas(m_chunkTextureAtlas->imageView, m_chunkTextureAtlas->sampler);
    }
    (void)savedImageView;
    (void)savedSampler;

    pipeline.drawMesh(cmd, *mesh, blockTransform, entityPos, 1.0, overlayColor, 0.0f, 0.0f);

    (void)context;
}

template <typename TEntity>
void HeldBlockLayer<TEntity>::_buildBlockMesh(
    const ::mc::BlockState& blockState, std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
{
    vertices.clear();
    indices.clear();

    // 从 BlockModelCache 获取方块外观
    BlockModelCache* modelCache = ChunkMesher::modelCache();
    if (modelCache == nullptr) {
        spdlog::warn("HeldBlockLayer: BlockModelCache is null, falling back to cube mesh");
        _buildFallbackCubeMesh(vertices, indices);
        return;
    }

    const BlockAppearance* appearance = modelCache->getBlockAppearance(&blockState);
    if (appearance == nullptr || appearance->elements.empty()) {
        _buildFallbackCubeMesh(vertices, indices);
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
        _buildFallbackCubeMesh(vertices, indices);
    }
}

template <typename TEntity>
void HeldBlockLayer<TEntity>::_buildFallbackCubeMesh(
    std::vector<model::ModelVertex>& vertices, std::vector<u32>& indices)
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

template <typename TEntity>
pipeline::EntityMesh* HeldBlockLayer<TEntity>::_getOrCreateBlockMesh(
    pipeline::EntityPipeline& pipeline, const ::mc::BlockState& blockState)
{
    // 按 BlockState 指针缓存网格（项目中方块状态指针来自 BlockRegistry，是稳定的）
    const auto it = m_blockMeshCache.find(&blockState);
    if (it != m_blockMeshCache.end() && it->second && it->second->indexCount > 0) {
        return it->second.get();
    }

    // 构建方块网格
    std::vector<model::ModelVertex> vertices;
    std::vector<u32> indices;
    _buildBlockMesh(blockState, vertices, indices);

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

template <typename TEntity>
void HeldBlockLayer<TEntity>::_destroyCachedMeshes(pipeline::EntityPipeline& pipeline)
{
    (void)pipeline;
    m_blockMeshCache.clear();
}

// 显式实例化
template class HeldBlockLayer<::mc::LivingEntity>;
template class HeldBlockLayer<::mc::EndermanEntity>;

} // namespace mc::client::renderer::entity::layer::entity
