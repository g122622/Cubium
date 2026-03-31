#pragma once

#include "core/Types.hpp"
#include "item/core/ItemStack.hpp"
#include "entity/inventory/ContainerTypes.hpp"
#include "resource/ResourceLocation.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <unordered_map>

namespace mc {

class Player;
class PlayerInventory;
class IInventory;
class ItemStack;
class Slot;
class Container;
class BlockEntity;

/**
 * @brief 容器菜单基类
 *
 * 管理槽位集合和物品交互逻辑。服务端和客户端各有容器实例，
 * 通过网络同步状态。
 *
 * 职责：
 * - 管理槽位和背包引用
 * - 处理玩家点击操作
 * - 检测合成结果
 * - 同步状态到客户端
 *
 * 槽位索引约定：
 * - 0~N-1: 容器槽位（如工作台网格、熔炉输入等）
 * - N~N+35: 玩家主背包和快捷栏
 * - N+36~N+39: 玩家装备槽（头盔、胸甲、护腿、靴子）
 * - N+40: 玩家副手槽
 */
class AbstractContainerMenu {
public:
    virtual ~AbstractContainerMenu() = default;

    /**
     * @brief 获取容器ID
     * @return 容器ID
     */
    [[nodiscard]] ContainerId getId() const { return m_id; }

    /**
     * @brief 获取槽位数量
     * @return 槽位总数
     */
    [[nodiscard]] i32 getSlotCount() const { return static_cast<i32>(m_slots.size()); }

    /**
     * @brief 获取槽位
     * @param index 槽位索引
     * @return 槽位指针，如果无效返回nullptr
     */
    [[nodiscard]] Slot* getSlot(i32 index);
    [[nodiscard]] const Slot* getSlot(i32 index) const;

    /**
     * @brief 处理槽位点击
     * @param slotIndex 槽位索引
     * @param button 鼠标按钮
     * @param clickType 点击类型
     * @param player 玩家
     * @return 操作后的物品堆
     */
    virtual ItemStack clicked(i32 slotIndex, i32 button, ClickType clickType, Player& player);

    /**
     * @brief 快速移动（Shift+点击）
     * @param slotIndex 槽位索引
     * @param player 玩家
     * @return 移动后的物品堆
     */
    virtual ItemStack quickMoveStack(i32 slotIndex, Player& player);

    /**
     * @brief 在指定范围内移动物品
     * @param stack 要移动的物品（会被修改）
     * @param startIndex 起始槽位索引
     * @param endIndex 结束槽位索引（包含）
     * @param reverse 是否反向搜索（从后向前）
     * @return 如果移动成功返回true
     */
    bool moveItemToRange(ItemStack& stack, i32 startIndex, i32 endIndex, bool reverse = false);

    /**
     * @brief 容器内容变化时调用
     * @param inventory 变化的背包
     */
    virtual void slotsChanged(IInventory* inventory) {
        (void)inventory;
    }

    /**
     * @brief 检查玩家是否可以访问容器
     * @param player 玩家
     * @return 如果可以访问返回true
     */
    [[nodiscard]] virtual bool stillValid(const Player& player) const = 0;

    /**
     * @brief 获取结果槽位索引
     * @return 结果槽位索引，不存在则返回-1
     */
    [[nodiscard]] virtual i32 getResultSlotIndex() const { return -1; }

    /**
     * @brief 关闭容器
     * @param player 玩家
     *
     * 将玩家持有的物品返回背包。
     */
    virtual void removed(Player& player);

    /**
     * @brief 获取玩家背包
     * @return 玩家背包指针
     */
    [[nodiscard]] PlayerInventory* getPlayerInventory() { return m_playerInventory; }
    [[nodiscard]] const PlayerInventory* getPlayerInventory() const { return m_playerInventory; }

    /**
     * @brief 获取玩家持有的物品
     * @return 持有物品的引用
     */
    [[nodiscard]] ItemStack& getCarriedItem() { return m_carried; }
    [[nodiscard]] const ItemStack& getCarriedItem() const { return m_carried; }

    /**
     * @brief 设置玩家持有的物品
     * @param stack 物品堆
     */
    void setCarriedItem(const ItemStack& stack);

    /**
     * @brief 广播容器变化
     *
     * 同步状态到所有观察者（客户端）。
     */
    virtual void broadcastChanges();

    /**
     * @brief 添加槽位变化监听器
     * @param listener 监听器回调
     * @return 监听器ID
     */
    i32 addListener(std::function<void(i32, ItemStack)> listener);

    /**
     * @brief 移除监听器
     * @param listenerId 监听器ID
     */
    void removeListener(i32 listenerId);

protected:
    /**
     * @brief 构造函数
     * @param id 容器ID
     * @param playerInventory 玩家背包
     */
    AbstractContainerMenu(ContainerId id, PlayerInventory* playerInventory);

    /**
     * @brief 添加槽位
     * @param slot 槽位
     * @return 槽位索引
     */
    i32 addSlot(std::unique_ptr<Slot> slot);

    /**
     * @brief 添加玩家背包槽位
     * @param startX 起始X坐标
     * @param startY 起始Y坐标
     */
    void addPlayerInventorySlots(i32 startX, i32 startY);

    /**
     * @brief 添加玩家快捷栏槽位
     * @param startX 起始X坐标
     * @param startY 起始Y坐标
     */
    void addPlayerHotbarSlots(i32 startX, i32 startY);

    /**
     * @brief 添加玩家护甲槽位
     * @param startX 起始X坐标
     * @param startY 起始Y坐标
     *
     * 护甲槽位顺序（从上到下）：
     * - 头盔：对应 PlayerInventory[36]
     * - 胸甲：对应 PlayerInventory[37]
     * - 护腿：对应 PlayerInventory[38]
     * - 靴子：对应 PlayerInventory[39]
     */
    void addPlayerArmorSlots(i32 startX, i32 startY);

    /**
     * @brief 添加玩家副手槽位
     * @param x X坐标
     * @param y Y坐标
     *
     * 副手槽对应 PlayerInventory[40]
     */
    void addPlayerOffhandSlot(i32 x, i32 y);

    /**
     * @brief 通知槽位变化
     * @param slotIndex 槽位索引
     * @param stack 新物品堆
     */
    void notifySlotChanged(i32 slotIndex, const ItemStack& stack);

    ContainerId m_id;
    PlayerInventory* m_playerInventory;
    std::vector<std::unique_ptr<Slot>> m_slots;
    ItemStack m_carried;  // 玩家鼠标持有的物品

    std::unordered_map<i32, std::function<void(i32, ItemStack)>> m_listeners;
    i32 m_nextListenerId = 0;

    // 槽位范围
    i32 m_playerInvStart = -1;   // 玩家背包起始索引
    i32 m_playerInvEnd = -1;     // 玩家背包结束索引
    i32 m_hotbarStart = -1;      // 快捷栏起始索引
    i32 m_hotbarEnd = -1;        // 快捷栏结束索引
};

} // namespace mc
