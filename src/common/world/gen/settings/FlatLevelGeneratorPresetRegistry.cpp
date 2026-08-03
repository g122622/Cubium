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

#include "common/world/gen/settings/FlatLevelGeneratorPresetRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/gen/settings/FlatLevelGeneratorSettings.hpp"
#include <utility>

namespace mc::world::gen::settings {

FlatLevelGeneratorPresetRegistry& FlatLevelGeneratorPresetRegistry::instance()
{
    static FlatLevelGeneratorPresetRegistry registry;
    return registry;
}

void FlatLevelGeneratorPresetRegistry::registerPreset(
    const resource::ResourceLocation& name, FlatLevelGeneratorSettings settings)
{
    m_presets[name] = std::move(settings);
}

const FlatLevelGeneratorSettings* FlatLevelGeneratorPresetRegistry::get(const resource::ResourceLocation& name) const
{
    const auto it = m_presets.find(name);
    return it == m_presets.end() ? nullptr : &it->second;
}

bool FlatLevelGeneratorPresetRegistry::has(const resource::ResourceLocation& name) const
{
    return m_presets.find(name) != m_presets.end();
}

void FlatLevelGeneratorPresetRegistry::clear() noexcept
{
    m_presets.clear();
}

void FlatLevelGeneratorPresetRegistry::markLoadedFromDatapack(bool loaded) noexcept
{
    m_loadedFromDatapack = loaded;
}

bool FlatLevelGeneratorPresetRegistry::isLoadedFromDatapack() const noexcept
{
    return m_loadedFromDatapack;
}

} // namespace mc::world::gen::settings
