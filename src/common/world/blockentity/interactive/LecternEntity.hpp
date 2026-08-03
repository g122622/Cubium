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
#include "common/world/block/BlockPos.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class IWorld;
class Player;
class ItemStack;

namespace blockentity {

/**
 * @brief 讲台方块实体
 *
 * 讲台用于展示和阅读书本，特点：
 * - 1个槽位存放书
 * - 支持书与笔、成书、附魔书
 * - 红石比较器输出当前页数
 * - 右键翻页
 *
 * 参考: net.minecraft.tileentity.LecternTileEntity
 */
class LecternEntity : public BlockEntity {
public:
    /// 书槽位索引
    static constexpr i32 SLOT_BOOK = 0;

    // ========== 构造函数 ==========

    /**
     * @brief 构造函数
     * @param pos 方块位置
     */
    explicit LecternEntity(const BlockPos& pos);

    /**
     * @brief 析构函数
     */
    ~LecternEntity() noexcept override;

    // ========== 书本接口 ==========

    /**
     * @brief 获取书本
     * @return 书本物品
     */
    [[nodiscard]] ItemStack getBook() const;

    /**
     * @brief 设置书本
     * @param book 书本物品
     * @return 如果设置成功返回true
     */
    bool setBook(const ItemStack& book);

    /**
     * @brief 移除书本
     * @return 被移除的书本
     */
    ItemStack removeBook();

    /**
     * @brief 检查是否有书
     * @return 如果有书返回true
     */
    [[nodiscard]] bool hasBook() const;

    /**
     * @brief 获取当前页码
     * @return 当前页码（从0开始）
     */
    [[nodiscard]] i32 getPage() const { return m_page; }

    /**
     * @brief 设置当前页码
     * @param page 页码
     */
    void setPage(i32 page);

    /**
     * @brief 获取总页数
     * @return 总页数，如果没有书返回0
     */
    [[nodiscard]] i32 getTotalPages() const;

    /**
     * @brief 翻到下一页
     * @return 如果成功翻页返回true
     */
    bool nextPage();

    /**
     * @brief 翻到上一页
     * @return 如果成功翻页返回true
     */
    bool prevPage();

    /**
     * @brief 获取红石比较器信号
     * @return 信号强度（基于当前页）
     */
    [[nodiscard]] i32 getComparatorSignal() const;

    // ========== 打开状态 ==========

    /**
     * @brief 获取打开计数
     * @return 当前打开的玩家数量
     */
    [[nodiscard]] i32 getOpenCount() const { return m_openCount; }

    /**
     * @brief 玩家打开讲台
     */
    void openContainer();

    /**
     * @brief 玩家关闭讲台
     */
    void closeContainer();

    // ========== Tick 更新 ==========

    void tick(IWorld& world) override;
    [[nodiscard]] bool needsTick() const noexcept override { return false; }

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;
    [[nodiscard]] std::unique_ptr<BlockEntity> clone() const override;

    /**
     * @brief 讲台方块实体的 NBT 仅允许 OP 玩家设置
     *
     * 参考 MC Java: BlockEntityType.OP_ONLY_CUSTOM_DATA 包含 LECTERN
     */
    [[nodiscard]] bool onlyOpsCanSetNbt() const noexcept override { return true; }

private:
    /**
     * @brief 检查物品是否是有效的书本
     * @param stack 物品
     * @return 如果是有效书本返回true
     */
    [[nodiscard]] static bool _isValidBook(const ItemStack& stack);

    /**
     * @brief 更新方块状态（HAS_BOOK属性）
     * @param world 世界引用
     */
    void _updateBlockState(IWorld& world);

    /**
     * @brief 翻页时触发红石脉冲
     *
     * 当页码发生变化时调用，通知讲台方块发出红石脉冲信号。
     * 脉冲持续 2 tick 后自动恢复为非供电状态。
     */
    void _signalPageChange();

    SimpleInventory m_inventory; ///< 1格物品存储
    i32 m_page = 0;              ///< 当前页码
    i32 m_openCount = 0;         ///< 打开计数
};

} // namespace blockentity
} // namespace mc
