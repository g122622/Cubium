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

namespace mc {

// 前向声明
class IInventory;
class Player;
class ServerPlayer;
class IWorld;

namespace crafting {
template <typename C>
class IRecipe;
}

/**
 * @brief 配方持有者接口
 *
 * 实现此接口的背包可以追踪当前使用的配方。
 * 主要用于：
 * - 合成结果槽位追踪配方（用于解锁配方成就）
 * - 熔炉等方块实体追踪当前配方
 *
 * 参考: net.minecraft.inventory.IRecipeHolder
 */
class IRecipeHolder {
public:
    virtual ~IRecipeHolder() = default;

    /**
     * @brief 设置当前使用的配方
     * @param recipe 配方指针，nullptr表示清除
     */
    virtual void setRecipeUsed(const crafting::IRecipe<IInventory>* recipe) = 0;

    /**
     * @brief 获取当前使用的配方
     * @return 配方指针，如果没有返回nullptr
     */
    [[nodiscard]] virtual const crafting::IRecipe<IInventory>* getRecipeUsed() const = 0;

    /**
     * @brief 合成完成时调用
     * @param player 玩家
     *
     * 解锁配方成就并清除当前配方。
     * MC 1.16.5: 如果配方不是动态的，解锁配方并清除。
     */
    virtual void onCrafting(Player& player);

    /**
     * @brief 检查是否可以使用配方
     * @param world 世界
     * @param player 服务端玩家
     * @param recipe 要使用的配方
     * @return 如果可以使用返回true
     *
     * MC 1.16.5: 如果开启了有限合成且配方未解锁，返回false。
     * 否则设置当前配方并返回true。
     */
    [[nodiscard]] virtual bool canUseRecipe(
        IWorld& world, ServerPlayer& player, const crafting::IRecipe<IInventory>* recipe);
};

} // namespace mc
