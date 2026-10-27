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

#include "common/resource/ResourceLocation.hpp"

#include <cstddef>
#include <vector>

namespace mc {
namespace item::instrument {

/**
 * @brief 乐器资源名有序集合（对应 vanilla Instrument 标签）
 *
 * 成员以资源名有序存储，保证随机抽取乐器的结果只取决于数据包内容与随机种子，
 * 与数据包文件的遍历顺序无关。标签文件位于 data/<namespace>/tags/instrument/<path>.json。
 */
class InstrumentTag {
public:
    /**
     * @brief 构造乐器标签
     * @param id 标签资源位置
     * @param replace 是否替换已有内容（多数据包合并语义）
     */
    explicit InstrumentTag(ResourceLocation id, bool replace = false);

    /**
     * @brief 标签资源位置
     */
    [[nodiscard]] const ResourceLocation& id() const noexcept
    {
        return m_id;
    }

    /**
     * @brief 是否替换已有内容
     */
    [[nodiscard]] bool isReplace() const noexcept
    {
        return m_replace;
    }

    /**
     * @brief 设置替换标志
     */
    void setReplace(bool replace) noexcept
    {
        m_replace = replace;
    }

    /**
     * @brief 添加成员（重复添加无效）
     */
    void add(const ResourceLocation& instrumentId);

    /**
     * @brief 批量添加成员
     */
    void addAll(const std::vector<ResourceLocation>& instrumentIds);

    /**
     * @brief 是否包含指定乐器
     */
    [[nodiscard]] bool contains(const ResourceLocation& instrumentId) const;

    /**
     * @brief 清空成员
     */
    void clear();

    /**
     * @brief 成员列表（按资源名升序）
     */
    [[nodiscard]] const std::vector<ResourceLocation>& members() const noexcept
    {
        return m_members;
    }

    /**
     * @brief 成员数量
     */
    [[nodiscard]] std::size_t size() const noexcept
    {
        return m_members.size();
    }

private:
    ResourceLocation m_id;
    bool m_replace;
    std::vector<ResourceLocation> m_members;
};

} // namespace item::instrument
} // namespace mc
