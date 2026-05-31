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

#include "EntityRendererManager.hpp"
#include "AnimatedMeshCache.hpp"
#include "client/renderer/trident/entity/effect/fire/FireEffect.hpp"
#include "client/renderer/trident/entity/model/ModelRegistration.hpp"
#include "client/renderer/trident/entity/model/animal/PolarBearModel.hpp"
#include "client/renderer/trident/entity/model/core/ModelFactory.hpp"
#include "client/renderer/trident/entity/pipeline/EntityTextureAtlas.hpp"
#include "client/renderer/trident/entity/renderer/RendererRegistration.hpp"
#include "client/renderer/trident/entity/renderer/projectile/ItemEntityRenderer.hpp"
#include "client/renderer/trident/entity/util/NameTagRenderer.hpp"
#include "client/renderer/trident/entity/util/ShadowRenderer.hpp"
#include "client/resource/EntityTextureLoader.hpp"
#include "client/resource/ItemTextureAtlas.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/orb/ExperienceOrbEntity.hpp"
#include "common/entity/experience/ExperienceUtils.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/perfetto/TraceEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector4.hpp"
#include <cmath>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity {

// 导入核心命名空间中的类
using core::EntityRenderer;
using model::ModelVertex;
using pipeline::EntityMesh;
using pipeline::EntityTextureAtlas;

namespace {

// 标准常量
inline constexpr f64 MODEL_Y_OFFSET = 1.501;
inline constexpr f64 MODEL_SCALE = 1.0 / 16.0;
inline constexpr f64 MODEL_MESH_SCALE = 1.0;

// 阴影最大距离
inline constexpr f64 SHADOW_MAX_DISTANCE = 256.0;

/**
 * @brief 规范化实体类型ID
 *
 * 将实体类型ID转换为标准格式（带命名空间前缀）
 * 例如："pig" -> "minecraft:pig", "minecraft:cow" -> "minecraft:cow"
 */
std::string normalizeEntityTypeId(const std::string& typeId)
{
    // 如果已有命名空间前缀，直接返回
    if (typeId.find(':') != std::string::npos) {
        return typeId;
    }
    // 添加默认命名空间
    return "minecraft:" + typeId;
}

} // anonymous namespace

EntityRendererManager::EntityRendererManager()
    : m_animatedMeshCache(std::make_unique<core::AnimatedMeshCache>())
{}

EntityRendererManager::~EntityRendererManager()
{
    // 销毁所有实体网格的Vulkan资源
    clearMeshes();
}

void EntityRendererManager::clearMeshes()
{
    if (m_pipeline) {
        for (auto& [id, mesh] : m_meshes) {
            m_pipeline->destroyMesh(mesh.mesh);
        }
    }
    m_meshes.clear();
}

void EntityRendererManager::setTextureAtlas(const EntityTextureAtlas* textureAtlas)
{
    m_textureAtlas = textureAtlas;
    // 图集变化后，旧网格的UV映射可能失效，强制重建
    clearMeshes();
}

void EntityRendererManager::setCameraInfo(
    const glm::dvec3& position, const glm::mat4& viewMatrix, const mc::math::frustum::Frustum& frustum)
{
    m_cameraPosition = position;
    m_viewMatrix = viewMatrix;
    m_frustum = frustum;
    m_hasCameraInfo = true;

    // 更新 NameTagRenderer 的相机信息
    util::NameTagRenderer::setCameraPosition(Vector3d(position.x, position.y, position.z));

    // 转换视图矩阵为 double 数组
    std::array<f64, 16> viewMatrixArray;
    for (i32 i = 0; i < 4; ++i) {
        for (i32 j = 0; j < 4; ++j) {
            viewMatrixArray[i * 4 + j] = static_cast<f64>(viewMatrix[j][i]);
        }
    }
    util::NameTagRenderer::setViewMatrix(viewMatrixArray);
    util::NameTagRenderer::setFrustum(frustum);
}

