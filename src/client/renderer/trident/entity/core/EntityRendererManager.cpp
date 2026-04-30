#include "EntityRendererManager.hpp"
#include "AnimatedMeshCache.hpp"
#include "../renderer/animal/AnimalRenderers.hpp"
#include "../renderer/animal/WolfRenderer.hpp"
#include "../renderer/animal/OcelotRenderer.hpp"
#include "../renderer/animal/CatRenderer.hpp"
#include "../renderer/animal/HorseRenderer.hpp"
#include "../renderer/animal/VillagerRenderer.hpp"
#include "../renderer/monster/MonsterRenderers.hpp"
#include "../renderer/vehicle/VehicleRenderers.hpp"
#include "../renderer/projectile/ProjectileRenderers.hpp"
#include "../model/animal/WolfModel.hpp"
#include "../model/animal/OcelotModel.hpp"
#include "../model/animal/CatModel.hpp"
#include "../model/animal/HorseModel.hpp"
#include "../model/animal/VillagerModel.hpp"
#include "../model/monster/ZombieModel.hpp"
#include "../model/monster/SkeletonModel.hpp"
#include "../model/monster/CreeperModel.hpp"
#include "../model/monster/SpiderModel.hpp"
#include "../model/monster/EndermanModel.hpp"
#include "../model/monster/BlazeModel.hpp"
#include "../renderer/projectile/ItemEntityRenderer.hpp"
#include "../renderer/projectile/ExperienceOrbRenderer.hpp"
#include "../pipeline/EntityTextureAtlas.hpp"
#include "../util/ShadowRenderer.hpp"
#include "../effect/fire/FireEffect.hpp"
#include "client/resource/EntityTextureLoader.hpp"
#include "client/resource/ItemTextureAtlas.hpp"
#include "client/world/entity/ClientEntity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/entities/item/ItemEntity.hpp"
#include "common/entity/entities/orb/ExperienceOrbEntity.hpp"
#include "common/entity/experience/ExperienceUtils.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/core/Item.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector4.hpp"
#include <spdlog/spdlog.h>
#include <cmath>

