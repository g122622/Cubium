#include "CoralFeature.hpp"
#include "../../../../util/Direction.hpp"
#include "../../../../util/math/random/Random.hpp"
#include "../../../WorldConstants.hpp"
#include "../../../block/VanillaBlocks.hpp"
#include "../../../chunk/ChunkPrimer.hpp"
#include "../../chunk/IChunkGenerator.hpp"
#include <array>
#include <cmath>

namespace mc {

namespace {

[[nodiscard]] const BlockState* getCoralBlockState(blocks::CoralColor color, bool isDead)
{
    if (isDead) {
        switch (color) {
            case blocks::CoralColor::Tube:
                return VanillaBlocks::DEAD_TUBE_CORAL_BLOCK ? &VanillaBlocks::DEAD_TUBE_CORAL_BLOCK->defaultState()
                                                            : nullptr;
            case blocks::CoralColor::Brain:
                return VanillaBlocks::DEAD_BRAIN_CORAL_BLOCK ? &VanillaBlocks::DEAD_BRAIN_CORAL_BLOCK->defaultState()
                                                             : nullptr;
            case blocks::CoralColor::Bubble:
                return VanillaBlocks::DEAD_BUBBLE_CORAL_BLOCK ? &VanillaBlocks::DEAD_BUBBLE_CORAL_BLOCK->defaultState()
                                                              : nullptr;
            case blocks::CoralColor::Fire:
                return VanillaBlocks::DEAD_FIRE_CORAL_BLOCK ? &VanillaBlocks::DEAD_FIRE_CORAL_BLOCK->defaultState()
                                                            : nullptr;
            case blocks::CoralColor::Horn:
                return VanillaBlocks::DEAD_HORN_CORAL_BLOCK ? &VanillaBlocks::DEAD_HORN_CORAL_BLOCK->defaultState()
                                                            : nullptr;
            default:
                return nullptr;
        }
    }

    switch (color) {
        case blocks::CoralColor::Tube:
            return VanillaBlocks::TUBE_CORAL_BLOCK ? &VanillaBlocks::TUBE_CORAL_BLOCK->defaultState() : nullptr;
        case blocks::CoralColor::Brain:
            return VanillaBlocks::BRAIN_CORAL_BLOCK ? &VanillaBlocks::BRAIN_CORAL_BLOCK->defaultState() : nullptr;
        case blocks::CoralColor::Bubble:
            return VanillaBlocks::BUBBLE_CORAL_BLOCK ? &VanillaBlocks::BUBBLE_CORAL_BLOCK->defaultState() : nullptr;
        case blocks::CoralColor::Fire:
            return VanillaBlocks::FIRE_CORAL_BLOCK ? &VanillaBlocks::FIRE_CORAL_BLOCK->defaultState() : nullptr;
        case blocks::CoralColor::Horn:
            return VanillaBlocks::HORN_CORAL_BLOCK ? &VanillaBlocks::HORN_CORAL_BLOCK->defaultState() : nullptr;
        default:
            return nullptr;
    }
}

[[nodiscard]] const BlockState* getCoralFanState(blocks::CoralColor color, bool isDead)
{
    if (isDead) {
        switch (color) {
            case blocks::CoralColor::Tube:
                return VanillaBlocks::DEAD_TUBE_CORAL_FAN ? &VanillaBlocks::DEAD_TUBE_CORAL_FAN->defaultState()
                                                          : nullptr;
            case blocks::CoralColor::Brain:
                return VanillaBlocks::DEAD_BRAIN_CORAL_FAN ? &VanillaBlocks::DEAD_BRAIN_CORAL_FAN->defaultState()
                                                           : nullptr;
            case blocks::CoralColor::Bubble:
                return VanillaBlocks::DEAD_BUBBLE_CORAL_FAN ? &VanillaBlocks::DEAD_BUBBLE_CORAL_FAN->defaultState()
                                                            : nullptr;
            case blocks::CoralColor::Fire:
                return VanillaBlocks::DEAD_FIRE_CORAL_FAN ? &VanillaBlocks::DEAD_FIRE_CORAL_FAN->defaultState()
                                                          : nullptr;
            case blocks::CoralColor::Horn:
                return VanillaBlocks::DEAD_HORN_CORAL_FAN ? &VanillaBlocks::DEAD_HORN_CORAL_FAN->defaultState()
                                                          : nullptr;
            default:
                return nullptr;
        }
    }

    switch (color) {
        case blocks::CoralColor::Tube:
            return VanillaBlocks::TUBE_CORAL_FAN ? &VanillaBlocks::TUBE_CORAL_FAN->defaultState() : nullptr;
        case blocks::CoralColor::Brain:
            return VanillaBlocks::BRAIN_CORAL_FAN ? &VanillaBlocks::BRAIN_CORAL_FAN->defaultState() : nullptr;
        case blocks::CoralColor::Bubble:
            return VanillaBlocks::BUBBLE_CORAL_FAN ? &VanillaBlocks::BUBBLE_CORAL_FAN->defaultState() : nullptr;
        case blocks::CoralColor::Fire:
            return VanillaBlocks::FIRE_CORAL_FAN ? &VanillaBlocks::FIRE_CORAL_FAN->defaultState() : nullptr;
        case blocks::CoralColor::Horn:
            return VanillaBlocks::HORN_CORAL_FAN ? &VanillaBlocks::HORN_CORAL_FAN->defaultState() : nullptr;
        default:
            return nullptr;
    }
}

[[nodiscard]] const BlockState* getCoralWallFanState(blocks::CoralColor color, Direction supportDirection, bool isDead)
{
    if (supportDirection == Direction::Up || supportDirection == Direction::Down ||
        supportDirection == Direction::None) {
        return nullptr;
    }

    Block* wallFanBlock = nullptr;
    if (isDead) {
        switch (color) {
            case blocks::CoralColor::Tube:
                wallFanBlock = VanillaBlocks::DEAD_TUBE_CORAL_WALL_FAN;
                break;
            case blocks::CoralColor::Brain:
                wallFanBlock = VanillaBlocks::DEAD_BRAIN_CORAL_WALL_FAN;
                break;
            case blocks::CoralColor::Bubble:
                wallFanBlock = VanillaBlocks::DEAD_BUBBLE_CORAL_WALL_FAN;
                break;
            case blocks::CoralColor::Fire:
                wallFanBlock = VanillaBlocks::DEAD_FIRE_CORAL_WALL_FAN;
                break;
            case blocks::CoralColor::Horn:
                wallFanBlock = VanillaBlocks::DEAD_HORN_CORAL_WALL_FAN;
                break;
            default:
                wallFanBlock = nullptr;
                break;
        }
    } else {
        switch (color) {
            case blocks::CoralColor::Tube:
                wallFanBlock = VanillaBlocks::TUBE_CORAL_WALL_FAN;
                break;
            case blocks::CoralColor::Brain:
                wallFanBlock = VanillaBlocks::BRAIN_CORAL_WALL_FAN;
                break;
            case blocks::CoralColor::Bubble:
                wallFanBlock = VanillaBlocks::BUBBLE_CORAL_WALL_FAN;
                break;
            case blocks::CoralColor::Fire:
                wallFanBlock = VanillaBlocks::FIRE_CORAL_WALL_FAN;
                break;
            case blocks::CoralColor::Horn:
                wallFanBlock = VanillaBlocks::HORN_CORAL_WALL_FAN;
                break;
            default:
                wallFanBlock = nullptr;
                break;
        }
    }

    if (wallFanBlock == nullptr) {
        return nullptr;
    }

    return &wallFanBlock->defaultState().with(BlockStateProperties::FACING(), supportDirection);
}

[[nodiscard]] bool isWaterAt(WorldGenRegion& world, const BlockPos& pos)
{
    const BlockState* state = world.getBlockState(pos);
    return state != nullptr && VanillaBlocks::WATER != nullptr && state->is(VanillaBlocks::WATER);
}

[[nodiscard]] i32 findOceanFloorY(WorldGenRegion& world, i32 x, i32 z)
{
    i32 oceanFloorY = world.getTopBlockY(x, z, HeightmapType::OceanFloorWG);
    if (oceanFloorY > 0) {
        return oceanFloorY;
    }

    // 某些场景可能未构建高度图，回退到显式扫描。
    for (i32 y = world::MAX_BUILD_HEIGHT - 1; y >= world::MIN_BUILD_HEIGHT + 1; --y) {
        const BlockState* state = world.getBlockState(x, y, z);
        if (state == nullptr || state->isAir()) {
            continue;
        }

        if (VanillaBlocks::WATER != nullptr && state->is(VanillaBlocks::WATER)) {
            continue;
        }

        return y;
    }

    return -1;
}

[[nodiscard]] bool placeCoralBase(WorldGenRegion& world, const BlockPos& pos, blocks::CoralColor color, bool isDead)
{
    const BlockState* coralState = getCoralBlockState(color, isDead);
    if (coralState == nullptr || !isWaterAt(world, pos)) {
        return false;
    }

    world.setBlockState(pos, coralState);
    return true;
}

void placeCoralDecorations(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    blocks::CoralColor color,
    bool isDead,
    bool includeDecorations)
{
    if (!includeDecorations) {
        return;
    }

    // 参考 MC CoralFeature.java 第28-37行
    const BlockPos topPos(pos.x, pos.y + 1, pos.z);
    if (isWaterAt(world, topPos)) {
        // MC第33行：25%概率放珊瑚扇
        if (random.nextFloat() < 0.25f) {
            // MC第35行：在珊瑚扇基础上，5%概率放海泡菜而不是珊瑚扇
            if (VanillaBlocks::SEA_PICKLE != nullptr && random.nextFloat() < 0.05f) {
                const i32 pickleCount = random.nextInt(4) + 1;
                const BlockState* pickleState =
                    &VanillaBlocks::SEA_PICKLE->defaultState().with(BlockStateProperties::PICKLES_1_4(), pickleCount);
                world.setBlockState(topPos, pickleState);
            } else if (const BlockState* fanState = getCoralFanState(color, isDead); fanState != nullptr) {
                world.setBlockState(topPos, fanState);
            }
        }
    }

    // MC第39-46行：水平方向20%概率放墙珊瑚扇
    const auto horizontalDirections = Directions::horizontal();
    for (Direction horizontal : horizontalDirections) {
        if (random.nextFloat() >= 0.20f) {
            continue;
        }

        const BlockPos sidePos = pos.offset(horizontal);
        if (!isWaterAt(world, sidePos)) {
            continue;
        }

        // 参考 MC CoralFeature.java 第43行：FACING应为direction而非opposite
        if (const BlockState* wallFanState = getCoralWallFanState(color, horizontal, isDead); wallFanState != nullptr) {
            world.setBlockState(sidePos, wallFanState);
        }
    }
}

[[nodiscard]] bool placeCoralWithDecorations(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    blocks::CoralColor color,
    bool isDead,
    bool includeDecorations)
{
    if (!placeCoralBase(world, pos, color, isDead)) {
        return false;
    }

    placeCoralDecorations(world, random, pos, color, isDead, includeDecorations);
    return true;
}

} // namespace

// ============================================================================
// CoralFeature 实现
// ============================================================================

bool CoralFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const CoralFeatureConfig& config)
{
    // 在当前区块内随机选择一个 X/Z，使用海底高度图定位珊瑚基点。
    const i32 placeX = pos.x + random.nextInt(16);
    const i32 placeZ = pos.z + random.nextInt(16);
    const i32 oceanFloorY = findOceanFloorY(world, placeX, placeZ);
    if (oceanFloorY <= 0) {
        return false;
    }

    const BlockPos placePos(placeX, oceanFloorY + 1, placeZ);

    // 检查是否可以放置
    if (!canPlaceAt(world, placePos)) {
        return false;
    }

    // 与原版一致，随机选择三类珊瑚结构之一。
    bool placed = false;
    switch (random.nextInt(3)) {
        case 0: {
            CoralTreeFeature feature;
            placed = feature.place(world, random, placePos, config);
            break;
        }
        case 1: {
            CoralMushroomFeature feature;
            placed = feature.place(world, random, placePos, config);
            break;
        }
        default: {
            CoralClawFeature feature;
            placed = feature.place(world, random, placePos, config);
            break;
        }
    }

    if (!placed) {
        placed = placeCoralWithDecorations(world, random, placePos, config.color, config.isDead, config.includeWallFan);
    }

    return placed;
}

