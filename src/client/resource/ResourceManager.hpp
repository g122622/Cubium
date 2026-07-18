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
#include "TextureAtlasBuilder.hpp"
#include "client/renderer/MeshTypes.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/util/Direction.hpp"
#include <array>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace mc {

// 前向声明

/**
 * @brief 方块外观信息
 *
 * 包含渲染方块所需的所有数据
 *
 * 面相关字段（faceTextures / faceTextureLocations / faceTintIndices /
 * faceTextureLayers）以扁平数组存储，索引 = Directions::index(dir)
 *（Down=0..East=5）。每个槽位为 std::optional：nullopt 表示该方向无数据，
 * 等价于原先 std::map<std::string, T> 用方向名作键、count==0 判定不存在的语义。
 *
 * 写入路径（_computeBlockAppearances / _createMissingAppearance）只写 6 个方向，
 * 从不写 "side"/"all"。原 map 读取处的 "side"/"all" 键查找因此永不命中，
 * 属历史遗留防御逻辑；改 array 后各查找仅按精确 Direction 取，行为与原 map
 * 完全等价（cross 植物的 north→south / west→east 选择仍由调用方显式级联实现）。
 */
struct BlockAppearance {
    struct FaceTextureLayer {
        TextureRegion texture;
        i32 tintIndex = -1;
    };

    std::vector<ModelElement> elements;
    std::array<std::optional<TextureRegion>, 6> faceTextures;                      // 方向 -> 纹理区域
    std::array<std::optional<ResourceLocation>, 6> faceTextureLocations;           // 方向 -> 纹理资源位置
    std::array<std::optional<i32>, 6> faceTintIndices;                             // 方向 -> tintindex（仅存储 >= 0）
    std::array<std::optional<std::vector<FaceTextureLayer>>, 6> faceTextureLayers; // 方向 -> 多层纹理（按模型顺序）
    TextureRegion particleTexture = {0.0, 0.0, 1.0, 1.0}; // 模型中 textures.particle 指定的粒子纹理
    ResourceLocation particleTextureLocation;             // 粒子纹理的资源位置
    bool hasParticleTexture = false;                      // 是否有有效的粒子纹理
    i32 xRotation = 0;                                    // X轴旋转
    i32 yRotation = 0;                                    // Y轴旋转
    bool uvLock = false;

    /**
     * @brief 是否存在任意面纹理（替代原 faceTextures.empty() 的语义取反）
     */
    [[nodiscard]] bool hasAnyFaceTexture() const
    {
        for (const auto& t : faceTextures) {
            if (t.has_value()) return true;
        }
        return false;
    }

    /**
     * @brief 是否存在任意多层纹理（替代原 faceTextureLayers.empty() 的语义取反）
     */
    [[nodiscard]] bool hasAnyFaceTextureLayer() const
    {
        for (const auto& l : faceTextureLayers) {
            if (l.has_value() && !l->empty()) return true;
        }
        return false;
    }

    /**
     * @brief 按精确方向取多层纹理（等价于原 map.find(directionName)）
     * @return 指向内部 vector 的指针，该方向无数据返回 nullptr
     */
    [[nodiscard]] const std::vector<FaceTextureLayer>* findFaceTextureLayers(Direction dir) const
    {
        const size_t idx = Directions::index(dir);
        if (idx < 6 && faceTextureLayers[idx] && !faceTextureLayers[idx]->empty()) {
            return &(*faceTextureLayers[idx]);
        }
        return nullptr;
    }

    /**
     * @brief 按精确方向取单层纹理区域（等价于原 map.find(directionName)）
     * @return 纹理区域指针，该方向无数据返回 nullptr
     */
    [[nodiscard]] const TextureRegion* findFaceTexture(Direction dir) const
    {
        const size_t idx = Directions::index(dir);
        if (idx < 6 && faceTextures[idx]) {
            return &(*faceTextures[idx]);
        }
        return nullptr;
    }

    /**
     * @brief 按方向取 tintindex（等价于原 map.find(directionName)）
     * @return tintindex，未找到返回 -1
     */
    [[nodiscard]] i32 findFaceTintIndex(Direction dir) const
    {
        const size_t idx = Directions::index(dir);
        if (idx < 6 && faceTintIndices[idx]) {
            return *faceTintIndices[idx];
        }
        return -1;
    }

    /**
     * @brief 按方向取纹理资源位置（等价于原 map.find(directionName)）
     * @return 资源位置指针，该方向无数据返回 nullptr
     */
    [[nodiscard]] const ResourceLocation* findFaceTextureLocation(Direction dir) const
    {
        const size_t idx = Directions::index(dir);
        if (idx < 6 && faceTextureLocations[idx]) {
            return &(*faceTextureLocations[idx]);
        }
        return nullptr;
    }