EntityRenderer* EntityRendererManager::getRenderer(const std::string& typeId)
{
    std::string normalizedId = normalizeEntityTypeId(typeId);
    auto it = m_renderers.find(normalizedId);
    if (it != m_renderers.end()) {
        return it->second.get();
    }
    return nullptr;
}

void EntityRendererManager::render(Entity& entity, f64 partialTicks)
{
    // 获取实体类型ID并查找渲染器（已在 _getOrCreateRenderer 中规范化）
    EntityRenderer* renderer = _getOrCreateRenderer(entity.getTypeId());
    if (renderer) {
        renderer->render(entity, partialTicks);
        if (m_renderShadows) {
            renderer->renderShadow(entity, partialTicks);
        }
        if (m_renderNameTags) {
            renderer->renderNameTag(entity);
        }
    }
}

void EntityRendererManager::renderWithPipeline(VkCommandBuffer cmd, ClientEntity& entity, f64 partialTicks)
{
    if (!m_pipeline) {
        return;
    }

    // 检查是否为 ItemEntity 或 ExperienceOrb
    std::string normalizedType = normalizeEntityTypeId(entity.typeId());
    bool isItemEntity = (normalizedType == ::mc::entity::EntityTypes::ITEM);
    bool isExperienceOrb = (normalizedType == ::mc::entity::EntityTypes::EXPERIENCE_ORB);
    bool useAnimatedMesh = _usesAnimatedMesh(normalizedType);

    // 对于 ItemEntity，使用 ItemTextureAtlas
    if (isItemEntity && m_itemTextureAtlas && m_itemTextureAtlas->isBuilt()) {
        // 绑定物品纹理图集
        m_pipeline->setTextureAtlas(m_itemTextureAtlas->imageView(), m_itemTextureAtlas->sampler());
    }

    // 获取渲染器
    EntityRenderer* renderer = _getOrCreateRenderer(entity.typeId());

    // 获取或创建网格
    EntityMesh* mesh = nullptr;
    core::AnimationContext context;

    if (useAnimatedMesh && renderer && renderer->supportsAnimation()) {
        // 动画实体路径：使用动画网格缓存
        // 设置 partialTicks（用于动画插值）
        context.partialTicks = partialTicks;

        // 创建带动画的模型
        auto animModel = _createModelForEntity(entity, context);
        if (animModel) {
            mesh = getOrCreateAnimatedMesh(entity, *animModel, context);
        }
    } else {
        // 静态实体路径：使用静态网格缓存
        mesh = getOrCreateMesh(entity);
    }

    if (!mesh || mesh->indexCount == 0) {
        // 恢复实体纹理图集（如果为 ItemEntity）
        // 必须在 early return 前恢复，否则后续实体会使用错误的纹理
        if (isItemEntity && m_textureAtlas && m_textureAtlas->isBuilt()) {
            m_pipeline->setTextureAtlas(m_textureAtlas->imageView(), m_textureAtlas->sampler());
        }
        return;
    }

    // 绑定管线
    m_pipeline->bind(cmd);

    // 绑定相机描述符集（set = 0）
    if (m_cameraDescriptorSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipeline->pipelineLayout(),
            0, // set = 0
            1,
            &m_cameraDescriptorSet,
            0,
            nullptr);
    }

    // 绑定纹理描述符（set = 1）
    m_pipeline->bindTextureDescriptor(cmd);

    // 计算模型矩阵
    std::array<f64, 16> modelMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    const f32 partialTickF32 = static_cast<f32>(partialTicks);

    if (isItemEntity) {
        // ItemEntity 特殊渲染：应用浮动和旋转动画
        f64 bobOffset = _calculateItemBobOffset(entity, partialTicks);
        f64 rotation = _calculateItemRotation(entity, partialTicks);
        i32 itemLayerCount = 1;
        if (const ItemStack* itemStack = entity.itemStack(); itemStack != nullptr && !itemStack->isEmpty()) {
            itemLayerCount = renderer::projectile::ItemEntityRenderer::getItemCountForRender(itemStack->getCount());
        }

        // Y 翻转
        modelMatrix[5] = -1.0f;

        // 应用 Y 轴旋转（物品自转）
        f64 rotRad = rotation * static_cast<f64>(math::DEG_TO_RAD);
        f64 cosRot = std::cos(rotRad);
        f64 sinRot = std::sin(rotRad);
        modelMatrix[0] = cosRot;
        modelMatrix[2] = sinRot;
        modelMatrix[8] = -sinRot;
        modelMatrix[10] = cosRot;

        // 获取插值位置并应用浮动偏移
        Vector3 posInterp = entity.getInterpolatedPosition(partialTickF32);
        Vector3f pos(
            static_cast<f32>(posInterp.x), static_cast<f32>(posInterp.y + bobOffset), static_cast<f32>(posInterp.z));

        for (i32 layerIndex = 0; layerIndex < itemLayerCount; ++layerIndex) {
            Vector3f layerPos = pos;
            if (layerIndex > 0) {
                layerPos.y += 0.015f * static_cast<f32>(layerIndex);
                layerPos.x += 0.01f * static_cast<f32>(layerIndex);
                layerPos.z += 0.005f * static_cast<f32>(layerIndex);
            }

            m_pipeline->drawMesh(
                cmd, *mesh, modelMatrix, layerPos, MODEL_SCALE * 16.0f, Vector4f(0.0f, 0.0f, 0.0f, 0.0f), 0.0f, 0.0f);
        }
    } else if (isExperienceOrb) {
        // ExperienceOrb 特殊渲染：应用浮动动画和动态大小
        f64 bobOffset = _calculateExperienceOrbBobOffset(entity.ticksExisted(), partialTicks);

        // Y 翻转
        modelMatrix[5] = -1.0f;

        // 获取经验值以确定大小
        i32 xpValue = entity.xpValue();
        i32 orbSize = mc::entity::experience::utils::getOrbSize(xpValue);

        // 根据经验球大小计算缩放因子
        // 大小等级 0-10，基础大小为 0.25，每级增加约 0.015
        f64 scale = MODEL_SCALE * (16.0f + static_cast<f64>(orbSize) * 0.5f);

        // 获取插值位置并应用浮动偏移
        Vector3 posInterp = entity.getInterpolatedPosition(partialTickF32);
        Vector3f pos(
            static_cast<f32>(posInterp.x), static_cast<f32>(posInterp.y + bobOffset), static_cast<f32>(posInterp.z));

        // 绘制网格
        m_pipeline->drawMesh(cmd, *mesh, modelMatrix, pos, scale, Vector4f(0.0f, 0.0f, 0.0f, 0.0f), 0.0f, 0.0f);
    } else {
        // 普通实体渲染
        // 变换顺序：scale(-1, -1, 1) → preRenderCallback → translate(0, -1.501, 0)

        // 应用 Y 轴旋转（yaw）- 在翻转之前应用
        f64 yaw = static_cast<f64>(entity.getInterpolatedYaw(partialTickF32));
        f64 yawRad = yaw * static_cast<f64>(math::DEG_TO_RAD);
        f64 cosYaw = std::cos(yawRad);
        f64 sinYaw = std::sin(yawRad);

        // 旋转矩阵（绕Y轴）
        modelMatrix[0] = cosYaw;
        modelMatrix[2] = sinYaw;
        modelMatrix[8] = -sinYaw;
        modelMatrix[10] = cosYaw;

        // scale(-1, -1, 1) - X和Y都取反
        // 在旋转后应用翻转
        for (i32 i = 0; i < 4; ++i) {
            const auto rowOffset = static_cast<std::size_t>(i * 4);
            modelMatrix[rowOffset] *= -1.0;     // X列取反
            modelMatrix[rowOffset + 1] *= -1.0; // Y列取反
        }

        // Y偏移 - 原版是向下偏移 1.501
        modelMatrix[13] = MODEL_Y_OFFSET;

        // 获取插值位置
        Vector3 posInterp = entity.getInterpolatedPosition(partialTickF32);
        Vector3f pos(static_cast<f32>(posInterp.x), static_cast<f32>(posInterp.y), static_cast<f32>(posInterp.z));

        // 获取受伤和死亡时间（用于着色器效果）
        f32 hurtTime = static_cast<f32>(entity.hurtTime()) / 10.0f; // 归一化到 0-1
        f32 deathTime = static_cast<f32>(entity.deathTime());

        // 绘制网格
        m_pipeline->drawMesh(
            cmd, *mesh, modelMatrix, pos, MODEL_SCALE, Vector4f(0.0f, 0.0f, 0.0f, 0.0f), hurtTime, deathTime);

        // 渲染层（盔甲、手持物品等）
        if (renderer && renderer->supportsLayers()) {
            // 在渲染层之前，传递纹理图集给渲染器
            // 这样层渲染器可以访问纹理UV区域信息进行UV重映射
            if (m_textureAtlas && m_textureAtlas->isBuilt()) {
                renderer->setTextureAtlas(m_textureAtlas);
            }
            renderer->renderLayersPipelineClient(entity, cmd, context, *m_pipeline);
        }

        // 渲染火焰效果（如果实体正在燃烧）
        if (entity.isOnFire()) {
            effect::fire::FireEffect::renderFire(cmd, entity, partialTicks, *m_pipeline);
        }

        // 渲染阴影
        if (m_renderShadows && !isItemEntity && !isExperienceOrb) {
            // 使用渲染器的 shadowSize 而非 width * 0.5
            f64 shadowRadius = renderer ? renderer->shadowSize() : static_cast<f64>(entity.width()) * 0.5;
            f64 shadowOpaque = renderer ? renderer->shadowAlpha() : 0.8;

            // 检查阴影大小和透明度
            if (shadowRadius > 0.0 && shadowOpaque > 0.0 && !entity.isInvisible()) {
                // 计算到相机的距离衰减
                util::ShadowRenderer::renderShadow(cmd, entity, partialTicks, shadowRadius, shadowOpaque, *m_pipeline);
            }
        }
    }

    // 恢复实体纹理图集（如果为 ItemEntity）
    if (isItemEntity && m_textureAtlas && m_textureAtlas->isBuilt()) {
        m_pipeline->setTextureAtlas(m_textureAtlas->imageView(), m_textureAtlas->sampler());
    }
}

