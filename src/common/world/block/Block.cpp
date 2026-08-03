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

#include "Block.hpp"
#include "../../entity/core/Entity.hpp"
#include "../../entity/damage/DamageSource.hpp"
#include "../../entity/entities/player/Player.hpp"
#include "../../entity/utils/ItemDropHelper.hpp"
#include "../../item/context/BlockItemUseContext.hpp"
#include "../../item/context/ItemUseContext.hpp"
#include "../../item/core/ItemStack.hpp"
#include "../../item/loot/LootTable.hpp"
#include "../../item/loot/LootTableManager.hpp"
#include "../../item/loot/context/LootContextBuilder.hpp"
#include "../../item/loot/context/LootParameterSets.hpp"
#include "../../item/loot/context/LootParams.hpp"
#include "../../physics/shape/BooleanOp.hpp"
#include "../../physics/shape/Shapes.hpp"
#include "../../physics/shape/VoxelShape.hpp"
#include "../../sound/SoundCategory.hpp"
#include "../../util/Direction.hpp"
#include "../../util/math/Vector3.hpp"
#include "../../util/math/random/IRandom.hpp"
#include "../IWorld.hpp"
#include "../blockentity/BlockEntity.hpp"
#include "../fluid/Fluid.hpp"
#include "../fluid/FluidRegistry.hpp"
#include "BlockPos.hpp"
#include "BlockRegistry.hpp"
#include "BlockSoundType.hpp"
#include "BlockState.hpp"
#include "BlockTags.hpp"
#include "FireInfoRegistry.hpp"
#include "Material.hpp"
#include "PlantType.hpp"
#include "SupportType.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/util/PiglinAi.hpp"
#include "common/entity/core/MoverType.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/block/IBlockAnimateContext.hpp"
#include "common/world/map/MaterialColor.hpp"
#include "registry/VanillaBlocks.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {

// ============================================================================
// VoxelShapes
// ============================================================================

namespace {
CollisionShape g_emptyShape = CollisionShape::empty();
CollisionShape g_fullBlockShape = CollisionShape::fullBlock();
} // namespace

const CollisionShape& VoxelShapes::empty()
{
    return g_emptyShape;
}

const CollisionShape& VoxelShapes::fullCube()
{
    return g_fullBlockShape;
}

CollisionShape VoxelShapes::cube(f32 x1, f32 y1, f32 z1, f32 x2, f32 y2, f32 z2)
{
    // 兼容两种写法：
    // 1) 像素坐标（0-16）- 与 Java 版 Block.makeCuboidShape 一致
    // 2) 归一化坐标（0-1）- 项目内已有大量此写法
    const f32 maxCoord = std::max({std::abs(x1), std::abs(y1), std::abs(z1), std::abs(x2), std::abs(y2), std::abs(z2)});

    if (maxCoord <= 1.0f) {
        return CollisionShape::box(x1, y1, z1, x2, y2, z2);
    }

    return CollisionShape::fromPixelBox(x1, y1, z1, x2, y2, z2);
}

// ============================================================================
// BlockProperties
// ============================================================================

namespace {
/**
 * @brief 根据材质获取默认的声音类型
 */
const BlockSoundType& getDefaultSoundType(const Material& material) noexcept
{
    // 木头材质 -> 木头声音
    if (&material == &Material::WOOD || &material == &Material::NETHER_WOOD) {
        return BlockSoundTypes::WOOD;
    }
    // 泥土材质 -> 泥土声音
    if (&material == &Material::EARTH) {
        return BlockSoundTypes::DIRT;
    }
    // 草材质 -> 草声音
    if (&material == &Material::PLANT || &material == &Material::REPLACEABLE_PLANT || &material == &Material::LEAVES ||
        &material == &Material::TALL_PLANTS || &material == &Material::OCEAN_PLANT ||
        &material == &Material::SEA_GRASS || &material == &Material::MOSS) {
        return BlockSoundTypes::GRASS;
    }
    // 沙子材质 -> 沙子声音
    if (&material == &Material::SAND) {
        return BlockSoundTypes::SAND;
    }
    // 玻璃材质 -> 玻璃声音
    if (&material == &Material::GLASS || &material == &Material::ICE) {
        return BlockSoundTypes::GLASS;
    }
    // 金属材质 -> 金属声音
    if (&material == &Material::IRON) {
        return BlockSoundTypes::METAL;
    }
    // 雪材质 -> 雪声音
    if (&material == &Material::SNOW) {
        return BlockSoundTypes::SNOW;
    }
    // 羊毛材质 -> 羊毛声音
    if (&material == &Material::WOOL) {
        return BlockSoundTypes::WOOL;
    }
    // 水材质 -> 水声音
    if (&material == &Material::WATER) {
        return BlockSoundTypes::WATER;
    }
    // 岩浆材质 -> 岩浆声音
    if (&material == &Material::LAVA) {
        return BlockSoundTypes::LAVA;
    }
    // 空气和其他材质 -> 默认石头声音
    return BlockSoundTypes::STONE;
}
} // namespace

