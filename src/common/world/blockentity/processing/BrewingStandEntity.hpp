#pragma once

#include "world/blockentity/ContainerBlockEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include <memory>

namespace mc {

class IWorld;
class Player;

namespace blockentity {

/**
 * @brief 酿造台方块实体
 *
 * 酿造台用于酿造药水，拥有：
 * - 3个药水瓶槽位
 * - 1个材料槽位
 * - 1个燃料槽位（烈焰粉）
 *
 * 参考: net.minecraft.tileentity.BrewingStandTileEntity
 */
class BrewingStandEntity : public ContainerBlockEntity {
public:
    /// 药水瓶槽位数量
    static constexpr i32 BOTTLE_SLOTS = 3;
    /// 材料槽位索引
    static constexpr i32 INGREDIENT_SLOT = 3;
    /// 燃料槽位索引
    static constexpr i32 FUEL_SLOT = 4;
    /// 总槽位数量
    static constexpr i32 TOTAL_SLOTS = 5;
    /// 烈焰粉燃烧时间（每次酿造消耗）
    static constexpr i32 FUEL_PER_BREW = 20;

    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit BrewingStandEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~BrewingStandEntity() override;

    // ========== IInventory 接口实现 ==========

    [[nodiscard]] IInventory* getInventory() override { return &m_inventory; }
    [[nodiscard]] const IInventory* getInventory() const override { return &m_inventory; }
    [[nodiscard]] i32 getContainerSize() const override { return TOTAL_SLOTS; }

    // ========== 酿造逻辑 ==========

    /**
     * @brief 获取燃料等级
     * @return 燃料等级 (0-20)
     */
    [[nodiscard]] i32 getFuelLevel() const { return m_fuel; }

    /**
     * @brief 设置燃料等级
     * @param fuel 燃料等级
     */
    void setFuelLevel(i32 fuel);

    /**
     * @brief 获取酿造时间
     * @return 酿造时间 (0-400)
     */
    [[nodiscard]] i32 getBrewTime() const { return m_brewTime; }

    /**
     * @brief 检查是否正在酿造
     * @return 如果正在酿造返回true
     */
    [[nodiscard]] bool isBrewing() const { return m_brewTime > 0; }

    /**
     * @brief 检查是否有燃料
     * @return 如果有燃料返回true
     */
    [[nodiscard]] bool hasFuel() const { return m_fuel > 0; }

    /**
     * @brief 检查槽位是否有瓶子
     * @param slot 槽位索引 (0-2)
     * @return 如果有瓶子返回true
     */
    [[nodiscard]] bool hasBottle(i32 slot) const;

    // ========== Tick 更新 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const override { return true; }

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

private:
    /**
     * @brief 检查是否可以酿造
     * @return 如果可以酿造返回true
     */
    [[nodiscard]] bool canBrew() const;

    /**
     * @brief 执行酿造
     * @param world 世界引用
     */
    void doBrew(IWorld& world);

    /**
     * @brief 消耗燃料
     */
    void consumeFuel();

    /**
     * @brief 更新方块状态
     * @param world 世界引用
     */
    void updateBlockState(IWorld& world);

    SimpleInventory m_inventory;   ///< 物品存储
    i32 m_brewTime = 0;            ///< 酿造时间 (0-400)
    i32 m_fuel = 0;                ///< 燃料等级 (0-20)
    bool m_lastBrewing = false;    ///< 上一帧是否在酿造
};

} // namespace blockentity
} // namespace mc
