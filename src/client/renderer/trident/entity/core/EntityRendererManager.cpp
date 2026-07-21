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
#include "client/renderer/trident/entity/model/animal/CatModel.hpp"
#include "client/renderer/trident/entity/model/animal/HorseModel.hpp"
#include "client/renderer/trident/entity/model/animal/LlamaModel.hpp"
#include "client/renderer/trident/entity/model/animal/OcelotModel.hpp"
#include "client/renderer/trident/entity/model/animal/PolarBearModel.hpp"
#include "client/renderer/trident/entity/model/animal/RabbitModel.hpp"
#include "client/renderer/trident/entity/model/animal/SheepModel.hpp"
#include "client/renderer/trident/entity/model/animal/WolfModel.hpp"
#include "client/renderer/trident/entity/model/aquatic/AquaticModels.hpp"
#include "client/renderer/trident/entity/model/aquatic/PufferfishModel.hpp"
#include "client/renderer/trident/entity/model/base/BipedModel.hpp"
#include "client/renderer/trident/entity/model/base/ElytraSpeedValue.hpp"
#include "client/renderer/trident/entity/model/core/ModelFactory.hpp"
#include "client/renderer/trident/entity/model/monster/EndermanModel.hpp"
#include "client/renderer/trident/entity/model/monster/MonsterVariantModels.hpp"
#include "client/renderer/trident/entity/model/monster/SkeletonModel.hpp"
#include "client/renderer/trident/entity/model/monster/SpecialMonsterModels.hpp"
#include "client/renderer/trident/entity/model/monster/ZombieModel.hpp"
#include "client/renderer/trident/entity/model/nether/NetherModels.hpp"
#include "client/renderer/trident/entity/model/player/PlayerModel.hpp"
#include "client/renderer/trident/entity/pipeline/EntityTextureAtlas.hpp"
#include "client/renderer/trident/entity/renderer/RendererRegistration.hpp"
#include "client/renderer/trident/entity/renderer/player/PlayerArmPoseResolver.hpp"
#include "client/renderer/trident/entity/renderer/projectile/ExperienceOrbRenderer.hpp"
#include "client/renderer/trident/entity/renderer/projectile/ItemEntityRenderer.hpp"
#include "client/renderer/trident/entity/util/NameTagRenderer.hpp"
#include "client/renderer/trident/entity/util/ShadowRenderer.hpp"
#include "client/resource/EntityTextureLoader.hpp"
#include "client/resource/ItemTextureAtlas.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/orb/ExperienceOrbEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/experience/ExperienceUtils.hpp"
#include "common/entity/registry/VanillaEntityTypeKeys.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/weapon/CrossbowItem.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/MathConstants.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector4.hpp"
// 方块纹理图集与末影人渲染器
#include "client/renderer/trident/chunk/ChunkRenderer.hpp"
#include "client/renderer/trident/entity/renderer/monster/MonsterRenderers.hpp"
#include "client/renderer/trident/entity/renderer/special/SpecialEntityRenderers.hpp"
#include <cmath>
#include <unordered_set>
#include <spdlog/spdlog.h>

using namespace mc::trace;

namespace mc::client::renderer::entity {

// 导入核心命名空间中的类
using core::EntityRenderer;
using model::ModelVertex;
using pipeline::EntityMesh;
using pipeline::EntityTextureAtlas;

namespace {

// 标准常量
inline constexpr f64 MODEL_Y_OFFSET = 0;
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
    // 图集变化后，旧网格的UV映射可能失效，强制重建静态和动画缓存
    clearMeshes();
    clearAnimatedMeshes();

    // 将实体纹理图集注入到方块渲染器（用于渲染方块后恢复纹理图集）
    // - FallingBlockRenderer: 渲染下落方块后恢复
    // - TNTRenderer: 渲染 TNT 方块后恢复
    auto* fallingBlockRendererRaw = _getOrCreateRenderer(::mc::entity::EntityTypeKeys::FALLING_BLOCK);
    if (auto* fallingBlockRenderer = dynamic_cast<renderer::special::FallingBlockRenderer*>(fallingBlockRendererRaw)) {
        fallingBlockRenderer->setEntityTextureAtlas(textureAtlas);
    }

