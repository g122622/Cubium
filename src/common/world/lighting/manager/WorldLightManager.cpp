#include "WorldLightManager.hpp"

#include "../../block/Block.hpp"
#include "../../block/VanillaBlocks.hpp"

#include <algorithm>
#include <array>
#include <queue>
#include <unordered_map>

namespace mc {

namespace {

struct SectionBuffer {
    std::vector<u8> sky;
    std::vector<u8> block;
    bool empty = true;

    SectionBuffer()
        : sky(NibbleArray::BYTE_SIZE, 0)
        , block(NibbleArray::BYTE_SIZE, 0)
    {
    }
};

struct ChunkState {
    explicit ChunkState(i32 sectionCount)
        : sections(static_cast<size_t>(sectionCount))
    {
    }

    bool enabled = false;
    bool retained = false;
    std::vector<SectionBuffer> sections;
};

struct LightOffset {
    i32 dx;
    i32 dy;
    i32 dz;
};

using LightChangedSection = std::pair<LightType, SectionPos>;

static constexpr std::array<LightOffset, 6> LIGHT_OFFSETS = {{
    {1, 0, 0},
    {-1, 0, 0},
    {0, 1, 0},
    {0, -1, 0},
    {0, 0, 1},
    {0, 0, -1},
}};

static i32 clampLight(i32 value)
{
    return std::clamp(value, 0, 15);
}

static i32 localIndex(i32 x, i32 y, i32 z)
{
    return NibbleArray::getIndex(x, y, z);
}

static u8 getNibbleValue(const std::vector<u8>& values, i32 index)
{
    if (values.empty()) {
        return 0;
    }

    const i32 packedIndex = index & 0xFFF;
    const size_t byteIndex = static_cast<size_t>(packedIndex >> 1);
    if (byteIndex >= values.size()) {
        return 0;
    }

    const u8 byte = values[byteIndex];
    if ((packedIndex & 1) == 0) {
        return static_cast<u8>(byte & 0x0F);
    }

    return static_cast<u8>((byte >> 4) & 0x0F);
}

static void setNibbleValue(std::vector<u8>& values, i32 index, i32 value)
{
    if (values.empty()) {
        values.assign(NibbleArray::BYTE_SIZE, 0);
    }

    const i32 packedIndex = index & 0xFFF;
    const size_t byteIndex = static_cast<size_t>(packedIndex >> 1);
    if (byteIndex >= values.size()) {
        return;
    }

    const u8 clamped = static_cast<u8>(clampLight(value));
    u8& byte = values[byteIndex];
    if ((packedIndex & 1) == 0) {
        byte = static_cast<u8>((byte & 0xF0) | clamped);
    } else {
        byte = static_cast<u8>((byte & 0x0F) | (clamped << 4));
    }
}

} // namespace

struct WorldLightManager::Impl {
    explicit Impl(IChunkLightProvider* provider, bool hasBlock, bool hasSky)
        : provider(provider)
        , hasBlockLight(hasBlock)
        , hasSkyLight(hasSky)
        , sectionCount(provider ? provider->getSectionCount() : world::CHUNK_SECTIONS)
    {
    }

    ChunkState& getOrCreateChunk(const ChunkPos& chunkPos)
    {
        auto [iter, inserted] = chunks.try_emplace(chunkPos, sectionCount);
        if (inserted) {
            iter->second.sections.resize(static_cast<size_t>(sectionCount));
        }
        return iter->second;
    }

    ChunkState* findChunk(const ChunkPos& chunkPos)
    {
        auto iter = chunks.find(chunkPos);
        if (iter == chunks.end()) {
            return nullptr;
        }
        return &iter->second;
    }

    const ChunkState* findChunk(const ChunkPos& chunkPos) const
    {
        auto iter = chunks.find(chunkPos);
        if (iter == chunks.end()) {
            return nullptr;
        }
        return &iter->second;
    }

