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

#include "common/item/instrument/InstrumentTag.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <functional>
#include <vector>

namespace mc {
namespace item::instrument {

/**
 * @brief 乐器标签注册表（对应 vanilla InstrumentTags）
 *
 * 单例注册表，管理全部乐器标签。内置标签占位在 initializeVanillaInstrumentTags 中登记，
 * 数据包 tags/instrument 目录中的标签经 InstrumentTagLoader 追加或替换成员。
 */
class InstrumentTags {
public:
    /**
     * @brief 普通山羊角标签（minecraft:regular_goat_horns）
     *
     * 普通山羊被剪角或死亡时掉落的乐器集合。
     */
    static InstrumentTag& REGULAR_GOAT_HORNS();

    /**
     * @brief 尖叫山羊角标签（minecraft:screaming_goat_horns）
     *
     * 尖叫山羊掉落的乐器集合。
     */
    static InstrumentTag& SCREAMING_GOAT_HORNS();

    /**
     * @brief 全部山羊角标签（minecraft:goat_horns）
     *
     * 引用上两者，供任意山羊角类判断使用。
     */
    static InstrumentTag& GOAT_HORNS();

    /**
     * @brief 获取单例注册表
     */
    static InstrumentTags& instance() noexcept;

    InstrumentTags(const InstrumentTags&) = delete;
    InstrumentTags& operator=(const InstrumentTags&) = delete;
    InstrumentTags(InstrumentTags&&) = delete;
    InstrumentTags& operator=(InstrumentTags&&) = delete;

    /**
     * @brief 登记标签（已存在则复用）
     * @param id 标签资源位置
     * @return 标签引用
     */
    InstrumentTag& registerTag(const ResourceLocation& id);

    /**
     * @brief 按资源位置查找标签
     */
    [[nodiscard]] const InstrumentTag* getTag(const ResourceLocation& id) const;

    /**
     * @brief 遍历全部标签
     */
    void forEachTag(const std::function<void(InstrumentTag&)>& callback);

    /**
     * @brief 清空全部标签（数据包重载前调用）
     */
    void clear();

    /**
     * @brief 条目数量
     */
    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_tags.size();
    }

private:
    InstrumentTags() = default;

    std::vector<InstrumentTag> m_tags;
};

/**
 * @brief 登记内置乐器标签占位（幂等）
 *
 * 标签成员留给数据包定义：vanilla 的 regular/screaming 成员列表来自数据包
 * tags/instrument 中的 JSON 文件，硬编码一份副本会与数据包内容分叉。
 */
void initializeVanillaInstrumentTags();

} // namespace item::instrument
} // namespace mc