bool EntityRendererManager::renderWithPipeline(
    VkCommandBuffer cmd, ClientEntity& entity, f64 partialTicks, const mc::math::frustum::Frustum& frustum)
{
    // 使用 FrustumUtils 创建实体包围盒
    // 使用插值位置以获得平滑的剔除效果
    const f32 partialTickF32 = static_cast<f32>(partialTicks);
    Vector3 pos = entity.getInterpolatedPosition(partialTickF32);
    AxisAlignedBB aabb = mc::math::frustum::FrustumUtils::createEntityAABB(pos, entity.width(), entity.height());

    // 使用世界坐标 AABB 进行视锥剔除
    if (!frustum.isAABBVisibleWorld(aabb)) {
        return false; // 实体不在视锥内，跳过渲染
    }

    // 实体在视锥内，正常渲染
    renderWithPipeline(cmd, entity, partialTicks);
    return true;
}

f64 EntityRendererManager::_calculateItemBobOffset(const ClientEntity& entity, f64 partialTick) const
{
    return renderer::projectile::ItemEntityRenderer::calculateBobOffset(
        entity.ticksExisted(), partialTick, entity.hoverStart());
}

f64 EntityRendererManager::_calculateItemRotation(const ClientEntity& entity, f64 partialTick) const
{
    return renderer::projectile::ItemEntityRenderer::calculateRotation(
        entity.ticksExisted(), partialTick, entity.hoverStart());
}

