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

#include "IDispenseItemBehavior.hpp"
#include "common/item/core/Item.hpp"
#include "common/item/core/ItemStack.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

namespace mc {
namespace blocks {

/**
 * @brief 发射行为注册表
 *
 * 管理物品到发射行为的映射。
 * 发射器在发射物品时会查询此注册表获取对应的行为。
 *
 * ## 使用示例
 * ```cpp
 * // 注册弹射物发射行为（通过 ProjectileItem 接口自动获取配置）
 * DispenseItemBehaviorRegistry::instance().registerProjectileBehavior(*Items::ARROW);
 *
 * // 获取发射行为
 * IDispenseItemBehavior* behavior = DispenseItemBehaviorRegistry::getBehavior(stack);
 * if (behavior) {
 *     behavior->dispense(world, pos, state, stack, inventory);
 * } else {
 *     // 使用默认行为
 *     DefaultDispenseItemBehavior defaultBehavior;
 *     defaultBehavior.dispense(source, stack);
 * }
 * ```
 */
class DispenseItemBehaviorRegistry {
public:
    /**
     * @brief 获取单例实例
     * @return 注册表实例
     */
    static DispenseItemBehaviorRegistry& instance();

    /**
     * @brief 注册发射行为
     *
     * @param itemId 物品ID
     * @param behavior 发射行为（所有权转移）
     */
    void registerBehavior(const std::string& itemId, std::unique_ptr<IDispenseItemBehavior> behavior);

    /**
     * @brief 注册发射行为（模板版本）
     *
     * @tparam T 行为类型
     * @tparam Args 构造函数参数类型
     * @param itemId 物品ID
     * @param args 构造函数参数
     */
    template <typename T, typename... Args>
    void registerBehavior(const std::string& itemId, Args&&... args)
    {
        registerBehavior(itemId, std::make_unique<T>(std::forward<Args>(args)...));
    }

    /**
     * @brief 获取发射行为
     *
     * @param stack 物品堆
     * @return 发射行为指针，如果未注册则返回 nullptr
     */
    [[nodiscard]] IDispenseItemBehavior* getBehavior(const ItemStack& stack) const;

    /**
     * @brief 获取发射行为
     *
     * @param itemId 物品ID
     * @return 发射行为指针，如果未注册则返回 nullptr
     */
    [[nodiscard]] IDispenseItemBehavior* getBehavior(const std::string& itemId) const;

    /**
     * @brief 检查是否有注册的行为
     *
     * @param itemId 物品ID
     * @return true 如果已注册
     */
    [[nodiscard]] bool hasBehavior(const std::string& itemId) const;

    /**
     * @brief 获取默认发射行为
     *
     * 当物品没有注册特殊行为时使用。
     *
     * @return 默认发射行为
     */
    [[nodiscard]] IDispenseItemBehavior* getDefaultBehavior();

    /**
     * @brief 初始化默认发射行为
     *
     * 注册所有默认的发射行为（箭矢、雪球、鸡蛋等）。
     * 应在游戏启动时调用。
     */
    void initDefaultBehaviors();

    /**
     * @brief 为实现了 ProjectileItem 接口的物品注册弹射物发射行为
     *
     * 自动通过 dynamic_cast 检查物品是否实现了 ProjectileItem 接口，
     * 如果是则创建 ProjectileDispenseBehavior 并注册。
     * 参考 MC Java 的 DispenserBlock.registerProjectileBehavior()。
     *
     * @param item 物品引用
     */
    void registerProjectileBehavior(const Item& item);

private:
    DispenseItemBehaviorRegistry();

    /// 物品ID -> 发射行为映射
    std::unordered_map<std::string, std::unique_ptr<IDispenseItemBehavior>> m_behaviors;

    /// 默认发射行为
    std::unique_ptr<IDispenseItemBehavior> m_defaultBehavior;
};

/**
 * @brief 便捷函数：注册发射行为
 *
 * @param itemId 物品ID
 * @param behavior 发射行为
 */
inline void registerDispenseBehavior(const std::string& itemId, std::unique_ptr<IDispenseItemBehavior> behavior)
{
    DispenseItemBehaviorRegistry::instance().registerBehavior(itemId, std::move(behavior));
}

/**
 * @brief 便捷函数：注册发射行为（模板版本）
 *
 * @tparam T 行为类型
 * @tparam Args 构造函数参数类型
 * @param itemId 物品ID
 * @param args 构造函数参数
 */
template <typename T, typename... Args>
inline void registerDispenseBehavior(const std::string& itemId, Args&&... args)
{
    DispenseItemBehaviorRegistry::instance().registerBehavior<T>(itemId, std::forward<Args>(args)...);
}

} // namespace blocks
} // namespace mc
