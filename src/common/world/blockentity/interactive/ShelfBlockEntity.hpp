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

#include "common/core/Types.hpp"
#include "common/entity/inventory/IInventory.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "world/blockentity/ContainerBlockEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {
namespace blockentity {

/**
 * @brief 书架方块实体
 *
 * 书架（Shelf）是一种交互式容器，特点：
 * - 3个槽位存储物品（可放置任意物品，不限于书籍）
 * - 没有GUI界面，通过交互直接放入/取出物品
 * - 红石比较器输出3位二进制编码信号（每个槽位占用与否对应1位）
 * - 不支持战利品表填充
 * - 不需要tick更新
 * - 可与相邻书架组成侧链连接（最多3个，红石充能时激活）
 *
 * 红石比较器信号计算：
 * - 槽位0占用 → +1
 * - 槽位1占用 → +2
 * - 槽位2占用 → +4
 * - 最大信号强度 = 7（所有槽位均占用）
 *
 * 参考: net.minecraft.block.entity.ShelfBlockEntity (MC 1.21.11)
 */
class ShelfBlockEntity : public ContainerBlockEntity {
public:
    /// 书架槽位数量（1行3列）
    static constexpr i32 SHELF_SIZE = 3;

    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit ShelfBlockEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~ShelfBlockEntity() noexcept override;

    // ========== IInventory 接口实现 ==========

    [[nodiscard]] IInventory* getInventory() override { return &m_inventory; }
    [[nodiscard]] const IInventory* getInventory() const override { return &m_inventory; }
    [[nodiscard]] i32 getContainerSize() const override { return SHELF_SIZE; }

    // ========== 书架特有接口 ==========

    /**
     * @brief 无更新通知地交换槽位物品
     *
     * 将指定槽位的物品取出，并将新物品放入该槽位。
     * 不会触发 setChanged() 通知，由调用方负责后续更新。
     *
     * @param slot 槽位索引 (0-2)
     * @param newItem 要放入槽位的新物品
     * @return 原来槽位中的物品
     */
    [[nodiscard]] ItemStack swapItemNoUpdate(i32 slot, const ItemStack& newItem);

    /**
     * @brief 通知方块实体已更改
     *
     * 向客户端同步方块实体数据。
     * 参考: net.minecraft.block.entity.ShelfBlockEntity.setChanged()
     */
    void markChanged();

    /**
     * @brief 计算红石比较器模拟信号
     *
     * 使用3位二进制编码，每个槽位是否占用对应1位：
     * - 槽位0占用: bit 0 = 1
     * - 槽位1占用: bit 1 = 2
     * - 槽位2占用: bit 2 = 4
     * - 最大输出: 7
     *
     * @return 信号强度 (0-7)
     */
    [[nodiscard]] i32 getAnalogOutputSignal() const;

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

private:
    SimpleInventory m_inventory; ///< 3格物品存储
};

} // namespace blockentity
} // namespace mc
