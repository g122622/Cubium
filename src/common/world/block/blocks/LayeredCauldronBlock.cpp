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
 * LIABILITY, CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "LayeredCauldronBlock.hpp"
#include "common/entity/core/LivingEntity.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/Items.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/items/armor/DyeableArmorItem.hpp"
#include "common/item/items/block/BannerItem.hpp"
#include "common/item/items/weapon/ShieldItem.hpp"
#include "common/item/potion/PotionUtils.hpp"
#include "common/item/potion/Potions.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/sound/SoundEvents.hpp"
#include "common/util/property/Properties.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/WorldEvents.hpp"
#include "common/world/block/blocks/cave/PointedDripstoneBlock.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"
#include "common/world/blockentity/interactive/BannerEntity.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/fluid/Fluids.hpp"
#include "common/world/gameevent/GameEvents.hpp"

namespace mc {
namespace blocks {

// ========== 构造函数 ==========

LayeredCauldronBlock::LayeredCauldronBlock(
    const BlockProperties& properties, world::biome::BiomeClimate::Precipitation precipitationType)
    : Block(properties)
    , m_precipitationType(precipitationType)
{
    // 创建状态容器，水位属性范围 1-3
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::LEVEL_1_3())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 默认水位为 1（最低有效水位）
    setDefaultState(defaultState().with(BlockStateProperties::LEVEL_1_3(), 1));

    // 炼药锅外部形状（与空炼药锅和岩浆炼药锅共享相同的几何形状）
    // 底部: (0, 0, 0) -> (16, 3, 16)
    // 壁: 2像素厚，内部12x12空间
    CollisionShape base = VoxelShapes::cube(0.0f, 0.0f, 0.0f, 1.0f, 3.0f / 16.0f, 1.0f);
    CollisionShape northWall = VoxelShapes::cube(0.0f, 3.0f / 16.0f, 0.0f, 1.0f, 1.0f, 2.0f / 16.0f);
    CollisionShape southWall = VoxelShapes::cube(0.0f, 3.0f / 16.0f, 14.0f / 16.0f, 1.0f, 1.0f, 1.0f);
    CollisionShape westWall = VoxelShapes::cube(0.0f, 3.0f / 16.0f, 2.0f / 16.0f, 2.0f / 16.0f, 1.0f, 14.0f / 16.0f);
    CollisionShape eastWall = VoxelShapes::cube(14.0f / 16.0f, 3.0f / 16.0f, 2.0f / 16.0f, 1.0f, 1.0f, 14.0f / 16.0f);

    m_outerShape = CollisionShape::combine(base, northWall, CollisionShape::CombineOp::OR);
    m_outerShape = CollisionShape::combine(m_outerShape, southWall, CollisionShape::CombineOp::OR);
    m_outerShape = CollisionShape::combine(m_outerShape, westWall, CollisionShape::CombineOp::OR);
    m_outerShape = CollisionShape::combine(m_outerShape, eastWall, CollisionShape::CombineOp::OR);

    // 内容形状（水位 1-3）
    //   BASE_CONTENT_HEIGHT = 6, HEIGHT_PER_LEVEL = 3
    //   getContentHeight(level) = (6 + level * 3) / 16.0
    //   水位1: 6 + 1*3 = 9像素  (9/16 = 0.5625)
    //   水位2: 6 + 2*3 = 12像素 (12/16 = 0.75)
    //   水位3: 6 + 3*3 = 15像素 (15/16 = 0.9375)
    constexpr f32 innerMinY = 4.0f / 16.0f;
    constexpr f32 innerX1 = 2.0f / 16.0f;
    constexpr f32 innerX2 = 14.0f / 16.0f;
    constexpr f32 innerZ1 = 2.0f / 16.0f;
    constexpr f32 innerZ2 = 14.0f / 16.0f;

    // 水位1：9像素高
    constexpr f32 contentHeight1 = 9.0f / 16.0f;
    m_contentShapes[0] = VoxelShapes::cube(innerX1, innerMinY, innerZ1, innerX2, contentHeight1, innerZ2);

    // 水位2：12像素高
    constexpr f32 contentHeight2 = 12.0f / 16.0f;
    m_contentShapes[1] = VoxelShapes::cube(innerX1, innerMinY, innerZ1, innerX2, contentHeight2, innerZ2);

    // 水位3：15像素高
    constexpr f32 contentHeight3 = 15.0f / 16.0f;
    m_contentShapes[2] = VoxelShapes::cube(innerX1, innerMinY, innerZ1, innerX2, contentHeight3, innerZ2);

