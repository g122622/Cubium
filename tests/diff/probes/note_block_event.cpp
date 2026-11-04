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

/**
 * @brief 差分能力：音符盒**响不响**与**发不发事件**（`NoteBlock#playNote`）。
 *
 * `list`：乐器 × 上方是否有方块
 * `run` ：一行 -> { "sounded": bool, "gameEvent": 事件 id | null }
 *
 * ## 关键的前置条件
 *
 * 音符盒不是任何情况都会响。原版 `NoteBlock#playNote`：
 *
 * <pre>
 *   if (乐器.worksAboveNoteBlock() || 上方方块是空气) {
 *       播放音符
 *       发出 GameEvent.NOTE_BLOCK_PLAY
 *   }
 * </pre>
 *
 * 也就是说：
 *
 * - **头颅类乐器**（`worksAboveNoteBlock` 为真）：不受上方方块影响，照响
 * - **其他乐器**：上方必须是空气 —— 被方块堵住的音符盒**一声不出、也不发事件**
 *
 * 游戏事件必须发在「让不让响」判定**之后**：被堵住的音符盒既不出声也不发事件。
 *
 * ## 抄错的后果
 *
 * 漏掉前置条件：上方埋了方块的音符盒照样发声，红石音乐里会因为「被盖住的音符盒仍然
 * 在响」而多出声音，且这些多出来的声音还会触发幽匿感测体。
 *
 * 事件发在判定之前：被堵住的音符盒不响却仍然报「播放了」，红石侦测端会误触发。
 */

#include <cstdio>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/item/Items.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/redstone/NoteBlock.hpp"
#include "common/world/block/registry/RedstoneBlocks.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
#include "common/world/gameevent/GameEvent.hpp"
#include "common/world/tick/manager/TickManager.hpp"

#include "vanilla_datapack.hpp"

#include "probe.hpp"

namespace mc::diff {
namespace {

using nlohmann::json;

void ensureInitialized()
{
    static const bool s_initialized = []() {
        datapack::initializeRegistries();
        Items::initialize();
        return true;
    }();
    (void)s_initialized;
}

constexpr const char* INSTRUMENT_NAMES[] = {"harp",
    "basedrum",
    "snare",
    "hat",
    "bass",
    "flute",
    "bell",
    "guitar",
    "chime",
    "xylophone",
    "iron_xylophone",
    "cow_bell",
    "didgeridoo",
    "bit",
    "banjo",
    "pling",
    "zombie",
    "skeleton",
    "creeper",
    "dragon",
    "wither_skeleton",
    "piglin",
    "custom_head"};
constexpr i32 INSTRUMENT_COUNT = static_cast<i32>(sizeof(INSTRUMENT_NAMES) / sizeof(INSTRUMENT_NAMES[0]));

/** 音符盒所在世界：音符盒在 (0,64,0)，上方格子可选被石头堵住。 */
class NoteSceneWorld final : public IWorld {
public:
    void setBlockedAbove(bool blocked) noexcept
    {
        m_blockedAbove = blocked;
    }

