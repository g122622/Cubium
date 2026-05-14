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

#include "IItemTier.hpp"
#include <memory>

namespace mc {
namespace item {
namespace tier {

/**
 * @brief 原版工具层级定义
 *
 * 提供原版 Minecraft 的六种材质层级：
 * - WOOD: 木制工具，低耐久低效率
 * - STONE: 石制工具，稍高耐久和效率
 * - IRON: 铁制工具，中等级别
 * - DIAMOND: 钻石工具，高耐久高效率
 * - GOLD: 金制工具，最高效率但耐久极低
 * - NETHERITE: 下界合金工具，最高耐久和效率
 *
 * | 层级      | 挖掘等级 | 耐久 | 效率  | 伤害 | 附魔值 |
 * |-----------|---------|------|-------|------|--------|
 * | WOOD      | 0       | 59   | 2.0   | 0.0  | 15     |
 * | STONE     | 1       | 131  | 4.0   | 1.0  | 5      |
 * | IRON      | 2       | 250  | 6.0   | 2.0  | 14     |
 * | DIAMOND   | 3       | 1561 | 8.0   | 3.0  | 10     |
 * | GOLD      | 0       | 32   | 12.0  | 0.0  | 22     |
 * | NETHERITE | 4       | 2031 | 9.0   | 4.0  | 15     |
 *
 * 参考: net.minecraft.item.ItemTier
 */
class ItemTiers {
public:
    /**
     * @brief 初始化所有层级
     *
     * 必须在 Items::initialize() 之后调用，
     * 因为修复材料需要引用已注册的物品。
     */
    static void initialize();

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] static bool isInitialized() { return s_initialized; }

    /** 木制工具层级 */
    [[nodiscard]] static const IItemTier& WOOD() { return *s_wood; }
    /** 石制工具层级 */
    [[nodiscard]] static const IItemTier& STONE() { return *s_stone; }
    /** 铁制工具层级 */
    [[nodiscard]] static const IItemTier& IRON() { return *s_iron; }
    /** 钻石工具层级 */
    [[nodiscard]] static const IItemTier& DIAMOND() { return *s_diamond; }
    /** 金制工具层级 */
    [[nodiscard]] static const IItemTier& GOLD() { return *s_gold; }
    /** 下界合金工具层级 */
    [[nodiscard]] static const IItemTier& NETHERITE() { return *s_netherite; }

private:
    static bool s_initialized;

    // 内部存储
    static std::unique_ptr<IItemTier> s_wood;
    static std::unique_ptr<IItemTier> s_stone;
    static std::unique_ptr<IItemTier> s_iron;
    static std::unique_ptr<IItemTier> s_diamond;
    static std::unique_ptr<IItemTier> s_gold;
    static std::unique_ptr<IItemTier> s_netherite;
};

} // namespace tier
} // namespace item
} // namespace mc