BlockProperties::BlockProperties(const Material& material)
    : m_material(&material)
    , m_hardness(0.0f)
    , m_resistance(0.0f)
    , m_lightLevel(0)
    , m_hasCollision(material.blocksMovement())
    , m_isSolid(material.isSolid())
    , m_isFlammable(material.isFlammable())
    , m_requiresTool(false)
    , m_isReplaceable(material.isReplaceable())
    , m_soundType(&getDefaultSoundType(material))
{}

// ============================================================================
// Block
// ============================================================================

Block* Block::getBlock(u32 blockId)
{
    return BlockRegistry::instance().getBlock(blockId);
}

Block* Block::getBlock(const ResourceLocation& id)
{
    return BlockRegistry::instance().getBlock(id);
}

BlockState* Block::getBlockState(u32 stateId)
{
    return BlockRegistry::instance().getBlockState(stateId);
}

void Block::forEachBlock(std::function<void(Block&)> callback)
{
    BlockRegistry::instance().forEachBlock(std::move(callback));
}

void Block::forEachBlockState(std::function<void(const BlockState&)> callback)
{
    BlockRegistry::instance().forEachBlockState(std::move(callback));
}

Block::Block(BlockProperties properties)
    : m_material(properties.m_material)
    , m_hardness(properties.m_hardness)
    , m_resistance(properties.m_resistance)
    , m_lightLevel(properties.m_lightLevel)
    , m_opacity(properties.m_opacity)
    , m_hasCollision(properties.m_hasCollision)
    , m_isSolid(properties.m_isSolid)
    , m_isFlammable(properties.m_isFlammable)
    , m_propagatesSkylightDown(properties.m_propagatesSkylightDown)
    , m_requiresTool(properties.m_requiresTool)
    , m_isReplaceable(properties.m_isReplaceable)
    , m_ticksRandomly(properties.m_ticksRandomly)
    , m_harvestTool(properties.m_harvestTool)
    , m_harvestLevel(properties.m_harvestLevel)
    , m_mapColor(properties.m_hasMapColor
              ? properties.m_mapColor
              : static_cast<world::map::MaterialColorId>(properties.m_material->materialColor()))
    , m_hasMapColor(properties.m_hasMapColor)
    , m_lootTableId(properties.m_lootTableId)
    , m_noLootTable(properties.m_noLootTable)
    , m_soundType(properties.m_soundType)
    , m_slipperiness(properties.m_slipperiness)
    , m_speedFactor(properties.m_speedFactor)
    , m_jumpFactor(properties.m_jumpFactor)
    , m_ignitedByLava(properties.m_ignitedByLava)
    , m_offsetType(properties.m_offsetType)
    , m_instrument(properties.m_instrument)
{
    // 所有方块都必须至少拥有一个基础状态。
    // 这与 Java 版 StateContainer 行为一致，可避免遗漏 createBlockState()
    // 时在注册阶段出现空指针崩溃。
    auto container = StateContainer<Block, BlockState>::Builder(*this).create(
        [](const Block& block,
            std::vector<size_t> values,
            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
            const std::vector<BlockState*>* allStates,
            u32 id) { return std::make_unique<BlockState>(block, std::move(values), propertyLayouts, allStates, id); });
    createBlockState(std::move(container));
}

void Block::createBlockState(std::unique_ptr<StateContainer<Block, BlockState>> container)
{
    m_stateContainer = std::move(container);
    m_defaultState = &m_stateContainer->baseState();
}

void Block::setDefaultState(const BlockState& state)
{
    m_defaultState = &state;
}

// ============================================================================
// 实体交互
// ============================================================================

