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

#include "ConfiguredCarverRegistry.hpp"
#include "common/world/gen/carver/WorldCarver.hpp"

namespace mc {

ConfiguredCarverRegistry& ConfiguredCarverRegistry::instance()
{
    static ConfiguredCarverRegistry s_instance;
    return s_instance;
}

void ConfiguredCarverRegistry::registerCarver(std::unique_ptr<ConfiguredCarverBase> carver, ResourceLocation id)
{
    const ConfiguredCarverBase* raw = carver.get();
    m_carversById[id] = raw;
    m_ownedCarvers.push_back(std::move(carver));
}

const ConfiguredCarverBase* ConfiguredCarverRegistry::get(const ResourceLocation& id) const noexcept
{
    auto it = m_carversById.find(id);
    return it != m_carversById.end() ? it->second : nullptr;
}

bool ConfiguredCarverRegistry::has(const ResourceLocation& id) const noexcept
{
    return m_carversById.find(id) != m_carversById.end();
}

void ConfiguredCarverRegistry::clear()
{
    m_carversById.clear();
    m_ownedCarvers.clear();
}

} // namespace mc