f64 EntityRendererManager::_calculateExperienceOrbBobOffset(u32 ticksExisted, f64 partialTick) const
{
    // 经验球浮动动画：sin(ticks * 0.05) * 0.1 + 0.2
    f64 ticks = static_cast<f64>(ticksExisted) + partialTick;
    return std::sin(ticks * 0.05f) * 0.1f + 0.3f; // 0.3 是基础高度偏移（略高于物品）
}

EntityMesh* EntityRendererManager::getOrCreateMesh(ClientEntity& entity)
{
    MC_TRACE_EVENT("rendering.entity",
        "EntityRendererManager::getOrCreateMesh",
        "entityId",
        entity.id(),
        "typeId",
        entity.typeId(),
        "itemRenderStateVersion",
        entity.itemRenderStateVersion());

    EntityId id = entity.id();
    auto it = m_meshes.find(id);

    if (it != m_meshes.end()) {
        if (normalizeEntityTypeId(entity.typeId()) == ::mc::entity::EntityTypes::ITEM &&
            it->second.itemRenderStateVersion != entity.itemRenderStateVersion()) {
            updateMesh(entity);
            it = m_meshes.find(id);
            if (it == m_meshes.end()) {
                return nullptr;
            }
        }
        return &it->second.mesh;
    }

    // 生成新网格
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;

    if (!_generateModelMesh(entity.typeId(), vertices, indices)) {
        return nullptr;
    }

    // 对于 ItemEntity，使用 ItemTextureAtlas 进行 UV 重映射
    std::string normalizedType = normalizeEntityTypeId(entity.typeId());
    if (normalizedType == ::mc::entity::EntityTypes::ITEM) {
        _remapItemEntityUv(entity, vertices);
    } else {
        // 普通实体使用实体纹理图集
        _remapUvToAtlasRegion(normalizedType, vertices);
    }

    // 创建GPU网格
    if (!m_pipeline) {
        return nullptr;
    }

    auto result = m_pipeline->createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::warn("Failed to create mesh for entity {}: {}", id, result.error().toString());
        return nullptr;
    }

    StaticMeshEntry meshEntry;
    meshEntry.mesh = std::move(result.value());
    meshEntry.mesh.posX = entity.x();
    meshEntry.mesh.posY = entity.y();
    meshEntry.mesh.posZ = entity.z();
    meshEntry.itemRenderStateVersion =
        normalizedType == ::mc::entity::EntityTypes::ITEM ? entity.itemRenderStateVersion() : 0;

    m_meshes[id] = std::move(meshEntry);
    return &m_meshes[id].mesh;
}

