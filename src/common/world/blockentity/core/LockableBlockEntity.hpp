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

#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "world/blockentity/ContainerBlockEntity.hpp"
#include <string>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class Player;
class ItemStack;

namespace blockentity {

/**
 * @brief 可锁定容器方块实体基类
 *
 * 为容器方块实体提供锁定和自定义名称功能。
 * 锁定机制允许玩家通过重命名物品（钥匙）来保护容器。
 *
 * 功能:
 * - 锁定状态管理
 * - 自定义名称显示
 * - 钥匙匹配检测（物品显示名匹配）
 *
 * 子类:
 * - ChestEntity（箱子）
 * - HopperEntity（漏斗）
 * - AbstractFurnaceEntity（熔炉）
 */
class LockableBlockEntity : public ContainerBlockEntity {
public:
    /**
     * @brief 检查容器是否被锁定
     * @return 如果容器被锁定返回true
     */
    [[nodiscard]] bool isLocked() const noexcept { return m_locked; }

    /**
     * @brief 设置锁定状态
     * @param locked 锁定状态
     */
    void setLocked(bool locked)
    {
        if (m_locked != locked) {
            m_locked = locked;
            setChanged();
        }
    }

    /**
     * @brief 获取锁定钥匙名称
     * @return 钥匙名称（物品显示名）
     */
    [[nodiscard]] const std::string& getLockKey() const noexcept { return m_lockKey; }

    /**
     * @brief 设置锁定钥匙名称
     * @param key 钥匙名称
     */
    void setLockKey(const std::string& key)
    {
        if (m_lockKey != key) {
            m_lockKey = key;
            setChanged();
        }
    }

    /**
     * @brief 检查玩家是否可以打开容器
     * @param player 玩家（可为nullptr）
     * @param heldItem 手持物品（用于钥匙匹配）
     * @return 如果可以打开返回true
     *
     * 锁定规则:
     * 1. 未锁定的容器：所有人可打开
     * 2. 锁定的容器：需要手持正确名称的物品
     * 3. 创造模式玩家：可以打开任何容器
     */
    [[nodiscard]] virtual bool canOpen(const Player* player, const ItemStack& heldItem) const;

    // ========== 重写 BlockEntity 接口 ==========

    [[nodiscard]] std::string getCustomName() const noexcept override { return m_customName; }
    void setCustomName(const std::string& name) override;

    /**
     * @brief 获取显示名称
     * @return 如果有自定义名称返回自定义名称，否则返回默认名称
     */
    [[nodiscard]] virtual std::string getDisplayName() const;

    // ========== 序列化 ==========

    bool load(const nlohmann::json& data) override;
    void save(nlohmann::json& data) const override;

protected:
    /**
     * @brief 构造函数
     * @param type 方块实体类型
     * @param pos 方块位置
     */
    LockableBlockEntity(BlockEntityType type, const BlockPos& pos)
        : ContainerBlockEntity(type, pos)
        , m_locked(false)
    {}

    /**
     * @brief 获取默认显示名称（子类重写）
     * @return 默认名称（如"container.chest"、"container.furnace"等）
     */
    [[nodiscard]] virtual std::string getDefaultName() const = 0;

private:
    bool m_locked;            ///< 是否被锁定
    std::string m_lockKey;    ///< 锁定钥匙名称
    std::string m_customName; ///< 自定义名称
};

} // namespace blockentity
} // namespace mc
