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
 * @brief 差分能力：音符盒被触发时播放的**音效三元组**（`BlockStateBase` 的方块事件）。
 *
 * `list`：23 种乐器 × 3 个音符值（0 / 12 / 24）= 69 个单元
 * `run` ：一行 -> { "sound": 音效 id, "volume": f32 位模式, "pitch": f32 位模式,
 *                  "particle": 粒子类型名或 null, "particleDx": f32 位模式 }
 *
 * ## 只比 pitch 是不够的
 *
 * `noteblock_pitch` 已经比过 `2^((note-12)/12)` 这个公式本身。但公式对不代表
 * **播放链路**对——真正决定玩家听到什么的是方块事件里做的这几件事：
 *
 * 1. **声音事件**：乐器 -> 音效 id 的映射（23 条）。写错一个就是某个乐器的音色全错。
 * 2. **音量**：原版恒为 `3.0`。写成 1.0 会让音符盒音量掉到三分之一，整首曲子被
 *    伴奏盖住。
 * 3. **音高**：可调音乐器用 `getPitchFromNote(note)`；**头颅类（MOB_HEAD / CUSTOM）
 *    固定 1.0**，不听音符值。这条分支最容易漏：漏了之后调音对头颅类无效，
 *    但也不会报错。
 * 4. **音符粒子**：只有可调音乐器才发，位置是方块上方 `+1.2`，颜色偏移 `note/24`。
 *    漏掉或写成所有乐器都发，视觉上都错。
 *
 * 前三项任一项错，声音就不对；第 4 项错，视觉不对。所以必须整体比对。
 *
 * ## 为什么用方块事件而不是直接调播放函数
 *
 * 原版 `NoteBlock#triggerEvent` 是播放的**唯一入口**：`neighborChanged` 通过
 * `levelEvent` 广播事件，服务端不自己放声音，每个客户端收到事件后各自播放——
 * 这样附近所有玩家都能听到。直接调 `playNote` 只在发起方本地出声，多人时
 * 其他人听不到。本能力因此绑定在 `Block::triggerEvent` 上，也正是要验证
 * Cubium 的音符盒确实接到了这条链路上。
 *
 * f32 一律输出位模式，避免跨语言浮点序列化的伪差异。
 */

#include <algorithm>
#include <bit>
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
#include "common/world/IWorld.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/redstone/NoteBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/border/WorldBorder.hpp"
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

/**
 * @brief 只记录副作用的极简世界。
 *
 * 音符盒的播放路径只碰三样东西：读方块状态、`playSound`、`addParticle`。
 * 其余能力一律以安全的空实现应答——本能力不验证它们。
 */
class PlaybackWorld final : public IWorld {
public:
    struct PlayedSound {
        std::string id;
        f32 volume = 0.0f;
        f32 pitch = 0.0f;
        bool present = false;
    };

    struct SpawnedParticle {
        i32 type = -1;
        f32 dx = 0.0f;
        bool present = false;
    };

    void reset()
    {
        m_sound = {};
        m_particle = {};
    }

    // ========== 方块访问 ==========

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        if (!isWithinWorldBounds(x, y, z)) {
            return &VanillaBlocks::AIR->defaultState();
        }
        const auto found = m_cells.find(BlockPos(x, y, z));
        return found == m_cells.end() ? &VanillaBlocks::AIR->defaultState() : found->second;
    }

    bool setBlockState(i32 x, i32 y, i32 z, const BlockState* state) override
    {
        m_cells[BlockPos(x, y, z)] = state;
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

    [[nodiscard]] bool isWithinWorldBounds(i32 /*x*/, i32 y, i32 /*z*/) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
    }

    // ========== 副作用捕获 ==========

    void playSound(const ResourceLocation& soundEventId,
        sound::SoundCategory /*category*/,
        const Vector3& /*position*/,
        f32 volume,
        f32 pitch) override
    {
        m_sound.id = soundEventId.toString();
        m_sound.volume = volume;
        m_sound.pitch = pitch;
        m_sound.present = true;
    }

    void addParticle(particle::ParticleTypeId type, const Vector3& /*pos*/, const Vector3& velocity) override
    {
        m_particle.type = static_cast<i32>(type);
        m_particle.dx = velocity.x;
        m_particle.present = true;
    }

    [[nodiscard]] const PlayedSound& sound() const noexcept
    {
        return m_sound;
    }

    [[nodiscard]] const SpawnedParticle& particle() const noexcept
    {
        return m_particle;
    }

    // ========== 与音符盒播放无关的能力：一律安全空实现 ==========

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
        throw std::logic_error("noteblock_trigger 场景不推进刻度：播放音符不应调度计划刻");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::logic_error("noteblock_trigger 场景不推进刻度：播放音符不应调度计划刻");
    }

private:
    std::map<BlockPos, const BlockState*> m_cells;
    math::Random m_random{0};
    world::border::WorldBorder m_worldBorder;
    PlayedSound m_sound;
    SpawnedParticle m_particle;
};

/// 23 种乐器的原版名字，顺序即单元枚举顺序
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

constexpr i32 INSTRUMENT_COUNT = static_cast<i32>(std::size(INSTRUMENT_NAMES));
/// 采样三个有代表性的音高：最低、基准（pitch=1.0）、最高
constexpr i32 SAMPLE_NOTES[] = {0, 12, 24};

std::string f32Bits(f32 value)
{
    char buffer[16];
    std::snprintf(buffer, sizeof(buffer), "0x%08x", std::bit_cast<u32>(value));
    return std::string(buffer);
}

void writeList(FILE* out)
{
    ensureInitialized();
    for (i32 instrument = 0; instrument < INSTRUMENT_COUNT; ++instrument) {
        for (const i32 note : SAMPLE_NOTES) {
            std::fprintf(out, "%s:%d\n", INSTRUMENT_NAMES[instrument], note);
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
    const i32 note = std::stoi(unit.substr(colon + 1));

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

    const Block* noteBlock = VanillaBlocks::NOTE_BLOCK;
    if (noteBlock == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    // 组装目标状态：指定乐器与音符值
    BlockState state = noteBlock->defaultState()
                           .with(BlockStateProperties::NOTE_BLOCK_INSTRUMENT(),
                               static_cast<BlockStateProperties::NoteBlockInstrument>(instrumentIndex))
                           .with(BlockStateProperties::NOTE_0_24(), note);

    PlaybackWorld world;
    const BlockPos pos(0, 64, 0);
    world.setBlockState(pos.x, pos.y, pos.z, &state);

    // 原版 `levelEvent(null, 0, pos, 0)`：id 恒为 0
    const bool handled = noteBlock->triggerEvent(state, world, pos, 0, 0);

    const PlaybackWorld::PlayedSound& sound = world.sound();
    const PlaybackWorld::SpawnedParticle& particle = world.particle();

    json result{{"unit", unit}, {"handled", handled}, {"hasParticle", particle.present}};
    // 事件未被处理时（玩家头颅且取不到自定义音效）原版一声不出，也就不存在音量与音高，
    // 两侧都省略这三个字段。
    if (handled) {
        result["sound"] = sound.present ? json(sound.id) : json(nullptr);
        result["volume"] = f32Bits(sound.volume);
        result["pitch"] = f32Bits(sound.pitch);
    }
    if (particle.present) {
        // 粒子类型按**名字**输出：两侧枚举下标不同（原版 NOTE 是 0，Cubium 是 56），
        // 按下标比只会得到一堆伪差异。
        result["particleType"] = "note";
        result["particleDx"] = f32Bits(particle.dx);
    }
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(noteblock_trigger, &writeList, &runLine)

} // namespace mc::diff
