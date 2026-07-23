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

#include "core/Types.hpp"
#include "entity/inventory/ContainerTypes.hpp"
#include "item/core/ItemStack.hpp"
#include "resource/ResourceLocation.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc {

// 拖拽相关常量
namespace DragConstants {
constexpr i32 EVENT_MASK = 0x3; // button 低2位是拖拽事件
constexpr i32 MODE_SHIFT = 2;   // button 高位是拖拽模式的位移
constexpr i32 MODE_MASK = 0x3;  // 拖拽模式掩码

constexpr i32 EVENT_START = 0;    // 开始拖拽
constexpr i32 EVENT_ADD_SLOT = 1; // 添加槽位
constexpr i32 EVENT_END = 2;      // 结束拖拽

constexpr i32 MODE_EVEN = 0;   // 均匀分发 (左键)
constexpr i32 MODE_SINGLE = 1; // 逐个分发 (右键)
constexpr i32 MODE_FILL = 2;   // 全部分发 (中键，仅创造模式)

constexpr i32 DRAG_MODE_NONE = -1; // 未开始拖拽
} // namespace DragConstants

// Forward declarations
class Player;

/**
 * @brief 物品丢弃回调类型
 *
 * 当物品需要丢弃到世界中时调用此回调。
 * @param stack 要丢弃的物品堆
 * @param player 触发丢弃的玩家
 * @param retainOwnership 是否保留所有权（如创造模式删除）
 */
using ItemDropCallback = std::function<void(const ItemStack& stack, Player& player, bool retainOwnership)>;

/**
 * @brief 整型引用持有者
 *
 * 用于容器中整型数据的同步（如熔炉燃烧进度、酿造时间等）。
 */
class IntReferenceHolder {
public:
    virtual ~IntReferenceHolder() noexcept = default;

    /**
     * @brief 获取当前值
     */
    [[nodiscard]] virtual i32 get() const = 0;

    /**
     * @brief 设置值
     */
    virtual void set(i32 value) = 0;

    /**
     * @brief 检查是否已修改
     */
    [[nodiscard]] virtual bool isDirty() const
    {
        i32 current = get();
        bool dirty = current != m_lastValue;
        m_lastValue = current;
        return dirty;
    }

private:
    mutable i32 m_lastValue = 0;
};

/**
 * @brief 基于函数的整型引用持有者
 */
class FunctionalIntReferenceHolder : public IntReferenceHolder {
public:
    using Getter = std::function<i32()>;
    using Setter = std::function<void(i32)>;

    FunctionalIntReferenceHolder(Getter getter, Setter setter)
        : m_getter(std::move(getter))
        , m_setter(std::move(setter))
    {}

    [[nodiscard]] i32 get() const override { return m_getter(); }
    void set(i32 value) override { m_setter(value); }

private:
    Getter m_getter;
    Setter m_setter;
};