    auto* tntRendererRaw = _getOrCreateRenderer(::mc::entity::EntityTypeKeys::TNT);
    if (auto* tntRenderer = dynamic_cast<renderer::special::TNTRenderer*>(tntRendererRaw)) {
        tntRenderer->setEntityTextureAtlas(textureAtlas);
    }
}

void EntityRendererManager::setBlockAtlas(VkImageView imageView, VkSampler sampler)
{
    m_blockImageView = imageView;
    m_blockSampler = sampler;
    // 将 blocks atlas 句柄注入到所有需要的渲染器
    // - EndermanRenderer: 用于 HeldBlockLayer（末影人手持方块）
    // - FallingBlockRenderer: 用于下落方块渲染
    // - TNTRenderer: 用于 TNT 实体方块渲染

    // 末影人渲染器
    auto* endermanRendererRaw = _getOrCreateRenderer(::mc::entity::EntityTypeKeys::ENDERMAN);
    if (auto* endermanRenderer = dynamic_cast<renderer::monster::EndermanRenderer*>(endermanRendererRaw)) {
        endermanRenderer->setBlockAtlas(imageView, sampler);
    }

    // 下落方块渲染器
    auto* fallingBlockRendererRaw = _getOrCreateRenderer(::mc::entity::EntityTypeKeys::FALLING_BLOCK);
    if (auto* fallingBlockRenderer = dynamic_cast<renderer::special::FallingBlockRenderer*>(fallingBlockRendererRaw)) {
        fallingBlockRenderer->setBlockAtlas(imageView, sampler);
        // 同时注入实体纹理图集（用于渲染后恢复）
        if (m_textureAtlas != nullptr) {
            fallingBlockRenderer->setEntityTextureAtlas(m_textureAtlas);
        }
    }

    // TNT 渲染器
    auto* tntRendererRaw = _getOrCreateRenderer(::mc::entity::EntityTypeKeys::TNT);
    if (auto* tntRenderer = dynamic_cast<renderer::special::TNTRenderer*>(tntRendererRaw)) {
        tntRenderer->setBlockAtlas(imageView, sampler);
        // 同时注入实体纹理图集（用于渲染后恢复）
        if (m_textureAtlas != nullptr) {
            tntRenderer->setEntityTextureAtlas(m_textureAtlas);
        }
    }
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

    // 更新 ShadowRenderer 的相机位置（用于阴影距离衰减）
    util::ShadowRenderer::setCameraPosition(Vector3d(position.x, position.y, position.z));

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
    std::string normalizedType = normalizeEntityTypeId(entity.getTypeId());
    bool isItemEntity = (normalizedType == ::mc::entity::EntityTypeKeys::ITEM);
    bool isExperienceOrb = (normalizedType == ::mc::entity::EntityTypeKeys::EXPERIENCE_ORB);

    // 对于 ItemEntity，使用 ItemTextureAtlas
    if (isItemEntity && m_itemTextureAtlas && m_itemTextureAtlas->isValid() && m_itemTextureAtlas->isUploaded()) {
        // ItemTextureAtlas 用 isValid()+isUploaded() 判定就绪：
        // isValid() 仅检查 m_image!=NULL（_createImage 后即 true，早于 upload），故需额外 isUploaded()
        // 绑定物品纹理图集
        m_pipeline->setTextureAtlas(m_itemTextureAtlas->imageView(), m_itemTextureAtlas->sampler());
    }

    // 获取渲染器
    EntityRenderer* renderer = _getOrCreateRenderer(entity.getTypeId());

    // 获取或创建网格
    EntityMesh* mesh = nullptr;
    core::AnimationContext context;

    if (isItemEntity || isExperienceOrb) {
        // 特殊路径：ItemEntity 和 ExperienceOrb 使用静态 billboard 网格
        mesh = getOrCreateMesh(entity);
    } else if (renderer) {
        // Path A: 渲染器有自定义管线网格提供者（Arrow, Boat, FishingBobber 等）
        core::PipelineMeshProvider* meshProvider = renderer->getPipelineMeshProvider();
        if (meshProvider) {
            // 通过 PipelineMeshProvider 生成自定义网格
            mesh = getOrCreateProviderMesh(entity, *meshProvider);
        }

        // Path B: 渲染器支持动画，使用 ModelFactory + AnimatedMeshCache
        if (!mesh && renderer->supportsAnimation()) {
            context.partialTicks = partialTicks;
            auto animModel = _createModelForEntity(entity, context);
            if (animModel) {
                mesh = getOrCreateAnimatedMesh(entity, *animModel, context);
            }
        }

        // Path C: 无可用网格路径 - 记录警告
        if (!mesh) {
            spdlog::warn("EntityRendererManager: No mesh path for entity type '{}'", normalizedType);
        }
    }

    if (!mesh || mesh->indexCount == 0) {
        // 恢复实体纹理图集（如果为 ItemEntity）
        // 必须在 early return 前恢复，否则后续实体会使用错误的纹理
        if (isItemEntity && m_textureAtlas && m_textureAtlas->isBuilt()) {
            m_pipeline->setTextureAtlas(m_textureAtlas->imageView(), m_textureAtlas->sampler());
        }
        return;
    }

    // 绑定管线 - 根据网格拓扑选择混合模式
    pipeline::BlendMode blendMode = pipeline::BlendMode::Alpha;
    if (renderer) {
        core::PipelineMeshProvider* meshProvider = renderer->getPipelineMeshProvider();
        if (meshProvider && meshProvider->getTopology() == VK_PRIMITIVE_TOPOLOGY_LINE_LIST) {
            blendMode = pipeline::BlendMode::Lines;
        }
    }
    m_pipeline->bind(cmd, blendMode);

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

        // billboard 顶点 y∈[0.25,0.5] 已在地面以上正确象限，无需 Y 翻转。
        // Y 翻转是给"模型坐标系 Y 向上"的生物 .geo 模型用的（见普通实体分支 scale(-1,-1,1)+translate(0,1.501,0)）。

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

            m_pipeline->drawMesh(cmd,
                *mesh,
                modelMatrix,
                layerPos,
                MODEL_SCALE * 16.0f,
                Vector4f(0.0f, 0.0f, 0.0f, 0.0f),
                0.0f,
                0.0f,
                0.0f);
        }
    } else if (isExperienceOrb) {
        // ExperienceOrb 特殊渲染：应用浮动动画、动态大小和颜色动画
        f64 bobOffset = _calculateExperienceOrbBobOffset(entity.ticksExisted(), partialTicks);

        // billboard 顶点 y∈[0.25,0.5] 已在地面以上正确象限，无需 Y 翻转（同 ItemEntity）。

        // 获取经验值以确定大小
        i32 xpValue = entity.xpValue();
        i32 orbSize = mc::entity::experience::utils::getOrbSize(xpValue);

        // 根据经验球大小计算缩放因子
        f64 scale = MODEL_SCALE * (16.0f + static_cast<f64>(orbSize) * 0.5f);

        // 获取插值位置并应用浮动偏移
        Vector3 posInterp = entity.getInterpolatedPosition(partialTickF32);
        Vector3f pos(
            static_cast<f32>(posInterp.x), static_cast<f32>(posInterp.y + bobOffset), static_cast<f32>(posInterp.z));

        // 计算颜色动画（对齐 MC Java 版 ExperienceOrbRenderer.submit）
        // 颜色在绿色和黄色之间循环，半透明
        // TODO: 当前使用 overlayColor 的 mix() 混合方式（color.rgb = mix(texColor, overlayColor.rgb,
        // overlayColor.a)）， 与 MC 原版的顶点颜色乘法（texColor * vertexColor）有视觉差异。mix 模式下纹理细节与颜色
        // 是线性插值关系，而 MC 的乘法模式是纹理颜色被动画色调调制。后续应改为顶点颜色乘法以完全对齐 MC。
        f64 time = static_cast<f64>(entity.ticksExisted()) + partialTicks;
        Vector4f orbColor = renderer::projectile::ExperienceOrbRenderer::calculateColor(time);

        // 经验球最小亮度：MC Java 中 getBlockLightLevel() 返回 clamp(worldLight + 7, 0, 15)，
        // 即最小方块光照为 7。在没有世界光照查询的情况下，使用 fullbright 因子模拟：
        // 7/15 ≈ 0.467，确保经验球在黑暗中也有一定可见度。
        const f32 orbMinBrightness = 7.0f / 15.0f;

        // 绘制网格
        m_pipeline->drawMesh(cmd, *mesh, modelMatrix, pos, scale, orbColor, 0.0f, 0.0f, orbMinBrightness);
    } else {
        // 普通实体渲染
        // 变换顺序：scale(-1, -1, 1) → preRenderCallback → translate(0, -1.501, 0)

        // 获取插值位置
        Vector3 posInterp = entity.getInterpolatedPosition(partialTickF32);
        Vector3f pos(static_cast<f32>(posInterp.x), static_cast<f32>(posInterp.y), static_cast<f32>(posInterp.z));

        // 默认 hurtTime / deathTime（用于着色器红色闪烁与死亡淡出）
        f32 hurtTime = static_cast<f32>(entity.hurtTime()) / 10.0f; // 归一化到 0-1
        f32 deathTime = static_cast<f32>(entity.deathTime());

        // 全亮光照：MC Java 中部分实体覆盖 getBlockLightLevel() 返回 15，
        // 使其在黑暗中也清晰可见（如烈焰人、岩浆怪、凋灵、恼鬼等）。
        f32 fullbright = (renderer && renderer->isFullbright()) ? 1.0f : 0.0f;

        // 渲染器可重写 computeCustomModelMatrix 提供完全自定义的模型矩阵
        // （例如船、矿车按 MC AbstractBoatRenderer / AbstractMinecartRenderer
        // 的变换链），此时跳过默认的 yaw + scale(-1,-1,1) + translate(0, 1.501, 0)。
        bool useCustomMatrix = false;
        if (renderer) {
            useCustomMatrix =
                renderer->computeCustomModelMatrix(entity, partialTicks, modelMatrix, hurtTime, deathTime);
        }

        if (!useCustomMatrix) {
            // 默认模型矩阵：rotateY(yaw) * scale(-1, -1, 1) * translate(0, 1.501, 0)

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
        }

        // 绘制网格
        m_pipeline->drawMesh(cmd,
            *mesh,
            modelMatrix,
            pos,
            MODEL_SCALE,
            Vector4f(0.0f, 0.0f, 0.0f, 0.0f),
            hurtTime,
            deathTime,
            fullbright);

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
    return renderer::projectile::ExperienceOrbRenderer::calculateBobOffset(ticksExisted, partialTick);
}

EntityMesh* EntityRendererManager::getOrCreateMesh(ClientEntity& entity)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Rendering.Entity,
        "EntityRendererManager::getOrCreateMesh",
        "entityId",
        entity.id(),
        "typeId",
        entity.getTypeId(),
        "itemRenderStateVersion",
        entity.itemRenderStateVersion());

    EntityInstanceId id = entity.id();
    auto it = m_meshes.find(id);

    if (it != m_meshes.end()) {
        if (normalizeEntityTypeId(entity.getTypeId()) == ::mc::entity::EntityTypeKeys::ITEM &&
            it->second.itemRenderStateVersion != entity.itemRenderStateVersion()) {
            updateMesh(entity);
            it = m_meshes.find(id);
            if (it == m_meshes.end()) {
                return nullptr;
            }
        }

        // 检查经验球的图标索引是否变化（XP 值合并后图标可能变化）
        std::string normalizedType = normalizeEntityTypeId(entity.getTypeId());
        if (normalizedType == ::mc::entity::EntityTypeKeys::EXPERIENCE_ORB) {
            i32 currentIconIndex = mc::entity::experience::utils::getOrbSize(entity.xpValue());
            if (it->second.xpOrbIconIndex != currentIconIndex) {
                updateMesh(entity);
                it = m_meshes.find(id);
                if (it == m_meshes.end()) {
                    return nullptr;
                }
            }
        }

        return &it->second.mesh;
    }

    // 生成新网格
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;

    if (!_generateModelMesh(entity.getTypeId(), vertices, indices)) {
        return nullptr;
    }

    // 对于 ItemEntity，使用 ItemTextureAtlas 进行 UV 重映射
    std::string normalizedType = normalizeEntityTypeId(entity.getTypeId());
    if (normalizedType == ::mc::entity::EntityTypeKeys::ITEM) {
        _remapItemEntityUv(entity, vertices);
    } else if (normalizedType == ::mc::entity::EntityTypeKeys::EXPERIENCE_ORB) {
        // ExperienceOrb 使用精灵图集中的特定图标，需要根据 XP 值选择正确的子区域
        if (m_textureAtlas && m_textureAtlas->isBuilt()) {
            _remapExperienceOrbUv(entity.xpValue(), *m_textureAtlas, vertices);
        }
    } else {
        // 普通实体使用实体纹理图集（玩家分支按 id 查动态皮肤区域）
        _remapUvToAtlasRegion(id, normalizedType, vertices);
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
        normalizedType == ::mc::entity::EntityTypeKeys::ITEM ? entity.itemRenderStateVersion() : 0;
    meshEntry.xpOrbIconIndex = normalizedType == ::mc::entity::EntityTypeKeys::EXPERIENCE_ORB
        ? mc::entity::experience::utils::getOrbSize(entity.xpValue())
        : -1;

    m_meshes[id] = std::move(meshEntry);
    return &m_meshes[id].mesh;
}