void Block::onLanded(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 默认实现：Y速度归零
    entity.setVelocity(entity.velocity().x, 0.0f, entity.velocity().z);
}

void Block::onEntityWalk(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(entity);
    // 默认实现：无操作
    // 子类可以重写此方法实现特殊行为（如岩浆块造成伤害、岩浆方块产生气泡等）
}

void Block::onFallenUpon(IWorld& world, const BlockPos& pos, const BlockState& state, Entity& entity, f32 fallDistance)
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(state);

    // 默认实现：施加普通摔落伤害
    // 参考: net.minecraft.block.Block#fallOn — 调用 entity.causeFallDamage(fallDistance, 1.0F, damageSources().fall())
    // 子类可重写此方法以自定义摔落行为：
    // - 石笋方块：施加增大伤害，不调用父类（替代普通摔落伤害）
    // - 海龟蛋方块：不调用父类（取消摔落伤害）
    // - 耕地方块：先执行踩踏逻辑，再调用父类（保留普通摔落伤害）
    entity.causeFallDamage(fallDistance, 1.0f, DamageSources::fall());
}

const CollisionShape& Block::getShape(const BlockState& state) const
{
    (void)state;
    return VoxelShapes::fullCube();
}

const CollisionShape& Block::getCollisionShape(const BlockState& state) const
{
    if (!m_hasCollision) {
        return VoxelShapes::empty();
    }
    return getShape(state);
}

const CollisionShape& Block::getOcclusionShape(const BlockState& state) const
{
    return getShape(state);
}

const CollisionShape& Block::getBlockSupportShape(const BlockState& state) const
{
    // 默认返回碰撞形状，与 MC 一致
    // 某些方块（如泥巴、灵魂沙）的碰撞形状比完整方块矮，但支撑形状是完整方块
    // 参考: net.minecraft.block.Block#getBlockSupportShape
    return getCollisionShape(state);
}

CollisionShape Block::getFaceOcclusionShape(const BlockState& state, Direction direction) const
{
    // 默认实现：如果遮挡形状是完整方块，返回完整方块
    // 否则返回遮挡形状在指定方向的面投影
    const CollisionShape& occlusion = getOcclusionShape(state);
    if (occlusion.isFullBlock()) {
        return CollisionShape::fullBlock();
    }
    // 对于非完整方块，返回遮挡形状在指定方向的面投影
    // 这用于光照系统判断光线是否能穿过相邻方块之间的边界
    return occlusion.getFaceShape(direction);
}

bool Block::useShapeForLightOcclusion(const BlockState& state) const
{
    (void)state;
    // 默认不使用形状进行光照遮挡
    // 台阶、楼梯、栅栏等非完整方块需要覆盖此方法返回 true
    return false;
}

bool Block::skipRendering(const BlockState& selfState, const BlockState& neighborState, Direction direction) const
{
    MC_UNUSED(selfState);
    MC_UNUSED(neighborState);
    MC_UNUSED(direction);
    // 默认不跳过渲染，子类可重写此方法实现特殊的面剔除逻辑
    return false;
}

const CollisionShape& Block::getEntityInsideCollisionShape(const BlockState& state) const
{
    // 默认返回完整方块形状，即只要实体 AABB 与方块网格重叠就认为实体在方块内部。
    // 炼药锅、细雪等方块重写此方法返回更精确的形状，使 onEntityCollision
    // 只在实体实际进入方块内容区域时触发。
    // 参考: net.minecraft.block.BlockBehaviour#getEntityInsideCollisionShape 默认返回 Shapes.block()
    return VoxelShapes::fullCube();
}

bool Block::isAir(const BlockState& state) const
{
    (void)state;
    // 基类默认返回 false
    // AirBlock 会覆盖此方法返回 true
    return false;
}

bool Block::isSolid(const BlockState& state) const
{
    (void)state;
    return m_isSolid;
}

bool Block::isOpaque(const BlockState& state) const
{
    (void)state;
    return m_material->isOpaque();
}

f32 Block::getShadeBrightness(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 默认实现与 MC 原版一致：
    // 碰撞形状为完整方块时返回 0.2F（产生 AO 阴影），否则返回 1.0F。
    // 子类可重写以实现特殊行为（如 MudBlock 碰撞形状不完整但仍需阴影）。
    return state.hasOpaqueCollisionShape() ? 0.2f : 1.0f;
}