bool CoralFeature::canPlaceAt(WorldGenRegion& world, const BlockPos& pos) const
{
    // 检查位置是否为水
    if (!isWater(world, pos)) {
        return false;
    }

    // 检查下方方块是否为固体
    BlockPos belowPos(pos.x, pos.y - 1, pos.z);
    const BlockState* belowState = world.getBlockState(belowPos);

    if (!belowState) {
        return false;
    }

    // 下方必须是固体方块
    return belowState->owner().isSolid(*belowState);
}

bool CoralFeature::isWater(WorldGenRegion& world, const BlockPos& pos) const
{
    const BlockState* state = world.getBlockState(pos);
    if (!state) {
        return false;
    }

    // 检查是否为水方块
    if (VanillaBlocks::WATER && state->blockId() == VanillaBlocks::WATER->blockId()) {
        return true;
    }

    return false;
}

void CoralFeature::placeCoralBlock(WorldGenRegion& world, const BlockPos& pos, blocks::CoralColor color) const
{
    [[maybe_unused]] const bool placed = placeCoralBase(world, pos, color, false);
}

void CoralFeature::placeCoralFan(
    WorldGenRegion& world, const BlockPos& pos, blocks::CoralColor color, Direction direction) const
{
    if (!isWaterAt(world, pos)) {
        return;
    }

    if (direction == Direction::Up) {
        if (const BlockState* fanState = getCoralFanState(color, false); fanState != nullptr) {
            world.setBlockState(pos, fanState);
        }
        return;
    }

    if (const BlockState* wallFanState = getCoralWallFanState(color, direction, false); wallFanState != nullptr) {
        world.setBlockState(pos, wallFanState);
    }
}

