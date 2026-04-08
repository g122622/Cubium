#pragma once

#include "common/core/Types.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/VanillaBlocks.hpp"
#include "common/world/chunk/ChunkPos.hpp"
#include "common/world/chunk/IChunk.hpp"
#include "common/world/lighting/IChunkLightProvider.hpp"
#include "common/world/lighting/LightType.hpp"

#include <unordered_map>
#include <vector>

namespace lighting_test {

inline void ensureVanillaBlocksInitialized()
{
    static bool initialized = false;
    if (!initialized) {
        mc::VanillaBlocks::initialize();
        initialized = true;
    }
}

struct ChangedSection {
    mc::LightType type;
    mc::SectionPos pos;
};

class TestLightingProvider final : public mc::IChunkLightProvider {
public:
    TestLightingProvider(mc::i32 minBuildHeight,
                         mc::i32 maxBuildHeight,
                         mc::i32 sectionCount,
                         bool hasSkyLight)
        : m_minBuildHeight(minBuildHeight)
        , m_maxBuildHeight(maxBuildHeight)
        , m_sectionCount(sectionCount)
        , m_hasSkyLight(hasSkyLight)
    {
        ensureVanillaBlocksInitialized();
    }

    void setBlock(const mc::BlockPos& pos, const mc::BlockState* state)
    {
        m_blocks[pos] = state;
    }

    void setBlock(mc::i32 x, mc::i32 y, mc::i32 z, const mc::BlockState* state)
    {
        setBlock(mc::BlockPos(x, y, z), state);
    }

    void clearChangedSections()
    {
        m_changedSections.clear();
    }

    [[nodiscard]] const std::vector<ChangedSection>& changedSections() const
    {
        return m_changedSections;
    }

    [[nodiscard]] bool hasChangedSection(mc::LightType type, const mc::SectionPos& pos) const
    {
        for (const auto& section : m_changedSections) {
            if (section.type == type && section.pos == pos) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] mc::IChunk* getChunkForLight(mc::ChunkCoord, mc::ChunkCoord) override
    {
        return nullptr;
    }

    [[nodiscard]] const mc::IChunk* getChunkForLight(mc::ChunkCoord, mc::ChunkCoord) const override
    {
        return nullptr;
    }

    [[nodiscard]] const mc::BlockState* getBlockStateForLight(const mc::BlockPos& pos) const override
    {
        const auto iter = m_blocks.find(pos);
        if (iter != m_blocks.end()) {
            return iter->second;
        }

        return &mc::VanillaBlocks::AIR->defaultState();
    }

    [[nodiscard]] mc::IWorld* getWorld() override
    {
        return nullptr;
    }

    [[nodiscard]] const mc::IWorld* getWorld() const override
    {
        return nullptr;
    }

    void markLightChanged(mc::LightType type, const mc::SectionPos& pos) override
    {
        m_changedSections.push_back(ChangedSection{type, pos});
    }

    [[nodiscard]] bool hasSkyLight() const override
    {
        return m_hasSkyLight;
    }

    [[nodiscard]] mc::i32 getMinBuildHeight() const override
    {
        return m_minBuildHeight;
    }

    [[nodiscard]] mc::i32 getMaxBuildHeight() const override
    {
        return m_maxBuildHeight;
    }

    [[nodiscard]] mc::i32 getSectionCount() const override
    {
        return m_sectionCount;
    }

private:
    std::unordered_map<mc::BlockPos, const mc::BlockState*> m_blocks;
    std::vector<ChangedSection> m_changedSections;
    mc::i32 m_minBuildHeight;
    mc::i32 m_maxBuildHeight;
    mc::i32 m_sectionCount;
    bool m_hasSkyLight;
};

} // namespace lighting_test