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

#include "Enchantment.hpp"
#include "common/core/Types.hpp"
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace mc {
namespace item {
namespace enchant {

/**
 * @brief 附魔注册表
 *
 * 管理所有已注册的附魔。支持：
 * - 按ID查找附魔
 * - 遍历所有附魔
 * - 初始化原版附魔
 *
 * 用法示例:
 * @code
 * // 初始化原版附魔
 * EnchantmentRegistry::initialize();
 *
 * // 获取附魔
 * const Enchantment* fortune = EnchantmentRegistry::get("minecraft:fortune");
 * if (fortune) {
 *     i32 maxLevel = fortune->maxLevel();
 * }
 *
 * // 遍历所有附魔
 * for (const auto& [id, enchantment] : EnchantmentRegistry::all()) {
 *     // ...
 * }
 * @endcode
 */
class EnchantmentRegistry {
public:
    /**
     * @brief 初始化原版附魔
     *
     * 注册所有 Minecraft 1.16.5 原版附魔。
     * 应在游戏启动时调用一次。
     */
    static void initialize();

    /**
     * @brief 注册附魔
     *
     * @param enchantment 附魔实例（注册表获得所有权）
     * @return 如果注册成功返回true
     */
    static bool registerEnchantment(std::unique_ptr<Enchantment> enchantment);

    /**
     * @brief 注册附魔（引用版本）
     *
     * @param enchantment 附魔实例（全局静态变量）
     * @return 如果注册成功返回true
     */
    static bool registerEnchantment(const Enchantment& enchantment);

    /**
     * @brief 按ID获取附魔
     *
     * @param id 附魔ID（如 "minecraft:fortune"）
     * @return 附魔指针，如果不存在返回nullptr
     */
    [[nodiscard]] static const Enchantment* get(const std::string& id);

    /**
     * @brief 检查附魔是否存在
     *
     * @param id 附魔ID
     * @return 如果存在返回true
     */
    [[nodiscard]] static bool has(const std::string& id);

    /**
     * @brief 获取所有已注册的附魔（仅包含所有权型）
     *
     * 注意：此方法仅返回注册表拥有所有权的附魔（通过 unique_ptr 注册），
     * 不包含通过引用注册的附魔。
     *
     * @return 附魔映射（ID -> 附魔）
     */
    [[nodiscard]] static const std::unordered_map<std::string, std::unique_ptr<Enchantment>>& all();

    /**
     * @brief 按类型获取附魔列表
     *
     * @param type 附魔类型
     * @return 该类型的所有附魔
     */
    [[nodiscard]] static std::vector<const Enchantment*> getByType(EnchantmentType type);

    /**
     * @brief 获取可用于指定物品类型的附魔列表
     *
     * @param itemType 物品类型
     * @return 可应用的附魔列表
     */
    [[nodiscard]] static std::vector<const Enchantment*> getAvailableForItem(u32 itemType);

    /**
     * @brief 清除所有注册的附魔
     *
     * 用于测试或重新初始化。
     */
    static void clear();

    /**
     * @brief 检查是否已初始化
     * @return 如果已初始化返回true
     */
    [[nodiscard]] static bool isInitialized();

private:
    /**
     * @brief 内部注册方法（不加锁）
     *
     * 注意：调用者必须持有 s_mutex 锁
     */
    static bool registerEnchantmentInternal(std::unique_ptr<Enchantment> enchantment);

    static std::unordered_map<std::string, std::unique_ptr<Enchantment>> s_enchantments;
    static bool s_initialized;
    // 使用递归互斥锁以支持 initialize() 内部批量注册时的同线程重入
    static std::recursive_mutex s_mutex;

    // 私有构造函数，防止实例化
    EnchantmentRegistry() = delete;
};

} // namespace enchant
} // namespace item
} // namespace mc