namespace mc::client::renderer::entity {

// 导入核心命名空间中的类
using core::EntityRenderer;
using pipeline::EntityMesh;
using pipeline::EntityTextureAtlas;
using model::ModelVertex;

// 导入 EntityTypes 常量命名空间
namespace EntityTypes = ::mc::entity::EntityTypes;

namespace {

// MC 1.16.5 标准常量
// 参考 LivingRenderer.java:95 - translate(0.0D, -1.501D, 0.0D)
inline constexpr f64 MODEL_Y_OFFSET = 1.501;
inline constexpr f64 MODEL_SCALE = 1.0 / 16.0;
inline constexpr f64 MODEL_MESH_SCALE = 1.0;

// 阴影最大距离 - 参考 EntityRendererManager.java:260
inline constexpr f64 SHADOW_MAX_DISTANCE = 256.0;

/**
 * @brief 规范化实体类型ID
 *
 * 将实体类型ID转换为标准格式（带命名空间前缀）
 * 例如："pig" -> "minecraft:pig", "minecraft:cow" -> "minecraft:cow"
 */
String normalizeEntityTypeId(const String& typeId) {
    // 如果已有命名空间前缀，直接返回
    if (typeId.find(':') != String::npos) {
        return typeId;
    }
    // 添加默认命名空间
    return "minecraft:" + typeId;
}

} // anonymous namespace

EntityRendererManager::EntityRendererManager()
    : m_animatedMeshCache(std::make_unique<core::AnimatedMeshCache>())
{
}

EntityRendererManager::~EntityRendererManager() {
    // 销毁所有实体网格的Vulkan资源
    clearMeshes();
}

void EntityRendererManager::clearMeshes() {
    if (m_pipeline) {
        for (auto& [id, mesh] : m_meshes) {
            m_pipeline->destroyMesh(mesh);
        }
    }
    m_meshes.clear();
}

void EntityRendererManager::setTextureAtlas(const EntityTextureAtlas* textureAtlas) {
    m_textureAtlas = textureAtlas;
    // 图集变化后，旧网格的UV映射可能失效，强制重建
    clearMeshes();
}

void EntityRendererManager::registerRenderer(const String& typeId, RendererCreator creator) {
    m_creators[typeId] = std::move(creator);
}

EntityRenderer* EntityRendererManager::getRenderer(const String& typeId) {
    String normalizedId = normalizeEntityTypeId(typeId);
    auto it = m_renderers.find(normalizedId);
    if (it != m_renderers.end()) {
        return it->second.get();
    }
    return nullptr;
}

void EntityRendererManager::render(Entity& entity, f64 partialTicks) {
    // 获取实体类型ID并查找渲染器（已在 getOrCreateRenderer 中规范化）
    EntityRenderer* renderer = getOrCreateRenderer(entity.getTypeId());
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

void EntityRendererManager::renderWithPipeline(VkCommandBuffer cmd, ClientEntity& entity, f64 partialTicks) {
    if (!m_pipeline) {
        return;
    }

    // 检查是否为 ItemEntity 或 ExperienceOrb
    String normalizedType = normalizeEntityTypeId(entity.typeId());
    bool isItemEntity = (normalizedType == EntityTypes::ITEM);
    bool isExperienceOrb = (normalizedType == EntityTypes::EXPERIENCE_ORB);
    bool useAnimatedMesh = usesAnimatedMesh(normalizedType);

    // 对于 ItemEntity，使用 ItemTextureAtlas
    if (isItemEntity && m_itemTextureAtlas && m_itemTextureAtlas->isBuilt()) {
        // 绑定物品纹理图集
        m_pipeline->setTextureAtlas(m_itemTextureAtlas->imageView(), m_itemTextureAtlas->sampler());
    }

    // 获取渲染器
    EntityRenderer* renderer = getOrCreateRenderer(entity.typeId());

    // 获取或创建网格
    EntityMesh* mesh = nullptr;
    core::AnimationContext context;

    if (useAnimatedMesh && renderer && renderer->supportsAnimation()) {
        // 动画实体路径：使用动画网格缓存
        // 设置 partialTicks（用于动画插值）
        context.partialTicks = partialTicks;

        // 创建带动画的模型
        auto animModel = createModelForEntity(entity, context);
        if (animModel) {
            mesh = getOrCreateAnimatedMesh(entity, *animModel, context);
        }
    } else {
        // 静态实体路径：使用静态网格缓存
        mesh = getOrCreateMesh(entity);
    }

    if (!mesh || mesh->indexCount == 0) {
        // 恢复实体纹理图集
        if (isItemEntity && m_textureAtlas && m_textureAtlas->isBuilt()) {
            // TODO：需要在下一帧渲染前恢复，或在此处恢复
        }
        return;
    }

    // 绑定管线
    m_pipeline->bind(cmd);

    // 绑定相机描述符集（set = 0）
    if (m_cameraDescriptorSet != VK_NULL_HANDLE) {
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipeline->pipelineLayout(),
            0,  // set = 0
            1,
            &m_cameraDescriptorSet,
            0,
            nullptr
        );
    }

    // 绑定纹理描述符（set = 1）
    m_pipeline->bindTextureDescriptor(cmd);

    // 计算模型矩阵
    std::array<f64, 16> modelMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    const f32 partialTickF32 = static_cast<f32>(partialTicks);

    if (isItemEntity) {
        // ItemEntity 特殊渲染：应用浮动和旋转动画
        f64 bobOffset = calculateItemBobOffset(entity.ticksExisted(), partialTicks);
        f64 rotation = calculateItemRotation(entity.ticksExisted(), partialTicks);

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
            static_cast<f32>(posInterp.x),
            static_cast<f32>(posInterp.y + bobOffset),
            static_cast<f32>(posInterp.z));

        // 绘制网格（使用更大的缩放）
        m_pipeline->drawMesh(cmd, *mesh, modelMatrix, pos, MODEL_SCALE * 16.0f,
                             Vector4f(0.0f, 0.0f, 0.0f, 0.0f), 0.0f, 0.0f);
    } else if (isExperienceOrb) {
        // ExperienceOrb 特殊渲染：应用浮动动画和动态大小
        f64 bobOffset = calculateExperienceOrbBobOffset(entity.ticksExisted(), partialTicks);

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
            static_cast<f32>(posInterp.x),
            static_cast<f32>(posInterp.y + bobOffset),
            static_cast<f32>(posInterp.z));