    // 填充形状（外部形状 ∪ 内容形状），用于实体内部碰撞检测
    for (i32 i = 0; i < 3; ++i) {
        m_filledShapes[i] = CollisionShape::combine(m_outerShape, m_contentShapes[i], CollisionShape::CombineOp::OR);
    }
}

// ========== 放置和更新 ==========

void LayeredCauldronBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 分层炼药锅不需要响应邻居更新
}

void LayeredCauldronBlock::handlePrecipitation(
    IWorld& world, const BlockPos& pos, world::biome::BiomeClimate::Precipitation precipitation)
{
    // 仅当降水类型与炼药锅类型匹配时才处理
    if (precipitation != m_precipitationType) {
        return;
    }

    // 确定降水触发概率：雨天 5%，雪天 10%
    f32 chance = 0.0f;
    if (precipitation == world::biome::BiomeClimate::Precipitation::Rain) {
        chance = 0.05f;
    } else if (precipitation == world::biome::BiomeClimate::Precipitation::Snow) {
        chance = 0.1f;
    }

    if (chance <= 0.0f) {
        return;
    }

    const BlockState* currentState = world.getBlockState(pos);
    if (currentState == nullptr) {
        return;
    }

    // 水位未满时才增加
    if (isFull(*currentState)) {
        return;
    }

    // 随机概率触发
    if (world.getRandom().nextFloat() >= chance) {
        return;
    }

    // 增加水位（使用 cycle 语义：level + 1，但不超过3）
    i32 level = getLevel(*currentState);
    if (level < 3) {
        BlockState newState = currentState->with(BlockStateProperties::LEVEL_1_3(), level + 1);
        world.setBlockState(pos, &newState, 3);
        world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, &newState);
    }
}

// ========== 交互 ==========

ActionResultType LayeredCauldronBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    MC_UNUSED(hit);

    ItemStack& heldItem = player.getHeldItem(hand);
    if (heldItem.isEmpty()) {
        return ActionResultType::Pass;
    }

    ActionResultType result = ActionResultType::Pass;

    // 水桶交互
    result = _handleBucketInteraction(world, pos, state, player, heldItem);
    if (result != ActionResultType::Pass) {
        return result;
    }

    // 玻璃瓶交互
    result = _handleBottleInteraction(world, pos, state, player, heldItem);
    if (result != ActionResultType::Pass) {
        return result;
    }

    // 皮革盔甲清洗
    result = _handleLeatherArmorCleaning(world, pos, state, player, heldItem);
    if (result != ActionResultType::Pass) {
        return result;
    }

    // 旗帜清洗
    result = _handleBannerCleaning(world, pos, state, player, heldItem);
    if (result != ActionResultType::Pass) {
        return result;
    }

    return ActionResultType::Pass;
}

// ========== 形状 ==========

const CollisionShape& LayeredCauldronBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_outerShape;
}

const CollisionShape& LayeredCauldronBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_outerShape;
}

const CollisionShape& LayeredCauldronBlock::getContentShape(i32 level) const
{
    if (level < 1 || level > 3) {
        return VoxelShapes::empty();
    }
    return m_contentShapes[static_cast<size_t>(level - 1)];
}

const CollisionShape& LayeredCauldronBlock::getEntityInsideCollisionShape(const BlockState& state) const
{
    i32 level = getLevel(state);
    // level 范围 1-3，对应 filledShapes 索引 0-2
    return m_filledShapes[static_cast<size_t>(level - 1)];
}

// ========== 实体碰撞 ==========

void LayeredCauldronBlock::onEntityCollision(
    const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    // 着火的实体会被灭火，同时水位降低1级
    if (entity.isOnFire() && entity.mayInteract(world, pos)) {
        entity.clearFire();
        if (!world.isClientSide()) {
            // TODO(_powder_snow_cauldron): 当细雪炼药锅实现后，
            // 如果 m_precipitationType == Snow，灭火时先将细雪炼药锅转换为水炼药锅
            // （保持相同水位），然后降低水位。
            // 当前仅支持水炼药锅，直接降低水位即可。
            lowerFillLevel(world, pos, state);
        }
    }
}

// ========== 红石 ==========

i32 LayeredCauldronBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 比较器信号 = 水位 (1-3)
    return getLevel(state);
}

// ========== 静态工具方法 ==========

i32 LayeredCauldronBlock::getLevel(const BlockState& state)
{
    return state.get(BlockStateProperties::LEVEL_1_3());
}

void LayeredCauldronBlock::setLevel(IWorld& world, const BlockPos& pos, const BlockState& state, i32 level)
{
    if (level < 1) level = 1;
    if (level > 3) level = 3;

    i32 currentLevel = getLevel(state);
    if (currentLevel != level) {
        BlockState newState = state.with(BlockStateProperties::LEVEL_1_3(), level);
        world.setBlockState(pos, &newState, 3);
    }
}

