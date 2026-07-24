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

#include "ContainerListener.hpp"
#include "IInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include <array>
#include <functional>
#include <vector>

namespace mc {

// Forward declarations
class Player;
namespace blockentity {
class EnderChestEntity;
} // namespace blockentity

namespace nbt::tags {
struct compound_tag;
} // namespace nbt::tags

/**
 * @brief 玩家末影箱物品栏
 *
 * 27 格物品存储，与普通箱子大小相同。
 * 数据存储在玩家数据中（NBT key: "EnderItems"），而非方块实体中。
 * 当末影箱方块被打开时，通过 setActiveChest 关联到对应的 EnderChestEntity
 * 以实现开盖动画和距离检查。
 *
 * 参考: net.minecraft.world.entity.player.PlayerEnderChestContainer
 */
class PlayerEnderChestInventory : public IInventory {
public:
    /// 末影箱槽位数量（与普通箱子相同）
    static constexpr i32 ENDER_CHEST_SIZE = 27;

    /// 末影箱槽位起始编号（用于 ItemSlotArgument 映射：200-226）
    static constexpr i32 SLOT_INDEX_START = 200;

    // ========== 构造/析构 ==========

    PlayerEnderChestInventory();
    ~PlayerEnderChestInventory() override = default;

    // 禁止拷贝
    PlayerEnderChestInventory(const PlayerEnderChestInventory&) = delete;
    PlayerEnderChestInventory& operator=(const PlayerEnderChestInventory&) = delete;

    // 允许移动
    PlayerEnderChestInventory(PlayerEnderChestInventory&&) noexcept = default;
    PlayerEnderChestInventory& operator=(PlayerEnderChestInventory&&) noexcept = default;

    // ========== IInventory 接口实现 ==========

    [[nodiscard]] i32 getContainerSize() const noexcept override { return ENDER_CHEST_SIZE; }
    [[nodiscard]] bool isEmpty() const override;
    [[nodiscard]] ItemStack getItem(i32 slot) const override;
    void setItem(i32 slot, const ItemStack& stack) override;
    ItemStack removeItem(i32 slot, i32 count) override;
    ItemStack removeItemNoUpdate(i32 slot) override;
    void clear() override;
    void setChanged() override;
    [[nodiscard]] bool isUsableByPlayer(const Player& player) const override;

    // ========== IInventory 打开/关闭重写 ==========

    /**
     * @brief 玩家打开容器时调用
     *
     * 委托到 startOpen()，处理开盖动画和音效。
     * 当 ChestContainer 打开时通过 IInventory::openInventory 调用。
     */
    void openInventory(Player& player) override;

    /**
     * @brief 玩家关闭容器时调用
     *
     * 委托到 stopOpen()，处理关盖动画、音效和清理活跃箱子引用。
     * 当 ChestContainer 关闭时通过 IInventory::closeInventory 调用。
     */
    void closeInventory(Player& player) override;

    // ========== 末影箱特有功能 ==========

    /**
     * @brief 设置当前关联的末影箱方块实体
     *
     * 当玩家打开末影箱时调用，关联到对应的 EnderChestEntity
     * 以实现开盖动画和 stillValid 距离检查。
     *
     * @param chest 末影箱方块实体指针，关闭时传 nullptr
     */
    void setActiveChest(blockentity::EnderChestEntity* chest) { m_activeChest = chest; }

    /**
     * @brief 获取当前关联的末影箱方块实体
     */
    [[nodiscard]] blockentity::EnderChestEntity* getActiveChest() const { return m_activeChest; }

    /**
     * @brief 检查当前活跃的末影箱是否仍然有效
     */
    [[nodiscard]] bool isActiveChestValid() const;

    /**
     * @brief 玩家打开末影箱
     */
    void startOpen(Player& player);

    /**
     * @brief 玩家关闭末影箱
     */
    void stopOpen(Player& player);

    // ========== 变更通知 ==========

    /**
     * @brief 设置变更回调
     *
     * 当末影箱物品发生变更时调用此回调。用于通知监听者（如容器菜单广播、玩家数据标记脏等）。
     * 参考 MC Java: SimpleContainer.addListener() / setChanged() 通知机制。
     *
     * @param callback 变更回调函数
     *
     * @note 此方法为便捷接口，内部将 callback 包装为 ContainerListener 注册。
     *       如果需要更精细的控制，请直接使用 addListener()/removeListener()。
     */
    void setOnChanged(std::function<void()> callback) { m_onChanged = std::move(callback); }

    /**
     * @brief 添加容器变更监听器
     *
     * 监听器在容器内容变更时通过 containerChanged() 被通知。
     * 参考: net.minecraft.world.SimpleContainer.addListener()
     *
     * @param listener 监听器指针（调用方负责确保指针在移除前有效）
     */
    void addListener(ContainerListener* listener) override;

    /**
     * @brief 移除容器变更监听器
     *
     * 参考: net.minecraft.world.SimpleContainer.removeListener()
     *
     * @param listener 要移除的监听器指针
     */
    void removeListener(ContainerListener* listener) override;

    // ========== 序列化 ==========

    /**
     * @brief 将末影箱物品序列化到 NBT 标签
     *
     * 写入格式：compound_list_tag，每个非空物品包含 "Slot" (byte) + ItemStack NBT 数据
     * 槽位编号为 0-26。
     *
     * @param tag 目标 NBT 复合标签
     */
    void toNbt(nbt::tags::compound_tag& tag) const;

    /**
     * @brief 从 NBT 标签反序列化末影箱物品
     *
     * @param tag 源 NBT 复合标签
     */
    void fromNbt(const nbt::tags::compound_tag& tag);

private:
    std::array<ItemStack, ENDER_CHEST_SIZE> m_items;
    blockentity::EnderChestEntity* m_activeChest = nullptr;
    std::vector<ContainerListener*> m_listeners; ///< 容器变更监听器列表
    std::function<void()> m_onChanged;           ///< 变更回调（兼容旧接口，由 setOnChanged 设置）
};

} // namespace mc