        // 绘制网格
        m_pipeline->drawMesh(cmd, *mesh, modelMatrix, pos, scale,
                             Vector4f(0.0f, 0.0f, 0.0f, 0.0f), 0.0f, 0.0f);
    } else {
        // 普通实体渲染
        // MC 1.16.5 变换顺序 (LivingRenderer.java:93-95):
        // 1. scale(-1.0F, -1.0F, 1.0F) - X和Y都取反
        // 2. preRenderCallback()
        // 3. translate(0.0D, -1.501D, 0.0D) - 向下偏移

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

        // MC 1.16.5: scale(-1, -1, 1) - X和Y都取反
        // 在旋转后应用翻转
        for (int i = 0; i < 4; ++i) {
            modelMatrix[i * 4] *= -1.0;     // X列取反
            modelMatrix[i * 4 + 1] *= -1.0; // Y列取反
        }

        // Y偏移 - 原版是向下偏移 1.501
        modelMatrix[13] = MODEL_Y_OFFSET;

        // 获取插值位置
        Vector3 posInterp = entity.getInterpolatedPosition(partialTickF32);
        Vector3f pos(
            static_cast<f32>(posInterp.x),
            static_cast<f32>(posInterp.y),
            static_cast<f32>(posInterp.z));

        // 获取受伤和死亡时间（用于着色器效果）
        f32 hurtTime = static_cast<f32>(entity.hurtTime()) / 10.0f;  // 归一化到 0-1
        f32 deathTime = static_cast<f32>(entity.deathTime());

        // 绘制网格
        m_pipeline->drawMesh(cmd, *mesh, modelMatrix, pos, MODEL_SCALE,
                             Vector4f(0.0f, 0.0f, 0.0f, 0.0f), hurtTime, deathTime);

        // 渲染层（盔甲、手持物品等）
        if (renderer && renderer->supportsLayers()) {
            renderer->renderLayersPipelineClient(entity, cmd, context, *m_pipeline);
        }

        // 渲染火焰效果（如果实体正在燃烧）
        if (entity.isOnFire()) {
            effect::fire::FireEffect::renderFire(cmd, entity, partialTicks, *m_pipeline);
        }

        // 渲染阴影
        // 参考 MC 1.16.5 EntityRendererManager.java:258-264
        // if (this.options.entityShadows && this.renderShadow && entityrenderer.shadowSize > 0.0F && !entityIn.isInvisible())
        if (m_renderShadows && !isItemEntity && !isExperienceOrb) {
            // 使用渲染器的 shadowSize 而非 width * 0.5
            f64 shadowRadius = renderer ? renderer->shadowSize() : static_cast<f64>(entity.width()) * 0.5;
            f64 shadowOpaque = renderer ? renderer->shadowAlpha() : 0.8;

            // 检查阴影大小和透明度
            if (shadowRadius > 0.0 && shadowOpaque > 0.0 && !entity.isInvisible()) {
                // 计算到相机的距离衰减
                // 参考 MC 1.16.5 EntityRendererManager.java:260
                // float f = (float)((1.0D - d1 / 256.0D) * (double)entityrenderer.shadowOpaque);
                // 注意：这里需要相机位置，暂时使用简化版本
                util::ShadowRenderer::renderShadow(cmd, entity, partialTicks, shadowRadius, shadowOpaque, *m_pipeline);
            }
        }
    }

    // 恢复实体纹理图集（如果为 ItemEntity）
    if (isItemEntity && m_textureAtlas && m_textureAtlas->isBuilt()) {
        m_pipeline->setTextureAtlas(m_textureAtlas->imageView(), m_textureAtlas->sampler());
    }
}

bool EntityRendererManager::renderWithPipeline(VkCommandBuffer cmd, ClientEntity& entity, f64 partialTicks,
                                                const mc::math::frustum::Frustum& frustum) {
    // 使用 FrustumUtils 创建实体包围盒
    // 使用插值位置以获得平滑的剔除效果
    const f32 partialTickF32 = static_cast<f32>(partialTicks);
    Vector3 pos = entity.getInterpolatedPosition(partialTickF32);
    AxisAlignedBB aabb = mc::math::frustum::FrustumUtils::createEntityAABB(
        pos, entity.width(), entity.height());

    // 使用世界坐标 AABB 进行视锥剔除
    if (!frustum.isAABBVisibleWorld(aabb)) {
        return false;  // 实体不在视锥内，跳过渲染
    }

    // 实体在视锥内，正常渲染
    renderWithPipeline(cmd, entity, partialTicks);
    return true;
}

f64 EntityRendererManager::calculateItemBobOffset(u32 ticksExisted, f64 partialTick) const {
    // 参考 MC 1.16.5 ItemEntityRenderer
    // 浮动动画：sin(ticks * 0.1) * 0.1
    f64 ticks = static_cast<f64>(ticksExisted) + partialTick;
    return std::sin(ticks * 0.1f) * 0.1f + 0.2f;  // 0.2 是基础高度偏移
}

f64 EntityRendererManager::calculateItemRotation(u32 ticksExisted, f64 partialTick) const {
    // 参考 MC 1.16.5 ItemEntityRenderer
    // 旋转速度：每 tick 旋转 2 度
    return static_cast<f64>(ticksExisted) * 2.0f + partialTick * 2.0f;
}

f64 EntityRendererManager::calculateExperienceOrbBobOffset(u32 ticksExisted, f64 partialTick) const {
    // 参考 MC 1.16.5 ExperienceOrbRenderer
    // 经验球浮动动画：sin(ticks * 0.05) * 0.1 + 0.2
    f64 ticks = static_cast<f64>(ticksExisted) + partialTick;
    return std::sin(ticks * 0.05f) * 0.1f + 0.3f;  // 0.3 是基础高度偏移（略高于物品）
}