    /**
     * @brief 收集所有存在纹理的方向（用于粒子随机选面）
     * @return 方向列表（按 Down..East 顺序）
     */
    [[nodiscard]] std::vector<Direction> collectFacesWithTexture() const
    {
        std::vector<Direction> result;
        for (size_t i = 0; i < 6; ++i) {
            if (faceTextures[i]) {
                result.push_back(Directions::fromIndex(i));
            }
        }
        return result;
    }

    /**
     * @brief 取首个存在纹理资源位置的方向（用于回退，等价于原 map.begin()）
     * @return 方向，无任何面返回 Direction::None
     */
    [[nodiscard]] Direction firstFaceWithTextureLocation() const
    {
        for (size_t i = 0; i < 6; ++i) {
            if (faceTextureLocations[i]) {
                return Directions::fromIndex(i);
            }
        }
        return Direction::None;
    }
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

    /**
     * @brief 构建纹理图集
     * @return 图集构建结果
     */
    [[nodiscard]] Result<AtlasBuildResult> buildTextureAtlas();

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
     * @brief 获取纹理区域
     * @param textureLocation 纹理资源位置
     * @return 纹理区域指针，找不到返回 nullptr
     */
    [[nodiscard]] const TextureRegion* getTextureRegion(const ResourceLocation& textureLocation) const;

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

    // ========================================================================
    // 访问器
    // ========================================================================

    /**
     * @brief 获取纹理图集构建结果
     */
    [[nodiscard]] const AtlasBuildResult& atlasResult() const { return m_atlasResult; }

    /**
     * @brief 检查图集是否已构建
     */
    [[nodiscard]] bool isAtlasBuilt() const { return m_atlasBuilt; }

    /**
     * @brief 获取纹理区域映射（用于测试）
     */
    [[nodiscard]] std::map<ResourceLocation, TextureRegion>& textureRegions() { return m_textureRegions; }
    [[nodiscard]] const std::map<ResourceLocation, TextureRegion>& textureRegions() const { return m_textureRegions; }

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
    // 配置
    // ========================================================================

    /**
     * @brief 设置是否在烘焙模型时自动构建图集
     */
    void setAutoBuildAtlas(bool enable) { m_autoBuildAtlas = enable; }

    /**
     * @brief 清除所有缓存
     */
    void clear();

    // ========================================================================
    // 路径兼容性工具
    // ========================================================================

    /**
     * @brief 获取 MC 1.12/1.13+ 路径变体的替代路径
     *
     * MC 1.13+ 使用 textures/block/ 和 textures/item/（单数），
     * MC 1.12  使用 textures/blocks/ 和 textures/items/（复数）。
     * 此方法在两种格式之间互相转换，用于纹理查找的兼容回退。
     *
     * 此外，还支持实体纹理的子目录/扁平格式互转：
     *   textures/entity/<name>/<name>[.ext] -> textures/entity/<name>[.ext]
     *   textures/entity/<name>[.ext]        -> textures/entity/<name>/<name>[.ext]
     * 注意：实体路径转换会保留文件扩展名（如 .png），仅在目录名与文件名
     * （不含扩展名）相同时才执行转换。
     *
     * @param path 纹理路径（例如 "textures/block/stone" 或 "textures/entity/pig/pig.png"）
     * @return 对应的替代路径，如果路径不匹配已知前缀则返回空字符串
     */
    [[nodiscard]] static std::string getAltTexturePath(const std::string& path);

private:
    std::vector<ResourcePackPtr> m_resourcePacks;
    BlockModelLoader m_modelLoader;
    BlockStateLoader m_blockStateLoader;

    // 已烘焙模型缓存
    std::map<ResourceLocation, BakedBlockModel> m_bakedModels;

    // 方块外观缓存
    std::map<std::string, BlockAppearance> m_blockAppearances;

    // 纹理图集区域映射
    std::map<ResourceLocation, TextureRegion> m_textureRegions;

    // 已构建的图集
    AtlasBuildResult m_atlasResult;

    bool m_autoBuildAtlas = true;
    bool m_atlasBuilt = false;

    // 烘焙所有模型
    [[nodiscard]] Result<void> _bakeAllModels();

    // 计算方块外观
    void _computeBlockAppearances();

    // 收集所有需要的纹理
    [[nodiscard]] std::set<ResourceLocation> _collectRequiredTextures() const;

    // 将纹理路径转换为资源位置
    [[nodiscard]] static ResourceLocation _texturePathToLocation(std::string_view path);

    // 使用 compat 层查找纹理区域（支持 MC 1.12/1.13+ 路径变体）
    // 优先尝试原始路径，若未找到则自动尝试 MC 1.12/1.13+ 路径变体：
    //   textures/block/ ↔ textures/blocks/
    //   textures/item/  ↔ textures/items/
    [[nodiscard]] const TextureRegion* _findTextureRegion(const ResourceLocation& texLoc) const;
};

} // namespace mc
