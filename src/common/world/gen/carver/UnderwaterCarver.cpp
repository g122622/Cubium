#include "UnderwaterCarver.hpp"
#include "../../block/VanillaBlocks.hpp"
#include "../../chunk/ChunkPrimer.hpp"
#include "../../../util/math/random/Random.hpp"
#include "../../../core/Constants.hpp"
#include <unordered_set>
#include <cmath>
#include <algorithm>

namespace mc::world::gen::carver {

// ============================================================================
// 水下可雕刻方块集合
// ============================================================================

static const std::unordered_set<u32>& getUnderwaterCarvableBlocks()
{
    static std::unordered_set<u32> blocks = {
        // 标准可雕刻方块
        VanillaBlocks::STONE->blockId(),
        VanillaBlocks::GRANITE->blockId(),
        VanillaBlocks::DIORITE->blockId(),
        VanillaBlocks::ANDESITE->blockId(),
        VanillaBlocks::DIRT->blockId(),
        VanillaBlocks::COARSE_DIRT->blockId(),
        VanillaBlocks::PODZOL->blockId(),
        VanillaBlocks::GRASS_BLOCK->blockId(),
        // 陶瓦（包括染色陶瓦）
        VanillaBlocks::TERRACOTTA->blockId(),
        VanillaBlocks::WHITE_TERRACOTTA->blockId(),
        VanillaBlocks::ORANGE_TERRACOTTA->blockId(),
        VanillaBlocks::MAGENTA_TERRACOTTA->blockId(),
        VanillaBlocks::LIGHT_BLUE_TERRACOTTA->blockId(),
        VanillaBlocks::YELLOW_TERRACOTTA->blockId(),
        VanillaBlocks::LIME_TERRACOTTA->blockId(),
        VanillaBlocks::PINK_TERRACOTTA->blockId(),
        VanillaBlocks::GRAY_TERRACOTTA->blockId(),
        VanillaBlocks::LIGHT_GRAY_TERRACOTTA->blockId(),
        VanillaBlocks::CYAN_TERRACOTTA->blockId(),
        VanillaBlocks::PURPLE_TERRACOTTA->blockId(),
        VanillaBlocks::BLUE_TERRACOTTA->blockId(),
        VanillaBlocks::BROWN_TERRACOTTA->blockId(),
        VanillaBlocks::GREEN_TERRACOTTA->blockId(),
        VanillaBlocks::RED_TERRACOTTA->blockId(),
        VanillaBlocks::BLACK_TERRACOTTA->blockId(),
        // 沙子和砂岩
        VanillaBlocks::SANDSTONE->blockId(),
        VanillaBlocks::RED_SANDSTONE->blockId(),
        VanillaBlocks::MYCELIUM->blockId(),
        VanillaBlocks::SNOW->blockId(),
        // 水下特有的可雕刻方块
        VanillaBlocks::SAND->blockId(),
        VanillaBlocks::GRAVEL->blockId(),
        VanillaBlocks::WATER->blockId(),
        VanillaBlocks::LAVA->blockId(),
        VanillaBlocks::OBSIDIAN->blockId(),
        // AIR 由 isAir() 检查
        // CAVE_AIR 暂未实现
        VanillaBlocks::PACKED_ICE->blockId()
    };
    return blocks;
}

// ============================================================================
// UnderwaterCaveCarver 实现
// ============================================================================

UnderwaterCaveCarver::UnderwaterCaveCarver()
    : CaveCarver(world::MAX_BUILD_HEIGHT)
{
}

bool UnderwaterCaveCarver::carve(
    ChunkPrimer& chunk,
    const BiomeProvider& biomeProvider,
    i32 seaLevel,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    CarvingMask& carvingMask,
    const ProbabilityConfig& config)
{
    // 参考 MC CaveWorldCarver.carveRegion
    math::Random rng(static_cast<u64>(chunkX) * 341873128712ULL +
                     static_cast<u64>(chunkZ) * 132897987541ULL +
                     static_cast<u64>(m_maxHeight));

    if (!shouldCarve(rng, chunkX, chunkZ, config)) {
        return false;
    }

    // 隧道长度范围
    const i32 tunnelLength = (getRange() * 2 - 1) * 16;

    // 确定洞穴数量
    const i32 numCaves = rng.nextInt(rng.nextInt(rng.nextInt(getMaxCaveCount()) + 1) + 1);

    bool carved = false;
    const i32 startX = chunkX << 4;
    const i32 startZ = chunkZ << 4;

    for (i32 i = 0; i < numCaves; ++i) {
        // 随机起始位置
        const f32 startXPos = static_cast<f32>(startX) + rng.nextFloat(0.0f, 16.0f);
        const f32 startZPos = static_cast<f32>(startZ) + rng.nextFloat(0.0f, 16.0f);
        const f32 startYPos = static_cast<f32>(getCaveStartY(rng));

        // 有概率生成大型圆形房间
        i32 numTunnels = 1;

        if (rng.nextInt(4) == 0) {
            // 生成房间（使用水下版本）
            const f32 roomRadius = rng.nextFloat(1.0f, 7.0f);
            carveEllipsoidUnderwater(chunk, biomeProvider, seaLevel, chunkX, chunkZ,
                      startXPos, startYPos, startZPos,
                      roomRadius, 0.5f,
                      carvingMask,
                      static_cast<i64>(rng.nextU64()));
            numTunnels += rng.nextInt(5);
        }

        // 生成隧道
        for (i32 tunnelIdx = 0; tunnelIdx < numTunnels; ++tunnelIdx) {
            // 随机方向
            const f32 yaw = rng.nextFloat(0.0f, math::TWO_PI);
            const f32 pitch = rng.nextFloat(-0.25f, 0.25f);
            const f32 radius = getCaveRadius(rng);

            // 隧道长度
            const i32 length = tunnelLength - rng.nextInt(tunnelLength / 4 + 1);

            // 使用水下版本的雕刻
            // 注意：这里简化处理，实际应该逐椭球雕刻
            // 暂时使用基类的隧道生成逻辑，但使用水下椭球方法
            math::Random tunnelRng(static_cast<u64>(rng.nextU64()));

            f32 currentX = startXPos;
            f32 currentY = startYPos;
            f32 currentZ = startZPos;
            f32 currentYaw = yaw;
            f32 currentPitch = pitch;

            for (i32 step = 0; step < length; ++step) {
                // 更新位置
                currentX += std::sin(currentYaw) * std::cos(currentPitch);
                currentY += std::sin(currentPitch);
                currentZ += std::cos(currentYaw) * std::cos(currentPitch);

                // 随机调整方向
                currentPitch *= 0.7f;
                currentYaw += (tunnelRng.nextFloat() - 0.5f) * 0.5f;
                currentPitch += (tunnelRng.nextFloat() - 0.5f) * 0.25f;

                // 每隔几步雕刻一个椭球
                if (step % 4 == 0) {
                    carveEllipsoidUnderwater(chunk, biomeProvider, seaLevel, chunkX, chunkZ,
                              currentX, currentY, currentZ,
                              radius * (1.0f + tunnelRng.nextFloat() * 0.3f),
                              radius * 0.5f,
                              carvingMask,
                              static_cast<i64>(tunnelRng.nextU64()));
                }
            }
        }

        carved = true;
    }

    return carved;
}

bool UnderwaterCaveCarver::shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const
{
    // 水下洞穴使用与普通洞穴相同的椭球检测
    // 参考 MC: return p_222708_3_ <= -0.7D || dx * dx + dy * dy + dz * dz >= 1.0D;
    (void)y;
    return dy <= -0.7f || dx * dx + dy * dy + dz * dz >= 1.0f;
}

bool UnderwaterCaveCarver::isUnderwaterCarvable(const BlockState& state)
{
    // 检查是否为空气
    if (state.isAir()) {
        return true;
    }

    // 检查是否在水下可雕刻方块列表中
    const auto& blocks = getUnderwaterCarvableBlocks();
    return blocks.find(state.blockId()) != blocks.end();
}

bool UnderwaterCaveCarver::isInCarvingRangeUnderwater(
    ChunkCoord chunkX, ChunkCoord chunkZ,
    f32 x, f32 z,
    i32 step, i32 maxSteps,
    f32 radius)
{
    // 参考 MC WorldCarver.func_222702_a_
    const f32 chunkCenterX = static_cast<f32>(chunkX * 16 + 8);
    const f32 chunkCenterZ = static_cast<f32>(chunkZ * 16 + 8);

    const f32 dx = x - chunkCenterX;
    const f32 dz = z - chunkCenterZ;

    const f32 remainingSteps = static_cast<f32>(maxSteps - step);
    const f32 maxDist = radius + 2.0f + 16.0f;

    return dx * dx + dz * dz - remainingSteps * remainingSteps <= maxDist * maxDist;
}

bool UnderwaterCaveCarver::carveEllipsoidUnderwater(
    ChunkPrimer& chunk,
    const BiomeProvider& /*biomeProvider*/,
    i32 /*seaLevel*/,
    ChunkCoord chunkX,
    ChunkCoord chunkZ,
    f32 centerX, f32 centerY, f32 centerZ,
    f32 horizontalRadius, f32 verticalRadius,
    CarvingMask& carvingMask,
    i64 seed)
{
    // 参考 MC UnderwaterCaveWorldCarver.func_227208_a_
    // MC原版：水下雕刻器不检查水域，且在Y==10处有特殊填充逻辑

    const i32 startX = static_cast<i32>(centerX - horizontalRadius - 1.0f);
    const i32 endX = static_cast<i32>(centerX + horizontalRadius + 1.0f);
    const i32 startY = static_cast<i32>(centerY - verticalRadius - 1.0f);
    const i32 endY = static_cast<i32>(centerY + verticalRadius + 1.0f);
    const i32 startZ = static_cast<i32>(centerZ - horizontalRadius - 1.0f);
    const i32 endZ = static_cast<i32>(centerZ + horizontalRadius + 1.0f);

    // 区块边界
    const i32 chunkStartX = chunkX << 4;
    const i32 chunkEndX = chunkStartX + 15;
    const i32 chunkStartZ = chunkZ << 4;
    const i32 chunkEndZ = chunkStartZ + 15;

    // 检查椭球是否在区块范围外
    if (endX < chunkStartX - 16 || startX > chunkEndX + 16 ||
        endZ < chunkStartZ - 16 || startZ > chunkEndZ + 16) {
        return false;
    }

    // 计算区块内有效范围
    const i32 localMinX = std::max(0, startX - chunkStartX);
    const i32 localMaxX = std::min(15, endX - chunkStartX);
    const i32 localMinZ = std::max(0, startZ - chunkStartZ);
    const i32 localMaxZ = std::min(15, endZ - chunkStartZ);

    // 水下雕刻器不检查流体（与普通雕刻器的区别）

    math::Random rng(static_cast<u64>(seed) + static_cast<u64>(chunkX) + static_cast<u64>(chunkZ));
    bool carved = false;

    for (i32 lx = localMinX; lx <= localMaxX; ++lx) {
        const i32 worldX = (chunkX << 4) + lx;
        const f32 dx = (static_cast<f32>(worldX) + 0.5f - centerX) / horizontalRadius;
        const f32 dxSq = dx * dx;

        for (i32 lz = localMinZ; lz <= localMaxZ; ++lz) {
            const i32 worldZ = (chunkZ << 4) + lz;
            const f32 dz = (static_cast<f32>(worldZ) + 0.5f - centerZ) / horizontalRadius;
            const f32 dzSq = dz * dz;

            // 检查是否在椭球投影范围内
            if (dxSq + dzSq >= 1.0f) {
                continue;
            }

            for (i32 y = endY; y >= startY; --y) {
                // 边界检查
                if (y < 1 || y >= m_maxHeight - 8) {
                    continue;
                }

                const f32 dy = (static_cast<f32>(y) - 0.5f - centerY) / verticalRadius;

                // 检查是否应该跳过
                if (shouldSkipEllipsoidPosition(dx, dy, dz, y)) {
                    continue;
                }

                // 检查雕刻掩码
                if (carvingMask.isCarved(lx, y, lz)) {
                    continue;
                }

                // 获取当前方块
                const BlockState* state = chunk.getBlockState(lx, y, lz);
                if (!state) {
                    continue;
                }

                // 检查是否可以雕刻
                if (!isUnderwaterCarvable(*state)) {
                    continue;
                }

                // 标记为已雕刻
                carvingMask.setCarved(lx, y, lz);

                // MC 1.16.5 水下雕刻器特殊填充逻辑
                // 参考 UnderwaterCaveWorldCarver.func_222728_a_
                const BlockState* fillBlock = nullptr;

                if (y == 10) {
                    // Y == 10: 25% 岩浆块，75% 黑曜石
                    if (rng.nextFloat() < 0.25f) {
                        fillBlock = VanillaBlocks::getState(VanillaBlocks::MAGMA);
                    } else {
                        fillBlock = VanillaBlocks::getState(VanillaBlocks::OBSIDIAN);
                    }
                } else if (y < 10) {
                    // Y < 10: 填充熔岩
                    fillBlock = VanillaBlocks::getState(VanillaBlocks::LAVA);
                } else {
                    // Y > 10: 填充水
                    // 检查周围是否有空气方块（决定是否放水）
                    // MC原版：如果周围没有空气，则放水；否则不放
                    // 简化处理：始终放水（水下雕刻本就在水中）
                    fillBlock = VanillaBlocks::getState(VanillaBlocks::WATER);
                }

                if (fillBlock) {
                    chunk.setBlockState(lx, y, lz, fillBlock);
                }

                carved = true;
            }
        }
    }

    return carved;
}

// ============================================================================
// UnderwaterCanyonCarver 实现
// ============================================================================

UnderwaterCanyonCarver::UnderwaterCanyonCarver()
    : CanyonCarver(world::MAX_BUILD_HEIGHT)
{
}

bool UnderwaterCanyonCarver::shouldSkipEllipsoidPosition(f32 dx, f32 dy, f32 dz, i32 y) const
{
    // 水下峡谷使用与普通峡谷相同的厚度检测
    return CanyonCarver::shouldSkipEllipsoidPosition(dx, dy, dz, y);
}

// ============================================================================
// 工厂函数
// ============================================================================

std::unique_ptr<UnderwaterCaveCarver> createUnderwaterCaveCarver()
{
    return std::make_unique<UnderwaterCaveCarver>();
}

std::unique_ptr<UnderwaterCanyonCarver> createUnderwaterCanyonCarver()
{
    return std::make_unique<UnderwaterCanyonCarver>();
}

} // namespace mc::world::gen::carver
