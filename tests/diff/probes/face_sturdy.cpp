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
 * @brief 差分能力：`isFaceSturdy` —— 方块的某个面能否提供支撑。
 *
 * **本探针调用 Cubium 的真实实现**
 * （`BlockState::isFaceSturdy(IWorld&, BlockPos, Direction, const SupportType&)`，
 * 见 `common/world/block/BlockState.cpp`）。
 *
 * ## 为什么这个能力重要
 *
 * `isFaceSturdy` 是**所有附着型方块存活判定的公共底层**：
 *
 * ```java
 * // 原版 AmethystClusterBlock
 * Direction direction = state.getValue(FACING);
 * BlockPos attached = pos.relative(direction.getOpposite());
 * return level.getBlockState(attached).isFaceSturdy(level, attached, direction);
 * ```
 *
 * 紫水晶簇、发光地衣、幽匿脉络、珊瑚、告示牌、火把、梯子 —— 它们的 `canSurvive`
 * 最终都落到这个方法上。
 *
 * 本能力不测 `canSurvive` 本身（那需要构造不同朝向的邻域，且涉及几十个方块的
 * `canPlace` 差异），只测**最底层的面朝支撑判定**，因此是纯计算、无歧义。
 *
 * ## 原版三种 SupportType
 *
 * ```java
 * public enum SupportType {
 *     FULL   { return Block.isFaceFull(state.getBlockSupportShape(level, pos), dir); },
 *     CENTER { return !Shapes.joinIsNotEmpty(faceShape, Block.column(2.0, 0.0, 10.0), ONLY_SECOND); },
 *     RIGID  { return !Shapes.joinIsNotEmpty(faceShape,
 *                   Shapes.join(Shapes.block(), Block.column(12.0, 0.0, 16.0), ONLY_FIRST), ONLY_SECOND); }
 * }
 * ```
 *
 * | 类型 | 要求 |
 * |---|---|
 * | `FULL` | 面投影**完全覆盖** 1×1 |
 * | `CENTER` | 面投影**包含**中心 2×2 像素柱 |
 * | `RIGID` | 面投影**包含**外环（1×1 去掉中心 12×12）|
 *
 * 三者对同一方块可以给出不同答案，例如：
 * - **完整方块**：`FULL=CENTER=RIGID=true`
 * - **台阶（下半）**：`FULL=false`（只占一半），但 `CENTER=true`、`RIGID=true`
 * - **地毯**：`FULL=false`、`CENTER=false`（太薄）、`RIGID=false`
 * - **玻璃板**：`FULL=false`、`CENTER=true`、`RIGID=false`
 *
 * ## 单元格式
 *
 * - `list`：`<block>|<Direction>|<SupportType>`，覆盖全部方块 × 6 方向 × 3 类型
 * - `run` ：输出 `"true"` / `"false"`
 *
 * 方向用 Java 常量名（`DOWN`/`UP`/`NORTH`/`SOUTH`/`WEST`/`EAST`）。
 * 支撑类型用 `FULL`/`CENTER`/`RIGID`。
 */
#include <algorithm>
#include <array>
#include <cstdio>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/Block.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/SupportType.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include "probe.hpp"
#include <typeinfo>

namespace mc::diff {
namespace {

using nlohmann::json;

void ensureBlocks()
{
    static bool s_initialized = false;
    if (!s_initialized) {
        VanillaBlocks::initialize();
        s_initialized = true;
    }
}

/**
 * @brief 最小只读世界：`isFaceSturdy` 只查自身方块，不会访问邻域。
 *
 * 若实现真的去查邻域，说明前提被打破——直接返回空气而不是抛异常，
 * 因为空气会导致判定为 false，差异会显现出来。
 */
class FaceWorld final : public IBlockReader {
public:
    void setBlock(const BlockPos& pos, const BlockState* state)
    {
        if (state == nullptr || state->isAir()) {
            m_cells.erase(pos);
            return;
        }
        m_cells[pos] = state;
    }

    [[nodiscard]] const BlockState* getBlockState(i32 x, i32 y, i32 z) const override
    {
        if (!isWithinWorldBounds(x, y, z)) {
            return &VanillaBlocks::AIR->defaultState();
        }
        const auto found = m_cells.find(BlockPos(x, y, z));
        return found == m_cells.end() ? &VanillaBlocks::AIR->defaultState() : found->second;
    }

    bool setBlockState(i32 /*x*/, i32 /*y*/, i32 /*z*/, const BlockState* /*state*/) override
    {
        throw std::logic_error("block_support 场景是只读的：支撑判定不应写入世界");
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
        return 15;
    }

