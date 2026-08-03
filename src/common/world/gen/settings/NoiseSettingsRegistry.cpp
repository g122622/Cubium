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

#include "common/world/gen/settings/NoiseSettingsRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/world/gen/settings/DimensionSettings.hpp"
#include <utility>

namespace mc::world::gen::settings {

NoiseSettingsRegistry& NoiseSettingsRegistry::instance()
{
    static NoiseSettingsRegistry registry;
    return registry;
}

void NoiseSettingsRegistry::registerSettings(const resource::ResourceLocation& name, DimensionSettings settings)
{
    m_settings[name] = std::move(settings);
}

const DimensionSettings* NoiseSettingsRegistry::get(const resource::ResourceLocation& name) const
{
    const auto it = m_settings.find(name);
    return it == m_settings.end() ? nullptr : &it->second;
}

bool NoiseSettingsRegistry::has(const resource::ResourceLocation& name) const
{
    return m_settings.find(name) != m_settings.end();
}

void NoiseSettingsRegistry::clear() noexcept
{
    m_settings.clear();
}

void NoiseSettingsRegistry::markLoadedFromDatapack(bool loaded) noexcept
{
    m_loadedFromDatapack = loaded;
}

bool NoiseSettingsRegistry::isLoadedFromDatapack() const noexcept
{
    return m_loadedFromDatapack;
}

} // namespace mc::world::gen::settings
