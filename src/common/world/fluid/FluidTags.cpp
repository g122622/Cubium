#include "FluidTags.hpp"
#include "Fluid.hpp"
#include "FluidRegistry.hpp"
#include <unordered_map>

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

std::unordered_map<ResourceLocation, std::unique_ptr<FluidTag>>& FluidTags::getTags()
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
        getTags()[ResourceLocation("minecraft:water")] = std::move(tag);
    }
    return *waterTag;
}

FluidTag& FluidTags::LAVA()
{
    static FluidTag* lavaTag = nullptr;
    if (lavaTag == nullptr) {
        auto tag = std::make_unique<FluidTag>(ResourceLocation("minecraft:lava"));
        lavaTag = tag.get();
        getTags()[ResourceLocation("minecraft:lava")] = std::move(tag);
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
    auto& tags = getTags();
    auto it = tags.find(id);
    return it != tags.end() ? it->second.get() : nullptr;
}

void FluidTags::forEachTag(std::function<void(FluidTag&)> callback)
{
    for (auto& [id, tag] : getTags()) {
        callback(*tag);
    }
}

} // namespace mc::fluid
