#pragma once

#include "../../core/Result.hpp"
#include "../IResourcePack.hpp"
#include "../compat/PackFormat.hpp"
#include "../compat/ResourceMapper.hpp"
#include "../compat/TextureMapper.hpp"
#include "../compat/unified/UnifiedBlockState.hpp"
#include "../compat/unified/UnifiedModel.hpp"
#include "../compat/unified/UnifiedResource.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace mc {
namespace resource {
namespace loader {

/**
 * @brief 资源加载统计
 */
struct ResourceLoadStats {
    u32 texturesLoaded = 0;
    u32 texturesFailed = 0;
    u32 modelsLoaded = 0;
    u32 modelsFailed = 0;
    u32 blockStatesLoaded = 0;
    u32 blockStatesFailed = 0;

    void reset()
    {
        texturesLoaded = 0;
        texturesFailed = 0;
        modelsLoaded = 0;
        modelsFailed = 0;
        blockStatesLoaded = 0;
        blockStatesFailed = 0;
    }
};

/**
 * @brief 资源加载管道
 *
 * 协调加载过程:
 * 1. 检测包格式
 * 2. 创建适当的映射器
 * 3. 加载资源并进行路径转换
 * 4. 转换为统一 IR
 *
 * 用法:
 * @code
 * ResourceLoader loader;
 * loader.addResourcePack(pack);
 * auto textures = loader.loadTextures();
 * auto models = loader.loadModels();
 * auto blockStates = loader.loadBlockStates();
 * @endcode
 */
class ResourceLoader {
public:
    ResourceLoader();
    ~ResourceLoader();

    // -------------------------------------------------------------------------
    // 资源包管理
    // -------------------------------------------------------------------------

    /**
     * @brief 添加资源包到加载管道
     *
     * 包的格式将自动检测。
     * 后添加的包具有更高的优先级。
     *
     * @param pack 要添加的资源包
     * @return 成功或错误
     */
    Result<void> addResourcePack(ResourcePackPtr pack);

    /**
     * @brief 添加具有显式格式的资源包
     *
     * @param pack 要添加的资源包
     * @param format 包格式版本
     * @return 成功或错误
     */
    Result<void> addResourcePack(ResourcePackPtr pack, compat::PackFormat format);

    /**
     * @brief 清除所有资源包
     */
    void clearResourcePacks();

    /**
     * @brief 获取已加载资源包的数量
     */
    size_t getPackCount() const noexcept { return m_packs.size(); }

    // -------------------------------------------------------------------------
    // 纹理加载
    // -------------------------------------------------------------------------

    /**
     * @brief 从所有包加载所有纹理
     *
     * 纹理加载时会应用格式适当的路径转换。
     * 后面的包会覆盖前面包中相同的纹理。
     *
     * @return 统一纹理向量
     */
    std::vector<compat::unified::UnifiedTexture> loadTextures();

    /**
     * @brief 按位置加载单个纹理
     *
     * @param location 纹理资源位置
     * @return 统一纹理或错误
     */
    Result<compat::unified::UnifiedTexture> loadTexture(const ResourceLocation& location);

    /**
     * @brief 设置纹理加载回调
     *
     * 在每个纹理加载成功/失败时调用。
     *
     * @param callback 回调函数
     */
    void setTextureCallback(std::function<void(const std::string&, bool)> callback)
    {
        m_textureCallback = std::move(callback);
    }

    // -------------------------------------------------------------------------
    // 模型加载
    // -------------------------------------------------------------------------

    /**
     * @brief 从所有包加载所有模型
     *
     * @return 统一模型向量
     */
    std::vector<compat::unified::UnifiedModel> loadModels();

    /**
     * @brief 按位置加载单个模型
     *
     * @param location 模型资源位置
     * @return 统一模型或错误
     */
    Result<compat::unified::UnifiedModel> loadModel(const ResourceLocation& location);

    // -------------------------------------------------------------------------
    // 方块状态加载
    // -------------------------------------------------------------------------

    /**
     * @brief 从所有包加载所有方块状态
     *
     * @return 统一方块状态向量
     */
    std::vector<compat::unified::UnifiedBlockState> loadBlockStates();

    /**
     * @brief 按位置加载单个方块状态
     *
     * @param location 方块状态资源位置
     * @return 统一方块状态或错误
     */
    Result<compat::unified::UnifiedBlockState> loadBlockState(const ResourceLocation& location);

    // -------------------------------------------------------------------------
    // 统计
    // -------------------------------------------------------------------------

    /**
     * @brief 获取加载统计
     */
    const ResourceLoadStats& getStats() const noexcept { return m_stats; }

    /**
     * @brief 重置加载统计
     */
    void resetStats() { m_stats.reset(); }

    // -------------------------------------------------------------------------
    // 工具方法
    // -------------------------------------------------------------------------

    /**
     * @brief 从元数据检测包格式
     *
     * @param pack 资源包
     * @return 检测到的包格式
     */
    static compat::PackFormat detectFormat(const IResourcePack& pack);

private:
    struct PackContext {
        ResourcePackPtr pack;
        compat::PackFormat format;
        std::unique_ptr<compat::ResourceMapper> mapper;
    };

    std::vector<PackContext> m_packs;
    ResourceLoadStats m_stats;
    std::function<void(const std::string&, bool)> m_textureCallback;

    /**
     * @brief 读取纹理文件并解码为像素
     *
     * @param pack 资源包
     * @param path 纹理路径
     * @return 像素数据或错误
     */
    Result<compat::unified::PixelData> readTexturePixels(const IResourcePack& pack, const std::string& path);

    /**
     * @brief 在包中查找纹理（带路径变体）
     *
     * @param unifiedPath 统一纹理路径
     * @return (包, 实际路径) 对，如果未找到则为空
     */
    std::pair<const IResourcePack*, std::string> findTexture(const std::string& unifiedPath);
};

} // namespace loader
} // namespace resource
} // namespace mc
