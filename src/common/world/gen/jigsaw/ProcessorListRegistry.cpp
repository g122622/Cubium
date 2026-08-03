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

#include "ProcessorListRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <memory>
#include <utility>
#include <spdlog/spdlog.h>

namespace mc {
namespace world {
namespace gen {
namespace jigsaw {

ProcessorListRegistry& ProcessorListRegistry::instance() noexcept
{
    static ProcessorListRegistry instance;
    return instance;
}

void ProcessorListRegistry::registerList(const ResourceLocation& id, std::unique_ptr<StructureProcessorList> list)
{
    if (!list) {
        spdlog::warn("ProcessorListRegistry: attempted to register null list for '{}'", id.toString());
        return;
    }
    m_lists[id] = std::move(list);
}

void ProcessorListRegistry::registerList(const ResourceLocation& id, const StructureProcessorList& list)
{
    auto cloned = list.clone();
    if (!cloned) {
        spdlog::warn("ProcessorListRegistry: failed to clone list for '{}'", id.toString());
        return;
    }
    m_lists[id] = std::move(cloned);
}

const StructureProcessorList* ProcessorListRegistry::getList(const ResourceLocation& id) const noexcept
{
    auto it = m_lists.find(id);
    if (it != m_lists.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool ProcessorListRegistry::hasList(const ResourceLocation& id) const noexcept
{
    return m_lists.find(id) != m_lists.end();
}

void ProcessorListRegistry::clear() noexcept
{
    m_lists.clear();
}

} // namespace jigsaw
} // namespace gen
} // namespace world
} // namespace mc