    SectionBuffer* findSection(const SectionPos& sectionPos)
    {
        ChunkState* chunk = findChunk(sectionPos.chunkPos());
        if (chunk == nullptr) {
            return nullptr;
        }
        if (sectionPos.y < 0 || sectionPos.y >= static_cast<i32>(chunk->sections.size())) {
            return nullptr;
        }
        return &chunk->sections[static_cast<size_t>(sectionPos.y)];
    }

    const SectionBuffer* findSection(const SectionPos& sectionPos) const
    {
        const ChunkState* chunk = findChunk(sectionPos.chunkPos());
        if (chunk == nullptr) {
            return nullptr;
        }
        if (sectionPos.y < 0 || sectionPos.y >= static_cast<i32>(chunk->sections.size())) {
            return nullptr;
        }
        return &chunk->sections[static_cast<size_t>(sectionPos.y)];
    }

    SectionBuffer* findSection(const BlockPos& pos)
    {
        return findSection(SectionPos(pos));
    }

    const SectionBuffer* findSection(const BlockPos& pos) const
    {
        return findSection(SectionPos(pos));
    }

    [[nodiscard]] const IChunk* getChunk(const ChunkPos& chunkPos, std::unordered_map<ChunkPos, const IChunk*>& chunkCache) const
    {
        auto cacheIter = chunkCache.find(chunkPos);
        if (cacheIter != chunkCache.end()) {
            return cacheIter->second;
        }

        if (provider == nullptr) {
            return nullptr;
        }

        const IChunk* chunk = provider->getChunkForLight(chunkPos.x, chunkPos.z);
        chunkCache.emplace(chunkPos, chunk);
        return chunk;
    }

    [[nodiscard]] const BlockState* getBlockState(std::unordered_map<ChunkPos, const IChunk*>& chunkCache,
                                                  const BlockPos& pos) const
    {
        const IChunk* chunk = getChunk(ChunkPos(pos), chunkCache);
        if (chunk != nullptr) {
            const BlockCoord localX = math::toLocalCoord(pos.x);
            const BlockCoord localY = math::toLocalCoord(pos.y);
            const BlockCoord localZ = math::toLocalCoord(pos.z);
            const BlockState* state = chunk->getBlock(localX, localY, localZ);
            if (state != nullptr) {
                return state;
            }

            return &VanillaBlocks::AIR->defaultState();
        }

        if (provider == nullptr) {
            return nullptr;
        }

        return provider->getBlockStateForLight(pos);
    }

    static std::vector<u8>& bufferForType(SectionBuffer& buffer, LightType type)
    {
        return (type == LightType::SKY) ? buffer.sky : buffer.block;
    }

    static const std::vector<u8>& bufferForType(const SectionBuffer& buffer, LightType type)
    {
        return (type == LightType::SKY) ? buffer.sky : buffer.block;
    }

    static i32 sectionLocalY(i32 y)
    {
        return y & 15;
    }

    [[nodiscard]] bool isWithinBuildHeight(i32 y) const
    {
        return y >= provider->getMinBuildHeight() && y < provider->getMaxBuildHeight();
    }

    [[nodiscard]] i32 getOpacity(std::unordered_map<ChunkPos, const IChunk*>& chunkCache,
                                 const BlockPos& pos,
                                 IWorld* world) const
    {
        if (!isWithinBuildHeight(pos.y)) {
            return 0;
        }

        const BlockState* state = getBlockState(chunkCache, pos);
        if (state == nullptr) {
            return 0;
        }

        return std::clamp(state->getBlock().getOpacity(*state, world, &pos), 0, 15);
    }

    [[nodiscard]] i32 getEmission(std::unordered_map<ChunkPos, const IChunk*>& chunkCache,
                                  const BlockPos& pos,
                                  IWorld* world) const
    {
        if (!isWithinBuildHeight(pos.y)) {
            return 0;
        }

        const BlockState* state = getBlockState(chunkCache, pos);
        if (state == nullptr) {
            return 0;
        }

        return clampLight(static_cast<i32>(state->getBlock().getLightLevel(*state, world, &pos)));
    }