i32 Block::getOpacity(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    (void)world;
    (void)pos;
    // 默认实现：
    // - 不透明方块 -> 15
    // - 透明方块且显式设置 opacity -> 使用显式值
    // - 透明方块未显式设置 opacity（默认 15 作为哨兵）
    //   -> propagatesSkylightDown ? 0 : 1
    if (isOpaque(state)) {
        return 15;
    }

    if (m_opacity != 15) {
        return m_opacity;
    }

    return propagatesSkylightDown(state, world, pos) ? 0 : 1;
}

bool Block::propagatesSkylightDown(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    (void)state;
    (void)world;
    (void)pos;
    // 默认返回属性值
    return m_propagatesSkylightDown;
}

const fluid::FluidState* Block::getFluidState(const BlockState& state) const
{
    (void)state;
    // 默认返回空流体
    // LiquidBlock会重写此方法返回对应的流体状态
    static const fluid::FluidState* emptyState = nullptr;
    if (emptyState == nullptr) {
        // 获取EmptyFluid的默认状态
        if (auto* emptyFluid = fluid::FluidRegistry::instance().getFluid(0)) {
            emptyState = &emptyFluid->defaultState();
        }
    }
    return emptyState;
}

void Block::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 默认实现：空操作
    // 需要tick行为的方块应重写此方法
    (void)world;
    (void)pos;
    (void)state;
    (void)random;
}

void Block::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random)
{
    // 默认行为：如果方块设置了 ticksRandomly = true 但没有重写 randomTick()，
    // 则在随机刻时执行 tick() 方法
    tick(world, pos, state, random);
}

void Block::animateTick(
    IBlockAnimateContext& context, const BlockPos& pos, const BlockState& state, math::IRandom& random) const
{
    // 默认实现：空操作
    // 需要客户端动画效果的方块应重写此方法（如营火烟雾、气泡柱粒子、红石矿石闪烁等）
    (void)context;
    (void)pos;
    (void)state;
    (void)random;
}

void Block::neighborChanged(
    IWorld& world, const BlockPos& pos, Block& neighborBlock, const BlockPos& neighborPos, bool isMoving)
{
    // 默认实现：空操作
    // 需要响应邻居变化的方块应重写此方法
    (void)world;
    (void)pos;
    (void)neighborBlock;
    (void)neighborPos;
    (void)isMoving;
}

void Block::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 默认实现：空操作
    // 需要特殊初始化的方块应重写此方法
    (void)world;
    (void)pos;
    (void)state;
}

void Block::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 默认实现：空操作
    // 需要特殊清理的方块应重写此方法
    (void)world;
    (void)pos;
    (void)state;
}

void Block::playerWillDestroy(IWorld& world, const BlockPos& pos, const BlockState& state, Player& player)
{
    MC_UNUSED(pos);

    // 检查方块是否属于 GUARDED_BY_PIGLINS 标签
    // 破坏这些方块会激怒附近的猪灵（不需要视线检查）
    // 参考 MC 1.21.11 Block.playerWillDestroy()
    if (BlockTags::GUARDED_BY_PIGLINS().contains(state)) {
        entity::PiglinAi::angerNearbyPiglins(world, player, false);
    }
}

void Block::spawnAfterBreak(
    IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack* tool, bool dropExp) const
{
    // 默认实现：空操作
    // 需要额外生成逻辑的方块应重写此方法（如 InfestedBlock 生成蠹虫）
    (void)world;
    (void)pos;
    (void)state;
    (void)tool;
    (void)dropExp;
}

