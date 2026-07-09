/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to Use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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

#include "ItemParticle.hpp"
#include "client/renderer/trident/chunk/ChunkMesher.hpp"
#include "client/resource/BlockModelCache.hpp"
#include "client/resource/ItemModelCache.hpp"
#include "client/resource/ItemTextureAtlas.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/items/block/BlockItemRegistry.hpp"
#include "common/physics/PhysicsConstants.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

namespace mc::client::renderer::trident::particle::particles {

namespace {

/**
 * @brief 默认 UV 坐标
 *
 * 如果无法获取物品纹理，使用此回退值（全纹理）。
 */
constexpr f64 DEFAULT_U0 = 0.0;
constexpr f64 DEFAULT_V0 = 0.0;
constexpr f64 DEFAULT_U1 = 1.0;
constexpr f64 DEFAULT_V1 = 1.0;

/**
 * @brief 从 BlockAppearance 中随机选择一个面的纹理
 *
 * 当方块模型没有指定粒子纹理时的回退策略。
 *
 * @param appearance 方块外观
 * @param rng 随机数生成器
 * @return 纹理区域和面名称，如果没有可用纹理返回 nullopt
 */
std::optional<std::pair<TextureRegion, std::string>> selectRandomFaceTexture(
    const BlockAppearance* appearance, math::Random& rng)
{
    if (!appearance || appearance->faceTextures.empty()) {
        return std::nullopt;
    }

    // 收集所有可用的面名称
    std::vector<std::string> faceNames;
    for (const auto& [name, region] : appearance->faceTextures) {
        if (!name.empty()) {
            faceNames.push_back(name);
        }
    }

    if (faceNames.empty()) {
        return std::nullopt;
    }

    // 随机选择一个面
    size_t randomIndex = static_cast<size_t>(rng.nextInt(static_cast<i32>(faceNames.size())));
    const std::string& selectedFace = faceNames[randomIndex];

    auto it = appearance->faceTextures.find(selectedFace);
    if (it != appearance->faceTextures.end()) {
        return std::make_pair(it->second, selectedFace);
    }

    return std::nullopt;
}

} // namespace

// 静态成员初始化
const mc::client::ItemTextureAtlas* ItemParticle::s_itemTextureAtlas = nullptr;

ItemParticle::ItemParticle(const glm::vec3& pos, const glm::vec3& velocity)
    : Particle(pos, velocity)
    , m_initialSize(DEFAULT_SIZE * (0.5 + m_random.nextFloat() * 0.5))
{
    setGravity(DEFAULT_GRAVITY);
    setSize(m_initialSize);
    setColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); // 使用纹理原色
    setFriction(FRICTION);
    setHasPhysics(true);
    setMaxAge(DEFAULT_LIFETIME * (0.8 + m_random.nextFloat() * 0.4));

    // 随机 UV 偏移：将 16x16 纹理划分为 4x4 区域，随机选取一个
    m_uvOffsetU = static_cast<f32>(m_random.nextInt(4));
    m_uvOffsetV = static_cast<f32>(m_random.nextInt(4));

    // 无 ItemStack 时使用占位纹理
    m_hasValidTexture = false;
    m_textureLocation = ResourceLocation("minecraft:particle/generic");
}

ItemParticle::ItemParticle(const glm::vec3& pos, const glm::vec3& velocity, const ItemStack& itemStack)
    : Particle(pos, velocity)
    , m_initialSize(DEFAULT_SIZE * (0.5 + m_random.nextFloat() * 0.5))
    , m_itemStack(itemStack)
{
    setGravity(DEFAULT_GRAVITY);
    setSize(m_initialSize);
    setColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)); // 使用纹理原色
    setFriction(FRICTION);
    setHasPhysics(true);
    setMaxAge(DEFAULT_LIFETIME * (0.8 + m_random.nextFloat() * 0.4));

    // 随机 UV 偏移：将 16x16 纹理划分为 4x4 区域，随机选取一个
    m_uvOffsetU = static_cast<f32>(m_random.nextInt(4));
    m_uvOffsetV = static_cast<f32>(m_random.nextInt(4));

    // 初始化物品纹理
    _initializeItemTexture();
}

