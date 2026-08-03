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

#include "client/renderer/MeshTypes.hpp"
#include "client/renderer/api/buffer/IStagingBufferPool.hpp"
#include "client/resource/atlas/AtlasHandle.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/metadata/AnimationMetadata.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace mc::client::renderer::trident {
class TextureAtlasTicker;
}

namespace mc::client::resource::atlas {

/**
 * @brief 单图集加载/运行时状态
 *
 * 每个具名图集一个 AtlasEntry，持有 Vulkan 资源、CPU 区域映射、动画 ticker。
 */
struct AtlasEntry {
    ResourceLocation atlasId;
    AtlasHandle handle;
    std::unique_ptr<renderer::trident::TextureAtlasTicker> ticker;
    std::map<ResourceLocation, TextureRegion> regions; ///< sprite 名 → UV 区域
    VkFilter filter = VK_FILTER_NEAREST;
    u32 width = 0;
    u32 height = 0;
    u32 nextInjectY = 0; ///< 运行时注入 sprite 的下一可用 Y（右下角追加策略）

    // ticker 是 PIMPL（仅前向声明），析构必须放 .cpp（TextureAtlasTicker.hpp 完整类型处）
    ~AtlasEntry();
};

/**
 * @brief 统一图集管理器（核心）
 *
 * 对齐原版 1.21.11 TextureManager + SpriteSourceList/SpriteLoader 的编排职责：
 * 用 `atlases/<id>.json` 数据驱动 N 个具名图集的完整加载链
 * （load → run sources → resolve 像素 → stitch 打包 → upload GPU），
 * 替代历史散落在 ItemTextureAtlas/EntityTextureAtlas/ResourceManager 等的硬编码纹理收集。
 *
 * 不持有资源包所有权，仅引用外部 ResourceManager::resourcePacks()。
 */
class AtlasManager {
public:
    AtlasManager() = default;
    ~AtlasManager();

    AtlasManager(const AtlasManager&) = delete;
    AtlasManager& operator=(const AtlasManager&) = delete;

    /**
     * @brief 初始化 Vulkan 句柄
     *
     * @param device Vulkan 设备
     * @param physicalDevice 物理设备
     * @param commandPool 命令池（纹理上传用）
     * @param graphicsQueue 图形队列（纹理上传用）
     * @param stagingPool 统一暂存缓冲池（所有图集像素上传经此子分配，不可为空）
     */
    void initialize(VkDevice device,
        VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool,
        VkQueue graphicsQueue,
        renderer::api::IStagingBufferPool* stagingPool);

    /// 销毁所有图集资源
    void destroy();

    /**
     * @brief 设置资源包引用（不持有所有权）
     *
     * @param packs 资源包列表（通常来自 ResourceManager::resourcePacks()）
     */
    void setResourcePacks(const std::vector<ResourcePackPtr>* packs);

    /**
     * @brief 加载单个图集
     *
     * 跑完整链：AtlasConfigLoader 读 JSON → sources run → resolve 像素 →
     * TextureAtlasBuilder 打包 → AtlasHandle 上传 GPU → 动画注册。
     * 若该图集已存在则先销毁重建。
     *
     * @param atlasId 图集 id（如 minecraft:blocks）
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> loadAtlas(const ResourceLocation& atlasId);

    /**
     * @brief 批量加载图集
     *
     * @param atlasIds 图集 id 列表
     * @return 成功或错误（单个图集失败只 warn 跳过，不中断）
     */
    [[nodiscard]] Result<void> loadAll(const std::vector<ResourceLocation>& atlasIds);

    /**
     * @brief 重载（销毁全部图集后重新 loadAll）
     *
     * @param atlasIds 图集 id 列表
     * @return 成功或错误
     */
    [[nodiscard]] Result<void> reload(const std::vector<ResourceLocation>& atlasIds);

    /// 查询图集（不存在返回 nullptr）
    [[nodiscard]] const AtlasEntry* findAtlas(const ResourceLocation& atlasId) const;

    /**
     * @brief 跨图集反查 sprite 区域
     *
     * @param spriteName sprite 资源位置
     * @return 区域指针；未找到返回 nullptr
     */
    [[nodiscard]] const TextureRegion* findSprite(const ResourceLocation& spriteName) const;