// ============================================================================
// CoralTreeFeature 实现
// ============================================================================

bool CoralTreeFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const CoralFeatureConfig& config)
{
    // 参考 MC CoralTreeFeature.java 第18-52行
    const i32 trunkHeight = random.nextInt(3) + 1;

    BlockPos topPos = pos;
    i32 placedTrunk = 0;
    for (i32 y = 0; y < trunkHeight; ++y) {
        const BlockPos trunkPos(pos.x, pos.y + y, pos.z);
        if (!placeCoralWithDecorations(world, random, trunkPos, config.color, config.isDead, config.includeWallFan)) {
            break;
        }
        topPos = trunkPos;
        ++placedTrunk;
    }

    if (placedTrunk == 0) {
        return false;
    }

    // MC第31行：nextInt(3) + 2，即2-4个分支
    const auto horizontalDirections = Directions::horizontal();
    const i32 branchCount = random.nextInt(3) + 2;
    for (i32 i = 0; i < branchCount; ++i) {
        const Direction direction = horizontalDirections[static_cast<size_t>(random.nextInt(4))];
        // MC第38行：分支长度nextInt(5) + 2，即2-6
        generateBranch(world,
            random,
            topPos,
            config.color,
            config.isDead,
            direction,
            random.nextInt(5) + 2, // 修正：应为2-6而非2-4
            config.includeWallFan);
    }

    return true;
}

