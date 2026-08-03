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

#include "TemplatePoolRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/gen/jigsaw/TemplatePool.hpp"
#include <memory>
#include <utility>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

TemplatePoolRegistry& TemplatePoolRegistry::instance() noexcept
{
    static TemplatePoolRegistry registry;
    return registry;
}

void TemplatePoolRegistry::registerPool(std::unique_ptr<TemplatePool> pool)
{
    if (pool) {
        m_pools[pool->getName()] = std::move(pool);
    }
}

const TemplatePool* TemplatePoolRegistry::getPool(const ResourceLocation& name) const noexcept
{
    auto it = m_pools.find(name);
    return it != m_pools.end() ? it->second.get() : nullptr;
}

void TemplatePoolRegistry::clear() noexcept
{
    m_pools.clear();
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
