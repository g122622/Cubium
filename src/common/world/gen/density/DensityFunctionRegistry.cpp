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

#include "common/world/gen/density/DensityFunctionRegistry.hpp"

namespace mc::world::gen::density {

namespace {
std::unordered_map<ResourceLocation, std::shared_ptr<DensityFunction>>& registry()
{
    static std::unordered_map<ResourceLocation, std::shared_ptr<DensityFunction>> s_registry;
    return s_registry;
}

bool& loadedFromDatapack()
{
    static bool s_loaded = false;
    return s_loaded;
}
} // namespace

DensityFunctionRegistry& DensityFunctionRegistry::instance()
{
    static DensityFunctionRegistry s_instance;
    return s_instance;
}

void DensityFunctionRegistry::registerFunction(const ResourceLocation& name, std::shared_ptr<DensityFunction> function)
{
    registry()[name] = std::move(function);
}

std::shared_ptr<DensityFunction> DensityFunctionRegistry::get(const ResourceLocation& name) const
{
    const auto& r = registry();
    auto it = r.find(name);
    return it == r.end() ? nullptr : it->second;
}

bool DensityFunctionRegistry::has(const ResourceLocation& name) const
{
    return registry().contains(name);
}

void DensityFunctionRegistry::clear() noexcept
{
    registry().clear();
}

void DensityFunctionRegistry::markLoadedFromDatapack(bool loaded) noexcept
{
    loadedFromDatapack() = loaded;
}

bool DensityFunctionRegistry::isLoadedFromDatapack() const noexcept
{
    return loadedFromDatapack();
}

} // namespace mc::world::gen::density
