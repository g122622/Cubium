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

#include "DragonRespawnAnimation.hpp"

#include "common/core/Types.hpp"
#include "common/entity/entities/effect/EffectEntities.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/BlockPos.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/dimension/end/EndDragonFight.hpp"
#include "common/world/explosion/ExplosionMode.hpp"
#include "common/world/gen/feature/end/EndSpikeFeature.hpp"
#include <cstddef>
#include <vector>

namespace mc {

// ============================================================================
// 辅助：获取柱顶光束目标位置
// ============================================================================

namespace {

/**
 * @brief 将水晶光束指向指定位置
 *
 * 对齐 MC 1.21.11 EndCrystal.setBeamTarget()。
 * 跳过 nullptr 水晶（防御性）。
 */
void setCrystalBeamTargets(std::vector<entity::EnderCrystalEntity*>& crystals, const BlockPos& target)
{
    for (entity::EnderCrystalEntity* crystal : crystals) {
        if (crystal != nullptr && !crystal->isRemoved()) {
            crystal->setBeamTarget(target);
        }
    }
}

} // namespace

// ============================================================================
// START 阶段
// ============================================================================

void dragon_respawn::tickStart(IWorld& world,
    EndDragonFight& fight,
    std::vector<entity::EnderCrystalEntity*>& crystals,
    i32 /*time*/,
    const BlockPos& /*portalLocation*/)
{
    // MC: BlockPos blockpos = new BlockPos(0, 128, 0);
    //     for (EndCrystal endcrystal : p_64019_) {
    //         endcrystal.setBeamTarget(blockpos);
    //     }
    //     p_64018_.setRespawnStage(PREPARING_TO_SUMMON_PILLARS);
    setCrystalBeamTargets(crystals, BlockPos(0, 128, 0));
    fight.setRespawnStage(world, DragonRespawnAnimation::PREPARING_TO_SUMMON_PILLARS);
}

// ============================================================================
// PREPARING_TO_SUMMON_PILLARS 阶段
// ============================================================================

void dragon_respawn::tickPreparingToSummonPillars(IWorld& world,
    EndDragonFight& fight,
    std::vector<entity::EnderCrystalEntity*>& /*crystals*/,
    i32 time,
    const BlockPos& /*portalLocation*/)
{
    // MC: if (p_64029_ < 100) {
    //         if (p_64029_ == 0 || p_64029_ == 50 || p_64029_ == 51
    //             || p_64029_ == 52 || p_64029_ >= 95) {
    //             p_64026_.levelEvent(3001, new BlockPos(0, 128, 0), 0);
    //         }
    //     } else {
    //         p_64027_.setRespawnStage(SUMMONING_PILLARS);
    //     }
    if (time < 100) {
        if (time == 0 || time == 50 || time == 51 || time == 52 || time >= 95) {
            world.playEvent(world::WorldEvents::ENDERMAN_GROWL_SOUND, BlockPos(0, 128, 0), 0);
        }
    } else {
        fight.setRespawnStage(world, DragonRespawnAnimation::SUMMONING_PILLARS);
    }
}

// ============================================================================
// SUMMONING_PILLARS 阶段
// ============================================================================

void dragon_respawn::tickSummoningPillars(IWorld& world,
    EndDragonFight& fight,
    std::vector<entity::EnderCrystalEntity*>& crystals,
    i32 time,
    const BlockPos& /*portalLocation*/)
{
    // MC: int i = 40;
    //     boolean flag = p_64038_ % 40 == 0;
    //     boolean flag1 = p_64038_ % 40 == 39;
    //     if (flag || flag1) {
    //         List<EndSpike> list = SpikeFeature.getSpikesForLevel(p_64035_);
    //         int j = p_64038_ / 40;
    //         if (j < list.size()) {
    //             EndSpike spike = list.get(j);
    //             if (flag) {
    //                 for (EndCrystal endcrystal : p_64037_) {
    //                     endcrystal.setBeamTarget(new BlockPos(spike.centerX, spike.height + 1, spike.centerZ));
    //                 }
    //             } else {
    //                 // flag1 分支：移除柱区方块 + 爆炸 + 重新生成柱子
    //                 int k = 10;
    //                 for (BlockPos blockpos : BlockPos.betweenClosed(
    //                     new BlockPos(spike.centerX - 10, spike.height - 10, spike.centerZ - 10),
    //                     new BlockPos(spike.centerX + 10, spike.height + 10, spike.centerZ + 10)
    //                 )) {
    //                     p_64035_.removeBlock(blockpos, false);
    //                 }
    //                 p_64035_.explode(null, spike.centerX + 0.5F, spike.height,
    //                                 spike.centerZ + 0.5F, 5.0F, Level.ExplosionInteraction.BLOCK);
    //                 SpikeConfiguration spikeconfiguration = new SpikeConfiguration(true,
    //                     ImmutableList.of(spike), new BlockPos(0, 128, 0));
    //                 Feature.END_SPIKE.place(spikeconfiguration, p_64035_,
    //                     p_64035_.getChunkSource().getGenerator(), RandomSource.create(),
    //                     new BlockPos(spike.centerX, 45, spike.centerZ));
    //             }
    //         } else if (flag) {
    //             p_64036_.setRespawnStage(SUMMONING_DRAGON);
    //         }
    //     }
    constexpr i32 PILLAR_PERIOD = 40;

    const bool flag = (time % PILLAR_PERIOD == 0);
    const bool flag1 = (time % PILLAR_PERIOD == PILLAR_PERIOD - 1);
    if (!flag && !flag1) {
        return;
    }

    // 获取柱子列表（由 EndDragonFight 提供世界种子生成）
    const std::vector<EndSpike> spikes = EndSpikeFeatureConfig::generateSpikes(fight.worldSeed());
    const i32 j = time / PILLAR_PERIOD;

    if (j < static_cast<i32>(spikes.size())) {
        const EndSpike& spike = spikes[static_cast<size_t>(j)];
        if (flag) {
            // 切换光束到当前柱子顶部
            setCrystalBeamTargets(crystals, BlockPos(spike.centerX, spike.height + 1, spike.centerZ));
        } else {
            // flag1 分支：移除柱区方块 + 爆炸 + 重新生成柱子
            // 移除柱区方块（spike.centerX±10, spike.height-10 到 spike.height+10, spike.centerZ±10）
            const BlockState* airState = VanillaBlocks::getState(VanillaBlocks::AIR);
            if (airState != nullptr) {
                for (i32 bx = spike.centerX - 10; bx <= spike.centerX + 10; ++bx) {
                    for (i32 by = spike.height - 10; by <= spike.height + 10; ++by) {
                        for (i32 bz = spike.centerZ - 10; bz <= spike.centerZ + 10; ++bz) {
                            // MC: p_64035_.removeBlock(blockpos, false)
                            // Cubium: setBlockState 设为空气
                            if (world.isWithinWorldBounds(bx, by, bz)) {
                                world.setBlockState(bx, by, bz, airState, 3);
                            }
                        }
                    }
                }
            }

            // 爆炸（模式 BLOCK = Break，破坏方块并掉落）
            // MC: p_64035_.explode(null, spike.centerX + 0.5F, spike.height,
            //                     spike.centerZ + 0.5F, 5.0F, Level.ExplosionInteraction.BLOCK);
            const Vector3 explodePos(static_cast<f32>(spike.centerX) + 0.5f,
                static_cast<f32>(spike.height),
                static_cast<f32>(spike.centerZ) + 0.5f);
            world.createExplosion(explodePos,
                5.0f,
                world::explosion::ExplosionMode::Break,
                false, // 不生成火焰
                nullptr);

            // 重新生成柱子（destroying=true 表示仅生成柱体，但此处使用 placeSpike 完整生成）
            // MC 的 SpikeConfiguration(true, ImmutableList.of(spike), new BlockPos(0, 128, 0))
            //   - destroying=true：在 SpikeFeature.place() 中走销毁分支
            //   - 但实际上在重生阶段，MC 的 SpikeFeature.place() 会调用 placeSpike()
            //     重新生成柱子和末影水晶
            // Cubium 直接调用 placeSpike()：
            math::Random rng(static_cast<u64>(time) ^ fight.worldSeed());
            EndSpikeFeatureConfig config(std::vector<EndSpike>{spike},
                false,               // destroying=false（placeSpike 不使用此标志）
                BlockPos(0, 128, 0), // crystalBeamTarget
                true);               // crystalInvulnerable（重生阶段柱顶水晶无敌）
            EndSpikeFeature feature;
            feature.placeSpike(world, rng, config, spike);
        }
    } else if (flag) {
        // 所有柱子处理完毕，切换到 SUMMONING_DRAGON
        fight.setRespawnStage(world, DragonRespawnAnimation::SUMMONING_DRAGON);
    }
}

// ============================================================================
// SUMMONING_DRAGON 阶段
// ============================================================================

void dragon_respawn::tickSummoningDragon(IWorld& world,
    EndDragonFight& fight,
    std::vector<entity::EnderCrystalEntity*>& crystals,
    i32 time,
    const BlockPos& /*portalLocation*/)
{
    // MC: if (p_64047_ >= 100) {
    //         p_64045_.setRespawnStage(END);
    //         p_64045_.resetSpikeCrystals();
    //         for (EndCrystal endcrystal : p_64046_) {
    //             endcrystal.setBeamTarget(null);
    //             p_64044_.explode(endcrystal, endcrystal.getX(), endcrystal.getY(),
    //                              endcrystal.getZ(), 6.0F, Level.ExplosionInteraction.NONE);
    //             endcrystal.discard();
    //         }
    //     } else if (p_64047_ >= 80) {
    //         p_64044_.levelEvent(3001, new BlockPos(0, 128, 0), 0);
    //     } else if (p_64047_ == 0) {
    //         for (EndCrystal endcrystal1 : p_64046_) {
    //             endcrystal1.setBeamTarget(new BlockPos(0, 128, 0));
    //         }
    //     } else if (p_64047_ < 5) {
    //         p_64044_.levelEvent(3001, new BlockPos(0, 128, 0), 0);
    //     }
    if (time >= 100) {
        fight.setRespawnStage(world, DragonRespawnAnimation::END);
        fight.resetSpikeCrystals(world);

        // 爆炸并 discard 所有重生水晶
        // MC: Level.ExplosionInteraction.NONE = 不破坏方块，仅造成伤害和粒子
        // Cubium: ExplosionMode::None
        for (entity::EnderCrystalEntity* crystal : crystals) {
            if (crystal == nullptr || crystal->isRemoved()) {
                continue;
            }
            // 清空光束
            crystal->setBeamTarget(BlockPos(0, 0, 0));
            // 爆炸（模式 NONE = None，不破坏方块）
            const Vector3 crystalPos(crystal->x(), crystal->y(), crystal->z());
            world.createExplosion(crystalPos,
                6.0f,
                world::explosion::ExplosionMode::None,
                false, // 不生成火焰
                crystal);
            // discard
            crystal->discard();
        }
    } else if (time >= 80) {
        world.playEvent(world::WorldEvents::ENDERMAN_GROWL_SOUND, BlockPos(0, 128, 0), 0);
    } else if (time == 0) {
        setCrystalBeamTargets(crystals, BlockPos(0, 128, 0));
    } else if (time < 5) {
        world.playEvent(world::WorldEvents::ENDERMAN_GROWL_SOUND, BlockPos(0, 128, 0), 0);
    }
}

// ============================================================================
// END 阶段（空操作）
// ============================================================================

void dragon_respawn::tickEnd(IWorld& /*world*/,
    EndDragonFight& /*fight*/,
    std::vector<entity::EnderCrystalEntity*>& /*crystals*/,
    i32 /*time*/,
    const BlockPos& /*portalLocation*/)
{
    // END 阶段为空操作：
    // EndDragonFight.setRespawnStage(END) 会创建新龙并清除重生状态，
    // 之后 respawnStage 被设为 nullptr，不再调用 tickEnd。
}

} // namespace mc