void LayeredCauldronBlock::lowerFillLevel(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    i32 level = getLevel(state);
    if (level <= 1) {
        // 水位降至0，替换为空炼药锅
        const BlockState* cauldronState = &block_registry::BuildingBlocks::CAULDRON->defaultState();
        world.setBlockState(pos, cauldronState, 3);
        world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, cauldronState);
    } else {
        // 降低水位1级
        BlockState newState = state.with(BlockStateProperties::LEVEL_1_3(), level - 1);
        world.setBlockState(pos, &newState, 3);
        world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, &newState);
    }
}

bool LayeredCauldronBlock::isFull(const BlockState& state)
{
    return getLevel(state) == 3;
}

// ========== 滴石填充 ==========

void LayeredCauldronBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    // 重新验证上方钟乳石尖端和流体类型
    std::optional<BlockPos> tipPos = PointedDripstoneBlock::findStalactiteTipAboveCauldron(world, pos);
    if (tipPos.has_value()) {
        const fluid::Fluid* fluid = PointedDripstoneBlock::getCauldronFillFluidType(world, tipPos.value());
        if (fluid != nullptr && fluid != fluid::Fluids::EMPTY() && canReceiveStalactiteDrip(*fluid)) {
            receiveStalactiteDrip(world, pos, state, *fluid);
        }
    }
}

bool LayeredCauldronBlock::canReceiveStalactiteDrip(const fluid::Fluid& fluid) const
{
    // 仅水炼药锅可接收水滴，且水位未满时
    // 注：此方法在静态上下文中被调用时需要额外检查水位，但此处仅检查流体类型和降水类型
    return fluid.isIn(fluid::FluidTags::WATER()) &&
        m_precipitationType == world::biome::BiomeClimate::Precipitation::Rain;
}

void LayeredCauldronBlock::receiveStalactiteDrip(
    IWorld& world, const BlockPos& pos, const BlockState& state, const fluid::Fluid& fluid)
{
    // 水滴：增加1级水位（如果未满）
    if (fluid.isIn(fluid::FluidTags::WATER()) && !isFull(state)) {
        i32 level = getLevel(state);
        BlockState newState = state.with(BlockStateProperties::LEVEL_1_3(), level + 1);
        world.setBlockState(pos, &newState, 3);
        world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, &newState);
        world.playEvent(world::WorldEvents::DRIP_WATER_INTO_CAULDRON_SOUND, pos, 0);
    }
}

// ========== 私有方法 ==========

