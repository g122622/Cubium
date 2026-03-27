#pragma once

#include "IDispenseItemBehavior.hpp"
#include "../../../item/Item.hpp"
#include "../../../item/ItemStack.hpp"
#include <unordered_map>
#include <memory>

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
 * // 注册发射行为
 * DispenseItemBehaviorRegistry::registerBehavior(Items::ARROW,
 *     std::make_unique<ProjectileDispenseBehavior>(ProjectileType::Arrow));
 *
 * // 获取发射行为
 * IDispenseItemBehavior* behavior = DispenseItemBehaviorRegistry::getBehavior(stack);
 * if (behavior) {
 *     behavior->dispense(source, stack);
 * } else {
 *     // 使用默认行为
 *     DefaultDispenseItemBehavior defaultBehavior;
 *     defaultBehavior.dispense(source, stack);
 * }
 * ```
 *
 * 参考: net.minecraft.block.DispenserBlock.DISPENSE_BEHAVIOR_REGISTRY
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
    void registerBehavior(const String& itemId, std::unique_ptr<IDispenseItemBehavior> behavior);

    /**
     * @brief 注册发射行为（模板版本）
     *
     * @tparam T 行为类型
     * @tparam Args 构造函数参数类型
     * @param itemId 物品ID
     * @param args 构造函数参数
     */
    template<typename T, typename... Args>
    void registerBehavior(const String& itemId, Args&&... args) {
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
    [[nodiscard]] IDispenseItemBehavior* getBehavior(const String& itemId) const;

    /**
     * @brief 检查是否有注册的行为
     *
     * @param itemId 物品ID
     * @return true 如果已注册
     */
    [[nodiscard]] bool hasBehavior(const String& itemId) const;

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

private:
    DispenseItemBehaviorRegistry();

    /// 物品ID -> 发射行为映射
    std::unordered_map<String, std::unique_ptr<IDispenseItemBehavior>> m_behaviors;

    /// 默认发射行为
    std::unique_ptr<IDispenseItemBehavior> m_defaultBehavior;
};

/**
 * @brief 便捷函数：注册发射行为
 *
 * @param itemId 物品ID
 * @param behavior 发射行为
 */
inline void registerDispenseBehavior(const String& itemId, std::unique_ptr<IDispenseItemBehavior> behavior) {
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
template<typename T, typename... Args>
inline void registerDispenseBehavior(const String& itemId, Args&&... args) {
    DispenseItemBehaviorRegistry::instance().registerBehavior<T>(itemId, std::forward<Args>(args)...);
}

} // namespace blocks
} // namespace mc