std::unique_ptr<Particle> ItemParticle::create(
    const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world)
{
    MC_UNUSED(world);
    // 无 ItemStack 回退：创建使用占位纹理的粒子
    return std::make_unique<ItemParticle>(pos, velocity);
}

std::unique_ptr<Particle> ItemParticle::createWithItemStack(
    const glm::vec3& pos, const glm::vec3& velocity, const ItemStack& itemStack)
{
    return std::make_unique<ItemParticle>(pos, velocity, itemStack);
}

void ItemParticle::tick(mc::client::ClientWorld* world)
{
    m_prevPosition = m_position;
    m_prevRoll = m_roll;

    m_age += 1.0;
    if (m_age >= m_maxAge) {
        setExpired();
        return;
    }

    // 应用重力
    m_velocity.y -= static_cast<f32>(m_gravity * mc::physics::PARTICLE_GRAVITY_MULTIPLIER);

    // 随机旋转
    m_roll += 0.1;

    // 移动并碰撞
    if (m_hasPhysics) {
        move(world, m_velocity);
    } else {
        m_position += m_velocity;
    }

    // 阻力衰减
    m_velocity.x *= static_cast<f32>(m_friction);
    m_velocity.z *= static_cast<f32>(m_friction);

    // 地面摩擦
    if (onGround()) {
        m_velocity.x *= 0.7f;
        m_velocity.z *= 0.7f;
    }

    // 70% 生命周期后淡出
    f64 lifeRatio = m_age / m_maxAge;
    if (lifeRatio > 0.7) {
        m_color.a = static_cast<f32>(1.0 - (lifeRatio - 0.7) / 0.3);
    }
}

ResourceLocation ItemParticle::getTextureLocation() const
{
    return m_textureLocation;
}

f64 ItemParticle::getScale(f64 partialTick) const
{
    MC_UNUSED(partialTick);
    return 1.0;
}

