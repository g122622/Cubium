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

#include "BoatEntity.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/entity/inventory/INamedContainerProvider.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/AdventureModePredicate.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/util/nbt/Nbt.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"
#include <memory>
#include <string>

namespace mc {

// Forward declarations
class AbstractContainerMenu;
class PlayerInventory;
class DamageSource;

namespace loot {
class LootTableManager;
}

namespace entity {

/**
 * @brief 箱子船实体
 *
 * 带有27格容器物品栏的船，玩家可以右键打开容器。
 * 与普通船的区别：
 * - 最多承载1名乘客（普通船为2名）
 * - 乘客位置略微偏移（0.15 vs 0.0）
 * - 被摧毁时掉落容器内的物品
 * - 支持比较器输出红石信号
 * - 实现 INamedContainerProvider 接口，支持打开容器菜单
 */
class ChestBoatEntity : public BoatEntity, public INamedContainerProvider {
public:
    /// 容器大小（27格，与箱子相同）
    static constexpr i32 CONTAINER_SIZE = 27;

    /**
     * @brief 实体工厂方法
     * @param world 世界实例
     * @return 实体实例
     */
    static std::unique_ptr<Entity> create(IWorld* world);

    explicit ChestBoatEntity(Type type = Type::OAK);

    ~ChestBoatEntity() override = default;

    // ========== Entity 接口重写 ==========

    /**
     * @brief 处理玩家交互
     *
     * 优先尝试让玩家乘坐；如果玩家不能乘坐（如蹲下或船已满），
     * 则打开容器菜单。
     */
    ActionResultType processInitialInteract(Player& player, Hand hand) override;

    /**
     * @brief 被摧毁时掉落物品和容器内容
     */
    void dropItem() override;

    /**
     * @brief 实体被移除时掉落容器内容
     */
    void remove() override;

    /**
     * @brief 箱子船最多承载1名乘客
     */
    [[nodiscard]] bool canAddPassenger(const Entity& passenger) const override
    {
        (void)passenger;
        return static_cast<i32>(getPassengers().size()) < 1 && getStatus() != BoatStatus::UnderWater;
    }

    /**
     * @brief 箱子船的乘客Y偏移
     */
    [[nodiscard]] f64 getMountedYOffset() const override;

    /**
     * @brief 获取箱子船对应的物品
     *
     * 重写基类方法，始终返回箱子船物品（而非普通船物品）。
     */
    [[nodiscard]] const Item* getBoatItem() const override;

    /**
     * @brief 取箱子船的 vanilla entity_type 注册表 id（按木种选 <wood>_chest_boat 变体）
     *
     * 重写 BoatEntity::getJavaEntityTypeId()：取箱子船变体（oak_chest_boat/mangrove_chest_boat/
     * bamboo_chest_raft 等）。
     */
    [[nodiscard]] u32 getJavaEntityTypeId() const override;

    /**
     * @brief 获取比较器输出信号强度
     *
     * 基于容器填充率计算信号强度（0-15）。
     */
    [[nodiscard]] i32 getComparatorOutput() const override;

    /**
     * @brief 保存附加数据到NBT
     *
     * 保存容器物品栏和战利品表信息。
     */
    void addAdditionalSaveData(nbt::tags::compound_tag& tag) const override;

    /**
     * @brief 从NBT读取附加数据
     *
     * 读取容器物品栏和战利品表信息。
     */
    Result<void> readAdditionalSaveData(const nbt::tags::compound_tag& tag) override;

    // ========== INamedContainerProvider 接口实现 ==========

    /**
     * @brief 创建箱子容器菜单
     *
     * 创建3行9列的通用容器菜单。
     * 如果玩家是旁观者模式且容器有未解包的战利品表，返回 nullptr。
     * 如果有未解包的战利品表，在创建菜单前先解包填充到容器中。
     *
     * @param containerId 容器ID（由服务端分配）
     * @param player 打开容器的玩家
     * @return 创建的容器菜单，旁观者模式下有未解包战利品表时返回 nullptr
     */
    [[nodiscard]] std::unique_ptr<AbstractContainerMenu> createMenu(i32 containerId, Player& player) override;