    void reset() noexcept
    {
        m_sounded = false;
        m_gameEvent.clear();
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        if (!isWithinWorldBounds(x, y, z)) {
            return &VanillaBlocks::AIR->defaultState();
        }
        if (x == 0 && y == 65 && z == 0) {
            return m_blockedAbove ? &VanillaBlocks::STONE->defaultState() : &VanillaBlocks::AIR->defaultState();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32, i32, i32, const BlockState*) override
    {
        return true;
    }

    [[nodiscard]] const fluid::FluidState* getFluidState(i32 x, i32 y, i32 z) const override
    {
        return getBlockState(x, y, z)->getFluidState();
    }

    [[nodiscard]] i32 getHeight(i32, i32) const override
    {
        return world::MIN_BUILD_HEIGHT;
    }

    [[nodiscard]] u8 getBlockLight(i32, i32, i32) const override
    {
        return 0;
    }

    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override
    {
        return 15;
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }

    void playSound(const ResourceLocation& /*soundEvent*/,
        sound::SoundCategory /*category*/,
        const Vector3& /*pos*/,
        f32 /*volume*/,
        f32 /*pitch*/) override
    {
        m_sounded = true;
    }

    void addParticle(particle::ParticleTypeId /*type*/, const Vector3& /*pos*/, const Vector3& /*velocity*/) override {}

    void gameEvent(const gameevent::GameEvent& event,
        const BlockPos& /*pos*/,
        const gameevent::GameEvent::Context& /*context*/) override
    {
        // Cubium 的事件 id 是裸路径，原版是完整标识符；补前缀让两侧表示法一致。
        m_gameEvent = std::string("minecraft:") + event.id();
    }

    [[nodiscard]] bool sounded() const noexcept
    {
        return m_sounded;
    }

    [[nodiscard]] const std::string& gameEvent() const noexcept
    {
        return m_gameEvent;
    }

    [[nodiscard]] bool hasBlockCollision(const AxisAlignedBB&) const override
    {
        return false;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getBlockCollisions(const AxisAlignedBB&) const override
    {
        return {};
    }

    [[nodiscard]] bool hasEntityCollision(const AxisAlignedBB&, const Entity*) const override
    {
        return false;
    }

    [[nodiscard]] std::vector<AxisAlignedBB> getEntityCollisions(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] PhysicsEngine* physicsEngine() override
    {
        return nullptr;
    }

    [[nodiscard]] const PhysicsEngine* physicsEngine() const override
    {
        return nullptr;
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInAABB(const AxisAlignedBB&, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] std::vector<Entity*> getEntitiesInRange(const Vector3&, f32, const Entity*) const override
    {
        return {};
    }

    [[nodiscard]] Entity* getEntity(EntityInstanceId) override
    {
        return nullptr;
    }

    [[nodiscard]] const Entity* getEntity(EntityInstanceId) const override
    {
        return nullptr;
    }

    [[nodiscard]] DimensionId dimension() const override
    {
        return 0;
    }

    [[nodiscard]] u64 seed() const override
    {
        return 0;
    }

    [[nodiscard]] u64 currentTick() const override
    {
        return 0;
    }

    [[nodiscard]] i64 dayTime() const override
    {
        return 6000;
    }

    [[nodiscard]] bool isClientSide() const override
    {
        return false;
    }

    [[nodiscard]] bool isHardcore() const override
    {
        return false;
    }

    [[nodiscard]] Difficulty difficulty() const override
    {
        return Difficulty::Normal;
    }

    [[nodiscard]] bool isRaining() const override
    {
        return false;
    }

    [[nodiscard]] bool isThundering() const override
    {
        return false;
    }

    [[nodiscard]] math::IRandom& getRandom() override
    {
        return m_random;
    }

    [[nodiscard]] const math::IRandom& getRandom() const override
    {
        return m_random;
    }

    [[nodiscard]] world::border::WorldBorder& worldBorder() override
    {
        return m_worldBorder;
    }

    [[nodiscard]] const world::border::WorldBorder& worldBorder() const override
    {
        return m_worldBorder;
    }

    [[nodiscard]] world::tick::TickManager& tickManager() override
    {
        throw std::logic_error("note_block_event 场景不推进刻度：播放音符不应调度计划刻");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::logic_error("note_block_event 场景不推进刻度：播放音符不应调度计划刻");
    }

private:
    bool m_blockedAbove = false;
    bool m_sounded = false;
    std::string m_gameEvent;
    math::Random m_random{0};
    world::border::WorldBorder m_worldBorder;
};

void writeList(FILE* out)
{
    ensureInitialized();
    for (i32 instrument = 0; instrument < INSTRUMENT_COUNT; ++instrument) {
        for (const char* blocked : {"false", "true"}) {
            std::fprintf(out, "%s:%s\n", INSTRUMENT_NAMES[instrument], blocked);
        }
    }
}

std::string runLine(std::string_view inputLine)
{
    ensureInitialized();
    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();

    const std::size_t colon = unit.find(':');
    if (colon == std::string::npos) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }
    const std::string instrumentName = unit.substr(0, colon);
    const bool blocked = unit.substr(colon + 1) == "true";

    i32 instrumentIndex = -1;
    for (i32 i = 0; i < INSTRUMENT_COUNT; ++i) {
        if (instrumentName == INSTRUMENT_NAMES[i]) {
            instrumentIndex = i;
            break;
        }
    }
    if (instrumentIndex < 0) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    Block* noteBlock = block_registry::RedstoneBlocks::NOTE_BLOCK;
    if (noteBlock == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    const auto instrument = static_cast<BlockStateProperties::NoteBlockInstrument>(instrumentIndex);
    const BlockState state = noteBlock->defaultState()
                                 .with(BlockStateProperties::NOTE_BLOCK_INSTRUMENT(), instrument)
                                 .with(BlockStateProperties::NOTE_0_24(), 12);

    NoteSceneWorld world;
    world.setBlockedAbove(blocked);

    // 这里只看**服务端是否触发播放**：原版 `playNote` 通过前置条件就发 blockEvent，
    // 具体在客户端响不响（例如头颅取不到自定义音效）由 `triggerEvent` 决定，不在本能力范围内。
    // 判据用 gameEvent —— 原版 playNote 通过前置条件后一定会发它。
    blocks::NoteBlock::triggerNote(world, BlockPos(0, 64, 0), state);

    const bool triggered = !world.gameEvent().empty();

    json result{{"unit", unit}};
    result["sounded"] = triggered;
    // 原版 Gson 会省略 null 字段，这里显式对齐：没触发就整个字段不出现。
    if (triggered) {
        result["gameEvent"] = world.gameEvent();
    }
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("note_block_event", &writeList, &runLine)

} // namespace mc::diff