bool Block::isSolidSide(const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const
{
    // 冰块特殊处理：冰块的侧面不被认为是实体面（用于流体流动判断）
    if (*m_material == Material::ICE) {
        return false;
    }
    // 默认实现：检查方块是否为固体且有碰撞
    (void)state;
    (void)world;
    (void)pos;
    (void)side;
    return m_isSolid && m_hasCollision;
}

const BlockState& Block::rotate(const BlockState& state, Rotation rotation) const
{
    // 默认实现：不旋转，返回原状态
    (void)rotation;
    return state;
}

const BlockState& Block::mirror(const BlockState& state, Mirror mirror) const
{
    // 默认实现：不镜像，返回原状态
    (void)mirror;
    return state;
}

const loot::LootTable* Block::getLootTable(const loot::LootTableManager& manager) const
{
    if (m_lootTableId.empty()) {
        return nullptr;
    }
    return manager.getTable(m_lootTableId);
}

ItemStack Block::getCloneItemStack(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 默认实现返回空物品堆，由外部系统通过 BlockItemRegistry 查找对应方块物品
    return ItemStack();
}

// ============================================================================
// 新增虚方法默认实现
// ============================================================================

void Block::fillStateContainer(StateContainer<Block, BlockState>& container)
{
    // 默认实现：无属性
    (void)container;
}

const BlockState& Block::getDefaultState() const
{
    return *m_defaultState;
}

BlockState Block::getStateForPlacement(BlockItemUseContext& context)
{
    // 默认实现：返回默认状态
    (void)context;
    return defaultState();
}

void Block::onBlockPlacedBy(IWorld& world, const BlockPos& pos, const BlockState& state, const ItemStack& stack)
{
    // 默认实现：空操作
    (void)world;
    (void)pos;
    (void)state;
    (void)stack;
}

BlockState Block::updatePostPlacement(const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos)
{
    // 默认实现：返回原状态
    (void)facing;
    (void)facingState;
    (void)world;
    (void)currentPos;
    (void)facingPos;
    return state;
}

BlockState Block::updateFromNeighbourShapes(const BlockState& state, IWorld& world, const BlockPos& pos)
{
    BlockState currentState = state;
    for (Direction direction : UPDATE_SHAPE_ORDER) {
        BlockPos neighborPos = pos.offset(direction);
        const BlockState* neighborState = world.getBlockState(neighborPos);
        if (neighborState == nullptr) {
            continue;
        }
        currentState = currentState.getBlockMutable().updatePostPlacement(
            currentState, direction, *neighborState, world, pos, neighborPos);
    }
    return currentState;
}

bool Block::isValidPosition(const BlockState& state, IBlockReader& world, const BlockPos& pos) const
{
    // 默认实现：总是有效
    (void)state;
    (void)world;
    (void)pos;
    return true;
}

bool Block::isReplaceable(const BlockState& state, const BlockItemUseContext& context) const
{
    // 默认实现：使用 BlockProperties 的 isReplaceable 值
    (void)context;
    return m_isReplaceable;
}

bool Block::canSustainPlant(
    const BlockState& state, IBlockReader& world, const BlockPos& pos, Direction facing, const IPlantable& plant) const
{
    // 默认实现：根据植物类型检查此方块是否可支撑该植物
    // 对齐 MC 1.21.11 中各土壤方块的 canSustainPlant 逻辑
    const PlantType plantType = plant.getPlantType(world, pos);

    switch (plantType) {
        case PlantType::Plains:
            // 平原植物（花草、树苗等）：可在泥土类方块和耕地上种植
            return BlockTags::DIRT().contains(state) ||
                (VanillaBlocks::FARMLAND != nullptr && state.is(VanillaBlocks::FARMLAND));

        case PlantType::Crop:
            // 农作物：只能在耕地上种植
            return VanillaBlocks::FARMLAND != nullptr && state.is(VanillaBlocks::FARMLAND);

        case PlantType::Desert:
            // 沙漠植物（仙人掌）：可在沙子上种植
            return BlockTags::SAND().contains(state);

        case PlantType::Beach:
            // 海滩植物（甘蔗）：可在泥土类方块和沙子上种植
            // 注意：甘蔗还需要相邻水源，此检查由甘蔗自身在 isValidPosition 中完成
            return BlockTags::DIRT().contains(state) || BlockTags::SAND().contains(state);

        case PlantType::Cave:
            // 洞穴植物（蘑菇）：可在菌丝、灰化土、绯红菌岩、诡异菌岩上无条件种植，
            // 或在低光照（< 13）下的固体方块上种植。
            // 光照检查由 MushroomBlock 自身在 isValidPosition 中完成。
            return BlockTags::MUSHROOM_GROW_BLOCK().contains(state);

        case PlantType::Water:
            // 水生植物：由植物自身在 mayPlaceOn/canSustain 中处理
            return false;

        case PlantType::Nether:
            // 下界植物（地狱疣、菌类等）：可在菌岩、菌丝、灵魂土和泥土类方块上种植
            return (VanillaBlocks::CRIMSON_NYLIUM != nullptr && state.is(VanillaBlocks::CRIMSON_NYLIUM)) ||
                (VanillaBlocks::WARPED_NYLIUM != nullptr && state.is(VanillaBlocks::WARPED_NYLIUM)) ||
                (VanillaBlocks::MYCELIUM != nullptr && state.is(VanillaBlocks::MYCELIUM)) ||
                (VanillaBlocks::SOUL_SOIL != nullptr && state.is(VanillaBlocks::SOUL_SOIL)) ||
                BlockTags::DIRT().contains(state) ||
                (VanillaBlocks::FARMLAND != nullptr && state.is(VanillaBlocks::FARMLAND));

        default:
            return false;
    }
}

BlockActionResult Block::onBlockActivated(const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    // 默认实现：返回 Pass（未处理）
    (void)state;
    (void)world;
    (void)pos;
    (void)player;
    (void)hand;
    (void)hit;
    return ActionResultType::Pass;
}

std::unique_ptr<BlockEntity> Block::createBlockEntity(const BlockPos& pos)
{
    // 默认实现：无方块实体
    (void)pos;
    return nullptr;
}

bool Block::triggerEvent(const BlockState& state, IWorld& world, const BlockPos& pos, i32 id, i32 type) const
{
    // 默认实现：如果有方块实体，委托给方块实体的 triggerEvent
    (void)state;
    auto* blockEntity = world.getBlockEntity(pos);
    return blockEntity != nullptr && blockEntity->triggerEvent(id, type);
}

i32 Block::getComparatorInputOverride(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    // 默认实现：无比较器输入覆盖
    (void)state;
    (void)world;
    (void)pos;
    return 0;
}

Material::PushReaction Block::getPushReaction(const BlockState& state) const
{
    // 默认实现：正常推动
    (void)state;
    return Material::PushReaction::Normal;
}

// ============================================================================
// 静态辅助方法
// ============================================================================

bool Block::shouldSideBeRendered(const BlockState& state, IWorld& world, const BlockPos& pos, Direction face)
{

    // 获取相邻方块的遮挡形状
    BlockPos neighborPos = pos.offset(face);
    const BlockState* neighborState = world.getBlockState(neighborPos);

    if (neighborState == nullptr || neighborState->isAir()) {
        // 相邻是空气，面应该渲染
        return true;
    }

    // 如果相邻方块是不透明完整方块，则面不需要渲染
    if (neighborState->isOpaqueCube(world, neighborPos)) {
        return false;
    }

    // 如果相邻方块是透明或不完整方块，需要渲染
    return true;
}

bool Block::hasSolidSideOnTop(IWorld& world, const BlockPos& pos)
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return false;
    }

    // 检查顶面是否为实体面
    return state->isSolidSide(world, pos, Direction::Up);
}

