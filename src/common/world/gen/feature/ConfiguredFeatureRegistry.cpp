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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OF OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ConfiguredFeatureRegistry.hpp"

namespace mc {

ConfiguredFeatureRegistry& ConfiguredFeatureRegistry::instance()
{
    static ConfiguredFeatureRegistry s_instance;
    return s_instance;
}

void ConfiguredFeatureRegistry::registerFeature(std::unique_ptr<ConfiguredFeatureBase> feature, ResourceLocation id)
{
    const ConfiguredFeatureBase* raw = feature.get();
    m_featuresById[id] = raw;
    m_ownedFeatures.push_back(std::move(feature));
}

const ConfiguredFeatureBase* ConfiguredFeatureRegistry::get(const ResourceLocation& id) const noexcept
{
    auto it = m_featuresById.find(id);
    return it != m_featuresById.end() ? it->second : nullptr;
}

bool ConfiguredFeatureRegistry::has(const ResourceLocation& id) const noexcept
{
    return m_featuresById.find(id) != m_featuresById.end();
}

void ConfiguredFeatureRegistry::clear()
{
    m_featuresById.clear();
    m_ownedFeatures.clear();
}

} // namespace mc
