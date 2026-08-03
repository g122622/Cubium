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

#include "FluidTags.hpp"
#include "Fluid.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

namespace mc::fluid {

// ============================================================================
// FluidTag 实现
// ============================================================================

bool FluidTag::contains(const Fluid& fluid) const
{
    return m_fluids.find(fluid.fluidLocation()) != m_fluids.end();
}

// ============================================================================
// FluidTags 实现
// ============================================================================

bool FluidTags::s_initialized = false;

std::unordered_map<ResourceLocation, std::unique_ptr<FluidTag>>& FluidTags::_getTags()
{
    static std::unordered_map<ResourceLocation, std::unique_ptr<FluidTag>> tags;
    return tags;
}

FluidTag& FluidTags::WATER()
{
    static FluidTag* waterTag = nullptr;
    if (waterTag == nullptr) {
        auto tag = std::make_unique<FluidTag>(ResourceLocation("minecraft:water"));
        waterTag = tag.get();
        _getTags()[ResourceLocation("minecraft:water")] = std::move(tag);
    }
    return *waterTag;
}

FluidTag& FluidTags::LAVA()
{
    static FluidTag* lavaTag = nullptr;
    if (lavaTag == nullptr) {
        auto tag = std::make_unique<FluidTag>(ResourceLocation("minecraft:lava"));
        lavaTag = tag.get();
        _getTags()[ResourceLocation("minecraft:lava")] = std::move(tag);
    }
    return *lavaTag;
}

void FluidTags::initialize()
{
    if (s_initialized) {
        return;
    }

    // 确保标签已创建
    WATER();
    LAVA();

    // 添加流体到标签
    // 水标签包含：water, flowing_water
    WATER().addAll({ResourceLocation("minecraft:water"), ResourceLocation("minecraft:flowing_water")});

    // 岩浆标签包含：lava, flowing_lava
    LAVA().addAll({ResourceLocation("minecraft:lava"), ResourceLocation("minecraft:flowing_lava")});

    s_initialized = true;
}

FluidTag* FluidTags::getTag(const ResourceLocation& id)
{
    auto& tags = _getTags();
    auto it = tags.find(id);
    return it != tags.end() ? it->second.get() : nullptr;
}

void FluidTags::forEachTag(std::function<void(FluidTag&)> callback)
{
    for (auto& [id, tag] : _getTags()) {
        callback(*tag);
    }
}

} // namespace mc::fluid