bool Block::hasEnoughSolidSide(IWorld& world, const BlockPos& pos, Direction direction)
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr || state->isAir()) {
        return false;
    }

    // 参考: net.minecraft.block.Block#hasEnoughSolidSide
    // 需要同时满足：1) 方块面是固体面  2) 碰撞形状在该方向的面投影覆盖整个面
    if (!state->isSolidSide(world, pos, direction)) {
        return false;
    }

    return doesSideFillSquare(state->getCollisionShape(), direction);
}

bool Block::canSupportCenter(IWorld& world, const BlockPos& pos, Direction direction)
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr || state->isAir()) {
        return false;
    }

    // MC 1.21.11: 仅当 direction == DOWN 且方块属于 UNSTABLE_BOTTOM_CENTER 标签（栅栏门）时拒绝
    if (direction == Direction::Down && BlockTags::UNSTABLE_BOTTOM_CENTER().contains(*state)) {
        return false;
    }

    // 委托到 BlockState.isFaceSturdy，使用 SupportType::Center 判定
    return state->isFaceSturdy(world, pos, direction, SupportType::Center);
}

bool Block::canSupportRigidBlock(IWorld& world, const BlockPos& pos)
{
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr || state->isAir()) {
        return false;
    }

    // MC 1.21.11: BlockState.isFaceSturdy(Direction.UP, SupportType.RIGID)
    return state->isFaceSturdy(world, pos, Direction::Up, SupportType::Rigid);
}