class Player;
class PlayerInventory;
class IInventory;
class ItemStack;
class Slot;
class BlockEntity;
class BlockPos;

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
    virtual void slotsChanged(IInventory* inventory) { (void)inventory; }

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
     * @brief 获取当前匹配的配方ID
     * @return 当前配方的资源位置ID，如果没有匹配配方则返回空
     *
     * 用于合成容器获取当前匹配的配方ID，以便同步到客户端。
     * 子类（如 CraftingMenu、InventoryCraftingMenu）应重写此方法。
     */
    [[nodiscard]] virtual ResourceLocation getCurrentRecipeId() const { return ResourceLocation(); }

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
     * @brief 设置物品丢弃回调
     * @param callback 回调函数
     *
     * 当物品需要丢弃到世界中时调用此回调。
     * 上层（ServerWorld/IntegratedServer）应注入实现来生成物品实体。
     */
    void setItemDropCallback(ItemDropCallback callback) { m_itemDropCallback = std::move(callback); }

    /**
     * @brief 获取物品丢弃回调
     * @return 回调函数
     */
    [[nodiscard]] const ItemDropCallback& getItemDropCallback() const { return m_itemDropCallback; }

    /**
     * @brief 丢弃物品到世界
     * @param stack 要丢弃的物品堆
     * @param player 触发丢弃的玩家
     * @param retainOwnership 是否保留所有权（如创造模式删除）
     *
     * 如果设置了丢弃回调，则调用回调；否则什么都不做。
     */
    void dropItem(const ItemStack& stack, Player& player, bool retainOwnership = false);

    /**
     * @brief 广播容器变化
     *
     * 同步状态到所有观察者（客户端）。
     * 只通知实际变化的槽位，优化网络同步效率。
     */
    virtual void broadcastChanges();

    /**
     * @brief 检测并发送所有变化
     *
     * 比较 m_lastSlotStates 和当前槽位状态，只发送变化的槽位。
     * 同时检查 IntReferenceHolder 的变化。
     */
    void detectAndSendChanges();

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

    /**
     * @brief 追踪一个整型数据引用
     * @param holder 整型引用持有者
     * @return 引用的索引
     *
     * 用于同步熔炉燃烧时间、酿造进度等数据。
     */
    i32 trackInt(std::unique_ptr<IntReferenceHolder> holder);

    /**
     * @brief 追踪一个整型数据引用（使用 getter/setter）
     * @param getter 获取值的函数
     * @param setter 设置值的函数
     * @return 引用的索引
     */
    i32 trackInt(std::function<i32()> getter, std::function<void(i32)> setter);

    /**
     * @brief 获取追踪的整型数据数量
     */
    [[nodiscard]] i32 getTrackedIntCount() const { return static_cast<i32>(m_trackedInts.size()); }

    /**
     * @brief 获取追踪的整型数据
     * @param index 索引
     */
    [[nodiscard]] IntReferenceHolder* getTrackedInt(i32 index);
    [[nodiscard]] const IntReferenceHolder* getTrackedInt(i32 index) const;

    /**
     * @brief 更新追踪的整型数据
     * @param index 索引
     * @param value 新值
     */
    void setTrackedInt(i32 index, i32 value);

    /**
     * @brief 注册整型数据变化监听器
     * @param listener 回调（index, value），在 detectAndSendChanges 检测到 tracked int 变化时触发
     * @return 监听器ID
     *
     * 用于服务端把熔炉燃烧/熔炼进度等数据经 WindowPropertyPacket 下推客户端。
     */
    i32 addIntListener(std::function<void(i32, i32)> listener);

    /**
     * @brief 移除整型数据变化监听器
     * @param listenerId 监听器ID
     */
    void removeIntListener(i32 listenerId);

    /**
     * @brief 获取当前事务ID并递增
     * @return 当前事务ID
     */
    [[nodiscard]] i16 incrementTransactionId() { return m_transactionId++; }

    /**
     * @brief 获取当前事务ID（不递增）
     * @return 当前事务ID
     */
    [[nodiscard]] i16 getTransactionId() const { return m_transactionId; }

    /**
     * @brief 设置事务ID（用于客户端同步）
     * @param id 事务ID
     */
    void setTransactionId(i16 id) { m_transactionId = id; }

    // ========== 静态工具方法 ==========

    /**
     * @brief 检查玩家是否在指定方块位置附近（64格距离内）
     * @param player 玩家
     * @param blockPos 方块位置
     * @param maxDistanceSq 最大距离平方（默认64*64=4096）
     * @return 如果玩家在指定距离内返回true
     */
    [[nodiscard]] static bool isWithinDistance(
        const Player& player, const BlockPos& blockPos, f32 maxDistanceSq = 4096.0f);

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

    /**
     * @brief 通知整型数据变化
     * @param index 数据索引
     * @param value 新值
     */
    void notifyIntChanged(i32 index, i32 value);

    /**
     * @brief 检查物品是否可以合并到指定槽位
     * @param stack 物品堆
     * @param slot 槽位
     * @return 如果可以合并返回true
     *
     * 子类可重写此方法以实现特殊合并逻辑。
     */
    [[nodiscard]] virtual bool canMergeSlot(const ItemStack& stack, const Slot& slot) const
    {
        (void)stack;
        (void)slot;
        return true;
    }

    /**
     * @brief 检查玩家是否可以进行合成操作
     * @param player 玩家
     * @return 如果可以合成返回true
     *
     * 用于防止多名玩家同时操作同一个容器。
     */
    [[nodiscard]] bool getCanCraft(const Player& player) const;

    /**
     * @brief 设置玩家是否可以进行合成操作
     * @param player 玩家
     * @param canCraft 是否可以合成
     */
    void setCanCraft(const Player& player, bool canCraft);

    /**
     * @brief 设置槽位物品（客户端同步用）
     * @param slotIndex 槽位索引
     * @param stack 物品堆
     */
    void putStackInSlot(i32 slotIndex, const ItemStack& stack);

    /**
     * @brief 设置所有槽位物品（客户端同步用）
     * @param stacks 物品堆列表
     */
    void setAll(const std::vector<ItemStack>& stacks);

    /**
     * @brief 清理容器内容
     * @param player 玩家
     * @param inventory 要清理的背包
     *
     * 将容器内容返回给玩家或丢弃到世界。
     */
    void clearContainer(Player& player, IInventory* inventory);

