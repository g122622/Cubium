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
#include "common/world/gen/feature/template/Template.hpp"
#include <memory>
#include <unordered_map>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

using feature::template_::StructureProcessorList;

/**
 * @brief 处理器列表注册表
 *
 * 按资源位置注册和查找 StructureProcessorList。
 * 模板池元素中的 processors 字段通过此注册表解析为实际的处理器列表。
 *
 * 使用方式：
 *   ProcessorListRegistry::instance().registerList(
 *       ResourceLocation("minecraft", "mossify_10_percent"), list);
 *   const auto* list = ProcessorListRegistry::instance().getList(
 *       ResourceLocation("minecraft", "mossify_10_percent"));
 */
class ProcessorListRegistry {
public:
    /**
     * @brief 获取单例实例
     */
    static ProcessorListRegistry& instance() noexcept;

    /**
     * @brief 注册处理器列表（移动语义）
     */
    void registerList(const ResourceLocation& id, std::unique_ptr<StructureProcessorList> list);

    /**
     * @brief 注册处理器列表（从现有列表克隆）
     */
    void registerList(const ResourceLocation& id, const StructureProcessorList& list);

    /**
     * @brief 按资源位置查找处理器列表
     * @return 处理器列表指针，未找到则返回 nullptr
     */
    [[nodiscard]] const StructureProcessorList* getList(const ResourceLocation& id) const noexcept;

    /**
     * @brief 检查是否已注册指定处理器列表
     */
    [[nodiscard]] bool hasList(const ResourceLocation& id) const noexcept;

    /**
     * @brief 清空所有注册的处理器列表
     */
    void clear() noexcept;

private:
    ProcessorListRegistry() = default;
    std::unordered_map<ResourceLocation, std::unique_ptr<StructureProcessorList>> m_lists;
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
