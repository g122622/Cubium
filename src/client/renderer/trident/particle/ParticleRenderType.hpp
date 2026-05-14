#pragma once

#include "common/core/Types.hpp"

namespace mc::client::renderer::trident::particle {

/**
 * @brief 粒子渲染类型
 *
 * 决定粒子的渲染方式（混合模式、纹理来源、光照处理）。
 * 参考 MC 1.16.5 IParticleRenderType
 *
 * 渲染顺序：
 * 1. TERRAIN_SHEET - 使用方块纹理图集
 * 2. PARTICLE_SHEET_OPAQUE - 不透明粒子纹理
 * 3. PARTICLE_SHEET_LIT - 发光粒子纹理（不受光照影响）
 * 4. PARTICLE_SHEET_TRANSLUCENT - 半透明粒子纹理
 * 5. CUSTOM - 自定义渲染
 */
enum class ParticleRenderType : u8 {
    /**
     * @brief 使用方块纹理图集
     *
     * 用于方块破坏粒子、下落灰尘等。
     * 纹理来源：方块纹理图集
     * 混合：禁用
     * 深度写入：启用
     */
    TERRAIN_SHEET = 0,

    /**
     * @brief 不透明粒子纹理
     *
     * 用于不透明粒子如末地烛、屏障等。
     * 纹理来源：粒子纹理图集
     * 混合：禁用
     * 深度写入：启用
     */
    PARTICLE_SHEET_OPAQUE = 1,

    /**
     * @brief 发光粒子纹理
     *
     * 用于发光粒子如火焰、熔岩等。
     * 纹理来源：粒子纹理图集
     * 光照：最大亮度（不受世界光照影响）
     * 混合：启用（Alpha 混合）
     * 深度写入：禁用
     */
    PARTICLE_SHEET_LIT = 2,

    /**
     * @brief 半透明粒子纹理
     *
     * 用于大多数半透明粒子如烟雾、药水效果等。
     * 纹理来源：粒子纹理图集
     * 混合：启用（Alpha 混合）
     * 深度写入：禁用
     */
    PARTICLE_SHEET_TRANSLUCENT = 3,

    /**
     * @brief 自定义渲染
     *
     * 用于需要特殊渲染逻辑的粒子。
     * 子类需重写 buildVertices() 方法。
     */
    CUSTOM = 4,

    /**
     * @brief 不渲染
     *
     * 用于元粒子（EmitterParticle 等），
     * 这类粒子仅用于逻辑，不需要渲染。
     */
    NO_RENDER = 5
};

/**
 * @brief 获取渲染类型的渲染顺序
 *
 * 渲染按此顺序进行，确保正确的透明度排序。
 *
 * @param type 渲染类型
 * @return 渲染顺序索引（0 最先渲染）
 */
[[nodiscard]] constexpr u32 getRenderOrder(ParticleRenderType type)
{
    return static_cast<u32>(type);
}

/**
 * @brief 渲染类型是否需要深度写入
 *
 * @param type 渲染类型
 * @return 是否需要深度写入
 */
[[nodiscard]] constexpr bool needsDepthWrite(ParticleRenderType type)
{
    return type == ParticleRenderType::TERRAIN_SHEET || type == ParticleRenderType::PARTICLE_SHEET_OPAQUE;
}

/**
 * @brief 渲染类型是否需要混合
 *
 * @param type 渲染类型
 * @return 是否需要 Alpha 混合
 */
[[nodiscard]] constexpr bool needsBlending(ParticleRenderType type)
{
    return type == ParticleRenderType::PARTICLE_SHEET_LIT || type == ParticleRenderType::PARTICLE_SHEET_TRANSLUCENT;
}

/**
 * @brief 渲染类型是否始终使用最大亮度
 *
 * @param type 渲染类型
 * @return 是否使用最大亮度
 */
[[nodiscard]] constexpr bool isAlwaysLit(ParticleRenderType type)
{
    return type == ParticleRenderType::PARTICLE_SHEET_LIT;
}

/**
 * @brief 渲染类型是否使用方块纹理图集
 *
 * @param type 渲染类型
 * @return 是否使用方块纹理图集
 */
[[nodiscard]] constexpr bool usesTerrainAtlas(ParticleRenderType type)
{
    return type == ParticleRenderType::TERRAIN_SHEET;
}

/**
 * @brief 渲染类型是否需要渲染
 *
 * @param type 渲染类型
 * @return 是否需要渲染
 */
[[nodiscard]] constexpr bool shouldRender(ParticleRenderType type)
{
    return type != ParticleRenderType::NO_RENDER;
}

} // namespace mc::client::renderer::trident::particle
