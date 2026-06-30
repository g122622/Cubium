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

#include "CauldronBlock.hpp"
#include "LavaCauldronBlock.hpp"
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

CauldronBlock::CauldronBlock(const BlockProperties& properties)
    : Block(properties)
{

    // 创建状态容器
    auto container =
        StateContainer<Block, BlockState>::Builder(*this)
            .add(BlockStateProperties::LEVEL_0_3())
            .create([](const Block& block,
                        std::vector<size_t> values,
                        const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
                        const std::vector<BlockState*>* allStates,
                        u32 id) {
                return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id);
            });
    createBlockState(std::move(container));

    // 设置默认状态
    setDefaultState(defaultState().with(BlockStateProperties::LEVEL_0_3(), 0));

    // 炼药锅外部形状：
    // 底部: (0, 0, 0) -> (16, 3, 16)
    // 壁: 4像素厚，内部12x12空间
    // 顶部边缘: 2像素宽

    // 底部
    CollisionShape base = VoxelShapes::cube(0.0f, 0.0f, 0.0f, 1.0f, 3.0f / 16.0f, 1.0f);

    // 四面墙壁
    CollisionShape northWall = VoxelShapes::cube(0.0f, 3.0f / 16.0f, 0.0f, 1.0f, 1.0f, 2.0f / 16.0f);
    CollisionShape southWall = VoxelShapes::cube(0.0f, 3.0f / 16.0f, 14.0f / 16.0f, 1.0f, 1.0f, 1.0f);
    CollisionShape westWall = VoxelShapes::cube(0.0f, 3.0f / 16.0f, 2.0f / 16.0f, 2.0f / 16.0f, 1.0f, 14.0f / 16.0f);
    CollisionShape eastWall = VoxelShapes::cube(14.0f / 16.0f, 3.0f / 16.0f, 2.0f / 16.0f, 1.0f, 1.0f, 14.0f / 16.0f);

    // 合并所有部分
    m_outerShape = CollisionShape::combine(base, northWall, CollisionShape::CombineOp::OR);
    m_outerShape = CollisionShape::combine(m_outerShape, southWall, CollisionShape::CombineOp::OR);
    m_outerShape = CollisionShape::combine(m_outerShape, westWall, CollisionShape::CombineOp::OR);
    m_outerShape = CollisionShape::combine(m_outerShape, eastWall, CollisionShape::CombineOp::OR);

    // 内容形状（水位）
    // 参考 MC 原版 LayeredCauldronBlock:
    //   BASE_CONTENT_HEIGHT = 6, HEIGHT_PER_LEVEL = 3
    //   getPixelContentHeight(level) = 6.0 + level * 3.0
    //   水位1: 6 + 1*3 = 9像素  (0.5625)
    //   水位2: 6 + 2*3 = 12像素 (0.75)
    //   水位3: 6 + 3*3 = 15像素 (0.9375)
    //
    // 内部区域: x=2..14, z=2..14 (12像素宽), y从4像素(=3/16)开始
    // Block.column(12, 4, contentHeight) = box(2, 4, 2, 14, contentHeight, 14)
    constexpr f32 innerMinY = 4.0f / 16.0f;
    constexpr f32 innerX1 = 2.0f / 16.0f;
    constexpr f32 innerX2 = 14.0f / 16.0f;
    constexpr f32 innerZ1 = 2.0f / 16.0f;
    constexpr f32 innerZ2 = 14.0f / 16.0f;

    // 水位0：空
    m_contentShapes[0] = VoxelShapes::empty();

    // 水位1：9像素高 (6 + 1*3 = 9, 即 9/16 = 0.5625)
    constexpr f32 contentHeight1 = 9.0f / 16.0f;
    m_contentShapes[1] = VoxelShapes::cube(innerX1, innerMinY, innerZ1, innerX2, contentHeight1, innerZ2);

    // 水位2：12像素高 (6 + 2*3 = 12, 即 12/16 = 0.75)
    constexpr f32 contentHeight2 = 12.0f / 16.0f;
    m_contentShapes[2] = VoxelShapes::cube(innerX1, innerMinY, innerZ1, innerX2, contentHeight2, innerZ2);

    // 水位3：15像素高 (6 + 3*3 = 15, 即 15/16 = 0.9375)
    constexpr f32 contentHeight3 = 15.0f / 16.0f;
    m_contentShapes[3] = VoxelShapes::cube(innerX1, innerMinY, innerZ1, innerX2, contentHeight3, innerZ2);

    // 填充形状（外部形状 ∪ 内容形状），用于实体内部碰撞检测
    // 参考 MC 原版: LayeredCauldronBlock.FILLED_SHAPES[level - 1]
    //   FILLED_SHAPES[i] = Shapes.or(SHAPE, Block.column(12, 4, contentHeight))
    for (i32 i = 0; i < 3; ++i) {
        m_filledShapes[i] =
            CollisionShape::combine(m_outerShape, m_contentShapes[i + 1], CollisionShape::CombineOp::OR);
    }
}

