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
 * IMPLIED, LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "client/renderer/MeshTypes.hpp"
#include "client/renderer/trident/particle/Particle.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/BlockState.hpp"
#include <memory>

namespace mc::client {
class ClientWorld;
class ItemTextureAtlas;
} // namespace mc::client

namespace mc::client::renderer::trident::particle::particles {

/**
 * @brief 物品粒子（物品破碎、史莱姆、雪球、蛛网等）
 *
 * 物品破碎时产生的粒子效果，使用物品纹理图集渲染。
 *
 * 纹理解析策略（对应 MC Java 1.21.11 ItemParticleProvider / TerParticle）：
 * - 方块物品（stone、dirt 等）：通过 BlockItemRegistry 解析为 BlockState，
 *   复用 BlockModelCache 获取方块粒子纹理（与 DiggingParticle 一致）。
 * - 非方块物品（工具、食物等）：通过 ItemModelCache 获取 BakedItemModel，
 *   再用 ItemTextureAtlas 解析 layer0 纹理区域。
 *
 * Item、ItemSlime、ItemCobweb、ItemSnowball 均使用此类，
 * 仅通过不同的 ParticleTypeId 注册来区分。
 *
 * TODO(架构限制): ParticleManager 当前仅绑定单一的 ParticleTextureAtlas 纹理
 * 渲染所有粒子，不支持按 ParticleRenderType 切换纹理图集。非方块物品的
 * ItemTextureAtlas UV 在渲染时采样错误纹理。完整修复需要 ParticleManager
 * 按渲染类型维护多套纹理图集描述符。详见 _initializeFromPlainItem() 的
 * TODO 注释和 particle/README.md 第 13 条。
 */
class ItemParticle : public Particle {
public:
    /**
     * @brief 默认构造函数（使用占位纹理）
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     */
    ItemParticle(const glm::vec3& pos, const glm::vec3& velocity);

    /**
     * @brief 带 ItemStack 的构造函数（推荐）
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     * @param itemStack 物品堆（用于解析纹理）
     */
    ItemParticle(const glm::vec3& pos, const glm::vec3& velocity, const ItemStack& itemStack);

    /**
     * @brief 默认工厂方法（使用占位石头纹理）
     */
    static std::unique_ptr<Particle> create(
        const glm::vec3& pos, const glm::vec3& velocity, mc::client::ClientWorld* world);

    /**
     * @brief 带 ItemStack 的工厂方法
     *
     * @param pos 初始位置
     * @param velocity 初始速度
     * @param itemStack 物品堆
     * @return 粒子实例
     */
    static std::unique_ptr<Particle> createWithItemStack(
        const glm::vec3& pos, const glm::vec3& velocity, const ItemStack& itemStack);

    void tick(mc::client::ClientWorld* world) override;

    [[nodiscard]] ParticleRenderType getRenderType() const override { return ParticleRenderType::TERRAIN_SHEET; }

    /**
     * @brief 获取纹理位置
     *
     * 对于方块物品返回方块粒子纹理位置，对于非方块物品返回物品纹理位置。
     * 若均不可用则返回占位纹理。对于 TERRAIN_SHEET 类型粒子，
     * 实际渲染使用 buildVertices 中预计算的 m_textureRegion。
     */
    [[nodiscard]] ResourceLocation getTextureLocation() const override;

    /**
     * @brief 生成渲染顶点数据
     *
     * 重写以使用预计算的物品纹理 UV 坐标。
     */
    void buildVertices(const glm::vec3& cameraPos,
        f64 partialTick,
        const ParticleTextureAtlas& atlas,
        std::vector<ParticleVertex>& outVertices) const override;

    [[nodiscard]] f64 getScale(f64 partialTick) const override;

    /**
     * @brief 设置物品纹理图集引用
     *
     * 必须在首次渲染物品粒子之前调用（由 TridentEngine 初始化时注入）。
     * 用于解析非方块物品的纹理坐标。传 nullptr 清除引用。
     *
     * @param atlas 物品纹理图集指针
     */
    static void setItemTextureAtlas(const mc::client::ItemTextureAtlas* atlas);

private:
    static constexpr f64 DEFAULT_GRAVITY = 0.03;
    static constexpr f64 DEFAULT_SIZE = 0.1;
    static constexpr f64 DEFAULT_LIFETIME = 20.0;
    static constexpr f64 FRICTION = 0.92;

    /// 物品纹理图集（由 TridentEngine 注入，用于非方块物品）
    static const mc::client::ItemTextureAtlas* s_itemTextureAtlas;

    f64 m_initialSize;

    /// 物品堆（用于纹理解析）
    ItemStack m_itemStack;

    /// 预计算的纹理 UV 区域
    TextureRegion m_textureRegion;

    /// 随机 UV 偏移（0-3），用于从 16x16 纹理中选取 4x4 区域
    f32 m_uvOffsetU = 0.0f;
    f32 m_uvOffsetV = 0.0f;

    /// 是否成功获取了纹理
    bool m_hasValidTexture = false;

    /// 纹理位置标识（用于 TERRAIN_SHEET 渲染类型）
    ResourceLocation m_textureLocation{"minecraft:particle/generic"};

    /**
     * @brief 初始化物品纹理
     *
     * 方块物品走 BlockModelCache 路径，非方块物品走 ItemModelCache + ItemTextureAtlas 路径。
     */
    void _initializeItemTexture();

    /**
     * @brief 从方块物品解析纹理（BlockModelCache 路径）
     *
     * @param blockState 方块状态
     * @return 是否成功解析
     */
    bool _initializeFromBlockItem(const BlockState& blockState);

    /**
     * @brief 从非方块物品解析纹理（ItemModelCache + ItemTextureAtlas 路径）
     *
     * @return 是否成功解析
     */
    bool _initializeFromPlainItem();
};

} // namespace mc::client::renderer::trident::particle::particles