EntityMesh* EntityRendererManager::getOrCreateMesh(ClientEntity& entity) {
    EntityId id = entity.id();
    auto it = m_meshes.find(id);

    if (it != m_meshes.end()) {
        return &it->second;
    }

    // 生成新网格
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;

    if (!generateModelMesh(entity.typeId(), vertices, indices)) {
        return nullptr;
    }

    // 对于 ItemEntity，使用 ItemTextureAtlas 进行 UV 重映射
    String normalizedType = normalizeEntityTypeId(entity.typeId());
    if (normalizedType == entity::EntityTypes::ITEM) {
        remapItemEntityUv(entity, vertices);
    } else {
        // 普通实体使用实体纹理图集
        remapUvToAtlasRegion(normalizedType, vertices);
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

    EntityMesh mesh = std::move(result.value());
    mesh.posX = entity.x();
    mesh.posY = entity.y();
    mesh.posZ = entity.z();

    m_meshes[id] = std::move(mesh);
    return &m_meshes[id];
}

void EntityRendererManager::updateMesh(ClientEntity& entity) {
    EntityId id = entity.id();
    auto it = m_meshes.find(id);

    if (it == m_meshes.end()) {
        return;
    }

    // 重新生成网格
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;

    if (!generateModelMesh(entity.typeId(), vertices, indices)) {
        return;
    }

    (void)m_pipeline->updateMesh(it->second, vertices, indices);
}

void EntityRendererManager::removeMesh(EntityId entityId) {
    auto it = m_meshes.find(entityId);
    if (it != m_meshes.end()) {
        if (m_pipeline) {
            m_pipeline->destroyMesh(it->second);
        }
        m_meshes.erase(it);
    }
}

void EntityRendererManager::initializeDefaults() {
    // 使用 EntityTypes 常量注册渲染器，避免重复注册
    // 所有注册都使用规范化的命名空间格式
    namespace ET = entity::EntityTypes;
    using namespace renderer::animal;
    using namespace renderer::monster;
    using namespace renderer::projectile;

    // 动物渲染器
    registerRenderer(ET::PIG, []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<PigRenderer>();
    });
    registerRenderer(ET::COW, []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<CowRenderer>();
    });
    registerRenderer(ET::SHEEP, []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<SheepRenderer>();
    });
    registerRenderer(ET::CHICKEN, []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<ChickenRenderer>();
    });
    registerRenderer(ET::WOLF, []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<WolfRenderer>();
    });
    registerRenderer(ET::OCELOT, []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<OcelotRenderer>();
    });
    registerRenderer(ET::CAT, []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<CatRenderer>();
    });
    registerRenderer(ET::HORSE, []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<HorseRenderer>();
    });
    registerRenderer(ET::VILLAGER, []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<VillagerRenderer>();
    });

    // 怪物渲染器
    registerRenderer(ET::ZOMBIE, []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<ZombieRenderer>();
    });
    registerRenderer(ET::SKELETON, []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<SkeletonRenderer>();
    });
    registerRenderer(ET::CREEPER, []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<CreeperRenderer>();
    });
    registerRenderer(ET::SPIDER, []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<SpiderRenderer>();
    });
    registerRenderer(ET::ENDERMAN, []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<EndermanRenderer>();
    });
    registerRenderer(ET::BLAZE, []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<BlazeRenderer>();
    });

    // 载具渲染器
    renderer::vehicle::registerVehicleRenderers(*this);

    // 投掷物渲染器
    renderer::projectile::registerProjectileRenderers(*this);

    // ItemEntity 渲染器
    registerRenderer(ET::ITEM, [this]() -> std::unique_ptr<EntityRenderer> {
        auto renderer = std::make_unique<ItemEntityRenderer>();
        if (m_itemTextureAtlas) {
            renderer->setItemTextureAtlas(m_itemTextureAtlas);
        }
        return renderer;
    });

    // ExperienceOrb 渲染器
    registerRenderer(ET::EXPERIENCE_ORB, []() -> std::unique_ptr<EntityRenderer> {
        return std::make_unique<ExperienceOrbRenderer>();
    });

    spdlog::debug("EntityRendererManager: Registered entity types: 9 animals, 5 monsters, vehicles, projectiles");
}

EntityRenderer* EntityRendererManager::getOrCreateRenderer(const String& typeId) {
    // 规范化实体类型ID
    String normalizedId = normalizeEntityTypeId(typeId);

    // 先查找已创建的渲染器
    auto it = m_renderers.find(normalizedId);
    if (it != m_renderers.end()) {
        return it->second.get();
    }

    // 查找创建函数
    auto creatorIt = m_creators.find(normalizedId);
    if (creatorIt == m_creators.end()) {
        return nullptr;
    }

    // 创建渲染器
    auto renderer = creatorIt->second();
    EntityRenderer* ptr = renderer.get();
    m_renderers[normalizedId] = std::move(renderer);
    return ptr;
}

