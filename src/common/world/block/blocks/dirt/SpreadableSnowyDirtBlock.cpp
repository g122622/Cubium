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

#include "SpreadableSnowyDirtBlock.hpp"
#include "../../../../core/Constants.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/IRandom.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../IWorld.hpp"
#include "../../../biome/Biome.hpp"
#include "../../../biome/BiomeGenerationSettings.hpp"
#include "../../../biome/BiomeIds.hpp"
#include "../../../biome/BiomeRegistry.hpp"
#include "../../../chunk/data/ChunkData.hpp"
#include "../../../fluid/Fluid.hpp"
#include "../../../gen/feature/vegetation/FlowerFeature.hpp"
#include "../../../gen/placement/PlacedFeature.hpp"
#include "../../../gen/placement/PlacedFeatureRegistry.hpp"
#include "../../../lighting/engine/LightEngineUtils.hpp"
#include "../../registry/VanillaBlocks.hpp"
#include "../../registry/VegetationBlocks.hpp"
#include "../ice/SnowBlock.hpp"
#include "common/particle/ParticleTypes.hpp"
#include "common/resource/ResourceLocation.hpp"

namespace mc::blocks {

// ============================================================================
// SpreadableSnowyDirtBlock 实现
// ============================================================================

SpreadableSnowyDirtBlock::SpreadableSnowyDirtBlock(BlockProperties properties)
    : SnowyDirtBlock(std::move(properties))
{
    // SNOWY 属性、默认状态、放置/邻居更新同步由基类 SnowyDirtBlock 负责，
    // 此处仅保留蔓延/退化所需的状态。
}

void SpreadableSnowyDirtBlock::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 检查是否满足蔓延条件
    if (!isSnowyConditions(world, pos, state)) {
        // 不满足条件，退化成泥土
        world.setBlockState(pos, &VanillaBlocks::DIRT->defaultState());
    } else {
        // 满足条件，尝试向周围蔓延
        // 需要 pos.up() 的光照 >= 9
        const u8 skyLight = world.getSkyLight(pos.x, pos.y + 1, pos.z);
        const u8 blockLight = world.getBlockLight(pos.x, pos.y + 1, pos.z);
        const u8 lightLevel = std::max(skyLight, blockLight);

        if (lightLevel >= game::GRASS_SPREAD_LIGHT_THRESHOLD) {
            const BlockState* defaultState = &getDefaultState();

            // 尝试向4个随机位置的泥土蔓延
            for (i32 i = 0; i < 4; ++i) {
                const i32 dx = random.nextInt(3) - 1; // -1, 0, 1
                const i32 dy = random.nextInt(5) - 3; // -3, -2, -1, 0, 1
                const i32 dz = random.nextInt(3) - 1; // -1, 0, 1

                const BlockPos targetPos(pos.x + dx, pos.y + dy, pos.z + dz);

                // 检查目标位置是否为泥土
                const BlockState* targetState = world.getBlockState(targetPos);
                if (targetState == nullptr || targetState->blockId() != VanillaBlocks::DIRT->blockId()) {
                    continue;
                }

                // 检查目标位置是否满足蔓延条件
                if (isSnowyAndNotUnderwater(world, targetPos, *defaultState)) {
                    // 检查目标位置上方是否有雪
                    // 蔓延时只检查 SNOW（雪层），不检查 SNOW_BLOCK（雪块）
                    const BlockPos abovePos(targetPos.x, targetPos.y + 1, targetPos.z);
                    const BlockState* aboveState = world.getBlockState(abovePos);
                    const bool hasSnow = aboveState != nullptr && aboveState->is(VanillaBlocks::SNOW);

                    // 设置新方块状态，包含 SNOWY 属性
                    const BlockState* newState = &defaultState->with(SNOWY(), hasSnow);
                    world.setBlockState(targetPos, newState);
                }
            }
        }
    }
}