void EntityRendererManager::updateMesh(ClientEntity& entity)
{
    EntityInstanceId id = entity.id();
    auto it = m_meshes.find(id);

    if (it == m_meshes.end()) {
        return;
    }

    // 重新生成网格
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;

    if (!_generateModelMesh(entity.getTypeId(), vertices, indices)) {
        return;
    }

    std::string normalizedType = normalizeEntityTypeId(entity.getTypeId());
    if (normalizedType == ::mc::entity::EntityTypeKeys::ITEM) {
        _remapItemEntityUv(entity, vertices);
    } else if (normalizedType == ::mc::entity::EntityTypeKeys::EXPERIENCE_ORB) {
        // ExperienceOrb 使用精灵图集中的特定图标
        if (m_textureAtlas && m_textureAtlas->isBuilt()) {
            _remapExperienceOrbUv(entity.xpValue(), *m_textureAtlas, vertices);
        }
    } else {
        _remapUvToAtlasRegion(id, normalizedType, vertices);
    }

    (void)m_pipeline->updateMesh(it->second.mesh, vertices, indices);
    if (normalizedType == ::mc::entity::EntityTypeKeys::ITEM) {
        it->second.itemRenderStateVersion = entity.itemRenderStateVersion();
    }
    if (normalizedType == ::mc::entity::EntityTypeKeys::EXPERIENCE_ORB) {
        it->second.xpOrbIconIndex = mc::entity::experience::utils::getOrbSize(entity.xpValue());
    }
}

void EntityRendererManager::removeMesh(EntityInstanceId entityId)
{
    auto it = m_meshes.find(entityId);
    if (it != m_meshes.end()) {
        if (m_pipeline) {
            m_pipeline->destroyMesh(it->second.mesh);
        }
        m_meshes.erase(it);
    }
}

void EntityRendererManager::removeEntityMeshes(EntityInstanceId entityId)
{
    // 清理静态网格
    removeMesh(entityId);

    // 清理动画网格
    if (m_animatedMeshCache) {
        m_animatedMeshCache->removeEntity(entityId, m_pipeline);
    }
}