void EntityRendererManager::updateMesh(ClientEntity& entity)
{
    EntityId id = entity.id();
    auto it = m_meshes.find(id);

    if (it == m_meshes.end()) {
        return;
    }

    // 重新生成网格
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;

    if (!_generateModelMesh(entity.typeId(), vertices, indices)) {
        return;
    }

    std::string normalizedType = normalizeEntityTypeId(entity.typeId());
    if (normalizedType == ::mc::entity::EntityTypes::ITEM) {
        _remapItemEntityUv(entity, vertices);
    } else {
        _remapUvToAtlasRegion(normalizedType, vertices);
    }

    (void)m_pipeline->updateMesh(it->second.mesh, vertices, indices);
    if (normalizedType == ::mc::entity::EntityTypes::ITEM) {
        it->second.itemRenderStateVersion = entity.itemRenderStateVersion();
    }
}

void EntityRendererManager::removeMesh(EntityId entityId)
{
    auto it = m_meshes.find(entityId);
    if (it != m_meshes.end()) {
        if (m_pipeline) {
            m_pipeline->destroyMesh(it->second.mesh);
        }
        m_meshes.erase(it);
    }
}

void EntityRendererManager::initializeDefaults()
{
    // 首先注册所有模型
    model::initializeModelRegistration();

    // 注册所有渲染器（使用工厂注册表模式）
    renderer::initializeRendererRegistration();

    // ItemEntity 渲染器需要设置 itemTextureAtlas
    auto* itemRenderer = _getOrCreateRenderer(::mc::entity::EntityTypes::ITEM);
    if (auto* itemEntityRenderer = dynamic_cast<renderer::projectile::ItemEntityRenderer*>(itemRenderer)) {
        if (m_itemTextureAtlas) {
            itemEntityRenderer->setItemTextureAtlas(m_itemTextureAtlas);
        }
    }
}

