#pragma once

#include "world/blockentity/core/LockableBlockEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include "world/blockentity/transport/IHopper.hpp"
#include "entity/inventory/IInventory.hpp"
#include "util/Direction.hpp"
#include <vector>

namespace mc {

// 前向声明
class ItemEntity;
class IWorld;

namespace blockentity {

/**
 * @brief 漏斗方块实体
 *
 * 实现5格存储容器的物品传输机制：
 * - 从上方容器或物品实体拉取物品
 * - 向下方容器输出物品
 * - 8 tick传输冷却
 * - 红石信号可禁用
 *
 * 参考: net.minecraft.tileentity.HopperTileEntity
 */
class HopperEntity : public LockableBlockEntity, public IHopper {
public:
    /// 漏斗槽位数量
    static constexpr i32 HOPPER_SIZE = 5;

    /// 传输冷却时间（tick）
    static constexpr i32 TRANSFER_COOLDOWN = 8;

    /// 漏斗链优化冷却时间（tick）
    static constexpr i32 TRANSFER_COOLDOWN_CHAIN = 7;

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit HopperEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~HopperEntity() override = default;

    // ========== BlockEntity 接口 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const override { return true; }

    /**
     * @brief 创建方块实体副本
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    // ========== IInventory 接口（通过组合） ==========

    /**
     * @brief 获取背包
     */
    [[nodiscard]] IInventory* getInventory() override { return &m_inventory; }
    [[nodiscard]] const IInventory* getInventory() const override { return &m_inventory; }

    /**
     * @brief 获取容器大小
     */
    [[nodiscard]] i32 getContainerSize() const override { return HOPPER_SIZE; }

    // ========== LockableBlockEntity 接口 ==========

protected:
    [[nodiscard]] String getDefaultName() const override { return "container.hopper"; }

public:
    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;

    // ========== IHopper 接口 ==========

    [[nodiscard]] IWorld* getWorld() override { return m_world; }
    [[nodiscard]] const IWorld* getWorld() const override { return m_world; }
    [[nodiscard]] double getXPos() const override { return static_cast<double>(getPos().x) + 0.5; }
    [[nodiscard]] double getYPos() const override { return static_cast<double>(getPos().y) + 0.5; }
    [[nodiscard]] double getZPos() const override { return static_cast<double>(getPos().z) + 0.5; }
    [[nodiscard]] BlockPos getHopperPos() const override { return getPos(); }

    // ========== 漏斗特定方法 ==========

    /**
     * @brief 检查漏斗是否为空
     * @return 如果所有槽位都为空返回true
     */
    [[nodiscard]] bool isEmpty() const { return m_inventory.isEmpty(); }

    /**
     * @brief 检查漏斗是否已满
     * @return 如果所有槽位都达到最大堆叠返回true
     */
    [[nodiscard]] bool isFull() const;

    /**
     * @brief 获取传输冷却
     * @return 当前冷却tick
     */
    [[nodiscard]] i32 getTransferCooldown() const { return m_transferCooldown; }

    /**
     * @brief 设置传输冷却
     * @param cooldown 冷却tick数
     */
    void setTransferCooldown(i32 cooldown);

    /**
     * @brief 检查是否处于传输冷却中
     * @return 如果正在冷却返回true
     */
    [[nodiscard]] bool isOnTransferCooldown() const { return m_transferCooldown > 0; }

    /**
     * @brief 检查是否可以传输
     * @return 如果冷却时间超过正常值返回true
     *
     * 用于漏斗链优化：当物品从一个漏斗传输到另一个漏斗时，
     * 目标漏斗的冷却时间会减少1 tick（7 tick而非8 tick）
     */
    [[nodiscard]] bool mayTransfer() const { return m_transferCooldown > TRANSFER_COOLDOWN; }

    /**
     * @brief 处理实体碰撞
     * @param world 世界
     * @param entity 碰撞的实体
     */
    void onEntityCollision(IWorld& world, Entity* entity);

    // ========== 静态工具方法 ==========

    /**
     * @brief 拉取物品到漏斗
     * @param hopper 目标漏斗
     * @return 如果成功拉取返回true
     *
     * 从上方容器或物品实体拉取物品到漏斗
     */
    static bool pullItems(IHopper& hopper);

    /**
     * @brief 从物品实体捕获物品
     * @param inventory 目标背包
     * @param itemEntity 物品实体
     * @return 如果成功捕获返回true
     */
    static bool captureItem(IInventory* inventory, ItemEntity* itemEntity);