// ========== 放置和更新 ==========

void CauldronBlock::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    MC_UNUSED(neighborBlock);
    MC_UNUSED(neighborPos);
    MC_UNUSED(isMoving);

    // 炼药锅不需要响应邻居更新
    // 水位变化由交互和雨天填充控制
}

void CauldronBlock::handlePrecipitation(
    IWorld& world, const BlockPos& pos, world::biome::BiomeClimate::Precipitation precipitation)
{
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

    // 只有水位未满时才增加
    i32 level = getLevel(*currentState);
    if (level >= 3) {
        return;
    }

    // 随机概率触发
    if (world.getRandom().nextFloat() >= chance) {
        return;
    }

    // 增加水位
    BlockState newState = currentState->with(BlockStateProperties::LEVEL_0_3(), level + 1);
    world.setBlockState(pos, &newState, 3);

    // 触发 BLOCK_CHANGE 游戏事件，通知附近的幽匿感测体
    world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, &newState);
}

// ========== 交互 ==========

ActionResultType CauldronBlock::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{

    MC_UNUSED(hit);

    // 获取手持物品
    ItemStack& heldItem = player.getHeldItem(hand);

    if (heldItem.isEmpty()) {
        return ActionResultType::Pass;
    }

    // 根据物品类型处理不同的交互
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

const CollisionShape& CauldronBlock::getShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_outerShape;
}

const CollisionShape& CauldronBlock::getCollisionShape(const BlockState& state) const
{
    MC_UNUSED(state);
    return m_outerShape;
}

const CollisionShape& CauldronBlock::getContentShape(i32 level) const
{
    if (level < 0 || level > 3) {
        return VoxelShapes::empty();
    }
    return m_contentShapes[static_cast<size_t>(level)];
}

const CollisionShape& CauldronBlock::getEntityInsideCollisionShape(const BlockState& state) const
{
    i32 level = getLevel(state);
    if (level <= 0) {
        // 空炼药锅：继承默认行为，返回完整方块形状
        // MC 原版中空炼药锅 (CauldronBlock) 继承 AbstractCauldronBlock 的默认行为，
        // 即 getEntityInsideCollisionShape 返回 Shapes.block()
        return VoxelShapes::fullCube();
    }
    // 有水时返回与水位对应的填充形状
    // 参考 MC 原版: LayeredCauldronBlock.FILLED_SHAPES[level - 1]
    return m_filledShapes[static_cast<size_t>(level - 1)];
}

// ========== 实体碰撞 ==========

void CauldronBlock::onEntityCollision(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    i32 level = getLevel(state);
    if (level <= 0) {
        // 空炼药锅，无实体碰撞效果
        return;
    }

    // 有水的炼药锅：着火的实体会被灭火，同时水位降低1级
    // 参考 MC 原版: LayeredCauldronBlock#entityInside
    if (entity.isOnFire() && entity.mayInteract(world, pos)) {
        // 灭火并降低水位
        entity.clearFire();
        if (!world.isClientSide()) {
            setLevel(world, pos, state, level - 1);
        }
    }
}

// ========== 红石 ==========

i32 CauldronBlock::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{

    MC_UNUSED(world);
    MC_UNUSED(pos);

    // 比较器信号 = 水位
    return getLevel(state);
}

// ========== 静态工具方法 ==========

i32 CauldronBlock::getLevel(const BlockState& state)
{
    return state.get(BlockStateProperties::LEVEL_0_3());
}