    /**
     * @brief 获取容器显示名称
     * @return 容器翻译键 "container.chestBoat"
     */
    [[nodiscard]] std::string getDisplayName() const override;

    // ========== 战利品表接口 ==========

    /**
     * @brief 检查是否有未解包的战利品表
     * @return 如果有未解包的战利品表返回 true
     */
    [[nodiscard]] bool hasLootTable() const noexcept { return !m_lootTable.empty(); }

    /**
     * @brief 获取战利品表ID
     * @return 战利品表资源位置字符串
     */
    [[nodiscard]] const std::string& getLootTable() const noexcept { return m_lootTable; }

    /**
     * @brief 获取战利品表种子
     * @return 种子值
     */
    [[nodiscard]] i64 getLootTableSeed() const noexcept { return m_lootTableSeed; }

    /**
     * @brief 设置战利品表
     *
     * 结构生成时调用此方法设置战利品表。
     * 玩家首次访问容器时，物品将从战利品表生成。
     *
     * @param lootTable 战利品表资源位置
     * @param seed 随机种子
     */
    void setLootTable(const std::string& lootTable, i64 seed);

    /**
     * @brief 解包战利品表
     *
     * 如果有未解包的战利品表，从 LootTableManager 解析并填充到容器中。
     * 填充完成后清除战利品表引用，防止重复填充。
     *
     * @param player 触发填充的玩家（可为 nullptr，用于懒解包场景）
     */
    void unpackLootTable(Player* player);

    // ========== IInventory 代理方法 ==========

    /**
     * @brief 获取容器大小
     */
    [[nodiscard]] i32 getContainerSize() const;

    /**
     * @brief 检查容器是否为空
     *
     * 如果有未解包的战利品表，返回 false（容器可能有物品）。
     */
    [[nodiscard]] bool isInventoryEmpty();

    /**
     * @brief 获取指定槽位的物品
     *
     * 访问前会触发战利品表懒解包（无玩家参数）。
     */
    [[nodiscard]] ItemStack getInventoryItem(i32 slot);

    /**
     * @brief 设置指定槽位的物品
     *
     * 访问前会触发战利品表懒解包（无玩家参数）。
     */
    void setInventoryItem(i32 slot, const ItemStack& stack);

    /**
     * @brief 从槽位移除指定数量的物品
     *
     * 访问前会触发战利品表懒解包（无玩家参数）。
     */
    ItemStack removeInventoryItem(i32 slot, i32 count);

    /**
     * @brief 从槽位移除物品（不触发通知）
     *
     * 访问前会触发战利品表懒解包（无玩家参数）。
     */
    ItemStack removeInventoryItemNoUpdate(i32 slot);

    /**
     * @brief 清空容器
     *
     * 清空前会触发战利品表懒解包（无玩家参数），确保物品已生成。
     */
    void clearInventory();

    /**
     * @brief 获取容器指针
     */
    [[nodiscard]] IInventory* getInventory();

    /**
     * @brief 检查玩家是否仍在交互范围内
     *
     * 玩家距离实体8格以内为有效。
     */
    [[nodiscard]] bool stillValid(const Player& player) const;

private:
    /// 27格容器物品栏（与箱子相同）
    std::unique_ptr<blockentity::SimpleInventory> m_inventory;

    /// 战利品表ID（用于延迟填充容器，如自然生成的箱子船）
    std::string m_lootTable;

    /// 战利品表种子
    i64 m_lootTableSeed = 0L;

    /// 战利品表是否已解包填充
    bool m_lootFilled = false;

    /**
     * @brief 掉落容器内的所有物品
     *
     * 受 doEntityDrops 游戏规则控制。
     */
    void dropInventoryContents();
};

} // namespace entity
} // namespace mc