void ItemParticle::buildVertices(const glm::vec3& cameraPos,
    f64 partialTick,
    const ParticleTextureAtlas& /*atlas*/,
    std::vector<ParticleVertex>& outVertices) const
{
    // 插值位置
    glm::dvec3 interpPos =
        glm::dvec3(m_prevPosition) + (glm::dvec3(m_position) - glm::dvec3(m_prevPosition)) * partialTick;

    // 插值旋转
    f64 interpRoll = m_prevRoll + (m_roll - m_prevRoll) * partialTick;

    // 计算 billboard 基向量
    glm::dvec3 toCamera = glm::dvec3(cameraPos) - interpPos;
    f64 dist = glm::length(toCamera);
    if (dist < 0.001) {
        return; // 太近了，跳过
    }
    toCamera = glm::normalize(toCamera);

    // 计算右向量和上向量（camera-facing billboard）
    glm::dvec3 right = glm::cross(glm::dvec3(0.0, 1.0, 0.0), toCamera);
    if (glm::length(right) < 0.001) {
        // 相机在正上方或正下方
        right = glm::dvec3(1.0, 0.0, 0.0);
    } else {
        right = glm::normalize(right);
    }
    glm::dvec3 up = glm::cross(toCamera, right);

    // 应用旋转
    if (std::abs(interpRoll) > 0.001) {
        f64 cosR = std::cos(interpRoll);
        f64 sinR = std::sin(interpRoll);
        glm::dvec3 newRight = right * cosR + up * sinR;
        glm::dvec3 newUp = -right * sinR + up * cosR;
        right = newRight;
        up = newUp;
    }

    glm::vec3 interpPosF(static_cast<f32>(interpPos.x), static_cast<f32>(interpPos.y), static_cast<f32>(interpPos.z));
    glm::vec3 rightF(static_cast<f32>(right.x), static_cast<f32>(right.y), static_cast<f32>(right.z));
    glm::vec3 upF(static_cast<f32>(up.x), static_cast<f32>(up.y), static_cast<f32>(up.z));

    // 获取缩放
    f64 scale = getScale(partialTick);
    f64 halfSize = m_size * scale * 0.5;
    const f32 halfSizeF = static_cast<f32>(halfSize);

    // 计算 UV 坐标
    // 从 16x16 纹理中选取 4x4 区域，模拟碎片效果（与 DiggingParticle 一致）
    f64 u0, v0, u1, v1;

    if (m_hasValidTexture) {
        // 使用预计算的纹理区域
        // 纹理区域是整个 16x16 纹理的 UV 坐标
        // 我们需要从中选取一个 4x4 的子区域
        // 4x4 子区域占整个纹理的 1/4
        f64 regionWidth = m_textureRegion.u1 - m_textureRegion.u0;
        f64 regionHeight = m_textureRegion.v1 - m_textureRegion.v0;

        // 计算子区域的起始位置
        f64 subU0 = m_textureRegion.u0 + (regionWidth * m_uvOffsetU / 4.0);
        f64 subV0 = m_textureRegion.v0 + (regionHeight * m_uvOffsetV / 4.0);
        f64 subU1 = subU0 + regionWidth / 4.0;
        f64 subV1 = subV0 + regionHeight / 4.0;

        u0 = subU0;
        v0 = subV0;
        u1 = subU1;
        v1 = subV1;
    } else {
        // 使用默认全纹理
        u0 = DEFAULT_U0;
        v0 = DEFAULT_V0;
        u1 = DEFAULT_U1;
        v1 = DEFAULT_V1;
    }

    // 四个顶点（quad）
    // 左下
    outVertices.push_back({interpPosF - rightF * halfSizeF - upF * halfSizeF,
        glm::vec2(static_cast<f32>(u0), static_cast<f32>(v1)), // UV: 左下
        m_color,
        static_cast<f32>(m_size * scale),
        m_color.a});
    // 右下
    outVertices.push_back({interpPosF + rightF * halfSizeF - upF * halfSizeF,
        glm::vec2(static_cast<f32>(u1), static_cast<f32>(v1)), // UV: 右下
        m_color,
        static_cast<f32>(m_size * scale),
        m_color.a});
    // 右上
    outVertices.push_back({interpPosF + rightF * halfSizeF + upF * halfSizeF,
        glm::vec2(static_cast<f32>(u1), static_cast<f32>(v0)), // UV: 右上
        m_color,
        static_cast<f32>(m_size * scale),
        m_color.a});
    // 左上
    outVertices.push_back({interpPosF - rightF * halfSizeF + upF * halfSizeF,
        glm::vec2(static_cast<f32>(u0), static_cast<f32>(v0)), // UV: 左上
        m_color,
        static_cast<f32>(m_size * scale),
        m_color.a});
}

// static
void ItemParticle::setItemTextureAtlas(const mc::client::ItemTextureAtlas* atlas)
{
    s_itemTextureAtlas = atlas;
}

void ItemParticle::_initializeItemTexture()
{
    // 空物品堆使用占位纹理
    if (m_itemStack.isEmpty()) {
        m_hasValidTexture = false;
        m_textureLocation = ResourceLocation("minecraft:particle/generic");
        return;
    }

    const Item* item = m_itemStack.getItem();
    if (item == nullptr) {
        m_hasValidTexture = false;
        m_textureLocation = ResourceLocation("minecraft:particle/generic");
        return;
    }

    // 优先尝试方块物品路径（stone、dirt 等方块物品复用方块纹理图集）
    if (BlockItemRegistry::instance().isBlockItem(item->itemId())) {
        const Block* block = BlockItemRegistry::instance().getBlock(item->itemId());
        if (block != nullptr) {
            if (_initializeFromBlockItem(block->defaultState())) {
                return;
            }
        }
    }

    // 非方块物品路径：ItemModelCache + ItemTextureAtlas
    if (_initializeFromPlainItem()) {
        return;
    }

    // 两条路径都失败，使用占位纹理
    m_hasValidTexture = false;
    m_textureLocation = ResourceLocation("minecraft:particle/generic");
}

