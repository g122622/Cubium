#include "CoralFeature.hpp"
#include "../../../chunk/ChunkPrimer.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../../util/Direction.hpp"

namespace mc {

// ============================================================================
// CoralFeature 实现
// ============================================================================

bool CoralFeature::place(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    const CoralFeatureConfig& config)
{
    // 寻找有效的放置位置（向下找到水下的地面）
    BlockPos placePos = pos;
    bool foundGround = false;

    for (i32 y = pos.y; y >= 1; --y) {
        BlockPos checkPos(pos.x, y, pos.z);
        const BlockState* state = world.getBlock(checkPos);

        if (state && !state->isAir()) {
            // 找到非空气方块
            placePos = BlockPos(pos.x, y + 1, placePos.z);
            foundGround = true;
            break;
        }
    }

    if (!foundGround) {
        return false;
    }

    // 检查是否可以放置
    if (!canPlaceAt(world, placePos)) {
        return false;
    }

    // 放置珊瑚方块
    placeCoralBlock(world, placePos, config.color);

    // 随机放置珊瑚扇
    if (config.includeWallFan && random.nextInt(3) == 0) {
        Direction direction = static_cast<Direction>(random.nextInt(4)); // 北东南西
        BlockPos fanPos(placePos.x, placePos.y + 1, placePos.z);
        placeCoralFan(world, fanPos, config.color, direction);
    }

    return true;
}

bool CoralFeature::canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const
{
    // 检查位置是否为水
    if (!isWater(world, pos)) {
        return false;
    }

    // 检查下方方块是否为固体
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlock(belowPos);

    if (!belowState) {
        return false;
    }

    // 下方必须是固体方块
    return belowState->owner().isSolid(*belowState);
}

bool CoralFeature::isWater(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlock(pos);
    if (!state) {
        return false;
    }

    // 检查是否为水方块
    if (VanillaBlocks::WATER && state->blockId() == VanillaBlocks::WATER->blockId()) {
        return true;
    }

    return false;
}

void CoralFeature::placeCoralBlock(
    WorldGenRegion& world,
    const BlockPos& pos,
    blocks::CoralColor color) const
{
    // TODO: 根据颜色获取对应的珊瑚方块状态
    // 目前使用占位符
    MC_UNUSED(color);

    // 放置珊瑚方块（需要注册珊瑚方块后更新）
    // switch (color) {
    //     case blocks::CoralColor::Tube:
    //         world.setBlock(pos, &VanillaBlocks::TUBE_CORAL->defaultState());
    //         break;
    //     ...
    // }
}

void CoralFeature::placeCoralFan(
    WorldGenRegion& world,
    const BlockPos& pos,
    blocks::CoralColor color,
    Direction direction) const
{
    // TODO: 根据颜色和方向放置珊瑚扇
    MC_UNUSED(color);
    MC_UNUSED(direction);

    // 放置珊瑚扇（需要注册珊瑚扇方块后更新）
}

// ============================================================================
// CoralTreeFeature 实现
// ============================================================================

bool CoralTreeFeature::place(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    const CoralFeatureConfig& config)
{
    // 生成主干
    i32 trunkHeight = random.nextInt(3) + 1;

    for (i32 y = 0; y < trunkHeight; ++y) {
        BlockPos trunkPos(pos.x, pos.y + y, pos.z);
        // 放置珊瑚方块
        // world.setBlock(trunkPos, getCoralState(config.color));
    }

    // 生成分支
    BlockPos topPos(pos.x, pos.y + trunkHeight - 1, pos.z);
    for (i32 i = 0; i < 4; ++i) {
        Direction dir = static_cast<Direction>(i);
        if (random.nextFloat() < 0.6f) {
            generateBranch(world, random, topPos, config.color, dir, random.nextInt(3) + 1);
        }
    }

    return true;
}

void CoralTreeFeature::generateBranch(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    blocks::CoralColor color,
    Direction direction,
    i32 length)
{
    MC_UNUSED(world);
    MC_UNUSED(random);
    MC_UNUSED(pos);
    MC_UNUSED(color);
    MC_UNUSED(direction);
    MC_UNUSED(length);

    // TODO: 实现分支生成逻辑
}

// ============================================================================
// CoralMushroomFeature 实现
// ============================================================================

bool CoralMushroomFeature::place(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    const CoralFeatureConfig& config)
{
    // 生成茎
    i32 stemHeight = random.nextInt(2) + 1;
    for (i32 y = 0; y < stemHeight; ++y) {
        BlockPos stemPos(pos.x, pos.y + y, pos.z);
        // 放置珊瑚方块
    }

    // 生成盖
    BlockPos capPos(pos.x, pos.y + stemHeight, pos.z);
    i32 radius = random.nextInt(2) + 1;
    generateCap(world, random, capPos, config.color, radius);

    return true;
}

void CoralMushroomFeature::generateCap(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    blocks::CoralColor color,
    i32 radius)
{
    MC_UNUSED(world);
    MC_UNUSED(random);
    MC_UNUSED(pos);
    MC_UNUSED(color);
    MC_UNUSED(radius);

    // TODO: 实现蘑菇盖生成逻辑
}

// ============================================================================
// CoralClawFeature 实现
// ============================================================================

bool CoralClawFeature::place(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    const CoralFeatureConfig& config)
{
    // 生成爪形结构
    i32 clawCount = random.nextInt(3) + 2;

    for (i32 i = 0; i < clawCount; ++i) {
        Direction dir = static_cast<Direction>(random.nextInt(4));
        generateClaw(world, random, pos, config.color, dir);
    }

    return true;
}

void CoralClawFeature::generateClaw(
    WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    blocks::CoralColor color,
    Direction direction)
{
    MC_UNUSED(world);
    MC_UNUSED(random);
    MC_UNUSED(pos);
    MC_UNUSED(color);
    MC_UNUSED(direction);

    // TODO: 实现爪形生成逻辑
}

// ============================================================================
// ConfiguredCoralFeature 实现
// ============================================================================

ConfiguredCoralFeature::ConfiguredCoralFeature(
    std::unique_ptr<CoralFeatureConfig> config,
    const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{
}

bool ConfiguredCoralFeature::place(
    WorldGenRegion& region,
    ChunkPrimer& chunk,
    IChunkGenerator& generator,
    math::Random& random,
    const BlockPos& pos)
{
    MC_UNUSED(chunk);
    MC_UNUSED(generator);
    return m_feature.place(region, random, pos, *m_config);
}

// ============================================================================
// CoralFeatures 实现
// ============================================================================

std::vector<std::unique_ptr<ConfiguredCoralFeature>> CoralFeatures::s_features;

void CoralFeatures::initialize()
{
    s_features.clear();
    s_features.push_back(createTubeCoral());
    s_features.push_back(createBrainCoral());
    s_features.push_back(createBubbleCoral());
    s_features.push_back(createFireCoral());
    s_features.push_back(createHornCoral());
}

const std::vector<std::unique_ptr<ConfiguredCoralFeature>>& CoralFeatures::getAllFeatures()
{
    return s_features;
}

std::vector<std::unique_ptr<ConfiguredCoralFeature>> CoralFeatures::getAllFeaturesAndClear()
{
    auto result = std::move(s_features);
    s_features.clear();
    return result;
}

std::unique_ptr<ConfiguredCoralFeature> CoralFeatures::createTubeCoral()
{
    auto config = std::make_unique<CoralFeatureConfig>(blocks::CoralColor::Tube, true);
    return std::make_unique<ConfiguredCoralFeature>(std::move(config), "tube_coral");
}

std::unique_ptr<ConfiguredCoralFeature> CoralFeatures::createBrainCoral()
{
    auto config = std::make_unique<CoralFeatureConfig>(blocks::CoralColor::Brain, true);
    return std::make_unique<ConfiguredCoralFeature>(std::move(config), "brain_coral");
}

std::unique_ptr<ConfiguredCoralFeature> CoralFeatures::createBubbleCoral()
{
    auto config = std::make_unique<CoralFeatureConfig>(blocks::CoralColor::Bubble, true);
    return std::make_unique<ConfiguredCoralFeature>(std::move(config), "bubble_coral");
}

std::unique_ptr<ConfiguredCoralFeature> CoralFeatures::createFireCoral()
{
    auto config = std::make_unique<CoralFeatureConfig>(blocks::CoralColor::Fire, true);
    return std::make_unique<ConfiguredCoralFeature>(std::move(config), "fire_coral");
}

std::unique_ptr<ConfiguredCoralFeature> CoralFeatures::createHornCoral()
{
    auto config = std::make_unique<CoralFeatureConfig>(blocks::CoralColor::Horn, true);
    return std::make_unique<ConfiguredCoralFeature>(std::move(config), "horn_coral");
}

} // namespace mc