void CoralTreeFeature::generateBranch(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    blocks::CoralColor color,
    bool isDead,
    Direction direction,
    i32 length,
    bool includeDecorations)
{
    BlockPos currentPos = pos;
    for (i32 i = 0; i < length; ++i) {
        currentPos = currentPos.offset(direction);
        if (i > 0 && random.nextFloat() < 0.45f) {
            currentPos = currentPos.offset(Direction::Up);
        }

        if (!placeCoralWithDecorations(world, random, currentPos, color, isDead, includeDecorations)) {
            break;
        }
    }
}

// ============================================================================
// CoralMushroomFeature 实现
// ============================================================================

bool CoralMushroomFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const CoralFeatureConfig& config)
{
    const i32 stemHeight = random.nextInt(2) + 1;
    bool placedAny = false;

    for (i32 y = 0; y < stemHeight; ++y) {
        const BlockPos stemPos(pos.x, pos.y + y, pos.z);
        if (!placeCoralWithDecorations(world, random, stemPos, config.color, config.isDead, config.includeWallFan)) {
            break;
        }
        placedAny = true;
    }

    if (!placedAny) {
        return false;
    }

    const BlockPos capPos(pos.x, pos.y + stemHeight, pos.z);
    const i32 radius = random.nextInt(2) + 2;
    generateCap(world, random, capPos, config.color, config.isDead, radius, config.includeWallFan);

    return true;
}

