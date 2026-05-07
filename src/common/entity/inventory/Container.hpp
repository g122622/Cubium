#pragma once

#include "../../core/Types.hpp"
#include "ContainerTypes.hpp"
#include "IInventory.hpp"
#include "Slot.hpp"
#include "../../item/core/ItemStack.hpp"
#include <vector>
#include <memory>
#include <functional>

namespace mc {

// Forward declarations
class IInventory;
class Player;
class Container;
class PlayerInventory;

/**
 * @brief 槽位范围结构
 */
struct SlotRange {
    i32 start;
    i32 end;  // 不包含

    SlotRange(i32 s, i32 e) : start(s), end(e) {}
    [[nodiscard]] i32 size() const { return end - start; }
    [[nodiscard]] bool contains(i32 slot) const { return slot >= start && slot < end; }
};

/**
 * @brief 容器类
 *
 * @deprecated 此类是早期实现，功能不完整。
 *             请使用 AbstractContainerMenu 代替，它提供了完整的容器功能：
 *             - 物品丢弃回调机制（setItemDropCallback/dropItem）
 *             - 拖拽分发状态机（MC 1.16.5 对齐）
 *             - Player 引用支持
 *             - 完整的网络同步
 *
 *             仅保留用于测试和 PlayerContainer 兼容性。
 *
 * 管理客户端-服务端的背包同步。
 * 持有多个槽位引用，支持物品操作和同步。
 *
 * 参考: net.minecraft.inventory.container.Container
 */
class Container {
public:
    // ========== 常量 ==========

    /// 无效槽位索引
    static constexpr i32 INVALID_SLOT = -1;

    /// 最大槽位数
    static constexpr i32 MAX_SLOTS = 256;

    // ========== 构造函数 ==========

    /**
     * @brief 构造容器
     * @param type 容器类型
     * @param id 容器ID（用于网络同步）
     */
    Container(ContainerType type, ContainerId id);

    virtual ~Container() = default;

    // 禁止拷贝
    Container(const Container&) = delete;
    Container& operator=(const Container&) = delete;

    // ========== 基本信息 ==========

    /**
     * @brief 获取容器类型
     */
    [[nodiscard]] ContainerType type() const { return m_type; }

    /**
     * @brief 获取容器ID
     */
    [[nodiscard]] ContainerId getId() const { return m_id; }

    /**
     * @brief 获取容器ID
     */
    [[nodiscard]] ContainerId id() const { return m_id; }

    /**
     * @brief 获取槽位数量
     */
    [[nodiscard]] i32 getSlotCount() const { return static_cast<i32>(m_slots.size()); }

    /**
     * @brief 获取鼠标持有的物品堆
     */
    [[nodiscard]] const ItemStack& getCarriedItem() const { return m_carriedItem; }

    /**
     * @brief 获取所有槽位
     */
    [[nodiscard]] const std::vector<std::unique_ptr<Slot>>& slots() const { return m_slots; }

    // ========== 槽位管理 ==========

    /**
     * @brief 添加槽位
     * @param slot 槽位（容器获得所有权）
     * @return 槽位索引
     */
    i32 addSlot(std::unique_ptr<Slot> slot);

    /**
     * @brief 添加槽位范围
     * @param inventory 背包
     * @param start 起始槽位索引
     * @param count 槽位数量
     * @param x 显示位置X起始
     * @param y 显示位置Y
     * @return 槽位范围
     */
    SlotRange addInventorySlots(IInventory* inventory, i32 start, i32 count, i32 x, i32 y);

    /**
     * @brief 获取槽位
     * @param index 槽位索引
     * @return 槽位指针，无效返回nullptr
     */
    [[nodiscard]] Slot* getSlot(i32 index);
    [[nodiscard]] const Slot* getSlot(i32 index) const;

    /**
     * @brief 获取槽位中的物品
     * @param index 槽位索引
     * @return 物品堆，无效返回空堆
     */
    [[nodiscard]] ItemStack getSlotItem(i32 index) const;

    // ========== 物品操作 ==========

    /**
     * @brief 处理槽位点击
     * @param slotIndex 槽位索引
     * @param button 鼠标按钮 (0=左键, 1=右键)
     * @param clickType 点击类型
     * @param player 玩家（用于创造模式检测等）
     * @return 操作后的鼠标物品
     */
    ItemStack clicked(i32 slotIndex, i32 button, ClickType clickType, ItemStack cursorItem);

    /**
     * @brief 快速移动物品 (Shift+点击)
     * @param slotIndex 槽位索引
     * @param cursorItem 当前鼠标物品
     * @return 操作后的鼠标物品
     */
    ItemStack quickMoveStack(i32 slotIndex, ItemStack cursorItem);

    /**
     * @brief 合并物品到指定范围
     * @param stack 要合并的物品
     * @param start 起始槽位
     * @param end 结束槽位（不包含）
     * @param reverse 是否反向遍历
     * @return 是否成功合并了物品
     */
    bool mergeItem(ItemStack& stack, i32 start, i32 end, bool reverse = false);

    /**
     * @brief 合并物品到指定范围（槽位范围版本）
     * @param stack 要合并的物品
     * @param range 槽位范围
     * @param reverse 是否反向遍历
     * @return 是否成功合并了物品
     */
    bool mergeItem(ItemStack& stack, const SlotRange& range, bool reverse = false) {
        return mergeItem(stack, range.start, range.end, reverse);
    }