bool EntityRendererManager::generateModelMesh(const String& typeId,
                                               std::vector<ModelVertex>& vertices,
                                               std::vector<u32>& indices) {
    // 规范化实体类型ID，统一使用命名空间格式进行比较
    String normalizedId = normalizeEntityTypeId(typeId);

    // 使用 EntityTypes 常量进行比较
    namespace ET = entity::EntityTypes;
    using namespace model::animal;

    if (normalizedId == ET::PIG) {
        PigModel model;
        model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, MODEL_MESH_SCALE);
        model.generateMesh(vertices, indices, MODEL_MESH_SCALE);
        remapUvToAtlasRegion(normalizedId, vertices);
        return true;
    }
    if (normalizedId == ET::COW) {
        CowModel model;
        model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, MODEL_MESH_SCALE);
        model.generateMesh(vertices, indices, MODEL_MESH_SCALE);
        remapUvToAtlasRegion(normalizedId, vertices);
        return true;
    }
    if (normalizedId == ET::SHEEP) {
        SheepModel model;
        model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, MODEL_MESH_SCALE);
        model.generateMesh(vertices, indices, MODEL_MESH_SCALE);
        remapUvToAtlasRegion(normalizedId, vertices);
        return true;
    }
    if (normalizedId == ET::CHICKEN) {
        ChickenModel model;
        model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, MODEL_MESH_SCALE);
        model.generateMesh(vertices, indices, MODEL_MESH_SCALE);
        remapUvToAtlasRegion(normalizedId, vertices);
        return true;
    }
    if (normalizedId == ET::WOLF) {
        WolfModel model;
        model.setAnimState(false, false, false, 0.0f, 0.0f, 0.0f);
        model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, MODEL_MESH_SCALE);
        model.generateMesh(vertices, indices, MODEL_MESH_SCALE);
        remapUvToAtlasRegion(normalizedId, vertices);
        return true;
    }
    if (normalizedId == ET::OCELOT) {
        OcelotModel model(0.0f);
        model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, MODEL_MESH_SCALE);
        model.generateMesh(vertices, indices, MODEL_MESH_SCALE);
        remapUvToAtlasRegion(normalizedId, vertices);
        return true;
    }
    if (normalizedId == ET::CAT) {
        CatModel model(0.0f);
        model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, MODEL_MESH_SCALE);
        model.generateMesh(vertices, indices, MODEL_MESH_SCALE);
        remapUvToAtlasRegion(normalizedId, vertices);
        return true;
    }
    if (normalizedId == ET::HORSE) {
        HorseModel model(0.0f);
        model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, MODEL_MESH_SCALE);
        model.generateMesh(vertices, indices, MODEL_MESH_SCALE);
        remapUvToAtlasRegion(normalizedId, vertices);
        return true;
    }
    if (normalizedId == ET::VILLAGER) {
        VillagerModel model(0.0f);
        model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, MODEL_MESH_SCALE);
        model.generateMesh(vertices, indices, MODEL_MESH_SCALE);
        remapUvToAtlasRegion(normalizedId, vertices);
        return true;
    }

    // 怪物模型
    if (normalizedId == ET::ZOMBIE) {
        model::monster::ZombieModel model;
        model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, MODEL_MESH_SCALE);
        model.generateMesh(vertices, indices, MODEL_MESH_SCALE);
        remapUvToAtlasRegion(normalizedId, vertices);
        return true;
    }
    if (normalizedId == ET::SKELETON) {
        model::monster::SkeletonModel model;
        model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, MODEL_MESH_SCALE);
        model.generateMesh(vertices, indices, MODEL_MESH_SCALE);
        remapUvToAtlasRegion(normalizedId, vertices);
        return true;
    }
    if (normalizedId == ET::CREEPER) {
        model::monster::CreeperModel model;
        model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, MODEL_MESH_SCALE);
        model.generateMesh(vertices, indices, MODEL_MESH_SCALE);
        remapUvToAtlasRegion(normalizedId, vertices);
        return true;
    }
    if (normalizedId == ET::SPIDER) {
        model::monster::SpiderModel model;
        model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, MODEL_MESH_SCALE);
        model.generateMesh(vertices, indices, MODEL_MESH_SCALE);
        remapUvToAtlasRegion(normalizedId, vertices);
        return true;
    }
    if (normalizedId == ET::ENDERMAN) {
        model::monster::EndermanModel model;
        model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, MODEL_MESH_SCALE);
        model.generateMesh(vertices, indices, MODEL_MESH_SCALE);
        remapUvToAtlasRegion(normalizedId, vertices);
        return true;
    }
    if (normalizedId == ET::BLAZE) {
        model::monster::BlazeModel model;
        model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, MODEL_MESH_SCALE);
        model.generateMesh(vertices, indices, MODEL_MESH_SCALE);
        remapUvToAtlasRegion(normalizedId, vertices);
        return true;
    }

    if (normalizedId == ET::ITEM) {
        // ItemEntity 使用简单的四边形网格
        // 物品图标会在渲染时根据 ItemStack 动态获取纹理
        generateItemEntityMesh(vertices, indices);
        return true;
    }
    if (normalizedId == ET::EXPERIENCE_ORB) {
        // ExperienceOrb 使用简单的四边形网格（billboard）
        // 颜色会根据经验和时间动态变化
        generateExperienceOrbMesh(vertices, indices);
        return true;
    }

    // 未知实体类型
    spdlog::debug("Unknown entity type for mesh generation: {}", normalizedId);
    return false;
}