bool Block::doesSideFillSquare(const CollisionShape& shape, Direction direction)
{
    // 如果形状是完整方块，则填充整个面
    if (shape.isFullBlock()) {
        return true;
    }

    // 获取形状在指定方向上的面投影，检查投影是否覆盖整个面
    // 参考: net.minecraft.block.Block#isFaceFull
    CollisionShape faceShape = shape.getFaceShape(direction);
    return faceShape.coversFullBlock();
}

bool Block::isFaceFull(const CollisionShape& shape, Direction direction)
{
    // 提取面投影并检查是否覆盖整个面
    // 参考: net.minecraft.block.Block#isFaceFull(VoxelShape, Direction)
    CollisionShape faceShape = shape.getFaceShape(direction);
    return faceShape.coversFullBlock();
}

bool Block::isExceptionForConnection(const BlockState& state)
{
    // 参考: net.minecraft.block.Block#isExceptionForConnection
    // 某些固体方块不应与栅栏、墙、玻璃板建立连接
    return BlockTags::LEAVES().contains(state) || BlockTags::SHULKER_BOXES().contains(state) ||
        state.is(BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "barrier"))) ||
        state.is(BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "carved_pumpkin"))) ||
        state.is(BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "jack_o_lantern"))) ||
        state.is(BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "melon"))) ||
        state.is(BlockRegistry::instance().getBlock(ResourceLocation("minecraft", "pumpkin")));
}

const BlockState& Block::pushEntitiesUp(
    const BlockState& oldState, const BlockState& newState, IWorld& world, const BlockPos& pos)
{
    // 参考: net.minecraft.world.level.block.Block#pushEntitiesUp
    // 当方块碰撞形状增大时，将嵌入方块内的实体向上推出。
    //
    // 算法（与 MC Java 1.21.11 一致）：
    // 1. 计算 oldCollision 与 newCollision 的"仅新增部分"差集形状
    //    （ONLY_SECOND：在 newState 中但不在 oldState 中），并平移到世界坐标。
    // 2. 若差集为空，直接返回 newState。
    // 3. 否则对差集 AABB 范围内的每个实体：
    //    a. 取实体碰撞箱上移 1 格后的 AABB。
    //    b. 沿 Y 轴向下（movement = -1.0）与差集形状做碰撞，得到最大可下落距离 d0（≤0）。
    //    c. 让实体相对上移 (1 + d0)，即正好停在差集形状顶部之上。
    //
    // 注：MC 使用 entity.teleportRelative(...)；本项目中等价的接口是
    // Entity::move(MoverType::Piston, delta) —— 该重载不进行碰撞检测，仅更新位置，
    // 与 teleportRelative 语义一致。

    VoxelShape oldShape = Shapes::fromCollisionShape(oldState.getCollisionShape());
    VoxelShape newShape = Shapes::fromCollisionShape(newState.getCollisionShape());
    VoxelShape diffShape = Shapes::joinUnoptimized(oldShape, newShape, BooleanOps::OnlySecond())
                               .move(static_cast<f64>(pos.x), static_cast<f64>(pos.y), static_cast<f64>(pos.z));

    if (diffShape.isEmpty()) {
        return newState;
    }

    AxisAlignedBB bounds = diffShape.bounds();
    auto entities = world.getEntitiesInAABB(bounds, nullptr);
    for (auto* entity : entities) {
        if (entity == nullptr) {
            continue;
        }

        // 实体碰撞箱向上平移 1 格（用于在差集形状顶部上方寻找落点）
        AxisAlignedBB shiftedBox = entity->boundingBox().offsetted(0.0f, 1.0f, 0.0f);

        // 沿 Y 轴向下碰撞计算：d0 为最大可下落距离（非正数）
        f64 d0 = diffShape.collide(Axis::Y, shiftedBox, -1.0);
        f32 pushUp = static_cast<f32>(1.0 + d0);
        if (pushUp > 0.0f) {
            entity->move(entity::MoverType::Piston, Vector3(0.0f, pushUp, 0.0f));
        }
    }

    return newState;
}

// ============================================================================
// 攻击和交互
// ============================================================================