EntityRenderer* EntityRendererManager::_getOrCreateRenderer(const std::string& typeId)
{
    // 规范化实体类型ID
    std::string normalizedId = normalizeEntityTypeId(typeId);

    // 先查找已创建的渲染器
    auto it = m_renderers.find(normalizedId);
    if (it != m_renderers.end()) {
        return it->second.get();
    }

    // 使用工厂创建渲染器
    auto renderer = core::RendererFactory::instance().createRenderer(normalizedId);
    if (!renderer) {
        return nullptr;
    }

    EntityRenderer* ptr = renderer.get();
    m_renderers[normalizedId] = std::move(renderer);
    return ptr;
}

bool EntityRendererManager::_generateModelMesh(
    const std::string& typeId, std::vector<ModelVertex>& vertices, std::vector<u32>& indices)
{
    // 规范化实体类型ID，统一使用命名空间格式进行比较
    std::string normalizedId = normalizeEntityTypeId(typeId);

    // 使用 EntityTypes 常量进行比较

    // 只有 ItemEntity 和 ExperienceOrb 使用静态网格
    // 所有生物实体都使用动画网格路径（通过 _createModelForEntity）
    if (normalizedId == ::mc::entity::EntityTypes::ITEM) {
        // ItemEntity 使用简单的四边形网格
        // 物品图标会在渲染时根据 ItemStack 动态获取纹理
        _generateBillboardMesh(vertices, indices, 0.25, 0.25);
        return true;
    }
    if (normalizedId == ::mc::entity::EntityTypes::EXPERIENCE_ORB) {
        // ExperienceOrb 使用简单的四边形网格（billboard）
        // 颜色会根据经验和时间动态变化
        _generateBillboardMesh(vertices, indices, 0.25, 0.25);
        return true;
    }

    // 其他实体类型使用动画网格，不在此生成静态网格
    return false;
}

void EntityRendererManager::_generateBillboardMesh(
    std::vector<ModelVertex>& vertices, std::vector<u32>& indices, f64 width, f64 height)
{
    // 生成一个双面 billboard 四边形网格
    // 用于 ItemEntity 和 ExperienceOrb 等静态网格实体

    const f64 halfWidth = width * 0.5;
    const f64 yOffset = 0.25; // 地面偏移

    // 创建双面四边形（billboard）
    // 正面朝向 +Z，背面朝向 -Z
    // 实际渲染时会根据摄像机朝向旋转
    vertices = {
        // 背面（法线 -Z）
        ModelVertex(-halfWidth, yOffset, 0.0, 0.0, 1.0, 0.0, 0.0, -1.0),
        ModelVertex(-halfWidth, yOffset + height, 0.0, 0.0, 0.0, 0.0, 0.0, -1.0),
        ModelVertex(halfWidth, yOffset + height, 0.0, 1.0, 0.0, 0.0, 0.0, -1.0),
        ModelVertex(halfWidth, yOffset, 0.0, 1.0, 1.0, 0.0, 0.0, -1.0),
        // 正面（法线 +Z）
        ModelVertex(halfWidth, yOffset, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0),
        ModelVertex(halfWidth, yOffset + height, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0),
        ModelVertex(-halfWidth, yOffset + height, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0),
        ModelVertex(-halfWidth, yOffset, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0),
    };

    indices = {
        // 背面
        0,
        1,
        2,
        0,
        2,
        3,
        // 正面
        4,
        5,
        6,
        4,
        6,
        7,
    };
}

