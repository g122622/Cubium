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

#include "common/world/block/blocks/dirt/GrassBlock.hpp"

#include "common/core/Constants.hpp"
#include "common/core/Types.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/biome/Biome.hpp"
#include "common/world/biome/BiomeRegistry.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/capability/IGrowable.hpp"
#include "common/world/block/core/Block.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/chunk/data/ChunkData.hpp"
#include <algorithm>
#include <optional>
#include <utility>

namespace mc {
namespace blocks {

// ============================================================================
// GrassBlock 实现
// ============================================================================

GrassBlock::GrassBlock(BlockProperties properties)
    : SpreadableSnowyDirtBlock(std::move(properties))
{}

bool GrassBlock::canGrow(IBlockReader& world, const BlockPos& pos, const BlockState& state, bool isClientSide) const
{
    MC_UNUSED(state);
    MC_UNUSED(isClientSide);
    // 草方块上方需要有空气才能使用骨粉
    const BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    return aboveState != nullptr && aboveState->isAir();
}

bool GrassBlock::canUseBonemeal(
    IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state) const
{
    MC_UNUSED(world);
    MC_UNUSED(random);
    MC_UNUSED(pos);
    MC_UNUSED(state);
    // 草方块骨粉总是有效（只要 canGrow 返回 true）
    return true;
}

void GrassBlock::grow(IWorld& world, math::IRandom& random, const BlockPos& pos, const BlockState& state)
{
    MC_UNUSED(state);
    // 在草方块上方散布花朵和短草
    //
    // 128 次循环散布：每次随机偏移位置，若下方是草方块且当前位置是空气，
    // 1/8 概率从生物群系的花卉 placed_feature 列表中随机选取一项，解析出
    // ConfiguredFlowerFeature 后从其配置中随机选择花朵方块状态放置；
    // 7/8 概率放置短草。

    const BlockPos abovePos(pos.x, pos.y + 1, pos.z);

    // 128 次循环散布
    for (i32 i = 0; i < 128; ++i) {
        BlockPos currentPos = abovePos;

        // 随机偏移位置
        for (i32 j = 0; j < i / 16; ++j) {
            i32 dx = random.nextInt(3) - 1;                           // -1, 0, 1
            i32 dy = (random.nextInt(3) - 1) * random.nextInt(3) / 2; // 垂直偏移
            i32 dz = random.nextInt(3) - 1;                           // -1, 0, 1
            currentPos = BlockPos(currentPos.x + dx, currentPos.y + dy, currentPos.z + dz);
        }

        // 检查当前位置下方是否是草方块
        const BlockPos belowPos(currentPos.x, currentPos.y - 1, currentPos.z);
        const BlockState* belowState = world.getBlockState(belowPos);
        if (belowState == nullptr || !belowState->is(VanillaBlocks::GRASS_BLOCK)) {
            continue;
        }

        // 检查当前位置是否有碰撞（完整方块阻挡）
        const BlockState* currentState = world.getBlockState(currentPos);
        if (currentState == nullptr || currentState->isSolid()) {
            continue;
        }

        // 如果当前位置已经是短草，10% 概率再次催熟
        if (currentState != nullptr && currentState->is(VanillaBlocks::SHORT_GRASS)) {
            if (random.nextInt(10) == 0) {
                // 短草实现 IGrowable，检查是否可以催熟
                IGrowable* growable = const_cast<IGrowable*>(dynamic_cast<const IGrowable*>(&currentState->owner()));
                if (growable != nullptr &&
                    growable->canGrow(static_cast<IBlockReader&>(world), currentPos, *currentState, false)) {
                    growable->grow(world, random, currentPos, *currentState);
                }
            }
            continue;
        }

        // 如果当前位置是空气，放置植被
        if (currentState != nullptr && currentState->isAir()) {
            // 1/8 概率放置花朵：从生物群系获取花列表
            if (random.nextInt(8) == 0) {
                // 获取散布位置对应的生物群系
                const ChunkData* chunk =
                    world.chunkManager()->tryToGetChunkInMem(currentPos.chunkX(), currentPos.chunkZ());
                if (chunk != nullptr) {
                    const BiomeId biomeId =
                        chunk->getBiomeAtBlock(currentPos.localX(), currentPos.y, currentPos.localZ());
                    const Biome& biome = BiomeRegistry::instance().get(biomeId);
                    const auto& flowerIds = biome.generationSettings().getFlowerFeatureIds();

                    if (!flowerIds.empty() && m_flowerPlacer) {
                        // 数据驱动：花卉 id 是 placed_feature 的 ResourceLocation，
                        // 通过 FlowerPlacer 回调（server 侧注入）解析 placed_feature，
                        // 再取其内部 configured_feature（应为 ConfiguredFlowerFeature），
                        // 从配置中随机选择花朵方块状态放置。
                        const ResourceLocation& chosenId =
                            flowerIds[random.nextInt(static_cast<i32>(flowerIds.size()))];

                        const BlockState* flower = m_flowerPlacer(chosenId, random);
                        if (flower != nullptr) {
                            world.setBlockState(currentPos, flower, world::BlockUpdateFlags::UPDATE_ALL);
                            continue;
                        }
                    }
                }

                // 生物群系没有花卉特征或获取失败时，回退到默认花朵（蒲公英）
                if (VanillaBlocks::DANDELION != nullptr) {
                    world.setBlockState(
                        currentPos, &VanillaBlocks::DANDELION->defaultState(), world::BlockUpdateFlags::UPDATE_ALL);
                }
            } else {
                // 7/8 概率放置短草
                if (VanillaBlocks::SHORT_GRASS != nullptr) {
                    world.setBlockState(
                        currentPos, &VanillaBlocks::SHORT_GRASS->defaultState(), world::BlockUpdateFlags::UPDATE_ALL);
                }
            }
        }
    }
}

} // namespace blocks
} // namespace mc
