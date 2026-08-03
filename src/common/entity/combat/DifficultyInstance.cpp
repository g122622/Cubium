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

#include "DifficultyInstance.hpp"
#include "DifficultyHelper.hpp"
#include "common/core/Types.hpp"
#include "common/entity/core/DataParameter.hpp"
#include "common/entity/core/EntitySize.hpp"
#include "common/entity/core/EntityType.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldConstants.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include "common/world/lighting/InternalLightUtils.hpp"
#include <algorithm>

namespace mc {
namespace entity::combat {

DifficultyInstance::DifficultyInstance(
    Difficulty baseDifficulty, i64 worldTime, i64 chunkInhabitedTime, f32 moonPhaseFactor)
    : m_baseDifficulty(baseDifficulty)
    , m_effectiveDifficulty(calculateDifficulty(baseDifficulty, worldTime, chunkInhabitedTime, moonPhaseFactor))
{}

DifficultyInstance::DifficultyInstance(Difficulty baseDifficulty)
    : m_baseDifficulty(baseDifficulty)
    , m_effectiveDifficulty(DifficultyHelper::getRegionalDifficultyBase(baseDifficulty) *
          static_cast<f32>(static_cast<i32>(baseDifficulty)))
{
    // 简化构造：使用 DifficultyHelper 的基值乘以难度等级作为有效难度
    // 这与完整构造在游戏初期（worldTime=0, chunkInhabitedTime=0）时的结果一致
    // 适用于测试、命令生成等不需要位置感知的场景
}

DifficultyInstance DifficultyInstance::at(const IWorld& world, const BlockPos& pos)
{
    const i64 worldTime = static_cast<i64>(world.getGameTime());

    // 获取区块居住时间
    i64 chunkInhabitedTime = 0;
    const ChunkCoord chunkX = world::toChunkCoord(pos.x);
    const ChunkCoord chunkZ = world::toChunkCoord(pos.z);
    if (const auto* chunk = world.getChunk(chunkX, chunkZ)) {
        chunkInhabitedTime = chunk->inhabitedTime();
    }

    // 计算月相因子
    const i32 moonPhase = InternalLightUtils::getMoonPhase(world.dayTime());
    const f32 moonPhaseFactor = InternalLightUtils::getMoonBrightness(moonPhase);

    return DifficultyInstance(world.difficulty(), worldTime, chunkInhabitedTime, moonPhaseFactor);
}

f32 DifficultyInstance::getSpecialMultiplier() const
{
    if (m_effectiveDifficulty < 2.0f) {
        return 0.0f;
    }
    if (m_effectiveDifficulty > 4.0f) {
        return 1.0f;
    }
    return (m_effectiveDifficulty - 2.0f) / 2.0f;
}

bool DifficultyInstance::isHard() const
{
    return m_effectiveDifficulty >= 3.0f;
}

bool DifficultyInstance::isHarderThan(f32 threshold) const
{
    return m_effectiveDifficulty > threshold;
}

f32 DifficultyInstance::calculateDifficulty(
    Difficulty baseDifficulty, i64 worldTime, i64 chunkInhabitedTime, f32 moonPhaseFactor)
{
    if (baseDifficulty == Difficulty::Peaceful) {
        return 0.0f;
    }

    const bool isHard = (baseDifficulty == Difficulty::Hard);

    // 基础值 0.75
    f32 f = 0.75f;

    // 世界时间因子：随着世界运行时间增加，最多贡献 0.25
    f32 timeGlobalFactor =
        std::clamp(static_cast<f32>(static_cast<f64>(worldTime) + static_cast<f64>(DIFFICULTY_TIME_GLOBAL_OFFSET)) /
                MAX_DIFFICULTY_TIME_GLOBAL,
            0.0f,
            1.0f);
    f += timeGlobalFactor * 0.25f;

    // 区块居住时间因子：随着区块被玩家居住的时间增加
    f32 chunkFactor = std::clamp(static_cast<f32>(chunkInhabitedTime) / MAX_DIFFICULTY_TIME_LOCAL, 0.0f, 1.0f) *
        (isHard ? 1.0f : 0.75f);

    // 月相因子：加到区块因子上，但不超过 timeGlobalFactor
    f32 moonFactor = std::clamp(moonPhaseFactor * 0.25f, 0.0f, timeGlobalFactor);
    chunkFactor += moonFactor;

    // Easy 难度下区块因子减半
    if (baseDifficulty == Difficulty::Easy) {
        chunkFactor *= 0.5f;
    }

    f += chunkFactor;

    // 最终有效难度 = 难度等级 * f
    return static_cast<f32>(static_cast<i32>(baseDifficulty)) * f;
}

} // namespace entity::combat
} // namespace mc
