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

#include "common/item/instrument/InstrumentDefinition.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc {
namespace item::instrument {

/**
 * @brief 乐器注册表（对应 vanilla `Registries.INSTRUMENT`）
 *
 * 单例。内置条目在 `initialize()` 中按 vanilla 顺序登记（与握手阶段下发给客户端的
 * instrument 注册表条目顺序一致），数据包 `instrument/` 目录中的定义可覆盖同名条目。
 */
class InstrumentRegistry {
public:
    /**
     * @brief 获取单例
     */
    static InstrumentRegistry& instance() noexcept;

    /**
     * @brief 登记内置山羊角乐器（幂等）
     *
     * 顺序与 vanilla 一致：admire / call / dream / feel / ponder / seek / sing / yearn。
     * 若数据包已加载同名条目则不覆盖（数据包优先）。
     */
    void initialize();

    /**
     * @brief 清空所有条目（数据包重载前调用）
     */
    void clear();

    /**
     * @brief 注册或覆盖乐器
     * @param id 乐器资源位置
     * @param instrument 乐器定义
     */
    void registerInstrument(const ResourceLocation& id, std::unique_ptr<InstrumentDefinition> instrument);

    /**
     * @brief 按资源位置查找乐器
     */
    [[nodiscard]] const InstrumentDefinition* get(const ResourceLocation& id) const;

    /**
     * @brief 条目数量
     */
    [[nodiscard]] std::size_t size() const noexcept { return m_instruments.size(); }

    /**
     * @brief 按注册顺序列出全部条目
     */
    [[nodiscard]] std::vector<std::pair<ResourceLocation, const InstrumentDefinition*>> getAll() const;

private:
    InstrumentRegistry() = default;

    std::vector<std::pair<ResourceLocation, std::unique_ptr<InstrumentDefinition>>> m_instruments;
    std::unordered_map<ResourceLocation, std::size_t> m_indexById;
};

} // namespace item::instrument
} // namespace mc