void EntityRendererManager::initializeDefaults()
{
    // 首先注册所有模型
    model::initializeModelRegistration();

    // 注册所有渲染器（使用工厂注册表模式）
    renderer::initializeRendererRegistration();

    // 注意：ItemEntity 的物品纹理图集由 TridentEngine::initializeItemRenderer() 末尾
    // 通过 setItemTextureAtlas() 注入（initializeItemRenderer 晚于 initializeEntityRenderer 调用）。
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

    // 使用 EntityTypeKeys 常量进行比较

    // 只有 ItemEntity 和 ExperienceOrb 使用静态网格
    // 所有生物实体都使用动画网格路径（通过 _createModelForEntity）
    if (normalizedId == ::mc::entity::EntityTypeKeys::ITEM) {
        // ItemEntity 使用简单的四边形网格
        // 物品图标会在渲染时根据 ItemStack 动态获取纹理
        _generateBillboardMesh(vertices, indices, 0.25, 0.25);
        return true;
    }
    if (normalizedId == ::mc::entity::EntityTypeKeys::EXPERIENCE_ORB) {
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

    // ItemTextureAtlas 注册时（ItemTextureAtlas.cpp）已用 item.itemId() 建 m_regionsByItemId 索引，
    // 直接按 itemId 查询即可命中。注意：不能用 getItemTexture(item->itemLocation())，
    // 因为 m_regionsByLocation 的 key 是 "textures/item/<path>" 与 "item/<path>"，不含裸 "<path>"，会落空。
    const TextureRegion* region = m_itemTextureAtlas->getItemTexture(item->itemId());

    if (!region) {
        // 按 itemId 去重的一次性 warn：同一物品缺失只报一次，避免每帧每实体刷屏
        static std::unordered_set<ItemId> s_warnedItemIds;
        const ItemId itemId = item->itemId();
        if (s_warnedItemIds.insert(itemId).second) {
            spdlog::warn("EntityRendererManager: ItemTextureAtlas 缺失物品 {} (itemId={}) 的纹理区域，"
                         "ItemEntity 将显示为无图集采样色块",
                item->itemLocation().toString(),
                static_cast<u32>(itemId));
        }
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
    EntityInstanceId entityId, const std::string& normalizedTypeId, std::vector<ModelVertex>& vertices) const
{
    if (!m_textureAtlas || !m_textureAtlas->isBuilt() || vertices.empty()) {
        return;
    }

    // 玩家分支：按 entityId 查动态皮肤区域（皮肤纹理上传到本图集，非 ClientSkinManager 孤儿图集）。
    // PlayerSkinRegionProvider 实现内部判定"该 entityId 是否玩家 + 皮肤是否就绪"，
    // 非玩家或未就绪返回 nullptr，回退到下方默认实体纹理路径。
    const TextureRegion* region = nullptr;
    if (m_skinRegionProvider) {
        region = m_skinRegionProvider->getSkinRegionForEntity(entityId);
    }

    // 非玩家分支：按 typeId 查默认实体纹理路径
    if (!region) {
        const auto texturePaths = EntityTextureLoader::getTexturePaths(normalizedTypeId);
        for (const auto& path : texturePaths) {
            region = m_textureAtlas->getRegion(path);
            if (region) {
                break;
            }
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

void EntityRendererManager::_remapExperienceOrbUv(
    i32 xpValue, const pipeline::EntityTextureAtlas& textureAtlas, std::vector<ModelVertex>& vertices) const
{
    if (vertices.empty()) {
        return;
    }

    // 查找经验球纹理在图集中的区域
    const TextureRegion* region = nullptr;
    const auto texturePaths = EntityTextureLoader::getTexturePaths(::mc::entity::EntityTypeKeys::EXPERIENCE_ORB);
    for (const auto& path : texturePaths) {
        region = textureAtlas.getRegion(path);
        if (region) {
            break;
        }
    }

    if (!region) {
        spdlog::warn("_remapExperienceOrbUv: No atlas region found for experience_orb");
        return;
    }

    // 计算精灵图标在图集中的 UV 坐标
    // 经验球纹理为 64x64 精灵图集，4列×3行布局，每个图标 16x16 像素
    f64 iconU0, iconV0, iconU1, iconV1;
    renderer::projectile::ExperienceOrbRenderer::calculateIconUV(
        mc::entity::experience::utils::getOrbSize(xpValue), iconU0, iconV0, iconU1, iconV1);

    // 将图集内的图标 UV 映射到纹理图集中的实际位置
    const f64 atlasDu = region->u1 - region->u0;
    const f64 atlasDv = region->v1 - region->v0;

    for (auto& vertex : vertices) {
        // 先将顶点 UV (0-1) 映射到图集中的图标子区域
        const f64 localU = iconU0 + static_cast<f64>(vertex.texCoord.x) * (iconU1 - iconU0);
        const f64 localV = iconV0 + static_cast<f64>(vertex.texCoord.y) * (iconV1 - iconV0);

        // 再将图标 UV 映射到纹理图集中的整体区域
        // localU/localV 是 0-1 范围内的图标 UV（相对于 64x64 图集）
        // 需要映射到图集中的实际子纹理区域
        vertex.texCoord.x = static_cast<f32>(region->u0 + localU * atlasDu);
        vertex.texCoord.y = static_cast<f32>(region->v0 + localV * atlasDv);
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
    return normalizedTypeId != ::mc::entity::EntityTypeKeys::ITEM &&
        normalizedTypeId != ::mc::entity::EntityTypeKeys::EXPERIENCE_ORB;
}

std::unique_ptr<model::EntityModel> EntityRendererManager::_createModelForEntity(
    ClientEntity& entity, core::AnimationContext& context)
{
    std::string normalizedId = normalizeEntityTypeId(entity.getTypeId());

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

    // 游泳动画渐变量（对应 MC 1.21.11 HumanoidRenderState.swimAmount =
    // LivingEntity.getSwimAmount(partialTick)）。ClientEntity::tick 本地推进
    // m_swimAmount/m_swimAmountO 副本，此处插值读取写入 context.swimAmount，
    // 供 DrownedModel::setAngles 的手臂/腿部游泳覆盖动画使用。
    context.swimAmount = entity.getInterpolatedSwimAmount(static_cast<f32>(context.partialTicks));

    // 北极熊站立动画
    const std::string& typeId = entity.getTypeId();
    if (typeId == "minecraft:polar_bear" || typeId == "polar_bear") {
        context.standingProgress = entity.getStandingAnimationScale(static_cast<f32>(context.partialTicks));
    } else {
        context.standingProgress = 0.0f;
    }

    // 河豚膨胀状态
    if (typeId == "minecraft:pufferfish" || typeId == "pufferfish") {
        context.puffState = entity.puffState();
    } else {
        context.puffState = 0;
    }

    // 吃草动画计时器（羊等实体的低头吃草动画）
    if (typeId == "minecraft:sheep" || typeId == "sheep") {
        context.eatAnimationTimer = entity.eatAnimationTimer();
    } else {
        context.eatAnimationTimer = 0;
    }

    // 撞飞攻击动画计时器（疣猪兽/僵尸疣兽的甩头攻击动画）
    if (typeId == "minecraft:hoglin" || typeId == "hoglin" || typeId == "minecraft:zoglin" || typeId == "zoglin") {
        context.attackAnimationTicks = entity.flingAnimationTicks();
    } else {
        context.attackAnimationTicks = 0;
    }

    // 狼甩水动画状态（对应 MC Wolf.tick() 的甩水状态机）
    if (typeId == "minecraft:wolf" || typeId == "wolf") {
        // 插值甩水进度：lerp(partialTicks, shakeAnimO, shakeAnim)
        const f32 shakeAnimO = entity.wolfShakeAnimO();
        const f32 shakeAnim = entity.wolfShakeAnim();
        context.wolfShakeAnim = shakeAnimO + (shakeAnim - shakeAnimO) * static_cast<f32>(context.partialTicks);

        // 插值乞求角度：lerp(partialTicks, interestedAngleO, interestedAngle)
        const f32 interestedO = entity.wolfInterestedAngleO();
        const f32 interested = entity.wolfInterestedAngle();
        context.wolfInterestedAngle = interestedO + (interested - interestedO) * static_cast<f32>(context.partialTicks);

        // 计算湿润着色值（对应 MC Wolf.getWetShade()）
        // !isWet ? 1.0 : min(0.75 + shakeAnim / 2.0 * 0.25, 1.0)
        if (!entity.wolfIsWet()) {
            context.wolfWetShade = 1.0f;
        } else {
            context.wolfWetShade = std::min(0.75f + context.wolfShakeAnim / 2.0f * 0.25f, 1.0f);
        }

        // 愤怒状态（对应 MC 1.21.11 Wolf.isAngry()，由 NeutralMob 默认方法计算 angerTime > 0）
        // 数据流：服务端 WolfEntity::setAngry/setAngerTime 写入 DATA_ANGER_TIME_PARAM
        //   → EntityTracker 广播 EntityMetadataPacket
        //   → ClientEntity::syncMetadataFromDataManager 调用 setWolfIsAngry
        //   → 此处读取 entity.wolfIsAngry() 写入 context.isAngry
        //   → WolfModel::setAnimState 读取以决定尾巴 Y 旋转（愤怒时锁 1.539f ≈ 88°）
        context.isAngry = entity.wolfIsAngry();
    } else {
        context.wolfShakeAnim = 0.0f;
        context.wolfInterestedAngle = 0.0f;
        context.wolfWetShade = 1.0f;
        context.isAngry = false;
    }

    // 凋灵侧头朝向（对应 MC 1.21.11 WitherBoss.aiStep() 中 j=0..1 循环）
    // 客户端不运行 WitherEntity::aiStep()，由 ClientEntity::tickWitherSideHeads
    // 在 ClientEntityManager::tick() 中本地镜像计算侧头 yaw/pitch。
    // 此处从 ClientEntity 读取插值后的角度，并转换为模型所需的"身体相对"角度。
    //
    // MC 1.21.11 WitherBossModel.setupHeadRotation:
    //   head.yRot = (yHeadRots[index] - bodyRot) * PI / 180
    //   head.xRot = xHeadRots[index] * PI / 180
    // 因此 context.witherSideHeadYaw 存储 (absoluteYaw - bodyRot)，
    // context.witherSideHeadPitch 存储 absolutePitch。
    if (typeId == "minecraft:wither" || typeId == "wither") {
        // bodyRot 对应 MC yBodyRot，Cubium 中为 renderYawOffset（= yaw on ClientEntity）
        // 使用插值后的 bodyYaw（与 netHeadYaw 计算中使用的 bodyYaw 一致）
        const f32 bodyYawF = static_cast<f32>(bodyYaw);
        for (i32 i = 0; i < 2; ++i) {
            const f32 absYaw = entity.getInterpolatedWitherSideHeadYaw(i, static_cast<f32>(context.partialTicks));
            const f32 absPitch = entity.getInterpolatedWitherSideHeadPitch(i, static_cast<f32>(context.partialTicks));
            // 转换为身体相对偏航（对应 MC yHeadRots[i] - bodyRot）
            f32 relYaw = absYaw - bodyYawF;
            // 归一化到 [-180, 180]
            relYaw = math::wrapDegrees(relYaw);
            context.witherSideHeadYaw[i] = relYaw;
            context.witherSideHeadPitch[i] = absPitch;
        }
    } else {
        context.witherSideHeadYaw.fill(0.0f);
        context.witherSideHeadPitch.fill(0.0f);
    }

    // 皮肤区域版本号：玩家实体记录图集 contentVersion（动态皮肤区域增删触发重做 UV）；
    // 非玩家写入常量 0，避免他人换肤波及非玩家实体的 mesh 重做。
    if (entity.entityType() == ::mc::entity::VanillaEntityTypeKeys::PLAYER) {
        context.skinRegionVersion = m_textureAtlas ? m_textureAtlas->contentVersion() : 0;
    } else {
        context.skinRegionVersion = 0;
    }

    // 计算哈希
    context.computeHash();

    // 使用 ModelFactory 创建模型
    auto model = model::ModelFactory::instance().createModel(normalizedId);
    if (model) {
        // 通用 BipedModel 状态推送：鞘翅飞行状态 + 速度因子
        // 必须在 setAngles 之前调用，因为 setAngles 中使用 m_speedValue 作为手臂/腿部摆动振幅的除数。
        // 覆盖所有 BipedModel 派生模型（玩家、僵尸、骷髅、末影人、猪灵等）。
        auto* bipedModel = dynamic_cast<model::BipedModel*>(model.get());
        if (bipedModel != nullptr) {
            _applyBipedElytraState(*bipedModel, entity);
        }

        // 海豚运动状态推送：水平速度平方
        // 必须在 setAngles 之前调用，因为 DolphinModel::setAngles 直接读取 m_motionMagnitude
        // 决定是否播放游泳摆尾动画。对应 MC 1.21.11 DolphinRenderer 中
        // isMoving = deltaMovement.horizontalDistanceSqr() > 1.0E-7 的填充。
        auto* dolphinModel = dynamic_cast<model::aquatic::DolphinModel*>(model.get());
        if (dolphinModel != nullptr) {
            _applyDolphinMotionState(*dolphinModel, entity);
        }

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

        // 羊吃草动画
        if (normalizedId == "sheep" || normalizedId == "minecraft:sheep") {
            auto* sheepModel = dynamic_cast<model::animal::SheepModel*>(model.get());
            if (sheepModel != nullptr) {
                sheepModel->setEatAnimationTimer(context.eatAnimationTimer);
                sheepModel->setLivingAnimations(context.limbSwing, context.limbSwingAmount, context.partialTicks);
            }
        }

        // 疣猪兽/僵尸疣兽攻击动画（甩头）
        if (normalizedId == "hoglin" || normalizedId == "minecraft:hoglin" || normalizedId == "zoglin" ||
            normalizedId == "minecraft:zoglin") {
            auto* boarModel = dynamic_cast<model::nether::BoarModel*>(model.get());
            if (boarModel != nullptr) {
                boarModel->setAttackAnimationTicks(context.attackAnimationTicks);
            }
        }

        // 狼甩水动画（对应 MC WolfModel.setupAnim + WolfRenderState）
        if (normalizedId == "wolf" || normalizedId == "minecraft:wolf") {
            auto* wolfModel = dynamic_cast<model::animal::WolfModel*>(model.get());
            if (wolfModel != nullptr) {
                // isSitting 来自 ClientEntity 元数据状态
                // isAngry 来自 context.isAngry（由 DATA_ANGER_TIME_PARAM 同步）
                // isWet 使用 wolfIsWet（对应 MC Wolf.isWet）
                // tailAngle 对应 MC 1.21.11 Wolf.getTailAngle()：
                //   - 愤怒时返回 1.539f（≈88°，尾巴笔直抬起）
                //   - 驯服时根据生命值变化（health 越低尾巴越低）
                //   - 未驯服时返回 PI/5（≈36°）
                //   客户端目前无 health 元数据，未驯服/驯服状态均使用 PI/5 作为基础值，
                //   愤怒状态覆盖为 1.539f。TODO: 待 health 元数据同步后实现动态 tailAngle。
                // shakeAnim 使用插值后的 context.wolfShakeAnim
                // interestedAngle 使用 context.wolfInterestedAngle * 0.15 * PI（对应 MC getHeadRollAngle）
                const f32 tailAngle = context.isAngry ? 1.539f : static_cast<f32>(math::PI / 5.0);
                const f32 interestedHeadRoll = context.wolfInterestedAngle * 0.15f * static_cast<f32>(math::PI);
                wolfModel->setAnimState(static_cast<bool>(context.isSitting),
                    static_cast<bool>(context.isAngry),
                    entity.wolfIsWet(),
                    tailAngle,
                    context.wolfShakeAnim,
                    interestedHeadRoll);
                // setLivingAnimations 处理坐下/站立姿态、步态动画和抖水 Z 旋转
                wolfModel->setLivingAnimations(context.limbSwing, context.limbSwingAmount, context.partialTicks);
                // 设置湿润着色（对应 MC WolfRenderer 中的 getWetShade tint）
                wolfModel->setTint(context.wolfWetShade, context.wolfWetShade, context.wolfWetShade);
            }
        }

        // 马及其变种（驴/骡/骷髅马/僵尸马）主模型动画
        // 第三人称马走 GPU 管线路径，需要调用 setLivingAnimations 处理步态/姿态。
        // ClientEntity 当前未镜像马鞍/骑乘/grassEating/rearing 等马专用状态
        // （AbstractHorseEntity 用非 DataParameter 的本地标志位），故此处仅推进
        // 通用步态动画，专用姿态保持模型默认值。TODO: 待马专用状态同步到客户端后补齐。
        if (normalizedId == "horse" || normalizedId == "minecraft:horse" || normalizedId == "donkey" ||
            normalizedId == "minecraft:donkey" || normalizedId == "mule" || normalizedId == "minecraft:mule" ||
            normalizedId == "skeleton_horse" || normalizedId == "minecraft:skeleton_horse" ||
            normalizedId == "zombie_horse" || normalizedId == "minecraft:zombie_horse") {
            auto* horseModel = dynamic_cast<model::animal::HorseModel*>(model.get());
            if (horseModel != nullptr) {
                horseModel->setLivingAnimations(context.limbSwing, context.limbSwingAmount, context.partialTicks);
            }
        }

        // 羊驼及商队羊驼主模型动画
        // ClientEntity 当前未镜像 hasChest 等羊驼专用状态，仅推进通用步态动画。
        // TODO: 待羊驼专用状态同步到客户端后补齐 setHasChest。
        if (normalizedId == "llama" || normalizedId == "minecraft:llama" || normalizedId == "trader_llama" ||
            normalizedId == "minecraft:trader_llama") {
            auto* llamaModel = dynamic_cast<model::animal::LlamaModel*>(model.get());
            if (llamaModel != nullptr) {
                llamaModel->setLivingAnimations(context.limbSwing, context.limbSwingAmount, context.partialTicks);
            }
        }

        // 猫主模型动画
        // ClientEntity 已镜像 isCatLieDown/isCatRelaxStateOne/isSitting（由 CatEntity
        // 的 DataParameter 同步），但缺插值量 getLieDownAmount/getRelaxStateOneAmount，
        // 此处以 bool 转 0.0/1.0 近似。sleepPoseAmount 恒 0（无对应元数据）。
        if (normalizedId == "cat" || normalizedId == "minecraft:cat") {
            auto* catModel = dynamic_cast<model::animal::CatModel*>(model.get());
            if (catModel != nullptr) {
                const f32 lieDownAmount = entity.isCatLieDown() ? 1.0f : 0.0f;
                const f32 relaxStateAmount = entity.isCatRelaxStateOne() ? 1.0f : 0.0f;
                catModel->setCatAnimState(lieDownAmount, relaxStateAmount, 0.0f);
                catModel->setSitting(entity.isSitting());
                catModel->setLivingAnimations(context.limbSwing, context.limbSwingAmount, context.partialTicks);
            }
        }

        // 豹猫主模型动画
        // ClientEntity 当前未镜像 crouching(pose)/fleeing 等豹猫专用状态，
        // 仅推进通用步态动画。TODO: 待豹猫专用状态同步后补齐 setCrouching/setSprinting。
        if (normalizedId == "ocelot" || normalizedId == "minecraft:ocelot") {
            auto* ocelotModel = dynamic_cast<model::animal::OcelotModel*>(model.get());
            if (ocelotModel != nullptr) {
                ocelotModel->setLivingAnimations(context.limbSwing, context.limbSwingAmount, context.partialTicks);
            }
        }

        // 兔子跳跃动画（对应 MC 1.21.11 RabbitModel.setupAnim + Rabbit.getJumpCompletion）
        // 数据流：服务端 RabbitEntity::startJumping() 广播 RabbitJump(1)
        //   → 客户端 ClientEntity::setRabbitJumpStart() (jumpDuration=10)
        //   → ClientEntity::tick() 中 tickRabbitJump() 推进 jumpTicks
        //   → 此处读取 rabbitJumpCompletion(partialTick) 计算 jumpRotation
        //   → RabbitModel::setJumpRotation(sin(completion * PI))
        //   → setAngles 中根据 m_jumpRotation 计算 thigh/foot/arm 旋转角度
        if (normalizedId == "rabbit" || normalizedId == "minecraft:rabbit") {
            auto* rabbitModel = dynamic_cast<model::animal::RabbitModel*>(model.get());
            if (rabbitModel != nullptr) {
                // MC 1.21.11 RabbitRenderer.getWhiteOverlayPower + RabbitModel.setupAnim:
                //   this.jumpRotation = Mth.sin(rabbit.getJumpCompletion(partialTick) * PI);
                const f32 jumpCompletion = entity.rabbitJumpCompletion(static_cast<f32>(context.partialTicks));
                const f32 jumpRotation = std::sin(jumpCompletion * static_cast<f32>(math::PI));
                rabbitModel->setJumpRotation(jumpRotation);
                // setLivingAnimations 在 RabbitModel 中已不需要做额外工作
                // （jumpRotation 已通过 setJumpRotation 设置，setAngles 中会使用）
                rabbitModel->setLivingAnimations(context.limbSwing, context.limbSwingAmount, context.partialTicks);
            }
        }

        // 河豚膨胀状态模型切换
        if (normalizedId == "pufferfish" || normalizedId == "minecraft:pufferfish") {
            // ModelFactory 默认创建 PufferfishSmallModel，
            // 根据膨胀状态替换为正确的模型
            if (context.puffState == 1) {
                model = std::make_unique<model::aquatic::PufferfishMediumModel>();
                model->setAngles(context.limbSwing,
                    context.limbSwingAmount,
                    context.ageInTicks,
                    context.netHeadYaw,
                    context.headPitch,
                    context.scale * 16.0);
            } else if (context.puffState >= 2) {
                model = std::make_unique<model::aquatic::PufferfishBigModel>();
                model->setAngles(context.limbSwing,
                    context.limbSwingAmount,
                    context.ageInTicks,
                    context.netHeadYaw,
                    context.headPitch,
                    context.scale * 16.0);
            }
            // puffState == 0 使用 ModelFactory 创建的 PufferfishSmallModel
        }

        // 玩家弩装填/持握动画参数
        // 第三人称玩家走 GPU 管线路径，需要在此设置 BipedModel 的弩状态字段，
        // 否则 handleCrossbowCharge 中 progress 恒为 0、副手角度恒为初始值。
        if (normalizedId == "player" || normalizedId == "minecraft:player") {
            auto* playerModel = dynamic_cast<model::player::PlayerModel*>(model.get());
            if (playerModel != nullptr) {
                _applyPlayerCrossbowState(*playerModel, entity, context);
            }
        }

        // 骷髅拉弓手臂姿态
        // 第三人称骷髅走 GPU 管线路径，需要在此根据 ClientEntity::isChargingBow()
        // （通过 AbstractSkeletonEntity::DATA_CHARGING_BOW_PARAM 同步）设置
        // SkeletonModel 的右臂 ArmPose 为 BowAndArrow，触发 BipedModel::handleRightArmPose
        // 的拉弓动画。覆盖普通骷髅（skeleton）、流浪者（stray）和沼骸骨（bogged）。
        // 凋灵骷髅（wither_skeleton）不持弓，不进入此分支。
        if (normalizedId == "skeleton" || normalizedId == "minecraft:skeleton" || normalizedId == "stray" ||
            normalizedId == "minecraft:stray" || normalizedId == "bogged" || normalizedId == "minecraft:bogged") {
            auto* skeletonModel = dynamic_cast<model::monster::SkeletonModel*>(model.get());
            if (skeletonModel != nullptr) {
                _applySkeletonArmPose(*skeletonModel, entity, context);
            }
        }

        // 僵尸激怒/攻击中手臂动画
        // 第三人称僵尸及其变体走 GPU 管线路径，需要在此根据 ClientEntity::isAggressive()
        // （通过 MobEntity::DATA_MOB_FLAGS_PARAM 位 2 同步）与 getInterpolatedSwingProgress
        // 推送 ZombieModel 的 setAggressive / setSwingProgress，并重新调用 setAngles
        // 使 animateZombieArms 的攻击抬臂动画生效。
        // 覆盖普通僵尸（zombie）、尸壳（husk）、溺尸（drowned）、僵尸村民
        // （zombie_villager）、巨人（giant）。变体模型均继承自 ZombieModel，
        // 故 dynamic_cast<ZombieModel*> 可统一命中。
        if (normalizedId == "zombie" || normalizedId == "minecraft:zombie" || normalizedId == "husk" ||
            normalizedId == "minecraft:husk" || normalizedId == "drowned" || normalizedId == "minecraft:drowned" ||
            normalizedId == "zombie_villager" || normalizedId == "minecraft:zombie_villager" ||
            normalizedId == "giant" || normalizedId == "minecraft:giant") {
            auto* zombieModel = dynamic_cast<model::monster::ZombieModel*>(model.get());
            if (zombieModel != nullptr) {
                // 溺尸三叉戟投掷手臂姿态：必须在 _applyZombieState 之前设置，因为后者
                // 末尾会重新调用 setAngles，DrownedModel::setAngles 读取 m_rightArmPose
                // 重新应用 ThrowSpear 姿态（animateZombieArms 会覆盖该姿态）。对应 MC
                // 1.21.11 DrownedRenderer.getArmPose 在 extractRenderState 阶段填充
                // rightArmPos，setupAnim 阶段读取。
                if (normalizedId == "drowned" || normalizedId == "minecraft:drowned") {
                    auto* drownedModel = dynamic_cast<model::monster::DrownedModel*>(zombieModel);
                    if (drownedModel != nullptr) {
                        _applyDrownedTridentPose(*drownedModel, entity);
                    }
                }
                _applyZombieState(*zombieModel, entity, context);
            }
        }

        // 末影人携带方块/尖叫状态
        // 第三人称末影人走 GPU 管线路径，需要在此根据 ClientEntity::endermanHeldBlockState()
        // 和 endermanScreaming() 推送 EndermanModel 的 setCarrying/setAttacking，
        // 否则 setAngles 中的携带方块姿态和攻击姿态不会生效。
        // 对应 CPU 路径中 EndermanRenderer::_updateEndermanState 的逻辑。
        if (normalizedId == "enderman" || normalizedId == "minecraft:enderman") {
            auto* endermanModel = dynamic_cast<model::monster::EndermanModel*>(model.get());
            if (endermanModel != nullptr) {
                endermanModel->setCarrying(entity.endermanHeldBlockState() != nullptr);
                endermanModel->setAttacking(entity.endermanScreaming());
            }
        }

        // 凋灵侧头朝向（对应 MC 1.21.11 WitherBossModel.setupHeadRotation）
        // 第三人称凋灵走 GPU 管线路径，需要在此将 AnimationContext 中已计算的
        // 侧头偏航/俯仰角传递给 WitherModel，覆盖 setAngles 中默认的"复制主头"逻辑。
        // context.witherSideHeadYaw 已是"身体相对"偏航（对应 MC yHeadRots[i] - bodyRot），
        // context.witherSideHeadPitch 是绝对俯仰（对应 MC xHeadRots[i]）。
        if (normalizedId == "wither" || normalizedId == "minecraft:wither") {
            auto* witherModel = dynamic_cast<model::monster::WitherModel*>(model.get());
            if (witherModel != nullptr) {
                witherModel->setSideHeadRotations(context.witherSideHeadYaw[0],
                    context.witherSideHeadPitch[0],
                    context.witherSideHeadYaw[1],
                    context.witherSideHeadPitch[1]);
            }
        }

        return model;
    }

    spdlog::warn("_createModelForEntity: No model found for entity type: {}", normalizedId);
    return nullptr;
}

void EntityRendererManager::_applyPlayerCrossbowState(
    model::player::PlayerModel& playerModel, ClientEntity& entity, const core::AnimationContext& context)
{
    // 默认值：弩参数归零，ArmPose 不设置（保持 setAngles 默认的 Empty）
    f32 ticksUsingItem = 0.0f;
    f32 maxChargeDuration = 0.0f;

    // 仅本地玩家可通过 Player 对象读取 use-item 状态
    // TODO: 远程玩家的 use-item 状态需通过网络同步到 ClientEntity 后才能驱动弩动画。
    //       当前远程玩家在第三人称下不会出现弩装填/持握动画，弓箭/三叉戟等姿态
    //       同样缺失。实现路径：
    //       1. 服务端 LivingEntity 在 setActiveHand/stopActiveHand 时发送
    //          ServerboundUseItemPacket 对应的客户端状态包
    //          （ClientboundUseItemPacket 或扩展 metadata）
    //       2. ClientEntity 增加 m_activeHand / m_activeItem / m_activeItemUseCount 字段
    //          及其元数据同步逻辑（DATA_LIVING_FLAGS_PARAM bit 0/1）
    //       3. 此处通过 entity.activeHand() / entity.activeItem() / entity.getItemInUseCount()
    //          读取，与 _applyPlayerCrossbowState 中本地玩家的 use-item 读取逻辑合并到公共工具函数
    ::mc::Player* localPlayer =
        (m_localPlayerAccessor && entity.id() == m_localPlayerEntityId) ? m_localPlayerAccessor() : nullptr;

    if (localPlayer != nullptr && localPlayer->isUsingItem()) {
        const ::mc::ItemStack& activeStack = localPlayer->getActiveItem();
        const ::mc::Item* activeItem = activeStack.getItem();
        if (activeItem == ::mc::Items::CROSSBOW) {
            // maxCrossbowChargeDuration = CrossbowItem.getChargeTime(stack)
            maxChargeDuration = static_cast<f32>(::mc::item::CrossbowItem::getChargeTime(activeStack));
            // ticksUsingItem = useDuration - useItemRemaining + partialTick
            // Cubium 中 CrossbowItem::getUseDuration = getChargeTime + 3
            const i32 useDuration = activeItem->getUseDuration(activeStack);
            const i32 remaining = localPlayer->getItemInUseCount();
            const i32 elapsed = useDuration - remaining;
            ticksUsingItem = static_cast<f32>(elapsed) + static_cast<f32>(context.partialTicks);
        }
    }

    playerModel.setMaxCrossbowChargeDuration(maxChargeDuration);
    playerModel.setCrossbowChargeTicks(ticksUsingItem);

    // ArmPose 解析（含双手协调与主/副手映射）
    if (localPlayer != nullptr) {
        const auto poses = renderer::player::PlayerArmPoseResolver::resolveArmPoses(*localPlayer);
        playerModel.setArmPose(poses.leftArmPose, poses.rightArmPose);
        playerModel.setMainHand(localPlayer->isRightHanded() ? model::HandSide::Right : model::HandSide::Left);
        if (localPlayer->isSwingInProgress()) {
            playerModel.setSwingingHand(localPlayer->swingingHand() == ::mc::Hand::MainHand
                    ? (localPlayer->isRightHanded() ? model::HandSide::Right : model::HandSide::Left)
                    : (localPlayer->isRightHanded() ? model::HandSide::Left : model::HandSide::Right));
        }
        // 蹲伏/游泳状态（影响 setAngles 中的身体角度）
        playerModel.setCrouching(localPlayer->isSneaking());
        playerModel.setSwimming(localPlayer->isSwimming());
        // TODO: 远程玩家的 ArmPose/主手/挥动手/蹲伏/游泳状态需通过网络同步到
        //       ClientEntity 后在此处补齐
    }

    // 关键：重新调用 setAngles 让弩参数与 ArmPose 通过 handleRightArmPose/
    // handleLeftArmPose 生效。_createModelForEntity 在创建模型后已调用过一次
    // setAngles，但那时 ArmPose/弩参数尚未设置，handleRightArmPose/
    // handleLeftArmPose 中的 CrossbowCharge/CrossbowHold 分支不会触发。此处
    // 重新 setAngles 使弩动画在 GPU 管线路径下真正生效，避免形成孤岛代码。
    playerModel.setAngles(context.limbSwing,
        context.limbSwingAmount,
        context.ageInTicks,
        context.netHeadYaw,
        context.headPitch,
        context.scale * 16.0);
}

void EntityRendererManager::_applySkeletonArmPose(
    model::monster::SkeletonModel& skeletonModel, const ClientEntity& entity, const core::AnimationContext& context)
{
    // 对应 MC 1.21.11 AbstractSkeletonRenderer.getArmPose：
    //   当 isAggressive && mainHandItem.is(Items.BOW) 时返回 BOW_AND_ARROW。
    // 本项目用 chargingBow 布尔字段替代 isAggressive + isHoldingBow 组合判断，
    // 由 AbstractSkeletonEntity::tick 根据 isUsingItem + 持弓状态设置，
    // 通过 DATA_CHARGING_BOW_PARAM 同步到 ClientEntity::isChargingBow()。
    //
    // 骷髅默认右撇子（MC 原版骷髅不区分主手，统一用右手持弓拉弓），
    // 故仅设置右臂 ArmPose。BipedModel::handleRightArmPose 的 BowAndArrow 分支
    // 会同时设置右臂和左臂的角度（双手拉弓协调），无需额外设置左臂。
    using model::ArmPose;
    if (entity.isChargingBow()) {
        skeletonModel.setRightArmPose(ArmPose::BowAndArrow);
        // 左臂保持 Empty，由 BipedModel::handleRightArmPose 的 BowAndArrow 分支
        // 统一设置双手角度（右臂 Y=-0.1+headYaw，左臂 Y=0.1+headYaw+0.4）。
    } else {
        skeletonModel.setRightArmPose(ArmPose::Empty);
    }

    // TODO: 弩姿态（CrossbowCharge/CrossbowHold）需要 use-item 状态网络同步
    // （参考 _applyPlayerCrossbowState 的 TODO 注释）。待 ClientEntity 增加
    // isUsingItem/getActiveItem/getItemInUseCount 后，在此处根据持弩状态设置
    // CrossbowCharge/CrossbowHold 姿态，并调用 setCrossbowChargeTicks/
    // setMaxCrossbowChargeDuration 推送弩装填进度。

    // 关键：重新调用 setAngles 让 ArmPose 通过 handleRightArmPose/handleLeftArmPose
    // 生效。_createModelForEntity 在创建模型后已调用过一次 setAngles，但那时
    // ArmPose 尚未设置，handleRightArmPose 的 BowAndArrow 分支不会触发。此处
    // 重新 setAngles 使拉弓动画在 GPU 管线路径下真正生效，避免形成孤岛代码。
    skeletonModel.setAngles(context.limbSwing,
        context.limbSwingAmount,
        context.ageInTicks,
        context.netHeadYaw,
        context.headPitch,
        context.scale * 16.0);
}

void EntityRendererManager::_applyZombieState(
    model::monster::ZombieModel& zombieModel, const ClientEntity& entity, const core::AnimationContext& context)
{
    // 对应 MC 1.21.11 AbstractZombieModel.setupAnim() 调用：
    //   AnimationUtils.animateZombieArms(leftArm, rightArm, isAggressive, renderState);
    //
    // isAggressive：通过 MobEntity::DATA_MOB_FLAGS_PARAM 位 2 同步到 ClientEntity。
    //   服务端写入路径：MeleeAttackGoal::startExecuting → setAggroed(true)
    //     → MobEntity::setAggressive(true) → 数据参数置位 MOB_FLAG_AGGRESSIVE。
    //   客户端读取路径：syncMetadataFromDataManager → setIsAggressive。
    //
    // attackTime（对应 m_swingProgress）：挥手进度，由 getInterpolatedSwingProgress
    //   提供（基于 LivingEntity 的 m_swingProgressInt 与 partialTicks 插值）。
    //   服务端触发路径：LivingEntity::swing() 广播 EntityAnimationPacket(SwingMainHand)，
    //     客户端收到后调用 LivingEntity::swing() 重置 m_swingProgressInt = -1 并启动挥手。
    //
    // ZombieModel::setAngles 读取 m_swingProgress 计算 f2/f3 攻击动画因子，并按
    // m_isAggressive 选择基础抬臂角度 -PI/(aggressive?1.5:2.25)。两项均须在 setAngles
    // 之前推送，否则使用模型默认值（aggressive=false, swingProgress=0）导致攻击姿态缺失。

    zombieModel.setAggressive(entity.isAggressive());
    zombieModel.setSwingProgress(entity.getInterpolatedSwingProgress(static_cast<f32>(context.partialTicks)));

    // 推送游泳动画渐变量与实际游泳状态
    // 对应 MC 1.21.11 HumanoidMobRenderer.extractHumanoidRenderState：
    //   p_362998_.swimAmount = p_365104_.getSwimAmount(p_363706_);
    // 以及 HumanoidRenderState.isVisuallySwimming 的填充（来自 LivingEntity.isVisuallySwimming()）。
    // ZombieModel/DrownedModel 通过 m_swimAnimation / m_isActuallySwimming 字段在 setAngles 中
    // 读取这两个值。必须在 setAngles 之前推送，否则使用模型默认值（0.0/false）导致
    // DrownedModel 的游泳手臂/腿部覆盖动画缺失。BipedModel 的 handleSwimAnimation（玩家爬行式
    // 游泳）也会读取 m_swimAnimation，但对僵尸/溺尸而言 setAngles 不会调用 handleSwimAnimation，
    // 因此该推送只对 DrownedModel::setAngles 中的溺尸专属覆盖动画生效。
    zombieModel.setSwimAnimation(context.swimAmount);
    zombieModel.setActuallySwimming(context.isSwimming);

    // 关键：重新调用 setAngles 让 setAggressive / setSwingProgress / setSwimAnimation
    // 通过 animateZombieArms / DrownedModel 覆盖逻辑生效。_createModelForEntity 在创建模型后
    // 已调用过一次 setAngles，但那时 m_isAggressive 与 m_swingProgress 均为模型默认值，
    // 攻击抬臂动画与游泳覆盖动画不会正确触发。此处重新 setAngles 使激怒抬臂、挥手动画、
    // 溺尸游泳覆盖在 GPU 管线路径下真正生效，避免形成孤岛代码。
    zombieModel.setAngles(context.limbSwing,
        context.limbSwingAmount,
        context.ageInTicks,
        context.netHeadYaw,
        context.headPitch,
        context.scale * 16.0);
}

void EntityRendererManager::_applyDrownedTridentPose(
    model::monster::DrownedModel& drownedModel, const ClientEntity& entity)
{
    // 对应 MC 1.21.11 DrownedRenderer.getArmPose：
    //   if (entity.getMainArm() == hand && entity.isAggressive() &&
    //       itemHeld.is(Items.TRIDENT))
    //     return HumanoidModel.ArmPose.THROW_TRIDENT;
    //
    // 僵尸类实体在 MC 原版中始终为右撇子（无 MainArm NBT 字段，LivingEntity.getMainArm()
    // 默认返回 RIGHT），故仅设置右臂 ArmPose，与 _applySkeletonArmPose 仅设置右臂
    // BowAndArrow 的处理方式一致。
    //
    // 必须在 _applyZombieState 之前调用：_applyZombieState 末尾会重新调用 setAngles，
    // DrownedModel::setAngles 在 ZombieModel::setAngles（animateZombieArms 覆盖手臂角度）
    // 之后重新应用 ThrowSpear 姿态（xRot = xRot*0.5 - PI, yRot = 0）。若 ThrowSpear 在
    // setAngles 之后才设置，该重应用逻辑成为死代码。
    //
    // 信号选择：MC 原版同时检查 isAggressive() 与主手三叉戟物品。本项目当前溺尸的
    // 三叉戟是 m_hasTrident 行为标志（DrownedTridentAttackGoal 据此激活），而非装备
    // 物品（DrownedEntity 未在 populateDefaultEquipmentSlots 中装备三叉戟到主手），
    // 且 ClientEntity 尚未同步主手装备（getMainHandItem 恒为 nullptr）。故此处以
    // isAggressive() 作为投掷姿态的唯一触发信号——DrownedTridentAttackGoal::startExecuting
    // 会置位 aggroed，MeleeAttackGoal 也会置位 aggroed，两者均对应「激怒中即将攻击」
    // 的语义，ThrowSpear 姿态在此期间显示是合理的近似。待 ClientEntity 主手装备同步
    // 落地后，可恢复完整的三叉戟物品判定。
    using model::ArmPose;
    const bool shouldThrowSpear = entity.isAggressive();
    if (shouldThrowSpear) {
        drownedModel.setRightArmPose(ArmPose::ThrowSpear);
    } else {
        drownedModel.setRightArmPose(ArmPose::Empty);
    }
}

void EntityRendererManager::_applyBipedElytraState(model::BipedModel& bipedModel, const ClientEntity& entity)
{
    // 对应 MC 1.21.11 HumanoidMobRenderer.extractHumanoidRenderState 中 isFallFlying /
    // speedValue 的填充逻辑。纯计算抽取到 elytra::computeSpeedValue 自由函数中，
    // 便于在单元测试中直接验证公式（无需链接 Vulkan/EntityRendererManager）。
    //
    // Cubium 中 ClientEntity::velocity() 返回当前 tick 的速度向量（无 prevVelocity 字段），
    // 直接使用 velocity().lengthSquared() 作为 deltaMovement.lengthSqr() 的等价物。
    // speedValue 作为手臂/腿部摆动振幅的除数（见 BipedModel::setAngles），越大摆动越慢。
    const bool isFallFlying = entity.isFallFlying();
    const f32 lengthSq = entity.velocity().lengthSquared();
    const f32 speedValue = model::elytra::computeSpeedValue(isFallFlying, lengthSq);

    bipedModel.setFallFlying(isFallFlying);
    bipedModel.setSpeedValue(speedValue);
}

void EntityRendererManager::_applyDolphinMotionState(
    model::aquatic::DolphinModel& dolphinModel, const ClientEntity& entity)
{
    // 对应 MC 1.21.11 DolphinRenderer：
    //   p_364903_.isMoving = p_480257_.getDeltaMovement().horizontalDistanceSqr() > 1.0E-7;
    // Vec3.horizontalDistanceSqr() 只取 xz 分量（x*x + z*z），不含 Y。
    // 因此不能直接用 velocity().lengthSquared()（那是 3D 含 Y 的）。
    // DolphinModel::setAngles 依据 m_motionMagnitude 是否超过 MOTION_THRESHOLD (1.0E-7)
    // 决定播放游泳摆尾动画或恢复静态尾鳍角度。
    const Vector3 vel = entity.velocity();
    const f64 horizontalDistanceSqr = static_cast<f64>(vel.x) * vel.x + static_cast<f64>(vel.z) * vel.z;
    dolphinModel.setMotionMagnitude(horizontalDistanceSqr);
}

pipeline::EntityMesh* EntityRendererManager::getOrCreateAnimatedMesh(
    ClientEntity& entity, model::EntityModel& model, const core::AnimationContext& context)
{
    if (!m_pipeline || !m_animatedMeshCache) {
        return nullptr;
    }

    std::string normalizedId = normalizeEntityTypeId(entity.getTypeId());

    // 设置 UV 重映射回调（按 entityId 识别玩家皮肤区域，非玩家走 typeId 默认路径）
    m_animatedMeshCache->setUvRemapFunc(
        [this](EntityInstanceId entityId, const std::string& typeId, std::vector<ModelVertex>& vertices) {
            _remapUvToAtlasRegion(entityId, typeId, vertices);
        });

    return m_animatedMeshCache->getOrUpdateMesh(entity.id(), model, normalizedId, context, *m_pipeline);
}

pipeline::EntityMesh* EntityRendererManager::getOrCreateProviderMesh(
    ClientEntity& entity, core::PipelineMeshProvider& provider)
{
    if (!m_pipeline) {
        return nullptr;
    }

    EntityInstanceId entityId = entity.id();
    auto it = m_meshes.find(entityId);

    if (it != m_meshes.end()) {
        // 已有缓存，检查是否需要更新
        if (provider.needsMeshUpdate(entity)) {
            std::vector<ModelVertex> vertices;
            std::vector<u32> indices;
            if (provider.generateMesh(entity, vertices, indices) && !vertices.empty() && !indices.empty()) {
                auto result = m_pipeline->updateMesh(it->second.mesh, vertices, indices);
                if (!result.success()) {
                    // 更新失败，重建网格
                    m_pipeline->destroyMesh(it->second.mesh);
                    auto createResult = m_pipeline->createMesh(vertices, indices);
                    if (createResult.success()) {
                        it->second.mesh = std::move(createResult.value());
                        it->second.itemRenderStateVersion = 0;
                    } else {
                        spdlog::error(
                            "EntityRendererManager: Failed to recreate provider mesh for entity {}", entityId);
                        m_meshes.erase(it);
                        return nullptr;
                    }
                }
            }
        }
        return &it->second.mesh;
    }

    // 首次创建
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    if (!provider.generateMesh(entity, vertices, indices) || vertices.empty() || indices.empty()) {
        return nullptr;
    }

    auto result = m_pipeline->createMesh(vertices, indices);
    if (!result.success()) {
        spdlog::error("EntityRendererManager: Failed to create provider mesh for entity {}: {}",
            entityId,
            result.error().message());
        return nullptr;
    }

    StaticMeshEntry entry;
    entry.mesh = std::move(result.value());
    entry.itemRenderStateVersion = 0;
    auto [newIt, inserted] = m_meshes.emplace(entityId, std::move(entry));
    return &newIt->second.mesh;
}

} // namespace mc::client::renderer::entity