private:
    /**
     * @brief 处理拾取/放置点击
     */
    ItemStack _handleClickPick(Slot& slot, i32 slotIndex, const ItemStack& slotStack, i32 button);

    /**
     * @brief 处理Shift+点击快速移动
     */
    ItemStack _handleQuickMove(Slot& slot, i32 slotIndex, const ItemStack& slotStack);

    /**
     * @brief 处理数字键交换
     */
    ItemStack _handleSwap(Slot& slot, i32 slotIndex, const ItemStack& slotStack, i32 button);

    /**
     * @brief 处理创造模式中键复制
     */
    ItemStack _handleClone(Slot& slot, i32 slotIndex, const ItemStack& slotStack, Player& player);

    /**
     * @brief 处理丢弃
     */
    ItemStack _handleThrow(Slot& slot, i32 slotIndex, const ItemStack& slotStack, i32 button);

    /**
     * @brief 处理拖拽分发
     *
     * 处理 ADD_SLOT 事件以及发送到实际槽位的 END 事件。当 END 事件触发且仅有一个
     * 拖拽槽位时，会降级为普通 PICKUP 点击，从而触发槽位覆写协议
     * （对应 MC 1.21.11 AbstractContainerMenu#doClick 中 quickcraftSlots.size()==1 的降级路径）。
     *
     * @param slot 当前点击的槽位
     * @param slotIndex 当前槽位索引
     * @param button 编码后的按钮值（低2位=事件状态，高位=拖拽模式）
     * @param player 玩家引用（用于单槽降级时递归调用 clicked）
     */
    ItemStack _handleQuickCraft(Slot& slot, i32 slotIndex, i32 button, Player& player);

    /**
     * @brief 重置拖拽状态
     */
    void _resetDrag();

    /**
     * @brief 从 button 参数中提取拖拽事件状态
     */
    static i32 _getDragEvent(i32 button);

    /**
     * @brief 从 button 参数中提取拖拽模式
     */
    static i32 _extractDragMode(i32 button);

    /**
     * @brief 检查拖拽模式是否有效
     */
    [[nodiscard]] bool _isValidDragMode(i32 dragMode) const;

    /**
     * @brief 检查是否可以拖拽到指定槽位
     */
    [[nodiscard]] bool _canDragIntoSlot(Slot& slot, const ItemStack& stack) const;

    /**
     * @brief 处理双击拾取全部
     */
    ItemStack _handlePickupAll(Slot& slot, i32 slotIndex, const ItemStack& slotStack);

    /**
     * @brief 尝试槽位覆写协议（收纳袋等特殊物品）
     *
     * 对应 MC 1.21.11 AbstractContainerMenu#tryItemClickBehaviourOverride。
     * 在常规拾取/放置逻辑之前调用，给物品机会自定义交互行为：
     * - 若光标物品（carried）重写了 overrideStackedOnOther 且返回 true，则跳过默认逻辑
     * - 否则若槽位物品重写了 overrideOtherStackedOnMe 且返回 true，则跳过默认逻辑
     *
     * @param slot 被点击的槽位
     * @param clickAction 点击动作（Primary=左键，Secondary=右键）
     * @param player 玩家
     * @return 是否处理了此次点击（true 表示跳过默认逻辑）
     */
    bool _tryItemClickBehaviourOverride(Slot& slot, SlotClickAction clickAction, Player& player);

    /**
     * @brief 处理拖拽分发的 START/END 事件（使用 -999 槽位）
     *
     * 拖拽分发协议中，START 和 END 事件发送到 -999 槽位。
     * START 初始化拖拽状态，END 分发物品到所有目标槽位。
     * 当 END 事件触发且仅有一个拖拽槽位时，会降级为普通 PICKUP 点击，
     * 从而触发槽位覆写协议（对应 MC 1.21.11 AbstractContainerMenu#doClick 中
     * quickcraftSlots.size()==1 的降级路径）。
     *
     * @param button 编码后的按钮值（低2位=事件状态，高位=拖拽模式）
     * @param player 玩家引用（用于单槽降级时递归调用 clicked）
     */
    void _handleQuickCraftStartEnd(i32 button, Player& player);

    /**
     * @brief 分发物品到单个拖拽槽位
     * @param toDistribute 要分发的物品（会被修改）
     * @param slotIdx 目标槽位索引
     * @param amount 要分发的数量
     * @return 实际分发的数量
     */
    i32 _distributeToDragSlot(ItemStack& toDistribute, i32 slotIdx, i32 amount);