    [[nodiscard]] u8 getSkyLight(i32, i32, i32) const override
    {
        return 15;
    }

    [[nodiscard]] bool isWithinWorldBounds(i32, i32 y, i32) const override
    {
        return y >= world::MIN_BUILD_HEIGHT && y < world::MAX_BUILD_HEIGHT;
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
        throw std::logic_error("block_support 不推进刻度：支撑判定不应调度计划刻");
    }

    [[nodiscard]] const world::tick::TickManager& tickManager() const override
    {
        throw std::logic_error("block_support 不推进刻度：支撑判定不应调度计划刻");
    }

private:
    std::map<BlockPos, const BlockState*> m_cells;
    math::Random m_random{0};
    world::border::WorldBorder m_worldBorder;
};

struct DirRef {
    const char* javaName;
    mc::Direction value;
};

constexpr DirRef DIRECTIONS[] = {
    {"DOWN", mc::Direction::Down},
    {"UP", mc::Direction::Up},
    {"NORTH", mc::Direction::North},
    {"SOUTH", mc::Direction::South},
    {"WEST", mc::Direction::West},
    {"EAST", mc::Direction::East},
};

struct SupportRef {
    const char* name;
    const mc::SupportType& (*accessor)();
};

constexpr SupportRef SUPPORT_TYPES[] = {
    {"FULL",
        []() -> const mc::SupportType& {
            return mc::SupportType::Full;
        }},
    {"CENTER",
        []() -> const mc::SupportType& {
            return mc::SupportType::Center;
        }},
    {"RIGID",
        []() -> const mc::SupportType& {
            return mc::SupportType::Rigid;
        }},
};

const DirRef* findDir(const std::string& name)
{
    for (const DirRef& entry : DIRECTIONS) {
        if (name == entry.javaName) {
            return &entry;
        }
    }
    return nullptr;
}

const SupportRef* findSupport(const std::string& name)
{
    for (const SupportRef& entry : SUPPORT_TYPES) {
        if (name == entry.name) {
            return &entry;
        }
    }
    return nullptr;
}

/// 按注册名取默认状态。
const BlockState* stateByName(const std::string& name)
{
    const Block* block = BlockRegistry::instance().getBlock(mc::ResourceLocation(name));
    if (block == nullptr) {
        return nullptr;
    }
    return &block->defaultState();
}

void writeList(FILE* out)
{
    ensureBlocks();
    std::vector<std::string> names;
    BlockRegistry::instance().forEachBlock([&](Block& block) {
        names.push_back(block.blockLocation().toString());
    });
    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    for (const auto& name : names) {
        for (const DirRef& dir : DIRECTIONS) {
            for (const SupportRef& support : SUPPORT_TYPES) {
                std::fprintf(out, "%s|%s|%s\n", name.c_str(), dir.javaName, support.name);
            }
        }
    }
}

std::vector<std::string> splitPipe(const std::string& text)
{
    std::vector<std::string> tokens;
    std::string current;
    for (char ch : text) {
        if (ch == '|') {
            tokens.push_back(current);
            current.clear();
        } else {
            current.push_back(ch);
        }
    }
    tokens.push_back(current);
    return tokens;
}

std::string runLine(std::string_view inputLine)
{
    ensureBlocks();

    const json input = json::parse(inputLine);
    const std::string unit = input.at("unit").get<std::string>();
    const auto parts = splitPipe(unit);
    if (parts.size() != 3) {
        throw std::runtime_error("face_sturdy: 单元应为 <block>|<dir>|<type>，实际：" + unit);
    }

    const BlockState* state = stateByName(parts[0]);
    if (state == nullptr) {
        throw std::runtime_error("face_sturdy: 未知方块：" + parts[0]);
    }
    const DirRef* dir = findDir(parts[1]);
    if (dir == nullptr) {
        throw std::runtime_error("face_sturdy: 未知方向：" + parts[1]);
    }
    const SupportRef* support = findSupport(parts[2]);
    if (support == nullptr) {
        throw std::runtime_error("face_sturdy: 未知支撑类型：" + parts[2]);
    }

    static FaceWorld world;
    const BlockPos pos(0, 0, 0);

    const bool result = state->isFaceSturdy(world, pos, dir->value, support->accessor());

    return json{{"unit", unit}, {"value", result ? "true" : "false"}}.dump();
}

} // namespace

MC_DIFF_DEFINE_PROBE("face_sturdy", &writeList, &runLine)

} // namespace mc::diff