    [[nodiscard]] i32 getLightValue(const std::unordered_map<SectionPos, SectionBuffer>& buffers,
                                    LightType type, const BlockPos& pos) const
    {
        if (!isWithinBuildHeight(pos.y)) {
            return 0;
        }

        const SectionPos sectionPos(pos);
        auto iter = buffers.find(sectionPos);
        if (iter == buffers.end()) {
            return 0;
        }

        const SectionBuffer& buffer = iter->second;
        const std::vector<u8>& values = bufferForType(buffer, type);
        return static_cast<i32>(getNibbleValue(values, localIndex(pos.x, sectionLocalY(pos.y), pos.z)));
    }

    bool setLightValue(std::unordered_map<SectionPos, SectionBuffer>& buffers,
                       LightType type, const BlockPos& pos, i32 value)
    {
        if (!isWithinBuildHeight(pos.y)) {
            return false;
        }

        const SectionPos sectionPos(pos);
        auto iter = buffers.find(sectionPos);
        if (iter == buffers.end()) {
            return false;
        }

        SectionBuffer& buffer = iter->second;
        std::vector<u8>& values = bufferForType(buffer, type);
        const i32 index = localIndex(pos.x, sectionLocalY(pos.y), pos.z);
        const u8 clamped = static_cast<u8>(clampLight(value));
        if (clamped <= getNibbleValue(values, index)) {
            return false;
        }

        setNibbleValue(values, index, clamped);
        return true;
    }

    void seedSkyLighting(std::unordered_map<SectionPos, SectionBuffer>& buffers,
                         std::unordered_map<ChunkPos, const IChunk*>& chunkCache,
                         IWorld* world,
                         std::vector<BlockPos>& queue)
    {
        if (!hasSkyLight || provider == nullptr) {
            return;
        }

        for (const auto& [chunkPos, chunkState] : chunks) {
            if (!chunkState.enabled) {
                continue;
            }

            MC_UNUSED(getChunk(chunkPos, chunkCache));

            const i32 baseX = chunkPos.x * world::CHUNK_WIDTH;
            const i32 baseZ = chunkPos.z * world::CHUNK_WIDTH;

            for (i32 localX = 0; localX < world::CHUNK_WIDTH; ++localX) {
                for (i32 localZ = 0; localZ < world::CHUNK_WIDTH; ++localZ) {
                    i32 skyLight = 15;
                    for (i32 y = provider->getMaxBuildHeight() - 1; y >= provider->getMinBuildHeight(); --y) {
                        const BlockPos pos(baseX + localX, y, baseZ + localZ);
                        const i32 opacity = getOpacity(chunkCache, pos, world);
                        skyLight = clampLight(skyLight - opacity);

                        if (setLightValue(buffers, LightType::SKY, pos, skyLight) && skyLight > 0) {
                            queue.push_back(pos);
                        }

                        if (skyLight == 0 && opacity >= 15) {
                            break;
                        }
                    }
                }
            }
        }
    }