void EntityRendererManager::generateItemEntityMesh(std::vector<ModelVertex>& vertices,
                                                    std::vector<u32>& indices) {
    // 生成一个简单的四边形网格用于 ItemEntity
    // 物品图标是一个面向摄像机的 billboard（双面渲染）
    // 尺寸参考 MC 1.16.5：物品在地面上的渲染大小约为 0.25 块

    constexpr f64 HALF_SIZE = 0.125f;  // 物品尺寸的一半 (0.25 / 2)
    constexpr f64 Y_OFFSET = 0.25f;    // 地面偏移

    // 创建一个垂直的四边形（面向 +Z 方向）
    // 实际渲染时会根据摄像机朝向旋转
    vertices = {
        // 背面（法线 -Z）
        ModelVertex(-HALF_SIZE, Y_OFFSET, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f),  // 左下
        ModelVertex(-HALF_SIZE, Y_OFFSET + 0.25f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f),  // 左上
        ModelVertex(HALF_SIZE, Y_OFFSET + 0.25f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f),  // 右上
        ModelVertex(HALF_SIZE, Y_OFFSET, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f),  // 右下
        // 正面（法线 +Z）
        ModelVertex(HALF_SIZE, Y_OFFSET, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f),  // 左下
        ModelVertex(HALF_SIZE, Y_OFFSET + 0.25f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f),  // 左上
        ModelVertex(-HALF_SIZE, Y_OFFSET + 0.25f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f),  // 右上
        ModelVertex(-HALF_SIZE, Y_OFFSET, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f),  // 右下
    };

    indices = {
        // 背面
        0, 1, 2, 0, 2, 3,
        // 正面
        4, 5, 6, 4, 6, 7
    };
}

void EntityRendererManager::generateExperienceOrbMesh(std::vector<ModelVertex>& vertices,
                                                       std::vector<u32>& indices) {
    // 生成一个简单的四边形网格用于经验球
    // 经验球使用 billboard 方式渲染，始终面向摄像机
    // 参考 MC 1.16.5 ExperienceOrbRenderer

    // 经验球基础大小：0.25 块，会根据经验值等级动态缩放
    constexpr f64 HALF_SIZE = 0.125f;  // 基础尺寸的一半
    constexpr f64 Y_OFFSET = 0.25f;    // 地面偏移

    // 创建双面四边形（billboard）
    // 颜色会在渲染时根据经验值和时间动态计算
    vertices = {
        // 背面（法线 -Z）
        ModelVertex(-HALF_SIZE, Y_OFFSET, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f),
        ModelVertex(-HALF_SIZE, Y_OFFSET + 0.25f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f),
        ModelVertex(HALF_SIZE, Y_OFFSET + 0.25f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f),
        ModelVertex(HALF_SIZE, Y_OFFSET, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f),
        // 正面（法线 +Z）
        ModelVertex(HALF_SIZE, Y_OFFSET, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f),
        ModelVertex(HALF_SIZE, Y_OFFSET + 0.25f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f),
        ModelVertex(-HALF_SIZE, Y_OFFSET + 0.25f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f),
        ModelVertex(-HALF_SIZE, Y_OFFSET, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f),
    };

    indices = {
        // 背面
        0, 1, 2, 0, 2, 3,
        // 正面
        4, 5, 6, 4, 6, 7
    };
}

