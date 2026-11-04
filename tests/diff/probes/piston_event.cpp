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
 * @brief 差分能力：活塞**缩回**时的音效与游戏事件（`PistonBaseBlock#move`）。
 *
 * `list`：活塞种类 × 朝向
 * `run` ：一行 -> { "sound": 音效 id, "volume": 音量, "gameEvent": 事件 id, "extended": 缩回后是否伸出 }
 *
 * ## 为什么活塞必须发声
 *
 * 活塞是红石装置里**动得最多**的元件。它每次动作除了改方块状态，还要：
 *
 * - 播放 `block.piston.contract`（缩回）或 `block.piston.extend`（伸出），音量 0.5
 * - 发出 `BLOCK_DEACTIVATE`（缩回）或 `BLOCK_ACTIVATE`（伸出）游戏事件
 *
 * 音高带随机（缩回 `random * 0.15 + 0.6`），让连续伸缩的活塞声不完全一致。
 *
 * ## 抄错的后果
 *
 * 漏掉音效：活塞**完全无声**，玩家判断不出装置有没有在工作。
 * 漏掉事件：活塞在幽匿电路里完全隐形，依赖它触发的侦测装置全部失灵。
 * 事件方向弄反（缩回发 ACTIVATE）：幽匿感测体的响应频率对不上，
 * 依赖「活塞动作」频率的电路行为会整体偏移。
 *
 * ## 本能力的范围
 *
 * 只覆盖**缩回**：伸出要走 PistonStructureHelper 计算推动链，需要在场景里搭出
 * 一整套方块；缩回在活塞前方为空时直接走完，不需要推动链。
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
#include "common/world/block/blocks/redstone/PistonBlock.hpp"
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

constexpr const char* PISTON_KINDS[] = {"piston", "sticky_piston"};
constexpr const char* FACINGS[] = {"north", "south", "west", "east", "up", "down"};

/** 活塞所在世界：活塞在 (0,64,0)，其余全空。随机数固定种子保证确定性。 */
class PistonSceneWorld final : public IWorld {
public:
    void reset()
    {
        m_sound.clear();
        m_gameEvent.clear();
        m_volume = 0.0f;
        m_random = math::Random(12345);
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        if (!isWithinWorldBounds(x, y, z)) {
            return &VanillaBlocks::AIR->defaultState();
        }
        return &VanillaBlocks::AIR->defaultState();
    }

    bool setBlockState(i32, i32, i32, const BlockState* /*state*/) override
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

    void playSound(const ResourceLocation& soundEvent,
        sound::SoundCategory /*category*/,
        const Vector3& /*pos*/,
        f32 volume,
        f32 /*pitch*/) override
    {
        m_sound = soundEvent.toString();
        m_volume = volume;
    }

    void gameEvent(const gameevent::GameEvent& event,
        const BlockPos& /*pos*/,
        const gameevent::GameEvent::Context& /*context*/) override
    {
        // Cubium 的事件 id 是裸路径，原版是完整标识符；补前缀对齐表示法。
        m_gameEvent = std::string("minecraft:") + event.id();
    }

    [[nodiscard]] const std::string& sound() const noexcept
    {
        return m_sound;
    }

    [[nodiscard]] f32 volume() const noexcept
    {
        return m_volume;
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
        throw std::logic_error("piston_event 场景不推进刻度：活塞缩回不应调度计划刻");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::logic_error("piston_event 场景不推进刻度：活塞缩回不应调度计划刻");
    }

private:
    std::string m_sound;
    std::string m_gameEvent;
    f32 m_volume = 0.0f;
    math::Random m_random{12345};
    world::border::WorldBorder m_worldBorder;
};

const char* directionName(Direction direction)
{
    switch (direction) {
        case Direction::Down:
            return "down";
        case Direction::Up:
            return "up";
        case Direction::North:
            return "north";
        case Direction::South:
            return "south";
        case Direction::West:
            return "west";
        case Direction::East:
            return "east";
        case Direction::None:
            break;
    }
    return "none";
}

void writeList(FILE* out)
{
    ensureInitialized();
    for (const char* kind : PISTON_KINDS) {
        for (const char* facing : FACINGS) {
            std::fprintf(out, "%s:%s\n", kind, facing);
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
    const std::string kind = unit.substr(0, colon);
    const std::string facingName = unit.substr(colon + 1);

    Block* piston = nullptr;
    if (kind == "piston") {
        piston = block_registry::RedstoneBlocks::PISTON;
    } else if (kind == "sticky_piston") {
        piston = block_registry::RedstoneBlocks::STICKY_PISTON;
    }
    if (piston == nullptr) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    Direction facing = Direction::None;
    for (const char* candidate : FACINGS) {
        if (facingName == candidate) {
            const auto parsed = Directions::fromName(candidate);
            if (parsed.has_value()) {
                facing = *parsed;
            }
            break;
        }
    }
    if (facing == Direction::None) {
        return json{{"unit", unit}, {"missing", true}}.dump();
    }

    // 缩回前的状态：已伸出
    const BlockState state = piston->defaultState()
                                 .with(BlockStateProperties::FACING(), facing)
                                 .with(BlockStateProperties::EXTENDED(), true);

    PistonSceneWorld world;
    world.reset();

    auto& pistonBlock = static_cast<blocks::PistonBlock&>(*piston);
    const bool ok = pistonBlock.retract(world, BlockPos(0, 64, 0), state);

    json result{{"unit", unit}};
    result["sound"] = world.sound().empty() ? json(nullptr) : json(world.sound());
    result["volume"] = world.volume();
    result["gameEvent"] = world.gameEvent().empty() ? json(nullptr) : json(world.gameEvent());
    result["retracted"] = ok;
    return result.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE(piston_event, &writeList, &runLine)

} // namespace mc::diff