    void seedBlockLighting(std::unordered_map<SectionPos, SectionBuffer>& buffers,
                           std::unordered_map<ChunkPos, const IChunk*>& chunkCache,
                           IWorld* world,
                           std::vector<BlockPos>& queue)
    {
        if (!hasBlockLight || provider == nullptr) {
            return;
        }

        for (const auto& [chunkPos, chunkState] : chunks) {
            if (!chunkState.enabled) {
                continue;
            }

            MC_UNUSED(getChunk(chunkPos, chunkCache));

            const i32 baseX = chunkPos.x * world::CHUNK_WIDTH;
            const i32 baseZ = chunkPos.z * world::CHUNK_WIDTH;

            for (i32 sectionIndex = 0; sectionIndex < sectionCount; ++sectionIndex) {
                const SectionPos sectionPos(chunkPos.x, sectionIndex, chunkPos.z);
                auto iter = buffers.find(sectionPos);
                if (iter == buffers.end()) {
                    continue;
                }

                const SectionBuffer& sectionState = chunkState.sections[static_cast<size_t>(sectionIndex)];
                if (sectionState.empty) {
                    continue;
                }

                for (i32 localY = 0; localY < world::CHUNK_SECTION_HEIGHT; ++localY) {
                    const i32 worldY = sectionIndex * world::CHUNK_SECTION_HEIGHT + localY;
                    if (!isWithinBuildHeight(worldY)) {
                        continue;
                    }

                    for (i32 localZ = 0; localZ < world::CHUNK_WIDTH; ++localZ) {
                        for (i32 localX = 0; localX < world::CHUNK_WIDTH; ++localX) {
                            const BlockPos pos(baseX + localX, worldY, baseZ + localZ);
                            const i32 emission = getEmission(chunkCache, pos, world);
                            if (emission <= 0) {
                                continue;
                            }

                            if (setLightValue(buffers, LightType::BLOCK, pos, emission)) {
                                queue.push_back(pos);
                            }
                        }
                    }
                }
            }
        }
    }

    void propagateLight(std::unordered_map<SectionPos, SectionBuffer>& buffers,
                        std::unordered_map<ChunkPos, const IChunk*>& chunkCache,
                        IWorld* world,
                        std::vector<BlockPos>& queue,
                        LightType type)
    {
        size_t cursor = 0;
        while (cursor < queue.size()) {
            const BlockPos current = queue[cursor++];

            const i32 currentLevel = getLightValue(buffers, type, current);
            if (currentLevel <= 0) {
                continue;
            }

            for (const LightOffset& offset : LIGHT_OFFSETS) {
                const BlockPos neighbour(current.x + offset.dx,
                                         current.y + offset.dy,
                                         current.z + offset.dz);

                const SectionPos neighbourSection(neighbour);
                if (buffers.find(neighbourSection) == buffers.end()) {
                    continue;
                }

                const i32 attenuation = std::max(1, getOpacity(chunkCache, neighbour, world));
                const i32 nextLevel = currentLevel - attenuation;
                if (nextLevel <= 0) {
                    continue;
                }

                if (setLightValue(buffers, type, neighbour, nextLevel)) {
                    queue.push_back(neighbour);
                }
            }
        }
    }

    void recomputeLight(LightType type,
                        std::unordered_map<SectionPos, SectionBuffer>& buffers,
                        std::unordered_map<ChunkPos, const IChunk*>& chunkCache,
                        IWorld* world,
                        std::vector<LightChangedSection>& changedSections)
    {
        if (provider == nullptr) {
            return;
        }

        std::vector<BlockPos> queue;
        queue.reserve(buffers.size() * 32 + 256);
        if (type == LightType::SKY) {
            seedSkyLighting(buffers, chunkCache, world, queue);
        } else {
            seedBlockLighting(buffers, chunkCache, world, queue);
        }

        propagateLight(buffers, chunkCache, world, queue, type);

        for (auto& [chunkPos, chunkState] : chunks) {
            if (!chunkState.enabled) {
                continue;
            }

            for (i32 sectionIndex = 0; sectionIndex < sectionCount; ++sectionIndex) {
                const SectionPos sectionPos(chunkPos.x, sectionIndex, chunkPos.z);
                auto bufferIter = buffers.find(sectionPos);
                if (bufferIter == buffers.end()) {
                    continue;
                }

                SectionBuffer& computed = bufferIter->second;
                SectionBuffer& current = chunkState.sections[static_cast<size_t>(sectionIndex)];
                std::vector<u8>& currentValues = bufferForType(current, type);
                const std::vector<u8>& computedValues = bufferForType(computed, type);

                if (currentValues != computedValues) {
                    currentValues = computedValues;
                    changedSections.push_back({type, sectionPos});
                }
            }
        }
    }