void EntityRendererManager::remapItemEntityUv(ClientEntity& entity, std::vector<ModelVertex>& vertices) {
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
        spdlog::debug("No item texture found for ItemEntity with item: {}", itemId.toString());
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

void EntityRendererManager::remapUvToAtlasRegion(const String& normalizedTypeId,
                                                 std::vector<ModelVertex>& vertices) const {
    if (!m_textureAtlas || !m_textureAtlas->isBuilt() || vertices.empty()) {
        // spdlog::info("remapUvToAtlasRegion: early return for '{}' - atlas null: {}, built: {}, vertices empty: {}",
        //              normalizedTypeId, m_textureAtlas == nullptr, m_textureAtlas && m_textureAtlas->isBuilt(), vertices.empty());
        return;
    }

    const TextureRegion* region = nullptr;
    const auto texturePaths = EntityTextureLoader::getTexturePaths(normalizedTypeId);
    // spdlog::info("remapUvToAtlasRegion: trying {} paths for '{}'", texturePaths.size(), normalizedTypeId);
    for (const auto& path : texturePaths) {
        // spdlog::info("remapUvToAtlasRegion: checking path '{}'", path.toString());
        region = m_textureAtlas->getRegion(path);
        if (region) {
            // spdlog::info("remapUvToAtlasRegion: found region for '{}'", path.toString());
            break;
        }
    }

    if (!region) {
        spdlog::warn("remapUvToAtlasRegion: No atlas region found for entity type: {}", normalizedTypeId);
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

void EntityRendererManager::clearAnimatedMeshes() {
    if (m_animatedMeshCache) {
        m_animatedMeshCache->clear();
    }
}

bool EntityRendererManager::usesAnimatedMesh(const String& normalizedTypeId) const {
    // ItemEntity 和 ExperienceOrb 使用静态网格
    // 所有生物实体使用动画网格
    return normalizedTypeId != entity::EntityTypes::ITEM &&
           normalizedTypeId != entity::EntityTypes::EXPERIENCE_ORB;
}

std::unique_ptr<model::EntityModel> EntityRendererManager::createModelForEntity(
    ClientEntity& entity,
    core::AnimationContext& context
) {
    String normalizedId = normalizeEntityTypeId(entity.typeId());
    namespace ET = entity::EntityTypes;
    using namespace model::animal;
    using namespace model::monster;

    // 从 ClientEntity 读取动画状态
    // MC 1.16.5 公式 (LivingRenderer.java:100):
    // limbSwing = entity.limbSwing - entity.limbSwingAmount * (1.0F - partialTicks)
    f64 limbSwingAmount = static_cast<f64>(entity.limbSwingAmount());
    context.limbSwing = static_cast<f64>(entity.limbSwing()) - limbSwingAmount * (1.0 - context.partialTicks);

    // 幼体动画速度加倍 (LivingRenderer.java:101-103)
    if (entity.isChild()) {
        context.limbSwing *= 3.0;
    }

    // limbSwingAmount 使用插值 (LivingRenderer.java:99)
    context.limbSwingAmount = static_cast<f64>(entity.prevLimbSwingAmount()) +
                             static_cast<f64>(entity.limbSwingAmount() - entity.prevLimbSwingAmount()) * context.partialTicks;

    // 限制最大值 (LivingRenderer.java:105-107)
    if (context.limbSwingAmount > 1.0) {
        context.limbSwingAmount = 1.0;
    }

    context.ageInTicks = static_cast<f64>(entity.ticksExisted());

    // 计算头部偏航角（相对于身体）
    f64 bodyYaw = static_cast<f64>(entity.prevRenderYawOffset()) +
                  static_cast<f64>(entity.renderYawOffset() - entity.prevRenderYawOffset()) * context.partialTicks;
    f64 headYaw = static_cast<f64>(entity.prevRotationYawHead()) +
                  static_cast<f64>(entity.rotationYawHead() - entity.prevRotationYawHead()) * context.partialTicks;
    context.netHeadYaw = headYaw - bodyYaw;
    // 归一化到 -180 到 180
    while (context.netHeadYaw < -180.0) context.netHeadYaw += 360.0;
    while (context.netHeadYaw > 180.0) context.netHeadYaw -= 360.0;

    context.headPitch = static_cast<f64>(entity.prevPitch()) +
                      static_cast<f64>(entity.pitch() - entity.prevPitch()) * context.partialTicks;
    context.scale = entity.isChild() ? 0.5 * (1.0 / 16.0) : (1.0 / 16.0);
    context.isChild = entity.isChild();
    context.isSitting = entity.isSitting();
    context.isSneaking = entity.isSneaking();
    context.isSwimming = entity.isSwimming();
    context.isRiding = entity.isRiding();
    context.swingProgress = entity.getInterpolatedSwingProgress(static_cast<f32>(context.partialTicks));

    // 计算哈希
    context.computeHash();

    // 根据实体类型创建模型
    if (normalizedId == ET::PIG) {
        auto model = std::make_unique<PigModel>();
        model->setAngles(context.limbSwing, context.limbSwingAmount, context.ageInTicks, context.netHeadYaw, context.headPitch, context.scale * 16.0);
        return model;
    }
    if (normalizedId == ET::COW) {
        auto model = std::make_unique<CowModel>();
        model->setAngles(context.limbSwing, context.limbSwingAmount, context.ageInTicks, context.netHeadYaw, context.headPitch, context.scale * 16.0);
        return model;
    }
    if (normalizedId == ET::SHEEP) {
        auto model = std::make_unique<SheepModel>();
        model->setAngles(context.limbSwing, context.limbSwingAmount, context.ageInTicks, context.netHeadYaw, context.headPitch, context.scale * 16.0);
        return model;
    }
    if (normalizedId == ET::CHICKEN) {
        auto model = std::make_unique<ChickenModel>();
        model->setAngles(context.limbSwing, context.limbSwingAmount, context.ageInTicks, context.netHeadYaw, context.headPitch, context.scale * 16.0);
        return model;
    }
    if (normalizedId == ET::WOLF) {
        auto model = std::make_unique<WolfModel>();
        model->setAnimState(false, false, false, 0.0f, 0.0f, 0.0f);
        model->setAngles(context.limbSwing, context.limbSwingAmount, context.ageInTicks, context.netHeadYaw, context.headPitch, context.scale * 16.0);
        return model;
    }
    if (normalizedId == ET::OCELOT) {
        auto model = std::make_unique<OcelotModel>(0.0f);
        model->setAngles(context.limbSwing, context.limbSwingAmount, context.ageInTicks, context.netHeadYaw, context.headPitch, context.scale * 16.0);
        return model;
    }
    if (normalizedId == ET::CAT) {
        auto model = std::make_unique<CatModel>(0.0f);
        model->setAngles(context.limbSwing, context.limbSwingAmount, context.ageInTicks, context.netHeadYaw, context.headPitch, context.scale * 16.0);
        return model;
    }
    if (normalizedId == ET::HORSE) {
        auto model = std::make_unique<HorseModel>(0.0f);
        model->setAngles(context.limbSwing, context.limbSwingAmount, context.ageInTicks, context.netHeadYaw, context.headPitch, context.scale * 16.0);
        return model;
    }
    if (normalizedId == ET::VILLAGER) {
        auto model = std::make_unique<VillagerModel>(0.0f);
        model->setAngles(context.limbSwing, context.limbSwingAmount, context.ageInTicks, context.netHeadYaw, context.headPitch, context.scale * 16.0);
        return model;
    }
    if (normalizedId == ET::ZOMBIE) {
        auto model = std::make_unique<ZombieModel>();
        model->setAngles(context.limbSwing, context.limbSwingAmount, context.ageInTicks, context.netHeadYaw, context.headPitch, context.scale * 16.0);
        return model;
    }
    if (normalizedId == ET::SKELETON) {
        auto model = std::make_unique<SkeletonModel>();
        model->setAngles(context.limbSwing, context.limbSwingAmount, context.ageInTicks, context.netHeadYaw, context.headPitch, context.scale * 16.0);
        return model;
    }
    if (normalizedId == ET::CREEPER) {
        auto model = std::make_unique<CreeperModel>();
        model->setAngles(context.limbSwing, context.limbSwingAmount, context.ageInTicks, context.netHeadYaw, context.headPitch, context.scale * 16.0);
        return model;
    }
    if (normalizedId == ET::SPIDER) {
        auto model = std::make_unique<SpiderModel>();
        model->setAngles(context.limbSwing, context.limbSwingAmount, context.ageInTicks, context.netHeadYaw, context.headPitch, context.scale * 16.0);
        return model;
    }
    if (normalizedId == ET::ENDERMAN) {
        auto model = std::make_unique<EndermanModel>();
        model->setAngles(context.limbSwing, context.limbSwingAmount, context.ageInTicks, context.netHeadYaw, context.headPitch, context.scale * 16.0);
        return model;
    }
    if (normalizedId == ET::BLAZE) {
        auto model = std::make_unique<BlazeModel>();
        model->setAngles(context.limbSwing, context.limbSwingAmount, context.ageInTicks, context.netHeadYaw, context.headPitch, context.scale * 16.0);
        return model;
    }

    // 未知实体类型
    return nullptr;
}

pipeline::EntityMesh* EntityRendererManager::getOrCreateAnimatedMesh(
    ClientEntity& entity,
    model::EntityModel& model,
    const core::AnimationContext& context
) {
    if (!m_pipeline || !m_animatedMeshCache) {
        return nullptr;
    }

    String normalizedId = normalizeEntityTypeId(entity.typeId());

    // 设置 UV 重映射回调
    m_animatedMeshCache->setUvRemapFunc([this, normalizedId](const String& typeId, std::vector<ModelVertex>& vertices) {
        remapUvToAtlasRegion(typeId, vertices);
    });

    return m_animatedMeshCache->getOrUpdateMesh(
        entity.id(),
        model,
        normalizedId,
        context,
        *m_pipeline
    );
}

} // namespace mc::client::renderer::entity
