#include "EntityRendererManager.hpp"
#include "../renderer/animal/AnimalRenderers.hpp"
#include "../renderer/projectile/ItemEntityRenderer.hpp"
#include "../renderer/projectile/ExperienceOrbRenderer.hpp"
#include "../pipeline/EntityTextureAtlas.hpp"
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
#include <spdlog/spdlog.h>

namespace mc::client::renderer::entity {

// 导入核心命名空间中的类
using core::EntityRenderer;
using pipeline::EntityMesh;
using pipeline::EntityTextureAtlas;
using model::ModelVertex;

// 导入 EntityTypes 常量命名空间
namespace EntityTypes = ::mc::entity::EntityTypes;

namespace {

inline constexpr f64 MODEL_SCALE = 1.0f / 16.0f;
inline constexpr f64 MODEL_MESH_SCALE = 1.0f;
inline constexpr f64 MODEL_Y_OFFSET = 1.5f; // 模型在实际渲染时的Y偏移量

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

    // 对于 ItemEntity，使用 ItemTextureAtlas
    if (isItemEntity && m_itemTextureAtlas && m_itemTextureAtlas->isBuilt()) {
        // 绑定物品纹理图集
        m_pipeline->setTextureAtlas(m_itemTextureAtlas->imageView(), m_itemTextureAtlas->sampler());
    }

    // 获取或创建网格
    EntityMesh* mesh = getOrCreateMesh(entity);
    if (!mesh || mesh->indexCount == 0) {
        // 恢复实体纹理图集
        if (isItemEntity && m_textureAtlas && m_textureAtlas->isBuilt()) {
            // 注意：需要在下一帧渲染前恢复，或在此处恢复
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
        m_pipeline->drawMesh(cmd, *mesh, modelMatrix, pos, MODEL_SCALE * 16.0f);
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
        m_pipeline->drawMesh(cmd, *mesh, modelMatrix, pos, scale);
    } else {
        // 普通实体渲染
        // MC 实体模型局部坐标系的 Y 轴方向与世界坐标相反，先做一次 Y 翻转
        modelMatrix[5] = -1.0f;
        // Y 翻转后，模型会相对地面下沉，需要补偿一个模型高度
        modelMatrix[7] = MODEL_Y_OFFSET;

        // 应用实体旋转（yaw）
        f64 yaw = static_cast<f64>(entity.getInterpolatedYaw(partialTickF32));
        f64 yawRad = yaw * static_cast<f64>(math::DEG_TO_RAD);
        f64 cosYaw = std::cos(yawRad);
        f64 sinYaw = std::sin(yawRad);

        // 旋转矩阵（绕Y轴）
        modelMatrix[0] = cosYaw;
        modelMatrix[2] = sinYaw;
        modelMatrix[8] = -sinYaw;
        modelMatrix[10] = cosYaw;

        // 获取插值位置
        Vector3 posInterp = entity.getInterpolatedPosition(partialTickF32);
        Vector3f pos(
            static_cast<f32>(posInterp.x),
            static_cast<f32>(posInterp.y),
            static_cast<f32>(posInterp.z));

        // 绘制网格
        m_pipeline->drawMesh(cmd, *mesh, modelMatrix, pos, MODEL_SCALE);
    }

    // 恢复实体纹理图集（如果为 ItemEntity）
    if (isItemEntity && m_textureAtlas && m_textureAtlas->isBuilt()) {
        m_pipeline->setTextureAtlas(m_textureAtlas->imageView(), m_textureAtlas->sampler());
    }
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
    using namespace renderer::projectile;

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

    spdlog::debug("EntityRendererManager: Registered {} entity types including ItemEntity and ExperienceOrb",
                  static_cast<size_t>(4) + 2);  // 4 animals + 1 item + 1 orb
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
        spdlog::info("remapUvToAtlasRegion: early return for '{}' - atlas null: {}, built: {}, vertices empty: {}",
                     normalizedTypeId, m_textureAtlas == nullptr, m_textureAtlas && m_textureAtlas->isBuilt(), vertices.empty());
        return;
    }

    const TextureRegion* region = nullptr;
    const auto texturePaths = EntityTextureLoader::getTexturePaths(normalizedTypeId);
    spdlog::info("remapUvToAtlasRegion: trying {} paths for '{}'", texturePaths.size(), normalizedTypeId);
    for (const auto& path : texturePaths) {
        spdlog::info("remapUvToAtlasRegion: checking path '{}'", path.toString());
        region = m_textureAtlas->getRegion(path);
        if (region) {
            spdlog::info("remapUvToAtlasRegion: found region for '{}'", path.toString());
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

} // namespace mc::client::renderer::entity