    /**
     * @brief 查询 sprite 区域（带路径变体回退）
     *
     * 先查原名，miss 则查 TexturePathVariant 变体路径。
     * @return 区域指针；未找到返回 nullptr
     */
    [[nodiscard]] const TextureRegion* findSpriteWithVariant(const ResourceLocation& spriteName) const;

    /**
     * @brief 按完整纹理路径查询 sprite 区域
     *
     * 接受 `minecraft:textures/block/stone` 这类带 `textures/` 前缀的完整路径，
     * 内部剥掉 `textures/` 前缀得到 sprite 名（`block/stone`），再走
     * findSpriteWithVariant（含 block↔blocks 等路径变体回退）。
     * 供 ResourceManager::_computeBlockAppearances 与 ChunkMesher 液体路径使用。
     *
     * @param textureLocation 完整纹理资源位置（如 minecraft:textures/block/stone）
     * @return 区域指针；未找到返回 nullptr
     */
    [[nodiscard]] const TextureRegion* findSpriteByTexturePath(const ResourceLocation& textureLocation) const;

    /**
     * @brief 在指定图集查询 sprite 区域
     *
     * @param atlasId 图集 id
     * @param spriteName sprite 资源位置
     * @return 区域指针；未找到返回 nullptr
     */
    [[nodiscard]] const TextureRegion* findSpriteInAtlas(
        const ResourceLocation& atlasId, const ResourceLocation& spriteName) const;

    /**
     * @brief 注入运行时 sprite（如玩家皮肤上传）
     *
     * 追加到指定图集末尾并上传 GPU 子区域。若图集无空间会失败。
     *
     * @param atlasId 图集 id
     * @param spriteName sprite 资源位置
     * @param pixels RGBA8 像素
     * @param width 宽度
     * @param height 高度
     * @return 区域指针；失败返回 nullptr
     */
    [[nodiscard]] const TextureRegion* injectRuntimeSprite(const ResourceLocation& atlasId,
        const ResourceLocation& spriteName,
        const std::vector<u8>& pixels,
        u32 width,
        u32 height);

    /// 聚合所有图集 ticker 的 tick
    void tickAnimations();

    /**
     * @brief 上传所有图集 ticker 待上传的动画帧
     *
     * @param cmd 非 null 时走异步热路径：所有 copy 录进该帧命令缓冲，随帧 submit，
     *            staging 区间登记到 frameIndex 回收桶，不阻塞 CPU（动画帧上传主路径）。
     *            为 null 时回退同步路径：独立 submit+wait，用于非渲染上下文（如测试/重载）。
     * @param frameIndex 当前帧索引（仅异步路径使用，用于 staging 回收桶登记）
     */
    void uploadPendingAnimationFrames(VkCommandBuffer cmd, u32 frameIndex);

    [[nodiscard]] bool isInitialized() const noexcept { return m_device != VK_NULL_HANDLE; }

    /// 获取 missingno 兜底区域（任意图集未命中时使用）
    [[nodiscard]] const TextureRegion* missingRegion() const noexcept { return m_missingRegion.get(); }

private:
    /// 根据 atlasId 查采样配置（blocks/items=NEAREST，particles/gui=LINEAR，默认 NEAREST）
    [[nodiscard]] static VkFilter _filterForAtlas(const ResourceLocation& atlasId);

    /// 获取资源包列表引用（m_packs 为空时返回空 vector）
    [[nodiscard]] const std::vector<ResourcePackPtr>& _packs() const;

    /// 销毁并移除一个图集
    void _destroyAtlas(const ResourceLocation& atlasId);

    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    renderer::api::IStagingBufferPool* m_stagingPool = nullptr;

    const std::vector<ResourcePackPtr>* m_packs = nullptr;
    static const std::vector<ResourcePackPtr> m_emptyPacks;

    std::unordered_map<ResourceLocation, std::unique_ptr<AtlasEntry>> m_atlases;

    /// missingno 兜底区域（一个 16×16 UV 块的占位区域，值在加载首个图集时设置）
    std::unique_ptr<TextureRegion> m_missingRegion;
};

} // namespace mc::client::resource::atlas
