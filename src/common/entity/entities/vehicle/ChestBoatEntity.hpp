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
#include "common/entity/inventory/INamedContainerProvider.hpp"
#include "common/world/blockentity/core/SimpleInventory.hpp"
#include <memory>

namespace mc {

// Forward declarations
class AbstractContainerMenu;
class PlayerInventory;
class DamageSource;

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
 *
 * 参考 MC Java: net.minecraft.world.entity.vehicle.boat.AbstractChestBoat
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
     *
     * 对应 MC Java AbstractChestBoat.remove()：当实体被销毁时，
     * 在服务端掉落容器内的所有物品。
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
     *
     * 对应 MC Java AbstractChestBoat 中的 rideHeight() 计算。
     */
    [[nodiscard]] f64 getMountedYOffset() const override;

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
     * 创建3行9列的通用容器菜单（对应 MC Java 的 ChestMenu.threeRows）。
     * 如果玩家是旁观者模式且容器有战利品表，返回 nullptr。
     *
     * @param containerId 容器ID（由服务端分配）
     * @param player 打开容器的玩家
     * @return 创建的容器菜单
     */
    [[nodiscard]] std::unique_ptr<AbstractContainerMenu> createMenu(i32 containerId, Player& player) override;

    /**
     * @brief 获取容器显示名称
     * @return 容器翻译键 "container.chestBoat"
     */
    [[nodiscard]] std::string getDisplayName() const override;

    // ========== IInventory 代理方法 ==========

    /**
     * @brief 获取容器大小
     */
    [[nodiscard]] i32 getContainerSize() const;

    /**
     * @brief 检查容器是否为空
     */
    [[nodiscard]] bool isInventoryEmpty() const;

    /**
     * @brief 获取指定槽位的物品
     */
    [[nodiscard]] ItemStack getInventoryItem(i32 slot) const;

    /**
     * @brief 设置指定槽位的物品
     */
    void setInventoryItem(i32 slot, const ItemStack& stack);

    /**
     * @brief 从槽位移除指定数量的物品
     */
    ItemStack removeInventoryItem(i32 slot, i32 count);

    /**
     * @brief 清空容器
     */
    void clearInventory();

    /**
     * @brief 获取容器指针
     */
    [[nodiscard]] IInventory* getInventory();

    /**
     * @brief 检查玩家是否仍在交互范围内
     *
     * 对应 MC Java ContainerEntity.isChestVehicleStillValid()。
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

    /**
     * @brief 掉落容器内的所有物品
     *
     * 对应 MC Java ContainerEntity.chestVehicleDestroyed()。
     * 受 doEntityDrops 游戏规则控制。
     */
    void dropInventoryContents();
};

} // namespace entity
} // namespace mc