    // ========== 槽位范围 ==========

    /**
     * @brief 设置玩家背包槽位范围（用于Shift+点击）
     * @param start 起始槽位
     * @param end 结束槽位
     */
    void setPlayerInventoryRange(i32 start, i32 end) {
        m_playerInventoryRange = SlotRange(start, end);
    }

    /**
     * @brief 获取玩家背包槽位范围
     */
    [[nodiscard]] const SlotRange& playerInventoryRange() const { return m_playerInventoryRange; }

    /**
     * @brief 设置容器槽位范围（用于Shift+点击）
     * @param start 起始槽位
     * @param end 结束槽位
     */
    void setContainerInventoryRange(i32 start, i32 end) {
        m_containerInventoryRange = SlotRange(start, end);
    }

    /**
     * @brief 获取容器槽位范围
     */
    [[nodiscard]] const SlotRange& containerInventoryRange() const { return m_containerInventoryRange; }

    // ========== 变更检测 ==========

    /**
     * @brief 检测是否有变更
     */
    [[nodiscard]] bool hasChanged() const { return m_changed; }

    /**
     * @brief 标记为已变更
     */
    void setChanged() { m_changed = true; }

    /**
     * @brief 清除变更标记
     */
    void clearChanged() { m_changed = false; }

    /**
     * @brief 获取变更计数器
     */
    [[nodiscard]] i32 getChangeCount() const { return m_changeCount; }

    // ========== 同步 ==========

    /**
     * @brief 获取所有槽位物品（用于同步）
     */
    [[nodiscard]] std::vector<ItemStack> getAllSlots() const;

    /**
     * @brief 设置所有槽位物品（用于同步）
     */
    void setAllSlots(const std::vector<ItemStack>& items);

    /**
     * @brief 序列化容器内容
     */
    void serialize(network::PacketSerializer& ser) const;

    /**
     * @brief 反序列化容器内容
     */
    [[nodiscard]] static Result<std::unique_ptr<Container>> deserialize(
        network::PacketDeserializer& deser);

    // ========== 事件回调 ==========

    /**
     * @brief 设置内容变更回调
     */
    void setOnContentChanged(std::function<void()> callback) {
        m_onContentChanged = std::move(callback);
    }

    /**
     * @brief 设置槽位变更回调
     */
    void setOnSlotChanged(std::function<void(i32)> callback) {
        m_onSlotChanged = std::move(callback);
    }

protected:
    /**
     * @brief 快速移动物品的实现（子类可重写）
     * @param slotIndex 槽位索引
     * @param cursorItem 当前鼠标物品
     * @return 操作后的鼠标物品
     */
    virtual ItemStack doQuickMove(i32 slotIndex, ItemStack cursorItem);

    /**
     * @brief 检查是否可以快速移动（子类可重写）
     * @param slotIndex 槽位索引
     */
    [[nodiscard]] virtual bool canQuickMove(i32 slotIndex) const;

private:
    /**
     * @brief 处理拾取点击
     */
    ItemStack handlePickClick(i32 slotIndex, i32 button, ItemStack cursorItem);

    /**
     * @brief 处理快速移动点击 (Shift+点击)
     */
    ItemStack handleQuickMoveClick(i32 slotIndex, ItemStack cursorItem);

    /**
     * @brief 处理丢弃点击
     */
    ItemStack handleThrowClick(i32 slotIndex, i32 button, ItemStack cursorItem);

    /**
     * @brief 处理拖拽
     */
    ItemStack handleDragClick(i32 slotIndex, i32 button, ItemStack cursorItem);

    /**
     * @brief 处理数字键交换
     */
    ItemStack handleSwapClick(i32 slotIndex, i32 button, ItemStack cursorItem);

    /**
     * @brief 处理创造模式克隆
     */
    ItemStack handleCloneClick(i32 slotIndex, ItemStack cursorItem);

    ContainerType m_type;
    ContainerId m_id;
    std::vector<std::unique_ptr<Slot>> m_slots;
    SlotRange m_playerInventoryRange{0, 0};
    SlotRange m_containerInventoryRange{0, 0};
    bool m_changed = false;
    i32 m_changeCount = 0;

    // 拖拽状态
    bool m_isDragging = false;
    i32 m_dragButton = -1;
    std::vector<i32> m_dragSlots;

    // 事件回调
    std::function<void()> m_onContentChanged;
    std::function<void(i32)> m_onSlotChanged;
    ItemStack m_carriedItem;
};

/**
 * @brief 玩家背包容器
 *
 * 包含玩家所有背包槽位。
 */
class PlayerContainer : public Container {
public:
    /**
     * @brief 构造玩家背包容器
     * @param playerInventory 玩家背包
     */
    explicit PlayerContainer(PlayerInventory* playerInventory);

    /**
     * @brief 获取玩家背包
     */
    [[nodiscard]] PlayerInventory* getPlayerInventory() const { return m_playerInventory; }

protected:
    ItemStack doQuickMove(i32 slotIndex, ItemStack cursorItem) override;
    [[nodiscard]] bool canQuickMove(i32 slotIndex) const override;

private:
    PlayerInventory* m_playerInventory;
};

} // namespace mc