void EntityRendererManager::_remapItemEntityUv(ClientEntity& entity, std::vector<ModelVertex>& vertices)
{
    if (!m_itemTextureAtlas || vertices.empty()) {
        return;
    }

    // 获取物品堆
    const ItemStack* itemStack = entity.itemStack();
    if (!itemStack || itemStack->isEmpty()) {
        return;
    }

    // 获取物品
    const Item* item = itemStack->getItem();
    if (!item) {
        return;
    }

    // 尝试获取物品纹理区域
    // itemId() returns u16, use itemLocation() which returns ResourceLocation that can be converted to string
    const ResourceLocation& itemId = item->itemLocation();
    const TextureRegion* region = m_itemTextureAtlas->getRegion(itemId.toString());
    if (!region) {
        // 尝试使用 item/ 路径
        ResourceLocation itemPath(itemId.namespace_(), "item/" + itemId.path());
        region = m_itemTextureAtlas->getRegion(itemPath.toString());
        if (!region) {
            ResourceLocation itemTexturePath(itemId.namespace_(), "textures/item/" + itemId.path());
            region = m_itemTextureAtlas->getRegion(itemTexturePath.toString());
        }
    }

    if (!region) {
        return;
    }

    // 重映射 UV 坐标
    const f64 du = region->u1 - region->u0;
    const f64 dv = region->v1 - region->v0;

    for (auto& vertex : vertices) {
        const f64 remappedU = region->u0 + static_cast<f64>(vertex.texCoord.x) * du;
        const f64 remappedV = region->v0 + static_cast<f64>(vertex.texCoord.y) * dv;
        vertex.texCoord.x = static_cast<f32>(remappedU);
        vertex.texCoord.y = static_cast<f32>(remappedV);
    }
}

void EntityRendererManager::_remapUvToAtlasRegion(
    const std::string& normalizedTypeId, std::vector<ModelVertex>& vertices) const
{
    if (!m_textureAtlas || !m_textureAtlas->isBuilt() || vertices.empty()) {
        return;
    }

    const TextureRegion* region = nullptr;
    const auto texturePaths = EntityTextureLoader::getTexturePaths(normalizedTypeId);
    for (const auto& path : texturePaths) {
        region = m_textureAtlas->getRegion(path);
        if (region) {
            break;
        }
    }

    if (!region) {
        spdlog::warn("_remapUvToAtlasRegion: No atlas region found for entity type: {}", normalizedTypeId);
        return;
    }

    const f64 du = region->u1 - region->u0;
    const f64 dv = region->v1 - region->v0;

    for (auto& vertex : vertices) {
        const f64 remappedU = region->u0 + static_cast<f64>(vertex.texCoord.x) * du;
        const f64 remappedV = region->v0 + static_cast<f64>(vertex.texCoord.y) * dv;
        vertex.texCoord.x = static_cast<f32>(remappedU);
        vertex.texCoord.y = static_cast<f32>(remappedV);
    }
}

void EntityRendererManager::clearAnimatedMeshes()
{
    if (m_animatedMeshCache) {
        m_animatedMeshCache->clear(m_pipeline);
    }
}

bool EntityRendererManager::_usesAnimatedMesh(const std::string& normalizedTypeId) const
{
    // ItemEntity 和 ExperienceOrb 使用静态网格
    // 所有生物实体使用动画网格
    return normalizedTypeId != ::mc::entity::EntityTypes::ITEM &&
        normalizedTypeId != ::mc::entity::EntityTypes::EXPERIENCE_ORB;
}

