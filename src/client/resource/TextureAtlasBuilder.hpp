#pragma once

#include "../renderer/MeshTypes.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <map>
#include <set>
#include <vector>

namespace mc {

class IResourcePack;

/**
 * @brief 纹理图集构建结果
 */
struct AtlasBuildResult {
    std::vector<u8> pixels;                            // RGBA8像素数据
    u32 width = 0;                                     // 图集宽度
    u32 height = 0;                                    // 图集高度
    std::map<ResourceLocation, TextureRegion> regions; // 纹理位置映射
};

/**
 * @brief 单个纹理信息
 */
struct TextureInfo {
    ResourceLocation location;
    std::vector<u8> pixels;
    u32 width = 0;
    u32 height = 0;
};

/**
 * @brief 纹理图集构建器
 *
 * 将多个纹理打包到一个大图集中，用于减少纹理切换
 */
class TextureAtlasBuilder {
public:
    TextureAtlasBuilder();

    // 设置图集尺寸
    void setMaxSize(u32 width, u32 height);

    // 设置纹理边距
    void setPadding(u32 padding);

    // 添加纹理
    [[nodiscard]] Result<void> addTexture(IResourcePack& resourcePack, const ResourceLocation& location);

    // 添加纹理 (直接提供像素数据)
    void addTexture(const ResourceLocation& location, const std::vector<u8>& pixels, u32 width, u32 height);

    // 添加纹理并按动画首帧尺寸裁剪（用于MC动画长条贴图）
    void addTextureFrame(const ResourceLocation& location,
        const std::vector<u8>& pixels,
        u32 width,
        u32 height,
        u32 frameWidth,
        u32 frameHeight);

    // 构建图集
    [[nodiscard]] Result<AtlasBuildResult> build();

    // 清除所有纹理
    void clear();

    // 获取已添加的纹理数量
    [[nodiscard]] size_t textureCount() const { return m_textures.size(); }

    // 获取纹理路径列表
    [[nodiscard]] std::vector<ResourceLocation> getTextureLocations() const;

private:
    u32 m_maxWidth = 4096;
    u32 m_maxHeight = 4096;
    u32 m_padding = 0;
    std::vector<TextureInfo> m_textures;
    std::set<ResourceLocation> m_addedLocations;

    // 矩形打包算法 (使用Skyline算法)
    struct SkylineNode {
        u32 x, y, width;
    };

    [[nodiscard]] bool canPlace(const std::vector<SkylineNode>& skyline,
        u32 width,
        u32 height,
        u32 maxWidth,
        u32 maxHeight,
        u32& outX,
        u32& outY,
        size_t& outIndex) const;

    void placeTexture(std::vector<SkylineNode>& skyline, u32 x, u32 y, u32 width, u32 height, size_t index);
};

} // namespace mc