bool SpreadableSnowyDirtBlock::isSnowyConditions(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    const BlockPos abovePos = pos.up();
    const BlockState* aboveState = world.getBlockState(abovePos);

    // 检查是否为雪层且层数为1
    // 只有1层雪时满足条件
    if (aboveState != nullptr && aboveState->is(VanillaBlocks::SNOW)) {
        // 检查 LAYERS 属性是否为 1
        // 使用 getOptional 安全获取，因为 SNOWY 状态会在这里被检查
        const std::optional<i32> layers = aboveState->getOptional(SnowBlock::LAYERS());
        if (layers.has_value() && layers.value() == 1) {
            return true;
        }
        // 多层雪会进入下面的光照检查逻辑
    }

    // 检查上方是否有完整水源
    if (aboveState != nullptr) {
        const fluid::FluidState* fluidState = aboveState->getFluidState();
        if (fluidState != nullptr && !fluidState->isEmpty() && fluidState->getLevel() == fluid::SOURCE_LEVEL) {
            return false; // 上方有完整水源，不满足条件
        }
    }

    // 结合上方方块的不透明度与面遮挡形状，判断是否达到满阻挡
    static const BlockState* s_airState = &VanillaBlocks::AIR->defaultState();
    const BlockState& resolvedAboveState = aboveState != nullptr ? *aboveState : *s_airState;
    const i32 lightBlockInto = LightEngineUtils::getLightBlockInto(
        world, state, pos, resolvedAboveState, abovePos, Direction::Up, resolvedAboveState.getOpacity());
    return lightBlockInto < game::MAX_LIGHT_LEVEL;
}

bool SpreadableSnowyDirtBlock::isSnowyAndNotUnderwater(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    if (!isSnowyConditions(world, pos, state)) {
        return false;
    }

    // 检查上方是否有水
    const BlockPos abovePos(pos.x, pos.y + 1, pos.z);
    const BlockState* aboveState = world.getBlockState(abovePos);
    if (aboveState == nullptr) {
        return true;
    }

    const fluid::FluidState* fluidState = aboveState->getFluidState();
    if (fluidState != nullptr && !fluidState->isEmpty()) {
        return false; // 上方有流体
    }

    return true;
}

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
                const ChunkData* chunk = world.getChunk(currentPos.chunkX(), currentPos.chunkZ());
                if (chunk != nullptr) {
                    const BiomeId biomeId =
                        chunk->getBiomeAtBlock(currentPos.localX(), currentPos.y, currentPos.localZ());
                    const Biome& biome = BiomeRegistry::instance().get(biomeId);
                    const auto& flowerIds = biome.generationSettings().getFlowerFeatureIds();

                    if (!flowerIds.empty()) {
                        // 数据驱动：花卉 id 是 placed_feature 的 ResourceLocation，
                        // 通过 PlacedFeatureRegistry 解析 placed_feature，再取其内部
                        // configured_feature（应为 ConfiguredFlowerFeature），从配置中
                        // 随机选择花朵方块状态放置。
                        const ResourceLocation& chosenId =
                            flowerIds[random.nextInt(static_cast<i32>(flowerIds.size()))];

                        const PlacedFeature* placedFeature = PlacedFeatureRegistry::instance().get(chosenId);
                        if (placedFeature != nullptr) {
                            auto* flowerFeature =
                                dynamic_cast<const ConfiguredFlowerFeature*>(placedFeature->feature());
                            if (flowerFeature != nullptr) {
                                // 从花卉配置中随机选择花朵方块状态
                                const BlockState* flower = flowerFeature->getConfig().getRandomFlower(random);
                                if (flower != nullptr) {
                                    world.setBlockState(currentPos, flower, 3);
                                }
                            }
                        }
                        continue;
                    }
                }

                // 生物群系没有花卉特征或获取失败时，回退到默认花朵（蒲公英）
                if (VanillaBlocks::DANDELION != nullptr) {
                    world.setBlockState(currentPos, &VanillaBlocks::DANDELION->defaultState(), 3);
                }
            } else {
                // 7/8 概率放置短草
                if (VanillaBlocks::SHORT_GRASS != nullptr) {
                    world.setBlockState(currentPos, &VanillaBlocks::SHORT_GRASS->defaultState(), 3);
                }
            }
        }
    }
}

// ============================================================================
// MyceliumBlock 实现
// ============================================================================

MyceliumBlock::MyceliumBlock(BlockProperties properties)
    : SpreadableSnowyDirtBlock(std::move(properties))
{}

void MyceliumBlock::animateTick(
    IBlockAnimateContext& context, const BlockPos& pos, const BlockState& state, math::IRandom& random) const
{
    MC_UNUSED(state);
    // MC 原版: 1/10 概率在菌丝方块上方生成菌丝粒子（SuspendedTownParticle）
    if (random.nextInt(10) == 0) {
        f32 x = static_cast<f32>(pos.x) + random.nextFloat();
        f32 y = static_cast<f32>(pos.y) + 1.1f;
        f32 z = static_cast<f32>(pos.z) + random.nextFloat();
        context.addAnimateParticle(particle::ParticleTypeId::Mycelium, Vector3(x, y, z), Vector3(0.0f, 0.0f, 0.0f));
    }
}

} // namespace mc::blocks