void CoralMushroomFeature::generateCap(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    blocks::CoralColor color,
    bool isDead,
    i32 radius,
    bool includeDecorations)
{
    // 参考 MC CoralMushroomFeature.java 第27行
    // MC使用复杂的边界检测逻辑来生成蘑菇形状
    const i32 i = radius + 1; // X范围
    const i32 j = radius;     // Y范围
    const i32 k = radius + 1; // Z范围
    const i32 l = 0;          // 向下偏移

    for (i32 i1 = 0; i1 <= i; ++i1) {
        for (i32 j1 = 0; j1 <= j; ++j1) {
            for (i32 k1 = 0; k1 <= k; ++k1) {
                BlockPos capPos(pos.x + i1, pos.y + j1 - l, pos.z + k1);

                // MC第27行的复杂条件
                const bool cond1 = (i1 != 0 && i1 != i) || (j1 != 0 && j1 != j);
                const bool cond2 = (k1 != 0 && k1 != k) || (j1 != 0 && j1 != j);
                const bool cond3 = (i1 != 0 && i1 != i) || (k1 != 0 && k1 != k);
                const bool cond4 = (i1 == 0 || i1 == i || j1 == 0 || j1 == j || k1 == 0 || k1 == k);

                if (cond1 && cond2 && cond3 && cond4) {
                    if (random.nextFloat() < 0.1f) {
                        continue; // 10%概率跳过
                    }
                    [[maybe_unused]] const bool placed =
                        placeCoralWithDecorations(world, random, capPos, color, isDead, includeDecorations);
                }
            }
        }
    }
}

// ============================================================================
// CoralClawFeature 实现
// ============================================================================

bool CoralClawFeature::place(
    WorldGenRegion& world, math::Random& random, const BlockPos& pos, const CoralFeatureConfig& config)
{
    bool placedAny = placeCoralWithDecorations(world, random, pos, config.color, config.isDead, config.includeWallFan);

    const auto directions = Directions::horizontal();
    const Direction mainDirection = directions[static_cast<size_t>(random.nextInt(4))];
    const std::array<Direction, 3> clawDirections = {
        mainDirection, Directions::rotateY(mainDirection), Directions::rotateYCCW(mainDirection)};

    for (Direction direction : clawDirections) {
        if (random.nextFloat() < 0.75f) {
            generateClaw(world, random, pos, config.color, config.isDead, direction, config.includeWallFan);
            placedAny = true;
        }
    }

    return placedAny;
}

void CoralClawFeature::generateClaw(WorldGenRegion& world,
    math::Random& random,
    const BlockPos& pos,
    blocks::CoralColor color,
    bool isDead,
    Direction direction,
    bool includeDecorations)
{
    i32 clawLength = random.nextInt(3) + 2;
    BlockPos currentPos = pos;
    Direction currentDirection = direction;

    for (i32 i = 0; i < clawLength; ++i) {
        currentPos = currentPos.offset(currentDirection);
        if (i > 0 && random.nextFloat() < 0.35f) {
            currentPos = currentPos.offset(Direction::Up);
        }

        if (!placeCoralWithDecorations(world, random, currentPos, color, isDead, includeDecorations)) {
            break;
        }

        if (i > 0 && random.nextFloat() < 0.25f) {
            currentDirection =
                random.nextBoolean() ? Directions::rotateY(currentDirection) : Directions::rotateYCCW(currentDirection);
        }
    }
}