ActionResultType LayeredCauldronBlock::_handleBucketInteraction(
    IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem)
{
    const Item* item = heldItem.getItem();
    if (item == nullptr) {
        return ActionResultType::Pass;
    }

    i32 currentLevel = getLevel(state);

    // 水桶：装满到水位3（如果未满）
    if (item == Items::WATER_BUCKET) {
        if (currentLevel < 3 && !world.isClientSide()) {
            setLevel(world, pos, state, 3);
            world.playSound(SoundEvents::ITEM_BUCKET_EMPTY,
                sound::SoundCategory::Blocks,
                Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
                1.0f,
                1.0f);

            if (!player.abilities().creativeMode) {
                heldItem.shrink(1);
                if (heldItem.isEmpty()) {
                    heldItem = ItemStack(Items::BUCKET, 1);
                    player.inventory().setChanged();
                } else {
                    ItemStack emptyBucket(Items::BUCKET, 1);
                    player.inventory().add(emptyBucket);
                    if (!emptyBucket.isEmpty()) {
                        ItemDropHelper::spawnItemAtEntity(&player, emptyBucket, 0.5f, world.getRandom());
                    }
                }
            }
        }
        return ActionResultType::Success;
    }

    // 空桶：从满的炼药锅取水（仅水位3时可用）
    if (item == Items::BUCKET) {
        if (currentLevel == 3 && !world.isClientSide()) {
            // 取水后替换为空炼药锅
            const BlockState* cauldronState = &block_registry::BuildingBlocks::CAULDRON->defaultState();
            world.setBlockState(pos, cauldronState, 3);
            world.playSound(SoundEvents::ITEM_BUCKET_FILL,
                sound::SoundCategory::Blocks,
                Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
                1.0f,
                1.0f);
            world.gameEvent(gameevent::GameEvents::FLUID_PICKUP, pos, cauldronState);

            if (!player.abilities().creativeMode) {
                heldItem.shrink(1);
                if (heldItem.isEmpty()) {
                    heldItem = ItemStack(Items::WATER_BUCKET, 1);
                    player.inventory().setChanged();
                } else {
                    ItemStack waterBucket(Items::WATER_BUCKET, 1);
                    player.inventory().add(waterBucket);
                    if (!waterBucket.isEmpty()) {
                        ItemDropHelper::spawnItemAtEntity(&player, waterBucket, 0.5f, world.getRandom());
                    }
                }
            }
        }
        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

ActionResultType LayeredCauldronBlock::_handleBottleInteraction(
    IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem)
{
    const Item* item = heldItem.getItem();
    if (item == nullptr) {
        return ActionResultType::Pass;
    }

    i32 currentLevel = getLevel(state);

    // 玻璃瓶：从炼药锅取水
    if (item == Items::GLASS_BOTTLE) {
        if (!world.isClientSide()) {
            // 创建水瓶
            ItemStack waterBottle = potion::PotionUtils::createPotionItem(potion::Potions::WATER);

            // 降低水位（可能替换为空炼药锅）
            lowerFillLevel(world, pos, state);

            world.playSound(SoundEvents::ITEM_BOTTLE_FILL,
                sound::SoundCategory::Blocks,
                Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
                1.0f,
                1.0f);

            if (!player.abilities().creativeMode) {
                heldItem.shrink(1);
                if (heldItem.isEmpty()) {
                    heldItem = waterBottle;
                    player.inventory().setChanged();
                } else {
                    player.inventory().add(waterBottle);
                    if (!waterBottle.isEmpty()) {
                        ItemDropHelper::spawnItemAtEntity(&player, waterBottle, 0.5f, world.getRandom());
                    }
                }
            }
        }
        return ActionResultType::Success;
    }

    // 水瓶：向炼药锅倒水
    if (item == Items::POTION && potion::PotionUtils::isWaterBottle(heldItem)) {
        if (currentLevel < 3 && !world.isClientSide()) {
            // 增加水位
            setLevel(world, pos, state, currentLevel + 1);

            world.playSound(SoundEvents::ITEM_BOTTLE_EMPTY,
                sound::SoundCategory::Blocks,
                Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
                1.0f,
                1.0f);

            if (!player.abilities().creativeMode) {
                ItemStack glassBottle(Items::GLASS_BOTTLE, 1);
                heldItem.shrink(1);
                if (heldItem.isEmpty()) {
                    heldItem = glassBottle;
                    player.inventory().setChanged();
                } else {
                    player.inventory().add(glassBottle);
                    if (!glassBottle.isEmpty()) {
                        ItemDropHelper::spawnItemAtEntity(&player, glassBottle, 0.5f, world.getRandom());
                    }
                }
            }
        }
        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

ActionResultType LayeredCauldronBlock::_handleLeatherArmorCleaning(
    IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem)
{
    MC_UNUSED(player);

    const Item* item = heldItem.getItem();
    if (item == nullptr) {
        return ActionResultType::Pass;
    }

    // 检查是否为皮革盔甲且有颜色
    const auto* dyeableArmor = dynamic_cast<const item::items::DyeableArmorItem*>(item);
    if (dyeableArmor != nullptr && item::items::DyeableArmorItem::hasColor(heldItem)) {
        if (!world.isClientSide()) {
            // 清除颜色
            item::items::DyeableArmorItem::clearColor(heldItem);

            // 降低水位
            lowerFillLevel(world, pos, state);

            // 触发 BLOCK_CHANGE 游戏事件
            const BlockState* newState = world.getBlockState(pos);
            if (newState != nullptr) {
                world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, newState);
            }
        }
        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

ActionResultType LayeredCauldronBlock::_handleBannerCleaning(
    IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem)
{
    MC_UNUSED(player);

    const Item* item = heldItem.getItem();
    if (item == nullptr) {
        return ActionResultType::Pass;
    }

    // 检查是否为旗帜或盾牌，且物品上有图案层
    const bool isBanner = dynamic_cast<const item::BannerItem*>(item) != nullptr;
    const bool isShield = (item == Items::SHIELD);

    if (!isBanner && !isShield) {
        return ActionResultType::Pass;
    }

    i32 patternCount = blockentity::BannerEntity::getPatternCount(heldItem);
    if (patternCount <= 0) {
        return ActionResultType::Pass;
    }

    // 服务端执行清洗逻辑
    if (!world.isClientSide()) {
        blockentity::BannerEntity::removeBannerData(heldItem);

        // 降低水位
        lowerFillLevel(world, pos, state);

        // 触发 BLOCK_CHANGE 游戏事件
        const BlockState* newState = world.getBlockState(pos);
        if (newState != nullptr) {
            world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, newState);
        }
    }

    return ActionResultType::Success;
}

} // namespace blocks
} // namespace mc
