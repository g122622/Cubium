#pragma once

#include "AutoSave.hpp"
#include "DirtyTracker.hpp"
#include <memory>
#include <vector>

namespace mc::world::storage {

/**
 * @brief 保存管理器
 *
 * 协调自动保存、脏追踪和手动保存操作。
 * 提供统一的保存入口点。
 *
 * 使用示例：
 * @code
 * SaveManager saveManager(storage);
 * saveManager.initialize(config);
 * saveManager.startAutoSave();
 *
 * // 标记Section为脏
 * saveManager.markDirty(sectionKey);
 *
 * // 在游戏tick中
 * saveManager.tick(currentTick);
 *
 * // 手动保存
 * saveManager.saveAll();
 *
 * // 关闭
 * saveManager.shutdown();
 * @endcode
 */
class SaveManager {
public:
    /**
     * @brief 构造函数
     * @param storage 存储服务引用
     */
    explicit SaveManager(WorldStorageService& storage);

    /**
     * @brief 析构函数
     */
    ~SaveManager();

    // 禁止拷贝
    SaveManager(const SaveManager&) = delete;
    SaveManager& operator=(const SaveManager&) = delete;

    // 允许移动
    SaveManager(SaveManager&&) noexcept = default;
    SaveManager& operator=(SaveManager&&) noexcept = default;

    // ========== 生命周期 ==========

    /**
     * @brief 初始化保存管理器
     * @param config 自动保存配置
     */
    void initialize(const AutoSaveConfig& config);

    /**
     * @brief 关闭保存管理器
     *
     * 停止自动保存并保存所有脏数据。
     */
    void shutdown();

    /**
     * @brief 启动自动保存
     */
    void startAutoSave();

    /**
     * @brief 停止自动保存
     */
    void stopAutoSave();

    /**
     * @brief 检查自动保存是否运行
     */
    [[nodiscard]] bool isAutoSaveRunning() const;

    // ========== 脏追踪 ==========

    /**
     * @brief 标记Section为脏
     * @param key Section标识
     */
    void markDirty(const SectionKey& key);

    /**
     * @brief 批量标记脏
     * @param keys Section标识列表
     */
    void markDirtyBatch(const std::vector<SectionKey>& keys);

    /**
     * @brief 获取脏Section数量
     */
    [[nodiscard]] size_t dirtyCount() const;

    /**
     * @brief 获取脏Section键列表
     */
    [[nodiscard]] std::vector<SectionKey> getDirtyKeys() const;

    // ========== 保存操作 ==========

    /**
     * @brief 执行一次保存
     *
     * 保存所有脏Section。
     *
     * @return 保存的Section数量
     */
    Result<size_t> saveNow();

    /**
     * @brief 保存并创建快照
     *
     * @param snapshotName 快照名称
     * @return 保存的Section数量
     */
    Result<size_t> saveNowWithSnapshot(const std::string& snapshotName);

    /**
     * @brief 保存所有数据
     *
     * 强制保存所有缓存中的Section（无论是否脏）。
     *
     * @return 保存的Section数量
     */
    Result<size_t> saveAll();

    // ========== 主循环 ==========

    /**
     * @brief 每tick调用
     * @param tickCount 当前tick计数
     */
    void tick(u64 tickCount);

    // ========== 统计 ==========

    /**
     * @brief 获取自动保存实例
     */
    [[nodiscard]] AutoSave* autoSave() { return m_autoSave.get(); }
    [[nodiscard]] const AutoSave* autoSave() const { return m_autoSave.get(); }

    /**
     * @brief 获取脏追踪器
     */
    [[nodiscard]] DirtyTracker& dirtyTracker() { return m_dirtyTracker; }
    [[nodiscard]] const DirtyTracker& dirtyTracker() const { return m_dirtyTracker; }

private:
    WorldStorageService& m_storage;
    DirtyTracker m_dirtyTracker;
    std::unique_ptr<AutoSave> m_autoSave;
    bool m_initialized = false;
};

} // namespace mc::world::storage
