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

#pragma once

#include "BlockModelLoader.hpp"
#include "BlockStateLoader.hpp"
#include "client/renderer/MeshTypes.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include <functional>
#include <map>
#include <memory>

namespace mc {

// 前向声明

/**
 * @brief 方块外观信息
 *
 * 包含渲染方块所需的所有数据
 */
struct BlockAppearance {
    struct FaceTextureLayer {
        TextureRegion texture;
        i32 tintIndex = -1;
    };

    std::vector<ModelElement> elements;
    std::map<std::string, TextureRegion> faceTextures;                      // 方向 -> 纹理区域
    std::map<std::string, ResourceLocation> faceTextureLocations;           // 方向 -> 纹理资源位置
    std::map<std::string, i32> faceTintIndices;                             // 方向 -> tintindex（仅存储 >= 0）
    std::map<std::string, std::vector<FaceTextureLayer>> faceTextureLayers; // 方向 -> 多层纹理（按模型顺序）
    TextureRegion particleTexture = {0.0, 0.0, 1.0, 1.0};                   // 模型中 textures.particle 指定的粒子纹理
    ResourceLocation particleTextureLocation;                               // 粒子纹理的资源位置
    bool hasParticleTexture = false;                                        // 是否有有效的粒子纹理
    i32 xRotation = 0;                                                      // X轴旋转
    i32 yRotation = 0;                                                      // Y轴旋转
    bool uvLock = false;
};

/**
 * @brief 解码后的 RGBA8 纹理数据
 *
 * 用于在资源包系统中读取并解码 PNG 后，向渲染模块传递像素。
 */
struct DecodedTexture {
    std::vector<u8> pixels; ///< RGBA8 像素数据（长度 = width * height * 4）
    u32 width = 0;          ///< 纹理宽度
    u32 height = 0;         ///< 纹理高度
};

/**
 * @brief 资源管理器
 *
 * 统一管理资源包、模型、方块状态和纹理
 */
class ResourceManager {
public:
    ResourceManager() = default;
    ~ResourceManager() = default;

    // ========================================================================
    // 资源包管理
    // ========================================================================

    /**
     * @brief 添加资源包
     * @param resourcePack 资源包指针
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> addResourcePack(ResourcePackPtr resourcePack);

    /**
     * @brief 清除所有资源包
     */
    void clearResourcePacks();

    /**
     * @brief 获取资源包数量
     */
    [[nodiscard]] size_t resourcePackCount() const { return m_resourcePacks.size(); }
    [[nodiscard]] const std::vector<ResourcePackPtr>& resourcePacks() const { return m_resourcePacks; }

    // ========================================================================
    // 资源加载
    // ========================================================================

    /**
     * @brief 加载所有资源
     *
     * 加载方块状态、模型、纹理等。
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadAllResources();

    /**
     * @brief 重新加载所有资源
     *
     * 清除缓存并重新加载所有资源。
     * 在资源包变更后调用。
     *
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> reload();

    // ========================================================================
    // 资源查询
    // ========================================================================

    /**
     * @brief 获取方块外观
     * @param blockId 方块资源位置
     * @param properties 属性映射
     * @return 方块外观指针，找不到返回 nullptr
     */
    [[nodiscard]] const BlockAppearance* getBlockAppearance(
        const ResourceLocation& blockId, const std::map<std::string, std::string>& properties) const;

    /**
     * @brief 按资源包优先级读取并解码 PNG 纹理
     *
     * 查找顺序与图集构建一致：后添加的资源包优先。
     * 仅解码 PNG，并强制输出 RGBA8。
     *
     * @param textureLocation 纹理资源位置（例如 minecraft:textures/environment/clouds）
     * @return 成功时返回像素、宽高；失败时返回错误
     *
     * @note 本方法不会写入纹理图集缓存，只做一次性加载。
     */
    [[nodiscard]] Result<DecodedTexture> loadTextureRGBA(const ResourceLocation& textureLocation) const;

    /**
     * @brief 获取已烘焙的模型
     * @param modelLocation 模型资源位置
     * @return 烘焙模型指针，找不到返回 nullptr
     */
    [[nodiscard]] const BakedBlockModel* getBakedModel(const ResourceLocation& modelLocation);

    /**
     * @brief 获取方块状态定义
     * @param blockId 方块资源位置
     * @return 方块状态定义指针，找不到返回 nullptr
     */
    [[nodiscard]] const BlockStateDefinition* getBlockState(const ResourceLocation& blockId) const;

    // ========================================================================
    // 访问器
    // ========================================================================

    /**
     * @brief 获取模型加载器
     */
    [[nodiscard]] BlockModelLoader& modelLoader() { return m_modelLoader; }
    [[nodiscard]] const BlockModelLoader& modelLoader() const { return m_modelLoader; }

    /**
     * @brief 获取方块状态加载器
     */
    [[nodiscard]] BlockStateLoader& blockStateLoader() { return m_blockStateLoader; }
    [[nodiscard]] const BlockStateLoader& blockStateLoader() const { return m_blockStateLoader; }

    /**
     * @brief 获取第一个资源包（用于纹理加载）
     * @return 资源包指针，如果没有则返回 nullptr
     */
    [[nodiscard]] IResourcePack* getFirstResourcePack();

    /**
     * @brief 按索引获取资源包
     * @param index 资源包索引
     * @return 资源包指针，如果索引无效则返回 nullptr
     */
    [[nodiscard]] IResourcePack* getResourcePack(size_t index);

    // ========================================================================
    // 缓存管理
    // ========================================================================

    /**
     * @brief 清除所有缓存
     */
    void clear();

    // ========================================================================
    // 方块外观计算
    // ========================================================================

    /**
     * @brief 计算所有方块状态的外观（面纹理区域、粒子纹理等）
     *
     * 纹理区域由外部注入的 regionLookup 回调提供（通常绑定到
     * AtlasManager::findSpriteWithVariant，查 blocks atlas 的 regions），
     * 因此必须在 AtlasManager 加载完 blocks atlas 之后调用。
     *
     * @param regionLookup 完整纹理资源位置（如 minecraft:textures/block/stone）→ UV 区域指针，未找到返回 nullptr
     */
    void computeBlockAppearances(const std::function<const TextureRegion*(const ResourceLocation&)>& regionLookup);

private:
    std::vector<ResourcePackPtr> m_resourcePacks;
    BlockModelLoader m_modelLoader;
    BlockStateLoader m_blockStateLoader;

    // 已烘焙模型缓存
    std::map<ResourceLocation, BakedBlockModel> m_bakedModels;

    // 方块外观缓存
    std::map<std::string, BlockAppearance> m_blockAppearances;

    // 烘焙所有模型
    [[nodiscard]] Result<void> _bakeAllModels();

    // 将纹理路径转换为资源位置
    [[nodiscard]] static ResourceLocation _texturePathToLocation(std::string_view path);
};

} // namespace mc