    void cleanupChunk(const ChunkPos& chunkPos)
    {
        auto iter = chunks.find(chunkPos);
        if (iter == chunks.end()) {
            return;
        }

        if (!iter->second.enabled && !iter->second.retained) {
            chunks.erase(iter);
        }
    }

    IChunkLightProvider* provider = nullptr;
    bool hasBlockLight = false;
    bool hasSkyLight = false;
    i32 sectionCount = 0;
    std::unordered_map<ChunkPos, ChunkState> chunks;
    bool dirty = false;
};

WorldLightManager::WorldLightManager(IChunkLightProvider* provider, bool hasBlockLight, bool hasSkyLight)
    : m_impl(std::make_unique<Impl>(provider, hasBlockLight, hasSkyLight))
{
}

WorldLightManager::~WorldLightManager() = default;

WorldLightManager::WorldLightManager(WorldLightManager&&) noexcept = default;
WorldLightManager& WorldLightManager::operator=(WorldLightManager&&) noexcept = default;

bool WorldLightManager::hasBlockLight() const
{
    return m_impl && m_impl->hasBlockLight;
}

bool WorldLightManager::hasSkyLight() const
{
    return m_impl && m_impl->hasSkyLight;
}

void WorldLightManager::checkBlock(i32 x, i32 y, i32 z)
{
    if (!m_impl) {
        return;
    }

    (void)x;
    (void)y;
    (void)z;
    m_impl->dirty = true;
}

void WorldLightManager::onBlockEmissionIncrease(i32 x, i32 y, i32 z, i32 lightLevel)
{
    if (!m_impl || !m_impl->hasBlockLight) {
        return;
    }

    (void)x;
    (void)y;
    (void)z;
    (void)lightLevel;
    m_impl->dirty = true;
}

bool WorldLightManager::hasLightWork() const
{
    return m_impl != nullptr && m_impl->dirty;
}

i32 WorldLightManager::tick(i32 maxUpdates, bool updateSkyLight, bool updateBlockLight)
{
    if (!m_impl || !m_impl->provider) {
        return maxUpdates;
    }

    if (!m_impl->dirty) {
        return maxUpdates;
    }

    std::unordered_map<SectionPos, SectionBuffer> buffers;
    buffers.reserve(m_impl->chunks.size() * static_cast<size_t>(m_impl->sectionCount));

    std::unordered_map<ChunkPos, const IChunk*> chunkCache;
    chunkCache.reserve(m_impl->chunks.size());

    IWorld* world = m_impl->provider->getWorld();

    for (const auto& [chunkPos, chunkState] : m_impl->chunks) {
        if (!chunkState.enabled) {
            continue;
        }

        chunkCache.emplace(chunkPos, m_impl->provider->getChunkForLight(chunkPos.x, chunkPos.z));

        for (i32 sectionIndex = 0; sectionIndex < m_impl->sectionCount; ++sectionIndex) {
            const SectionPos sectionPos(chunkPos.x, sectionIndex, chunkPos.z);
            buffers.try_emplace(sectionPos);
        }
    }

    std::vector<LightChangedSection> changedSections;
    changedSections.reserve(buffers.size());

    if (updateSkyLight && m_impl->hasSkyLight) {
        m_impl->recomputeLight(LightType::SKY, buffers, chunkCache, world, changedSections);
    }

    if (updateBlockLight && m_impl->hasBlockLight) {
        m_impl->recomputeLight(LightType::BLOCK, buffers, chunkCache, world, changedSections);
    }

    for (const LightChangedSection& section : changedSections) {
        if (m_impl->provider != nullptr) {
            m_impl->provider->markLightChanged(section.first, section.second);
        }
    }

    m_impl->dirty = false;
    return 0;
}

void WorldLightManager::updateSectionStatus(const SectionPos& pos, bool isEmpty)
{
    if (!m_impl) {
        return;
    }

    ChunkState& chunk = m_impl->getOrCreateChunk(pos.chunkPos());
    if (pos.y < 0 || pos.y >= static_cast<i32>(chunk.sections.size())) {
        return;
    }

    SectionBuffer& section = chunk.sections[static_cast<size_t>(pos.y)];
    section.empty = isEmpty;
}

void WorldLightManager::enableLightSources(const ChunkPos& pos, bool enable)
{
    if (!m_impl) {
        return;
    }

    ChunkState& chunk = m_impl->getOrCreateChunk(pos);
    chunk.enabled = enable;
    if (!enable) {
        m_impl->cleanupChunk(pos);
    }
}

i32 WorldLightManager::getLightSubtracted(const BlockPos& pos, i32 skyDarkening) const
{
    const i32 skyLight = static_cast<i32>(getSkyLight(pos.x, pos.y, pos.z)) - skyDarkening;
    const i32 blockLight = static_cast<i32>(getBlockLight(pos.x, pos.y, pos.z));
    return std::max(0, std::max(skyLight, blockLight));
}

u8 WorldLightManager::getBlockLight(i32 x, i32 y, i32 z) const
{
    if (!m_impl) {
        return 0;
    }

    if (m_impl->provider == nullptr || !m_impl->hasBlockLight) {
        return 0;
    }

    const BlockPos pos(x, y, z);
    const SectionBuffer* section = m_impl->findSection(pos);
    if (section == nullptr) {
        return 0;
    }

    const std::vector<u8>& values = section->block;
    return getNibbleValue(values, localIndex(x, y & 15, z));
}

u8 WorldLightManager::getSkyLight(i32 x, i32 y, i32 z) const
{
    if (!m_impl) {
        return 15;
    }

    if (m_impl->provider == nullptr || !m_impl->hasSkyLight) {
        return 0;
    }

    const BlockPos pos(x, y, z);
    const SectionBuffer* section = m_impl->findSection(pos);
    if (section == nullptr) {
        return 15;
    }

    const std::vector<u8>& values = section->sky;
    if (values.empty()) {
        return 15;
    }

    return getNibbleValue(values, localIndex(x, y & 15, z));
}

void WorldLightManager::setData(LightType type, const SectionPos& pos, const NibbleArray& array, bool retain)
{
    if (!m_impl) {
        return;
    }

    ChunkState& chunk = m_impl->getOrCreateChunk(pos.chunkPos());
    if (pos.y < 0 || pos.y >= static_cast<i32>(chunk.sections.size())) {
        return;
    }

    SectionBuffer& section = chunk.sections[static_cast<size_t>(pos.y)];
    std::vector<u8>& values = Impl::bufferForType(section, type);
    values = array.data();
    section.empty = array.isEmpty();
    chunk.retained = chunk.retained || retain;
}

std::vector<u8> WorldLightManager::getData(LightType type, const SectionPos& pos) const
{
    if (!m_impl) {
        return {};
    }

    const SectionBuffer* section = m_impl->findSection(pos);
    if (section == nullptr) {
        return {};
    }

    return Impl::bufferForType(*section, type);
}

void WorldLightManager::retainData(const ChunkPos& pos, bool retain)
{
    if (!m_impl) {
        return;
    }

    ChunkState* chunk = m_impl->findChunk(pos);
    if (chunk == nullptr) {
        if (!retain) {
            return;
        }
        chunk = &m_impl->getOrCreateChunk(pos);
    }

    chunk->retained = retain;
    if (!retain && !chunk->enabled) {
        m_impl->cleanupChunk(pos);
    }
}

String WorldLightManager::getDebugInfo(LightType type, const SectionPos& pos) const
{
    if (!m_impl) {
        return "WorldLightManager(uninitialised)";
    }

    const SectionBuffer* section = m_impl->findSection(pos);
    if (section == nullptr) {
        return "WorldLightManager(section missing)";
    }

    const std::vector<u8>& values = Impl::bufferForType(*section, type);
    return "WorldLightManager(size=" + std::to_string(values.size()) + ")";
}

} // namespace mc