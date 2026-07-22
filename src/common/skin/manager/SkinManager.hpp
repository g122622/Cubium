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

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/skin/core/GameProfile.hpp"
#include "common/skin/core/SkinTextures.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include "common/skin/loader/FileSkinLoader.hpp"
#include "common/skin/loader/HttpSkinLoader.hpp"
#include "common/skin/manager/DefaultSkinProvider.hpp"
#include "common/skin/manager/SkinCache.hpp"
#include "common/skin/network/PlayerSkinInfo.hpp"
#include "common/util/thread/UniversalWorkerPool.hpp"
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace mc::skin {

/**
 * @brief 皮肤加载回调
 */
struct SkinLoadCallbacks {
    std::function<void(const std::array<u8, 16>& uuid)> onSkinLoaded;
    std::function<void(const std::array<u8, 16>& uuid, const std::string& error)> onSkinFailed;
};

/**
 * @brief 皮肤管理器
 *
 * 核心职责：
 * - 管理玩家皮肤信息的缓存和加载
 * - 协调本地缓存、网络下载和默认皮肤
 * - 线程安全的皮肤信息访问
 *
 * 使用示例：
 * @code
 * SkinManager manager(gameDirectory.cacheDir() / "skins");
 * manager.initialize();
 *
 * // 添加玩家
 * GameProfile profile(uuid, "PlayerName");
 * profile.addProperty({"textures", base64Data});
 * auto skinInfo = manager.getOrCreatePlayerInfo(profile);
 *
 * // 获取皮肤
 * auto location = skinInfo->getSkinLocation();
 * auto skinType = skinInfo->getSkinType();
 * @endcode
 */
class SkinManager {
public:
    /**
     * @brief 构造皮肤管理器
     * @param cacheDir 缓存目录路径
     */
    explicit SkinManager(const std::string& cacheDir);

    ~SkinManager();

    // 禁止拷贝
    SkinManager(const SkinManager&) = delete;
    SkinManager& operator=(const SkinManager&) = delete;

    // 禁止移动（包含 std::mutex）
    SkinManager(SkinManager&&) noexcept = delete;
    SkinManager& operator=(SkinManager&&) noexcept = delete;

    // ========== 初始化 ==========

    /**
     * @brief 初始化皮肤管理器
     *
     * 加载默认皮肤、初始化缓存目录。
     *
     * @return 成功或错误
     */
    Result<void> initialize();

    /**
     * @brief 关闭皮肤管理器
     *
     * 等待所有加载任务完成，保存缓存元数据。
     */
    void shutdown();

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized.load(); }

    // ========== 玩家信息管理 ==========

    /**
     * @brief 获取或创建玩家皮肤信息
     *
     * 如果玩家信息不存在，创建新条目并启动皮肤加载。
     *
     * @param profile 玩家档案
     * @return 玩家皮肤信息
     */
    [[nodiscard]] std::shared_ptr<PlayerSkinInfo> getOrCreatePlayerInfo(const GameProfile& profile);

    /**
     * @brief 获取玩家皮肤信息
     * @param uuid 玩家UUID
     * @return 皮肤信息指针，不存在返回 nullptr
     */
    [[nodiscard]] std::shared_ptr<PlayerSkinInfo> getPlayerInfo(const std::array<u8, 16>& uuid) const;

    /**
     * @brief 获取玩家皮肤信息（通过UUID字符串）
     * @param uuidStr UUID字符串
     * @return 皮肤信息指针，不存在返回 nullptr
     */
    [[nodiscard]] std::shared_ptr<PlayerSkinInfo> getPlayerInfo(const std::string& uuidStr) const;

    /**
     * @brief 移除玩家皮肤信息
     * @param uuid 玩家UUID
     */
    void removePlayerInfo(const std::array<u8, 16>& uuid);

    /**
     * @brief 清除所有玩家信息
     */
    void clearAllPlayerInfos();

    /**
     * @brief 获取玩家数量
     */
    [[nodiscard]] size_t playerCount() const;

    // ========== 皮肤加载 ==========

    /**
     * @brief 请求加载玩家皮肤
     *
     * 异步加载皮肤，加载完成后调用回调。
     *
     * @param profile 玩家档案（包含 textures 属性）
     * @param callbacks 加载回调
     */
    void loadSkin(const GameProfile& profile, const SkinLoadCallbacks& callbacks = {});

    /**
     * @brief 检查皮肤是否已加载
     * @param uuid 玩家UUID
     */
    [[nodiscard]] bool isSkinLoaded(const std::array<u8, 16>& uuid) const;

    // ========== 默认皮肤 ==========

    /**
     * @brief 获取默认皮肤
     * @param uuid 用于确定 Steve/Alex
     * @return 皮肤 ResourceLocation
     */
    [[nodiscard]] ResourceLocation getDefaultSkin(const std::array<u8, 16>& uuid) const;

    /**
     * @brief 获取默认皮肤类型
     * @param uuid 用于确定宽/窄手臂
     * @return 皮肤类型
     */
    [[nodiscard]] SkinType getDefaultSkinType(const std::array<u8, 16>& uuid) const;

    /**
     * @brief 获取默认皮肤提供者
     */
    [[nodiscard]] const DefaultSkinProvider& defaultSkinProvider() const { return *m_defaultSkinProvider; }

    // ========== 缓存访问 ==========

    /**
     * @brief 获取皮肤缓存
     */
    [[nodiscard]] SkinCache& cache() { return *m_cache; }
    [[nodiscard]] const SkinCache& cache() const { return *m_cache; }

    // ========== 资源包设置 ==========

    /**
     * @brief 设置资源包列表（用于加载默认皮肤及资源包内皮肤）
     *
     * 必须在 initialize() 调用之前设置，否则 DefaultSkinProvider / FileSkinLoader
     * 将无法从资源包读取皮肤 PNG 纹理，回退到零像素占位数据或加载失败。
     * 列表顺序为添加顺序（低→高优先级），查找时反向遍历（后添加的优先），
     * 与 ResourceManager 的纹理加载惯例一致。
     *
     * @param resourcePacks 资源包指针列表（非所有权，调用方保证生命周期）
     */
    void setResourcePacks(std::vector<IResourcePack*> resourcePacks)
    {
        m_resourcePacks = std::move(resourcePacks);
        // 同步给 DefaultSkinProvider，确保后续 initialize 时能读取
        m_defaultSkinProvider->setResourcePacks(m_resourcePacks);
        // 同步给 FileSkinLoader（用于从资源包加载皮肤）
        m_fileLoader->setResourcePacks(m_resourcePacks);
    }

    // ========== 线程池设置 ==========

    /**
     * @brief 注入工作线程池用于异步皮肤加载
     *
     * 必须在 initialize() 之前调用，线程池由调用方拥有，必须保证生命周期
     * 长于 SkinManager（或在 shutdown 后释放）。传入 nullptr 切换回同步降级模式。
     *
     * @param workerPool 工作线程池指针（非所有权）
     */
    void setWorkerPool(util::UniversalWorkerPool* workerPool)
    {
        m_workerPool = workerPool;
        m_fileLoader->setWorkerPool(workerPool);
        m_httpLoader->setWorkerPool(workerPool);
    }

    // ========== 加载器访问 ==========

    /**
     * @brief 获取文件加载器（用于直接调用 load/loadAsync）
     */
    [[nodiscard]] FileSkinLoader& fileLoader() { return *m_fileLoader; }
    [[nodiscard]] const FileSkinLoader& fileLoader() const { return *m_fileLoader; }

    /**
     * @brief 获取 HTTP 加载器（用于直接调用 load/loadAsync）
     */
    [[nodiscard]] HttpSkinLoader& httpLoader() { return *m_httpLoader; }
    [[nodiscard]] const HttpSkinLoader& httpLoader() const { return *m_httpLoader; }

private:
    /**
     * @brief 从缓存加载皮肤
     */
    bool _loadFromCache(const SkinTextures& textures, std::shared_ptr<PlayerSkinInfo> info);

    /**
     * @brief 从 textures 属性解析并加载皮肤
     */
    void _loadFromTextures(
        const GameProfile& profile, std::shared_ptr<PlayerSkinInfo> info, const SkinLoadCallbacks& callbacks);

    /**
     * @brief 使用默认皮肤
     */
    void _useDefaultSkin(std::shared_ptr<PlayerSkinInfo> info);

    /**
     * @brief UUID 转字符串键
     */
    [[nodiscard]] static std::string _uuidToKey(const std::array<u8, 16>& uuid);

    std::string m_cacheDir;
    std::unique_ptr<SkinCache> m_cache;
    std::unique_ptr<DefaultSkinProvider> m_defaultSkinProvider;
    std::unique_ptr<FileSkinLoader> m_fileLoader;
    std::unique_ptr<HttpSkinLoader> m_httpLoader;
    util::UniversalWorkerPool* m_workerPool = nullptr;

    mutable std::mutex m_playerInfosMutex;
    std::unordered_map<std::string, std::shared_ptr<PlayerSkinInfo>> m_playerInfos; // key: uuid string

    std::vector<IResourcePack*> m_resourcePacks;

    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_shuttingDown{false};
};

} // namespace mc::skin