protected:
    ContainerId m_id;
    PlayerInventory* m_playerInventory;
    std::vector<std::unique_ptr<Slot>> m_slots;
    ItemStack m_carried; // 玩家鼠标持有的物品

    // 槽位变化检测缓存（用于优化网络同步）
    std::vector<ItemStack> m_lastSlotStates;

    // 整型数据追踪（用于同步熔炉进度、酿造时间等）
    std::vector<std::unique_ptr<IntReferenceHolder>> m_trackedInts;

    // 监听器
    std::unordered_map<i32, std::function<void(i32, ItemStack)>> m_listeners;
    std::unordered_map<i32, std::function<void(i32, i32)>> m_intListeners;
    i32 m_nextListenerId = 0;
    i32 m_nextIntListenerId = 0;

    // 槽位范围
    i32 m_playerInvStart = -1; // 玩家背包起始索引
    i32 m_playerInvEnd = -1;   // 玩家背包结束索引
    i32 m_hotbarStart = -1;    // 快捷栏起始索引
    i32 m_hotbarEnd = -1;      // 快捷栏结束索引

    // 拖拽状态
    i32 m_dragEvent = 0;                            // 拖拽事件状态 (0=无, 1=添加槽位, 2=结束)
    i32 m_dragMode = DragConstants::DRAG_MODE_NONE; // 拖拽模式 (-1=无, 0=均匀分发, 1=逐个分发, 2=全部分发)
    std::vector<i32> m_dragSlots;                   // 拖拽目标槽位列表

private:
    i16 m_transactionId = 0; // 事务ID计数器，用于防重放

    // 不能进行合成操作的玩家UUID集合
    std::unordered_set<std::string> m_cannotCraftPlayers;

    // 物品丢弃回调
    ItemDropCallback m_itemDropCallback;
};

} // namespace mc
