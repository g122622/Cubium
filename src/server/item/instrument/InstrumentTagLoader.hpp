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

#include <cstddef>

namespace mc::resource {
class DataPackRepository;
class IResourcePack;
} // namespace mc::resource

namespace mc {
namespace server::item {

/**
 * @brief 乐器标签加载器
 *
 * 加载路径：`data/<namespace>/tags/instrument/<path>.json`，JSON 结构与其余数据包标签一致
 * （`replace` + `values`，支持 `#` 引用与 `{"id": ..., "required": false}` 对象写法）。
 *
 * 必须在 `item::instrument::initializeVanillaInstrumentTags()` 与 `InstrumentLoader` 之后调用：
 * 前者提供内置标签占位，后者保证标签引用的乐器定义已存在。
 */
class InstrumentTagLoader {
public:
    InstrumentTagLoader() = delete;

    /**
     * @brief 从数据包仓库加载全部乐器标签
     * @return 处理的标签数量，或错误
     */
    [[nodiscard]] static Result<size_t> loadFromDataPackRepository(const resource::DataPackRepository& dataPackList);

    /**
     * @brief 从单个资源包加载全部乐器标签
     */
    [[nodiscard]] static Result<size_t> loadFromResourcePack(const resource::IResourcePack& pack);
};

} // namespace server::item
} // namespace mc