std::unique_ptr<model::EntityModel> EntityRendererManager::_createModelForEntity(
    ClientEntity& entity, core::AnimationContext& context)
{
    std::string normalizedId = normalizeEntityTypeId(entity.typeId());

    // 从 ClientEntity 读取动画状态
    // limbSwing = entity.limbSwing - entity.limbSwingAmount * (1.0 - partialTicks)
    f64 limbSwingAmount = static_cast<f64>(entity.limbSwingAmount());
    context.limbSwing = static_cast<f64>(entity.limbSwing()) - limbSwingAmount * (1.0 - context.partialTicks);

    // 幼体动画速度加倍
    if (entity.isChild()) {
        context.limbSwing *= 3.0;
    }

    // limbSwingAmount 使用插值
    context.limbSwingAmount = static_cast<f64>(entity.prevLimbSwingAmount()) +
        static_cast<f64>(entity.limbSwingAmount() - entity.prevLimbSwingAmount()) * context.partialTicks;

    // 限制最大值
    if (context.limbSwingAmount > 1.0) {
        context.limbSwingAmount = 1.0;
    }

    context.ageInTicks = static_cast<f64>(entity.ticksExisted()) + context.partialTicks;

    // 计算头部偏航角（相对于身体）
    f64 bodyYaw = static_cast<f64>(entity.prevRenderYawOffset()) +
        static_cast<f64>(entity.renderYawOffset() - entity.prevRenderYawOffset()) * context.partialTicks;
    f64 headYaw = static_cast<f64>(entity.prevRotationYawHead()) +
        static_cast<f64>(entity.rotationYawHead() - entity.prevRotationYawHead()) * context.partialTicks;
    context.netHeadYaw = headYaw - bodyYaw;
    // 归一化到 -180 到 180
    while (context.netHeadYaw < -180.0)
        context.netHeadYaw += 360.0;
    while (context.netHeadYaw > 180.0)
        context.netHeadYaw -= 360.0;

    context.headPitch = static_cast<f64>(entity.prevPitch()) +
        static_cast<f64>(entity.pitch() - entity.prevPitch()) * context.partialTicks;
    context.scale = 1.0 / 16.0;
    context.isChild = entity.isChild();
    context.isSitting = entity.isSitting();
    context.isSneaking = entity.isSneaking();
    context.isSwimming = entity.isSwimming();
    context.isRiding = entity.isRiding();
    context.swingProgress = entity.getInterpolatedSwingProgress(static_cast<f32>(context.partialTicks));

    // 北极熊站立动画
    const std::string& typeId = entity.typeId();
    if (typeId == "minecraft:polar_bear" || typeId == "polar_bear") {
        context.standingProgress = entity.getStandingAnimationScale(static_cast<f32>(context.partialTicks));
    } else {
        context.standingProgress = 0.0f;
    }

    // 计算哈希
    context.computeHash();

    // 使用 ModelFactory 创建模型
    auto model = model::ModelFactory::instance().createModel(normalizedId);
    if (model) {
        model->setAngles(context.limbSwing,
            context.limbSwingAmount,
            context.ageInTicks,
            context.netHeadYaw,
            context.headPitch,
            context.scale * 16.0);

        // 北极熊站立动画
        if (normalizedId == "polar_bear" || normalizedId == "minecraft:polar_bear") {
            auto* polarBearModel = dynamic_cast<model::animal::PolarBearModel*>(model.get());
            if (polarBearModel != nullptr) {
                polarBearModel->setStandingProgress(context.standingProgress);
            }
        }

        return model;
    }

    spdlog::warn("_createModelForEntity: No model found for entity type: {}", normalizedId);
    return nullptr;
}

pipeline::EntityMesh* EntityRendererManager::getOrCreateAnimatedMesh(
    ClientEntity& entity, model::EntityModel& model, const core::AnimationContext& context)
{
    if (!m_pipeline || !m_animatedMeshCache) {
        return nullptr;
    }

    std::string normalizedId = normalizeEntityTypeId(entity.typeId());

    // 设置 UV 重映射回调
    m_animatedMeshCache->setUvRemapFunc(
        [this, normalizedId](const std::string& typeId, std::vector<ModelVertex>& vertices) {
            _remapUvToAtlasRegion(typeId, vertices);
        });

    return m_animatedMeshCache->getOrUpdateMesh(entity.id(), model, normalizedId, context, *m_pipeline);
}

} // namespace mc::client::renderer::entity
