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
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc::fluid {

class Fluid;

/**
 * @brief 流体标签
 *
 * 用于标记具有相同特性的流体组。
 *
 * 用法示例:
 * @code
 * if (fluid.isIn(FluidTags::WATER())) {
 *     // 处理水相关逻辑
 * }
 * @endcode
 */
class FluidTag {
public:
    /**
     * @brief 构造流体标签
     *
     * @param id 标签资源位置
     */
    explicit FluidTag(const ResourceLocation& id)
        : m_id(id)
    {}

    /**
     * @brief 获取标签ID
     */
    [[nodiscard]] const ResourceLocation& id() const noexcept { return m_id; }

    /**
     * @brief 检查流体是否在此标签中
     *
     * @param fluid 要检查的流体
     * @return 是否在标签中
     */
    [[nodiscard]] bool contains(const Fluid& fluid) const;

    /**
     * @brief 添加流体到标签
     *
     * @param fluidId 流体资源位置
     */
    void add(const ResourceLocation& fluidId) { m_fluids.insert(fluidId); }

    /**
     * @brief 批量添加流体
     *
     * @param fluidIds 流体资源位置列表
     */
    void addAll(const std::vector<ResourceLocation>& fluidIds)
    {
        for (const auto& id : fluidIds) {
            m_fluids.insert(id);
        }
    }

    /**
     * @brief 获取标签中的所有流体ID
     */
    [[nodiscard]] const std::unordered_set<ResourceLocation>& fluids() const noexcept { return m_fluids; }

    /**
     * @brief 清空标签中的所有流体
     *
     * 用于数据包加载的 replace 语义：当数据包标签指定 replace=true 时，
     * 先清空已有标签内容（含内置默认值），再追加新内容。
     */
    void clear() { m_fluids.clear(); }

private:
    ResourceLocation m_id;
    std::unordered_set<ResourceLocation> m_fluids;
};

/**
 * @brief 内置流体标签集合
 */
class FluidTags {
public:
    /// 水标签（包含水和流动水）
    static FluidTag& WATER();

    /// 岩浆标签（包含岩浆和流动岩浆）
    static FluidTag& LAVA();

    /**
     * @brief 初始化所有内置标签
     *
     * 在 FluidRegistry::initialize() 之后调用
     */
    static void initialize();

    /**
     * @brief 根据ID获取标签
     *
     * @param id 标签资源位置
     * @return 标签指针，如果不存在返回 nullptr
     */
    [[nodiscard]] static FluidTag* getTag(const ResourceLocation& id);

    /**
     * @brief 注册或获取指定ID的标签
     *
     * 若标签已存在则直接返回；否则创建空标签并注册。
     * 供数据包标签加载器（FluidTagLoader）注册数据包定义的新标签。
     *
     * @param id 标签资源位置
     * @return 标签引用
     */
    static FluidTag& registerTag(const ResourceLocation& id);

    /**
     * @brief 遍历所有标签
     */
    static void forEachTag(std::function<void(FluidTag&)> callback);

private:
    FluidTags() = delete;

    static std::unordered_map<ResourceLocation, std::unique_ptr<FluidTag>>& _getTags();
    static bool s_initialized;
};

} // namespace mc::fluid
