/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
 * LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO
 * EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "TemplatePool.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <unordered_map>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

/**
 * @brief 模板池注册表
 *
 * 单例注册表，按 ResourceLocation 管理 TemplatePool。
 * 对应 MC 1.21 的 StructureTemplatePool 注册表。
 */
class TemplatePoolRegistry {
public:
    static TemplatePoolRegistry& instance() noexcept;

    void registerPool(std::unique_ptr<TemplatePool> pool);
    const TemplatePool* getPool(const ResourceLocation& name) const noexcept;
    void clear() noexcept;

private:
    TemplatePoolRegistry() = default;
    std::unordered_map<ResourceLocation, std::unique_ptr<TemplatePool>> m_pools;
};

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
