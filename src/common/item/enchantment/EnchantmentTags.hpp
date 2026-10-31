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

#include "common/core/Result.hpp"
#include "common/item/enchantment/EnchantmentTag.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>

namespace mc {

namespace resource {
class DataPackRepository;
}

/**
 * @brief 附魔标签注册表
 *
 * 承载 `data/minecraft/tags/enchantment/` 下的 15 个标签。
 *
 * 原版里这批标签承担了附魔的「获取途径」与「行为开关」两类语义：
 * 附魔台能不能刷出（`in_enchanting_table`）、能不能交易（`tradeable`）、
 * 战利品/生物装备/交易装备各自允许哪些（`on_random_loot` /
 * `on_mob_spawn_equipment` / `on_traded_equipment`）、某些方块行为是否被抑制
 * （`prevents_ice_melting` 等），以及提示框排序（`tooltip_order`，**它是有序的**）。
 *
 * 在此之前 Cubium 只用 `Enchantment::isTreasure()` / `isCurse()` 两个硬编码布尔值
 * 表达其中两个标签，其余 13 个没有对应物——等于把数据包里的 15 个标签压成了 2 个开关。
 *
 * TODO: `tooltip_order` 的**顺序**语义当前没有承载（成员集是无序 set）。若后续要用它
 * 排序提示框，需要额外保留一份有序数组。
 */
class EnchantmentTags {
public:
    /**
     * @brief 初始化注册表（创建全部标签对象，成员为空）
     *
     * 成员在 {@link loadFromDataPackRepository} 之后才有内容。
     */
    static void initialize();

    /**
     * @brief 检查是否已初始化
     */
    [[nodiscard]] static bool isInitialized() noexcept;

    /**
     * @brief 按 ID 查找标签
     * @return 标签指针，未找到返回 nullptr
     */
    [[nodiscard]] static EnchantmentTag* getTag(const ResourceLocation& id);

    /**
     * @brief 按 ID 注册标签（不存在则创建）
     */
    static EnchantmentTag& registerTag(const ResourceLocation& id);

    /**
     * @brief 遍历全部标签
     *
     * 内部是无序 map，调用方需要自行排序后再比较。
     */
    static void forEachTag(const std::function<void(EnchantmentTag&)>& callback);

    /**
     * @brief 从数据包加载全部附魔标签
     *
     * 走通用两阶段加载器：第一遍收集所有成员 id 与 `#` 标签引用，第二遍按依赖顺序
     * 递归展开（被引用的标签先填充）。原版这批标签普遍用引用写法，例如
     * `double_trade_price` 就是 `{"values": ["#minecraft:treasure"]}`。
     *
     * @return 处理成功的标签数量，或错误
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& dataPacks);

    /**
     * @brief 清空全部标签（测试用）
     */
    static void clearAll();

    /**
     * @brief 内部成员句柄表（供加载器的回调使用）
     *
     * 通用加载器用 `std::size_t` 作为成员句柄，这里把句柄映射回附魔 ID。
     * 公开它只是为了让实现文件里的回调 lambda 能取到，不属于业务接口。
     */
    [[nodiscard]] static std::vector<ResourceLocation>& _memberTable();

private:
    static std::unordered_map<ResourceLocation, std::unique_ptr<EnchantmentTag>>& _getTags();
};

} // namespace mc
