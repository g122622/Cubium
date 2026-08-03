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
#include "common/item/core/ItemStack.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/blockentity/BlockEntityType.hpp"
#include "entity/inventory/IInventory.hpp"
#include "util/assert/AssertMacros.hpp"
#include "world/blockentity/BlockEntity.hpp"
#include <cstddef>
#include <memory>
#include <string>
#include <nlohmann/json_fwd.hpp>

namespace mc {

class Player;

/**
 * @brief 容器方块实体基类
 *
 * 为有背包的方块实体提供通用功能。
 * 子类包括：箱子、漏斗、工作台、熔炉等。
 *
 * 功能：
 * - 背包管理
 * - 打开的玩家计数
 * - 自动保存触发
 */
class ContainerBlockEntity : public BlockEntity {
public:
    /**
     * @brief 获取容器
     * @return 容器指针，如果没有返回nullptr
     */
    [[nodiscard]] virtual IInventory* getInventory() { return nullptr; }
    [[nodiscard]] virtual const IInventory* getInventory() const { return nullptr; }

    /**
     * @brief 获取容器大小
     * @return 槽位数量
     */
    [[nodiscard]] virtual i32 getContainerSize() const { return 0; }

    /**
     * @brief 玩家打开容器
     *
     * 增加打开计数，用于音效和红石信号。
     * 观察者模式玩家不计入打开数。
     * @param player 打开容器的玩家（可为nullptr）
     */
    virtual void openContainer(Player* player);

    /**
     * @brief 玩家关闭容器
     *
     * 减少打开计数。
     * 观察者模式玩家不计入打开数。
     * @param player 关闭容器的玩家（可为nullptr）
     */
    virtual void closeContainer(Player* player);

    /**
     * @brief 获取打开容器的玩家数量
     * @return 打开计数
     */
    [[nodiscard]] i32 getOpenCount() const { return m_openCount; }

    /**
     * @brief 检查容器是否为空
     * @return 如果容器为空返回true
     */
    [[nodiscard]] virtual bool isEmpty() const
    {
        const IInventory* inv = getInventory();
        return inv ? inv->isEmpty() : true;
    }

    /**
     * @brief 清空容器
     *
     * 清除所有槽位的内容。
     */
    virtual void clearContainer()
    {
        IInventory* inv = getInventory();
        if (inv) {
            inv->clear();
        }
    }

    /**
     * @brief 检查玩家是否可以使用此容器
     * @param player 玩家
     * @param maxDistanceSq 最大距离的平方（默认64.0，即8格）
     * @return 如果玩家在范围内返回true
     *
     * 检查玩家与方块的距离是否在指定范围内。
     */
    [[nodiscard]] bool isUsableByPlayer(const Player& player, f32 maxDistanceSq = 64.0f) const;

    /**
     * @brief 保存数据到JSON
     * @param data 输出JSON数据
     */
    void save(nlohmann::json& data) const override
    {
        BlockEntity::save(data);

        // 保存背包内容
        const IInventory* inv = getInventory();
        if (inv) {
            nlohmann::json itemsJson = nlohmann::json::array();
            for (i32 i = 0; i < inv->getContainerSize(); ++i) {
                const ItemStack& stack = inv->getItem(i);
                if (!stack.isEmpty()) {
                    nlohmann::json itemJson = stack.toJson();
                    itemJson["Slot"] = i;
                    itemsJson.push_back(itemJson);
                }
            }
            data["Items"] = itemsJson;
        }

        // 保存自定义名称
        if (!getCustomName().empty()) {
            data["CustomName"] = getCustomName();
        }
    }

    /**
     * @brief 从JSON加载数据
     * @param data JSON数据
     * @return 是否成功
     */
    bool load(const nlohmann::json& data) override
    {
        if (!BlockEntity::load(data)) {
            return false;
        }

        // 加载自定义名称
        if (data.contains("CustomName") && data["CustomName"].is_string()) {
            setCustomName(data["CustomName"].get<std::string>());
        }

        // 加载背包内容
        IInventory* inv = getInventory();
        if (inv != nullptr && data.contains("Items") && data["Items"].is_array()) {
            inv->clear();

            const auto& items = data["Items"];
            for (std::size_t i = 0; i < items.size(); ++i) {
                const auto& itemJson = items[i];
                if (!itemJson.is_object() || itemJson.empty()) {
                    continue;
                }

                i32 slot = itemJson.value("Slot", -1);
                if (slot < 0 || slot >= inv->getContainerSize()) {
                    // 兼容早期仅按数组下标存储的格式
                    slot = static_cast<i32>(i);
                }

                if (slot < 0 || slot >= inv->getContainerSize()) {
                    continue;
                }

                auto stackResult = ItemStack::fromJson(itemJson);
                if (!stackResult.success()) {
                    continue;
                }

                inv->setItem(slot, stackResult.value());
            }
        }

        return true;
    }

protected:
    ContainerBlockEntity(BlockEntityType type, const BlockPos& pos)
        : BlockEntity(type, pos)
        , m_openCount(0)
    {}

    i32 m_openCount;
};

} // namespace mc