void Block::harvestBlock(IWorld& world,
    Player& player,
    const BlockPos& pos,
    const BlockState& state,
    BlockEntity* blockEntity,
    const ItemStack* stack)
{
    // 播放破坏音效
    const BlockSoundType& soundType = state.owner().getSoundType();
    world.playSound(soundType.getBreakSound(),
        sound::SoundCategory::Blocks,
        pos.center(),
        (soundType.getVolume() + 1.0f) / 2.0f,
        soundType.getPitch() * 0.8f);

    // 掉落物由 BlockDropHandler 处理
    MC_UNUSED(player);
    MC_UNUSED(blockEntity);
    MC_UNUSED(stack);
}

f32 Block::getPlayerRelativeBlockHardness(
    Player& player, IBlockReader& world, const BlockPos& pos, const BlockState& state) const
{
    // 基础挖掘速度 = 1 / (hardness * 30) 对于硬度 > 0
    // 创造模式：瞬间破坏
    MC_UNUSED(world);

    f32 hardness = state.hardness();
    if (hardness <= 0.0f) {
        // 硬度为0的方块（如空气、水）瞬间破坏
        return 1.0f;
    }

    // 创造模式瞬间破坏
    if (player.isCreative()) {
        return 1.0f;
    }

    // 获取玩家的挖掘速度倍率
    f32 digSpeed = player.getDigSpeed(state, pos);

    // 检查是否可以使用正确工具采集
    bool canHarvest = player.canHarvestBlock(state);

    // 方块相对硬度 = digSpeed / hardness / divisor
    // 正确工具: divisor = 30
    // 错误工具: divisor = 100 (慢约 3.3 倍)
    f32 divisor = canHarvest ? 30.0f : 100.0f;

    return digSpeed / hardness / divisor;
}

// ============================================================================
// 火焰相关默认实现
// ============================================================================

i32 Block::getFlammability(const BlockState& state, IWorld* world, const BlockPos* pos, Direction face) const noexcept
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(face);

    // 优先从 FireInfoRegistry 查询方块燃烧参数
    // 子类可通过重写此方法提供自定义值（会隐藏此默认实现）
    return blocks::FireInfoRegistry::instance().getFlammability(m_blockId);
}

i32 Block::getFireSpreadSpeed(
    const BlockState& state, IWorld* world, const BlockPos* pos, Direction face) const noexcept
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(face);

    // 优先从 FireInfoRegistry 查询方块蔓延速度
    // 子类可通过重写此方法提供自定义值（会隐藏此默认实现）
    return blocks::FireInfoRegistry::instance().getEncouragement(m_blockId);
}

// ============================================================================
// 掉落物品生成
// ============================================================================

void Block::dropResources(IWorld& world, const BlockPos& pos, const BlockState& state)
{
    // 仅在服务端生成掉落物品（lootTableManager 为空表示客户端）
    auto* lootTableManager = world.lootTableManager();
    if (lootTableManager == nullptr) {
        return;
    }

    const Block& block = state.getBlock();

    // 获取方块的掉落表
    const loot::LootTable* lootTable = block.getLootTable(*lootTableManager);
    if (lootTable == nullptr) {
        return;
    }

    // 构建掉落上下文（无玩家、无工具，与 MC 原版 Block.dropResources 行为一致）
    // MC 使用 ServerLevel.random（世界共享随机），不创建新 Random 对象
    auto contextBuilder = loot::LootContextBuilder(world);

    // 设置方块状态和位置参数
    contextBuilder.withParameter(loot::LootParams::BLOCK_STATE, const_cast<BlockState*>(&state));
    contextBuilder.withParameter(loot::LootParams::BLOCK_POS, const_cast<BlockPos*>(&pos));

    // 设置掉落表解析器和条件解析器
    contextBuilder.withLootTableResolver(
        [lootTableManager](const std::string& id) -> const loot::LootTable* { return lootTableManager->getTable(id); });
    contextBuilder.withPredicateResolver([lootTableManager](const std::string& id) -> const loot::LootCondition* {
        return lootTableManager->getPredicate(id);
    });

    auto context = contextBuilder.build(loot::LootParameterSets::block());
    if (context == nullptr) {
        return;
    }

    // 生成掉落物品
    auto drops = lootTable->generate(*context);
    if (drops.empty()) {
        return;
    }

    // 在世界中生成掉落物实体
    ItemDropHelper::spawnItemEntities(&world, pos, drops, world.getRandom());

    // 调用 spawnAfterBreak 触发额外效果（如刷怪笼生成怪物）
    block.spawnAfterBreak(world, pos, state, nullptr, false);
}

} // namespace mc