void CauldronBlock::setLevel(IWorld& world, const BlockPos& pos, const BlockState& state, i32 level)
{
    if (level < 0) level = 0;
    if (level > 3) level = 3;

    i32 currentLevel = getLevel(state);
    if (currentLevel != level) {
        BlockState newState = state.with(BlockStateProperties::LEVEL_0_3(), level);
        world.setBlockState(pos, &newState, 3);
    }
}

bool CauldronBlock::isEmpty(const BlockState& state)
{
    return getLevel(state) == 0;
}

bool CauldronBlock::isFull(const BlockState& state)
{
    return getLevel(state) == 3;
}

// ========== 私有方法 ==========

ActionResultType CauldronBlock::_handleBucketInteraction(
    IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem)
{

    const Item* item = heldItem.getItem();
    if (item == nullptr) {
        return ActionResultType::Pass;
    }

    i32 currentLevel = getLevel(state);

    // 水桶：装水到空的或未满的炼药锅
    if (item == Items::WATER_BUCKET) {
        if (currentLevel < 3 && !world.isClientSide()) {
            // 水桶装水：空炼药锅 -> 满炼药锅
            setLevel(world, pos, state, 3);
            world.playSound(SoundEvents::ITEM_BUCKET_EMPTY,
                sound::SoundCategory::Blocks,
                Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
                1.0f,
                1.0f);

            // 非创造模式：替换为空桶
            if (!player.abilities().creativeMode) {
                heldItem.shrink(1);
                if (heldItem.isEmpty()) {
                    heldItem = ItemStack(Items::BUCKET, 1);
                    player.inventory().setChanged();
                } else {
                    // 尝试添加空桶到背包
                    ItemStack emptyBucket(Items::BUCKET, 1);
                    player.inventory().add(emptyBucket);
                    if (!emptyBucket.isEmpty()) {
                        // 背包满了，在玩家位置掉落物品
                        ItemDropHelper::spawnItemAtEntity(&player, emptyBucket, 0.5f, world.getRandom());
                    }
                }
            }
        }
        return ActionResultType::Success;
    }

    // 岩浆桶：装岩浆到空炼药锅
    if (item == Items::LAVA_BUCKET) {
        if (currentLevel == 0 && !world.isClientSide()) {
            // 岩浆桶倒入空炼药锅：空炼药锅 → 岩浆炼药锅
            const BlockState* lavaCauldronState = &block_registry::BuildingBlocks::LAVA_CAULDRON->defaultState();
            world.setBlockState(pos, lavaCauldronState, 3);
            world.playSound(SoundEvents::ITEM_BUCKET_EMPTY_LAVA,
                sound::SoundCategory::Blocks,
                Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
                1.0f,
                1.0f);

            // 触发 FLUID_PLACE 游戏事件
            world.gameEvent(gameevent::GameEvents::FLUID_PLACE, pos, lavaCauldronState);

            // 非创造模式：替换为空桶
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
            return ActionResultType::Success;
        }
        // 非空炼药锅不能用岩浆桶交互，返回 Pass 让其他交互继续处理
        return ActionResultType::Pass;
    }

    // 空桶：从满的炼药锅取水
    if (item == Items::BUCKET) {
        if (currentLevel == 3 && !world.isClientSide()) {
            // 空桶取水：满炼药锅 -> 空炼药锅
            setLevel(world, pos, state, 0);
            world.playSound(SoundEvents::ITEM_BUCKET_FILL,
                sound::SoundCategory::Blocks,
                Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
                1.0f,
                1.0f);

            // 非创造模式：替换为水桶
            if (!player.abilities().creativeMode) {
                heldItem.shrink(1);
                if (heldItem.isEmpty()) {
                    heldItem = ItemStack(Items::WATER_BUCKET, 1);
                    player.inventory().setChanged();
                } else {
                    // 尝试添加水桶到背包
                    ItemStack waterBucket(Items::WATER_BUCKET, 1);
                    player.inventory().add(waterBucket);
                    if (!waterBucket.isEmpty()) {
                        // 背包满了，在玩家位置掉落物品
                        ItemDropHelper::spawnItemAtEntity(&player, waterBucket, 0.5f, world.getRandom());
                    }
                }
            }
        }
        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

ActionResultType CauldronBlock::_handleBottleInteraction(
    IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem)
{

    const Item* item = heldItem.getItem();
    if (item == nullptr) {
        return ActionResultType::Pass;
    }

    i32 currentLevel = getLevel(state);

    // 玻璃瓶：从炼药锅取水
    if (item == Items::GLASS_BOTTLE) {
        if (currentLevel > 0 && !world.isClientSide()) {
            // 创建水瓶
            ItemStack waterBottle = potion::PotionUtils::createPotionItem(potion::Potions::WATER);

            // 降低水位
            setLevel(world, pos, state, currentLevel - 1);

            world.playSound(SoundEvents::ITEM_BOTTLE_FILL,
                sound::SoundCategory::Blocks,
                Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
                1.0f,
                1.0f);

            // 非创造模式：替换为水瓶
            if (!player.abilities().creativeMode) {
                heldItem.shrink(1);
                if (heldItem.isEmpty()) {
                    heldItem = waterBottle;
                    player.inventory().setChanged();
                } else {
                    // 尝试添加水瓶到背包
                    player.inventory().add(waterBottle);
                    if (!waterBottle.isEmpty()) {
                        // 背包满了，在玩家位置掉落物品
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

            // 非创造模式：替换为玻璃瓶
            if (!player.abilities().creativeMode) {
                ItemStack glassBottle(Items::GLASS_BOTTLE, 1);
                heldItem.shrink(1);
                if (heldItem.isEmpty()) {
                    heldItem = glassBottle;
                    player.inventory().setChanged();
                } else {
                    // 尝试添加玻璃瓶到背包
                    player.inventory().add(glassBottle);
                    if (!glassBottle.isEmpty()) {
                        // 背包满了，在玩家位置掉落物品
                        ItemDropHelper::spawnItemAtEntity(&player, glassBottle, 0.5f, world.getRandom());
                    }
                }
            }
        }
        return ActionResultType::Success;
    }

    return ActionResultType::Pass;
}

ActionResultType CauldronBlock::_handleLeatherArmorCleaning(
    IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem)
{

    const Item* item = heldItem.getItem();
    if (item == nullptr) {
        return ActionResultType::Pass;
    }

    // 检查是否为皮革盔甲且有颜色
    const auto* dyeableArmor = dynamic_cast<const item::items::DyeableArmorItem*>(item);
    if (dyeableArmor != nullptr) {
        i32 currentLevel = getLevel(state);

        // 检查是否有自定义颜色且炼药锅有水
        if (currentLevel > 0 && item::items::DyeableArmorItem::hasColor(heldItem)) {
            if (!world.isClientSide()) {
                // 清除颜色
                item::items::DyeableArmorItem::clearColor(heldItem);

                // 降低水位
                setLevel(world, pos, state, currentLevel - 1);

                // 触发 BLOCK_CHANGE 游戏事件（通知附近的幽匿感测体）
                const BlockState* newState = world.getBlockState(pos);
                if (newState != nullptr) {
                    world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, newState);
                }
            }
            return ActionResultType::Success;
        }
    }

    return ActionResultType::Pass;
}

ActionResultType CauldronBlock::_handleBannerCleaning(
    IWorld& world, const BlockPos& pos, const BlockState& state, Player& player, ItemStack& heldItem)
{
    const Item* item = heldItem.getItem();
    if (item == nullptr) {
        return ActionResultType::Pass;
    }

    // 检查是否为旗帜或盾牌，且物品上有图案层
    // 参考 MC 1.21.11 CauldronInteraction.WATER 中的 bannerInteraction
    const bool isBanner = dynamic_cast<const item::BannerItem*>(item) != nullptr;
    // TODO(shield_cauldron): MC 原版 CauldronInteraction.WATER 中仅注册了16种旗帜物品的清洗交互，
    // 盾牌未注册。但盾牌使用与旗帜相同的 BlockEntityTag.Patterns NBT 结构
    // （参见 ShieldDecorationRecipe），且项目已有 BannerEntity::removeBannerData() 方法
    // 可统一处理。此处扩展支持盾牌清洗以保持功能完整性，如需严格对齐原版可移除此分支。
    const bool isShield = (item == Items::SHIELD);

    if (!isBanner && !isShield) {
        return ActionResultType::Pass;
    }

    // 检查物品是否有图案层（BlockEntityTag.Patterns 非空）
    i32 patternCount = blockentity::BannerEntity::getPatternCount(heldItem);
    if (patternCount <= 0) {
        // 没有图案的旗帜/盾牌，不执行清洗
        return ActionResultType::Pass;
    }

    i32 currentLevel = getLevel(state);
    if (currentLevel <= 0) {
        // 炼药锅为空，无法清洗
        return ActionResultType::Pass;
    }

    // 服务端执行清洗逻辑
    if (!world.isClientSide()) {
        // 移除最顶层图案（MC 原版使用 BannerPatternLayers.removeLast()）
        // 如果图案全部移除，BlockEntityTag 也会被自动清除
        blockentity::BannerEntity::removeBannerData(heldItem);

        // 降低炼药锅水位
        setLevel(world, pos, state, currentLevel - 1);

        // 触发 BLOCK_CHANGE 游戏事件（通知附近的幽匿感测体）
        const BlockState* newState = world.getBlockState(pos);
        if (newState != nullptr) {
            world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, newState);
        }
    }

    return ActionResultType::Success;
}

void CauldronBlock::_playFillSound(IWorld& world, const BlockPos& pos)
{
    world.playSound(SoundEvents::ITEM_BUCKET_FILL,
        sound::SoundCategory::Blocks,
        Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
        1.0f,
        1.0f);
}

void CauldronBlock::_playEmptySound(IWorld& world, const BlockPos& pos)
{
    world.playSound(SoundEvents::ITEM_BUCKET_EMPTY,
        sound::SoundCategory::Blocks,
        Vector3(static_cast<f32>(pos.x) + 0.5f, static_cast<f32>(pos.y), static_cast<f32>(pos.z) + 0.5f),
        1.0f,
        1.0f);
}

// ========== 滴石填充 ==========

void CauldronBlock::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    MC_UNUSED(random);

    // 当滴石调度了炼药锅的 tick 时，重新验证上方是否存在可滴水的钟乳石尖端
    // 参考 MC 原版 AbstractCauldronBlock.tick()：
    // 1. 向上搜索可滴水的钟乳石尖端
    // 2. 从尖端确定流体类型
    // 3. 如果炼药锅可以接收该流体，则执行滴水填充
    std::optional<BlockPos> tipPos = PointedDripstoneBlock::findStalactiteTipAboveCauldron(world, pos);
    if (tipPos.has_value()) {
        const fluid::Fluid* fluid = PointedDripstoneBlock::getCauldronFillFluidType(world, tipPos.value());
        if (fluid != nullptr && fluid != fluid::Fluids::EMPTY() && canReceiveStalactiteDrip(*fluid)) {
            receiveStalactiteDrip(world, pos, state, *fluid);
        }
    }
}

bool CauldronBlock::canReceiveStalactiteDrip(const fluid::Fluid& fluid)
{
    // 空炼药锅可以接收任何流体（水和岩浆）的滴石滴水
    MC_UNUSED(fluid);
    return true;
}

void CauldronBlock::receiveStalactiteDrip(
    IWorld& world, const BlockPos& pos, const BlockState& state, const fluid::Fluid& fluid)
{
    if (fluid.isIn(fluid::FluidTags::WATER())) {
        // 水滴：每次增加1级水位
        // TODO: 当实现 WaterCauldronBlock (LayeredCauldronBlock) 后，
        // 空炼药锅接收水滴应替换为水位1的 WaterCauldronBlock，
        // 而非在当前 CauldronBlock 上递增水位
        i32 currentLevel = getLevel(state);
        i32 newLevel = currentLevel + 1;
        if (newLevel <= 3) {
            BlockState newState = state.with(BlockStateProperties::LEVEL_0_3(), newLevel);
            world.setBlockState(pos, &newState, 3);
            world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, &newState);
            world.playEvent(world::WorldEvents::DRIP_WATER_INTO_CAULDRON_SOUND, pos, 0);
        }
    } else if (fluid.isIn(fluid::FluidTags::LAVA())) {
        // 岩浆滴：空炼药锅 → 替换为岩浆炼药锅
        const BlockState* lavaCauldronState = &block_registry::BuildingBlocks::LAVA_CAULDRON->defaultState();
        world.setBlockState(pos, lavaCauldronState, 3);
        world.gameEvent(gameevent::GameEvents::BLOCK_CHANGE, pos, lavaCauldronState);
        world.playEvent(world::WorldEvents::DRIP_LAVA_INTO_CAULDRON_SOUND, pos, 0);
    }
}

} // namespace blocks
} // namespace mc