// ============================================================================
// ConfiguredCoralFeature 实现
// ============================================================================

ConfiguredCoralFeature::ConfiguredCoralFeature(std::unique_ptr<CoralFeatureConfig> config, const char* featureName)
    : m_config(std::move(config))
    , m_name(featureName)
{}

bool ConfiguredCoralFeature::place(
    WorldGenRegion& region, ChunkPrimer& chunk, IChunkGenerator& generator, math::Random& random, const BlockPos& pos)
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
    s_features.push_back(createDeadTubeCoral());
    s_features.push_back(createDeadBrainCoral());
    s_features.push_back(createDeadBubbleCoral());
    s_features.push_back(createDeadFireCoral());
    s_features.push_back(createDeadHornCoral());
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
    auto config = std::make_unique<CoralFeatureConfig>(blocks::CoralColor::Tube, true, false);
    return std::make_unique<ConfiguredCoralFeature>(std::move(config), "tube_coral");
}

std::unique_ptr<ConfiguredCoralFeature> CoralFeatures::createBrainCoral()
{
    auto config = std::make_unique<CoralFeatureConfig>(blocks::CoralColor::Brain, true, false);
    return std::make_unique<ConfiguredCoralFeature>(std::move(config), "brain_coral");
}

std::unique_ptr<ConfiguredCoralFeature> CoralFeatures::createBubbleCoral()
{
    auto config = std::make_unique<CoralFeatureConfig>(blocks::CoralColor::Bubble, true, false);
    return std::make_unique<ConfiguredCoralFeature>(std::move(config), "bubble_coral");
}

std::unique_ptr<ConfiguredCoralFeature> CoralFeatures::createFireCoral()
{
    auto config = std::make_unique<CoralFeatureConfig>(blocks::CoralColor::Fire, true, false);
    return std::make_unique<ConfiguredCoralFeature>(std::move(config), "fire_coral");
}

std::unique_ptr<ConfiguredCoralFeature> CoralFeatures::createHornCoral()
{
    auto config = std::make_unique<CoralFeatureConfig>(blocks::CoralColor::Horn, true, false);
    return std::make_unique<ConfiguredCoralFeature>(std::move(config), "horn_coral");
}

std::unique_ptr<ConfiguredCoralFeature> CoralFeatures::createDeadTubeCoral()
{
    auto config = std::make_unique<CoralFeatureConfig>(blocks::CoralColor::Tube, true, true);
    return std::make_unique<ConfiguredCoralFeature>(std::move(config), "dead_tube_coral");
}

std::unique_ptr<ConfiguredCoralFeature> CoralFeatures::createDeadBrainCoral()
{
    auto config = std::make_unique<CoralFeatureConfig>(blocks::CoralColor::Brain, true, true);
    return std::make_unique<ConfiguredCoralFeature>(std::move(config), "dead_brain_coral");
}

std::unique_ptr<ConfiguredCoralFeature> CoralFeatures::createDeadBubbleCoral()
{
    auto config = std::make_unique<CoralFeatureConfig>(blocks::CoralColor::Bubble, true, true);
    return std::make_unique<ConfiguredCoralFeature>(std::move(config), "dead_bubble_coral");
}

std::unique_ptr<ConfiguredCoralFeature> CoralFeatures::createDeadFireCoral()
{
    auto config = std::make_unique<CoralFeatureConfig>(blocks::CoralColor::Fire, true, true);
    return std::make_unique<ConfiguredCoralFeature>(std::move(config), "dead_fire_coral");
}

std::unique_ptr<ConfiguredCoralFeature> CoralFeatures::createDeadHornCoral()
{
    auto config = std::make_unique<CoralFeatureConfig>(blocks::CoralColor::Horn, true, true);
    return std::make_unique<ConfiguredCoralFeature>(std::move(config), "dead_horn_coral");
}

} // namespace mc