    /**
     * @brief 获取指定位置的容器
     * @param world 世界
     * @param pos 位置
     * @return 容器指针，如果没有返回nullptr
     */
    [[nodiscard]] static IInventory* getInventoryAtPosition(IWorld* world, const BlockPos& pos);

    /**
     * @brief 获取漏斗源容器（上方一格）
     * @param hopper 漏斗
     * @return 容器指针，如果没有返回nullptr
     */
    [[nodiscard]] static IInventory* getSourceInventory(IHopper& hopper);

    /**
     * @brief 获取漏斗收集区域内的物品实体
     * @param hopper 漏斗
     * @return 物品实体列表
     */
    [[nodiscard]] static std::vector<ItemEntity*> getCaptureItems(IHopper& hopper);

    /**
     * @brief 将物品插入容器
     * @param source 源容器（可为nullptr）
     * @param destination 目标容器
     * @param stack 要插入的物品
     * @param direction 插入方向（可为Direction::None）
     * @return 剩余未插入的物品
     */
    static ItemStack putStackInInventoryAllSlots(
        IInventory* source,
        IInventory* destination,
        const ItemStack& stack,
        Direction direction);

private:
    /**
     * @brief 更新漏斗状态
     * @param pullFunc 拉取函数
     * @return 是否成功传输
     */
    bool updateHopper(std::function<bool()> pullFunc);

    /**
     * @brief 输出物品到目标容器
     * @return 如果成功输出返回true
     */
    bool transferItemsOut();

    /**
     * @brief 获取漏斗输出的目标容器
     * @return 容器指针，如果没有返回nullptr
     */
    [[nodiscard]] IInventory* getInventoryForHopperTransfer();

    /**
     * @brief 检查容器是否已满
     * @param inventory 容器
     * @param side 检查方向
     * @return 如果所有槽位都满了返回true
     */
    [[nodiscard]] static bool isInventoryFull(const IInventory* inventory, Direction side);

    /**
     * @brief 检查容器是否为空
     * @param inventory 容器
     * @param side 检查方向
     * @return 如果所有槽位都为空返回true
     */
    [[nodiscard]] static bool isInventoryEmpty(const IInventory* inventory, Direction side);

    /**
     * @brief 从容器槽位拉取物品
     * @param hopper 目标漏斗
     * @param inventory 源容器
     * @param slotIndex 槽位索引
     * @param direction 拉取方向
     * @return 如果成功拉取返回true
     */
    static bool pullItemFromSlot(
        IHopper& hopper,
        IInventory* inventory,
        i32 slotIndex,
        Direction direction);

    /**
     * @brief 插入物品到容器槽位
     * @param source 源容器（可为nullptr）
     * @param destination 目标容器
     * @param stack 要插入的物品
     * @param slotIndex 目标槽位
     * @param direction 插入方向
     * @return 剩余未插入的物品
     */
    static ItemStack insertStack(
        IInventory* source,
        IInventory* destination,
        const ItemStack& stack,
        i32 slotIndex,
        Direction direction);

    /**
     * @brief 检查是否可以将物品插入槽位
     * @param inventory 容器
     * @param stack 物品
     * @param slotIndex 槽位索引
     * @param direction 插入方向
     * @return 如果可以插入返回true
     */
    [[nodiscard]] static bool canInsertItemInSlot(
        const IInventory* inventory,
        const ItemStack& stack,
        i32 slotIndex,
        Direction direction);

    /**
     * @brief 检查是否可以从槽位提取物品
     * @param inventory 容器
     * @param stack 物品
     * @param slotIndex 槽位索引
     * @param direction 提取方向
     * @return 如果可以提取返回true
     */
    [[nodiscard]] static bool canExtractItemFromSlot(
        const IInventory* inventory,
        const ItemStack& stack,
        i32 slotIndex,
        Direction direction);

    /**
     * @brief 检查两个物品是否可以合并
     * @param stack1 物品1
     * @param stack2 物品2
     * @return 如果可以合并返回true
     */
    [[nodiscard]] static bool canCombine(const ItemStack& stack1, const ItemStack& stack2);

    // ========== 成员变量 ==========

    SimpleInventory m_inventory;    ///< 5格背包
    i32 m_transferCooldown = -1;    ///< 传输冷却（-1表示刚放置）
    u64 m_tickedGameTime = 0;       ///< 上次tick的游戏时间
    IWorld* m_world = nullptr;      ///< 世界指针（非拥有）
};

} // namespace blockentity
} // namespace mc