bool ItemParticle::_initializeFromBlockItem(const BlockState& blockState)
{
    // 获取 BlockModelCache
    BlockModelCache* modelCache = ChunkMesher::modelCache();
    if (!modelCache) {
        spdlog::warn("ItemParticle: BlockModelCache not available for block-item texture");
        return false;
    }

    // 获取方块外观
    const BlockAppearance* appearance = modelCache->getBlockAppearance(&blockState);
    if (!appearance) {
        // 尝试使用缺失模型外观
        appearance = modelCache->getMissingAppearance();
        if (!appearance) {
            return false;
        }
    }

    // 优先使用模型中 textures.particle 指定的粒子纹理
    if (appearance->hasParticleTexture) {
        m_textureRegion = appearance->particleTexture;
        m_textureLocation = appearance->particleTextureLocation;
        m_hasValidTexture = true;
        return true;
    }

    // 回退：从方块所有面纹理中随机选一个
    auto textureResult = selectRandomFaceTexture(appearance, m_random);
    if (textureResult.has_value()) {
        m_textureRegion = textureResult->first;
        m_hasValidTexture = true;

        // 从面纹理位置映射中获取选中面的纹理资源位置
        const auto& faceName = textureResult->second;
        auto locIt = appearance->faceTextureLocations.find(faceName);
        if (locIt != appearance->faceTextureLocations.end()) {
            m_textureLocation = locIt->second;
        } else if (!appearance->faceTextureLocations.empty()) {
            m_textureLocation = appearance->faceTextureLocations.begin()->second;
        } else {
            m_textureLocation = ResourceLocation("minecraft:block/stone");
        }
        return true;
    }

    return false;
}

bool ItemParticle::_initializeFromPlainItem()
{
    // ItemTextureAtlas 必须已注入
    if (s_itemTextureAtlas == nullptr) {
        spdlog::warn("ItemParticle: ItemTextureAtlas not injected, cannot resolve item texture");
        return false;
    }

    const Item* item = m_itemStack.getItem();
    if (item == nullptr) {
        return false;
    }

    // 通过 ItemModelCache 获取物品模型
    const resource::BakedItemModel* model = resource::ItemModelCache::instance().getItemModel(*item);
    if (model == nullptr) {
        spdlog::warn("ItemParticle: ItemModelCache has no model for item {}", item->itemLocation().toString());
        return false;
    }

    // 方块模型类型（ItemModelType::Block）的物品应已通过方块路径处理，
    // 到这里说明方块路径失败（如 BlockModelCache 不可用），回退到 layer0 纹理
    // Generated/Handheld/Custom 类型使用 textureLayers
    if (model->textureLayers.empty()) {
        spdlog::warn("ItemParticle: Item model {} has no texture layers", model->location.toString());
        return false;
    }

    // 取 layer0 作为粒子纹理（与 MC Java ItemParticleProvider 取第一层一致）
    const ResourceLocation& layer0 = model->textureLayers.front();
    const TextureRegion* region = s_itemTextureAtlas->getItemTexture(layer0);
    if (region == nullptr) {
        spdlog::warn("ItemParticle: ItemTextureAtlas has no texture for {}", layer0.toString());
        return false;
    }

    m_textureRegion = *region;
    m_textureLocation = layer0;
    m_hasValidTexture = true;
    return true;
}

} // namespace mc::client::renderer::trident::particle::particles
