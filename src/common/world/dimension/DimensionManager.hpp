#pragma once

#include "Dimension.hpp"
#include "../../core/Result.hpp"
#include <unordered_map>
#include <memory>
#include <vector>
#include <functional>

namespace mc {

/**
 * @brief 维度管理器
 *
 * 管理所有维度实例的注册表，提供维度访问、遍历等功能。
 * 这是维度系统的基础设施，服务端和客户端都可使用。
 *
 * 使用示例:
 * @code
 * DimensionManager manager;
 * manager.initialize(seed);
 *
 * // 访问维度
 * Dimension* overworld = manager.getDimension(DimensionManager::OVERWORLD);
 *
 * // 遍历所有维度
 * manager.forEachDimension([](Dimension& dim) {
 *     // 处理每个维度
 * });
 * @endcode
 *
 * @note 参考 MC 1.16.5 的维度注册表概念
 */
class DimensionManager {
public:
    // ========== 维度ID常量 ==========

    /// 主世界维度ID
    static constexpr DimensionId OVERWORLD = 0;

    /// 下界维度ID
    static constexpr DimensionId NETHER = 1;

    /// 末地维度ID
    static constexpr DimensionId THE_END = 2;

    // ========== 构造与析构 ==========

    DimensionManager() = default;
    virtual ~DimensionManager() = default;

    // 禁止拷贝
    DimensionManager(const DimensionManager&) = delete;
    DimensionManager& operator=(const DimensionManager&) = delete;

    // 允许移动
    DimensionManager(DimensionManager&&) noexcept = default;
    DimensionManager& operator=(DimensionManager&&) noexcept = default;

    // ========== 初始化 ==========

    /**
     * @brief 使用世界种子初始化维度
     *
     * 创建所有原版维度（主世界、下界、末地）。
     *
     * @param seed 世界种子
     */
    void initialize(u64 seed);

    /**
     * @brief 关闭维度管理器
     *
     * 清理所有维度实例。
     */
    void shutdown();

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] bool isInitialized() const { return m_initialized; }

    // ========== 维度注册 ==========

    /**
     * @brief 注册维度
     *
     * @param dimension 维度实例
     * @return 是否注册成功
     */
    bool registerDimension(std::unique_ptr<Dimension> dimension);

    /**
     * @brief 注销维度
     *
     * @param id 维度ID
     * @return 是否注销成功
     */
    bool unregisterDimension(DimensionId id);

    // ========== 维度访问 ==========

    /**
     * @brief 获取维度
     *
     * @param id 维度ID
     * @return 维度指针，如果不存在则返回 nullptr
     */
    [[nodiscard]] Dimension* getDimension(DimensionId id);
    [[nodiscard]] const Dimension* getDimension(DimensionId id) const;

    /**
     * @brief 检查维度是否存在
     */
    [[nodiscard]] bool hasDimension(DimensionId id) const;

    /**
     * @brief 获取主世界维度
     */
    [[nodiscard]] Dimension* getOverworld();
    [[nodiscard]] const Dimension* getOverworld() const;

    /**
     * @brief 获取下界维度
     */
    [[nodiscard]] Dimension* getNether();
    [[nodiscard]] const Dimension* getNether() const;

    /**
     * @brief 获取末地维度
     */
    [[nodiscard]] Dimension* getTheEnd();
    [[nodiscard]] const Dimension* getTheEnd() const;

    // ========== 维度类型 ==========

    /**
     * @brief 获取维度类型
     *
     * @param id 维度ID
     * @return 维度类型指针，如果不存在则返回 nullptr
     */
    [[nodiscard]] const DimensionType* getDimensionType(DimensionId id) const;

    /**
     * @brief 根据名称获取维度ID
     *
     * @param name 维度名称（如 "minecraft:overworld"）
     * @return 维度ID，如果不存在则返回 -1
     */
    [[nodiscard]] DimensionId getDimensionIdByName(const String& name) const;

    // ========== 遍历 ==========

    /**
     * @brief 遍历所有维度
     *
     * @param func 对每个维度调用的函数
     */
    void forEachDimension(std::function<void(Dimension&)> func);
    void forEachDimension(std::function<void(const Dimension&)> func) const;

    // ========== 信息 ==========

    /**
     * @brief 获取所有注册的维度ID
     */
    [[nodiscard]] std::vector<DimensionId> getDimensionIds() const;

    /**
     * @brief 获取维度数量
     */
    [[nodiscard]] size_t dimensionCount() const { return m_dimensions.size(); }

    /**
     * @brief 获取世界种子
     */
    [[nodiscard]] u64 worldSeed() const { return m_worldSeed; }

    /**
     * @brief 获取默认出生维度
     *
     * 默认为主世界。
     */
    [[nodiscard]] DimensionId defaultSpawnDimension() const { return OVERWORLD; }

protected:
    std::unordered_map<DimensionId, std::unique_ptr<Dimension>> m_dimensions;
    std::unordered_map<String, DimensionId> m_nameToId;
    u64 m_worldSeed = 0;
    bool m_initialized = false;

    /**
     * @brief 注册原版维度类型
     */
    void registerVanillaDimensions(u64 seed);
};

} // namespace mc
