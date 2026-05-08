#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/skin/core/SkinTypes.hpp"
#include "common/skin/core/SkinTextures.hpp"
#include "common/skin/core/GameProfile.hpp"
#include "common/skin/network/PlayerSkinInfo.hpp"
#include "common/skin/manager/SkinCache.hpp"
#include "common/skin/manager/DefaultSkinProvider.hpp"
#include <unordered_map>
#include <memory>
#include <mutex>
#include <functional>
#include <atomic>

namespace mc {
class IResourcePack;  // 前向声明
}

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
 * 参考 MC 1.16.5 SkinManager
 *
 * 使用示例：
 * @code
 * SkinManager manager("./cache/skins");
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

    // 允许移动
    SkinManager(SkinManager&&) noexcept = default;
    SkinManager& operator=(SkinManager&&) noexcept = default;

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
     * @brief 设置资源包（用于加载默认皮肤）
     */
    void setResourcePack(IResourcePack* resourcePack) { m_resourcePack = resourcePack; }

private:
    /**
     * @brief 从缓存加载皮肤
     */
    bool loadFromCache(const SkinTextures& textures, std::shared_ptr<PlayerSkinInfo> info);

    /**
     * @brief 从 textures 属性解析并加载皮肤
     */
    void loadFromTextures(const GameProfile& profile,
                          std::shared_ptr<PlayerSkinInfo> info,
                          const SkinLoadCallbacks& callbacks);

    /**
     * @brief 使用默认皮肤
     */
    void useDefaultSkin(std::shared_ptr<PlayerSkinInfo> info);

    /**
     * @brief UUID 转字符串键
     */
    [[nodiscard]] static std::string uuidToKey(const std::array<u8, 16>& uuid);

    std::string m_cacheDir;
    std::unique_ptr<SkinCache> m_cache;
    std::unique_ptr<DefaultSkinProvider> m_defaultSkinProvider;

    mutable std::mutex m_playerInfosMutex;
    std::unordered_map<std::string, std::shared_ptr<PlayerSkinInfo>> m_playerInfos;  // key: uuid string

    IResourcePack* m_resourcePack = nullptr;

    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_shuttingDown{false};
};

} // namespace mc::skin
