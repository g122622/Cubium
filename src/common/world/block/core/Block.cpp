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

#include "common/world/block/core/Block.hpp"
#include "common/core/BlockRaycastResult.hpp"
#include "common/core/Types.hpp"
#include "common/entity/ai/util/PiglinAi.hpp"
#include "common/entity/core/Entity.hpp"
#include "common/entity/core/MoverType.hpp"
#include "common/entity/damage/DamageSource.hpp"
#include "common/entity/entities/player/Player.hpp"
#include "common/entity/utils/ItemDropHelper.hpp"
#include "common/item/context/BlockItemUseContext.hpp"
#include "common/item/context/ItemUseContext.hpp"
#include "common/item/core/ActionResult.hpp"
#include "common/item/core/BlockActionResult.hpp"
#include "common/item/core/ItemStack.hpp"
#include "common/item/loot/LootTable.hpp"
#include "common/item/loot/LootTableManager.hpp"
#include "common/item/loot/context/LootContextBuilder.hpp"
#include "common/item/loot/context/LootParameterSets.hpp"
#include "common/item/loot/context/LootParams.hpp"
#include "common/physics/shape/BooleanOp.hpp"
#include "common/physics/shape/Shapes.hpp"
#include "common/physics/shape/VoxelShape.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/sound/SoundCategory.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/assert/AssertMacros.hpp"
#include "common/util/math/MathUtils.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/math/random/IRandom.hpp"
#include "common/util/property/StateContainer.hpp"
#include "common/util/property/StateHolder.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/block/base/material/Material.hpp"
#include "common/world/block/base/sound/BlockSoundType.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/base/types/BlockUpdateFlags.hpp"
#include "common/world/block/base/types/PlantType.hpp"
#include "common/world/block/capability/IBlockAnimateContext.hpp"
#include "common/world/block/fire/FireInfoRegistry.hpp"
#include "common/world/block/registry/BlockRegistry.hpp"
#include "common/world/block/registry/vanilla/NaturalBlocks.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/block/support/SupportType.hpp"
#include "common/world/block/tags/BlockTags.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/fluid/FluidRegistry.hpp"
#include "common/world/fluid/FluidTags.hpp"
#include "common/world/gamerule/GameRules.hpp"
#include "common/world/map/MaterialColor.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mc {

// ============================================================================
// BlockProperties
// ============================================================================

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
    , m_forceSolidOn(properties.m_forceSolidOn)
    , m_forceSolidOff(properties.m_forceSolidOff)
    , m_isSuffocating(properties.m_isSuffocating)
    , m_isViewBlocking(properties.m_isViewBlocking)
    , m_isFlammable(properties.m_isFlammable)
    , m_canOcclude(properties.m_canOcclude)
    , m_propagatesSkylightDown(properties.m_propagatesSkylightDown)
    , m_propagatesSkylightDownExplicit(properties.m_propagatesSkylightDownExplicit)
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
            StateValueIndices valueIndices,
            size_t propertyCount,
            const std::vector<StateHolder<Block, BlockState>::PropertyLayout>* propertyLayouts,
            const std::vector<BlockState*>* allStates,
            u32 id) {
            return std::make_unique<BlockState>(block, valueIndices, propertyCount, propertyLayouts, allStates, id);
        });
    createBlockState(std::move(container));
}

void Block::refreshLightCache()
{
    if (!m_stateContainer) {
        return;
    }
    for (const auto& state : m_stateContainer->validStates()) {
        if (state) {
            state->refreshLightCache();
        }
    }
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
    // 子类可重写此方法以自定义摔落行为：
    // - 石笋方块：施加增大伤害，不调用父类（替代普通摔落伤害）
    // - 海龟蛋方块：不调用父类（取消摔落伤害）
    // - 耕地方块：先执行踩踏逻辑，再调用父类（保留普通摔落伤害）
    entity.causeFallDamage(fallDistance, 1.0f, DamageSources::fall());
}

const CollisionShape& Block::getShape(const BlockState& state) const
{
    (void)state;
    return CollisionShape::fullBlockShape();
}

const CollisionShape& Block::getCollisionShape(const BlockState& state) const
{
    // 与原版一致：`noCollision()` 生成的方块（按钮、火把、告示牌、珊瑚、蘑菇……）
    // 碰撞形状为空，有别于它们的 `getShape`（选中框/渲染轮廓仍然非空）。
    // 注意 `noCollision()` 同时会关掉 `canOcclude`，两者是同一个开关。
    if (!m_hasCollision) {
        return CollisionShape::emptyShape();
    }
    return getShape(state);
}

Vector3 Block::getOffset(const BlockPos& pos) const noexcept
{
    if (m_offsetType == BlockOffsetType::None) {
        return Vector3(0.0f, 0.0f, 0.0f);
    }

    // 偏移量来自坐标哈希。`y` 固定取 0，使同一水平位置的所有高度得到相同偏移。
    const i64 seed = math::mthGetSeed(pos.x, 0, pos.z);

    // 水平分量：`(n/15 - 0.5) * 0.5`，取值 [-0.25, 0.25)，再 clamp 到 ±上限。
    const f32 maxHorizontal = getMaxHorizontalOffset();
    const auto horizontal = [&](i64 nibble) {
        const f32 raw = (static_cast<f32>(nibble & 15LL) / 15.0f - 0.5f) * 0.5f;
        return static_cast<f64>(math::clamp(raw, -maxHorizontal, maxHorizontal));
    };

    const f64 offsetX = horizontal(seed);
    const f64 offsetZ = horizontal(seed >> 8);

    if (m_offsetType == BlockOffsetType::XZ) {
        return Vector3(static_cast<f32>(offsetX), 0.0f, static_cast<f32>(offsetZ));
    }

    // XYZ：竖直分量只向下偏移，取值 [-(maxVertical), 0]。
    const f32 verticalRaw = (static_cast<f32>(seed >> 4 & 15LL) / 15.0f - 1.0f) * getMaxVerticalOffset();
    return Vector3(static_cast<f32>(offsetX), verticalRaw, static_cast<f32>(offsetZ));
}

const CollisionShape& Block::getCollisionShapeForEntity(
    const BlockState& state, const EntityCollisionContext& ctx, i32 blockY) const
{
    MC_UNUSED(ctx);
    MC_UNUSED(blockY);
    // 默认：碰撞形状不区分实体，委托无实体上下文版本。需要按实体区分的方块（如细雪）重写此方法。
    return getCollisionShape(state);
}

bool Block::isValidSpawn(
    IWorld& world, const BlockPos& pos, const BlockState& state, const entity::EntityType& entityType) const
{
    MC_UNUSED(entityType);
    // 原版默认谓词（BlockBehaviour.Properties.isValidSpawn）：顶面为完整支撑面，且自身发光等级 < 14。
    // 传送到此生成的生物需要站在实心、非高亮的方块上方。
    return state.isFaceSturdy(world, pos, Direction::Up, SupportType::Full) && state.lightLevel() < 14;
}

const CollisionShape& Block::getOcclusionShape(const BlockState& state) const
{
    // 返回方块的"遮挡形状"。注意本方法同时被两处使用：
    // 1) 光照引擎的形状遮挡检测：vanilla 对 noOcclusion 方块使用空遮挡形状
    //    （BlockStateBase：occlusionShape = canOcclude ? getOcclusionShape() : Shapes.empty()）；
    // 2) propagatesSkylightDown 的默认公式：vanilla 基于 getShape（不受 canOcclude 影响）。
    //
    // 为同时满足两者，本方法返回真实形状，"noOcclusion 方块不遮挡"由调用方按
    // canOcclude() 判定（见 BlockState::getFaceOcclusionShape）。若在此返回空形状，
    // 会连带修改第 2 条语义，使未重写 getShape 的 noOcclusion 方块（如铁栏杆）
    // 的天空光透传判定翻转。
    return getShape(state);
}

const CollisionShape& Block::getBlockSupportShape(const BlockState& state) const
{
    // 默认返回碰撞形状，与 MC 一致
    // 某些方块（如泥巴、灵魂沙）的碰撞形状比完整方块矮，但支撑形状是完整方块
    return getCollisionShape(state);
}

CollisionShape Block::getFaceOcclusionShape(const BlockState& state, Direction direction) const
{
    // noOcclusion 方块不参与遮挡计算：遮挡形状视为空（vanilla BlockStateBase 的做法）。
    if (!m_canOcclude) {
        return CollisionShape::empty();
    }
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
    return CollisionShape::fullBlockShape();
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
    // 原版 `BlockBehaviour.BlockStateBase#calculateSolid`：
    //   forceSolidOn  -> true
    //   forceSolidOff -> false
    //   碰撞形状为空  -> false
    //   否则按包围盒推断：
    //     (x + y + z) / 3 >= 35/48  -> true
    //     否则 y 尺寸 >= 1         -> true
    //   其余 -> false
    //
    // 关键在最后一步：**不是**「是否占满整格」，而是「三轴平均够厚」或「整格高」。
    // 因此门、栅栏门、告示牌这些碰撞形状远不满一格的方块，原版仍判为实体方块。
    if (m_forceSolidOn) {
        return true;
    }
    if (m_forceSolidOff) {
        return false;
    }

    const CollisionShape& shape = getCollisionShape(state);
    if (shape.isEmpty()) {
        return false;
    }

    f32 minX = std::numeric_limits<f32>::max();
    f32 minY = std::numeric_limits<f32>::max();
    f32 minZ = std::numeric_limits<f32>::max();
    f32 maxX = std::numeric_limits<f32>::lowest();
    f32 maxY = std::numeric_limits<f32>::lowest();
    f32 maxZ = std::numeric_limits<f32>::lowest();
    for (const AxisAlignedBB& box : shape.boxes()) {
        minX = std::min(minX, box.minX);
        minY = std::min(minY, box.minY);
        minZ = std::min(minZ, box.minZ);
        maxX = std::max(maxX, box.maxX);
        maxY = std::max(maxY, box.maxY);
        maxZ = std::max(maxZ, box.maxZ);
    }

    const f32 size = ((maxX - minX) + (maxY - minY) + (maxZ - minZ)) / 3.0f;
    if (size >= 35.0f / 48.0f) {
        return true;
    }
    return (maxY - minY) >= 1.0f;
}

bool Block::blocksMotion(const BlockState& state) const
{
    // 原版 `BlockStateBase#blocksMotion`：`isSolid()` 且**不是**蜘蛛网、竹笋。
    // 这两个方块看着有碰撞，原版却允许实体穿行。竹笋在本项目注册为 REPLACEABLE_PLANT
    // 材质、isSolid 已为 false，走下面的分支就会返回 false；蜘蛛网则需在此显式排除。
    if (!isSolid(state)) {
        return false;
    }
    return this != block_registry::NaturalBlocks::COBWEB;
}

bool Block::isCollisionShapeFullBlock(const BlockState& state, const BlockPos& pos) const
{
    // 与原版相同：按**碰撞形状**（不是外形、不是遮挡形状）是否覆盖整格判定，
    // 且必须走带位置的入口，让 `offsetType` 偏移参与计算。
    return getCollisionShapeAt(state, pos).coversFullBlock();
}

bool Block::isOpaque(const BlockState& state) const
{
    (void)state;
    return m_material->isOpaque();
}

bool Block::isPathfindable(const BlockState& state, PathComputationType type) const
{
    // 三种寻路类型的默认判据互不相同：
    //   Land / Air -> 碰撞形状「不完整」即可通行
    //   Water      -> 只认水（getFluidState().is(WATER)）
    //
    // 判据是「不完整」而不是「为空」：台阶、楼梯、栅栏门的碰撞形状非空但不完整，
    // 生物能从上方走过。用 isEmpty() 会让生物绕开所有台阶，表现为卡在台阶前不动。
    //
    // Water 类型不看形状：水中型生物只能游过水方块。若三种类型统一按形状判，
    // 陆行生物会走进水里、鱼会游进空气。
    switch (type) {
        case PathComputationType::Water: {
            const fluid::FluidState* fluidState = state.getFluidState();
            return fluidState != nullptr && !fluidState->isEmpty() &&
                fluidState->getFluid().isIn(fluid::FluidTags::WATER());
        }
        case PathComputationType::Land:
        case PathComputationType::Air:
        default:
            return !state.getCollisionShape().coversFullBlock();
    }
}

f32 Block::getShadeBrightness(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 原版判据只看「碰撞形状是否覆盖整格」，与 canOcclude / noOcclusion 无关：
    //   isCollisionShapeFullBlock(state, world, pos) ? 0.2F : 1.0F
    //
    // 这一点很容易写错。用 canOcclude 判定会在两个方向上同时出错：
    // - 树叶、冰、荧石、信标、潜影盒形状是整格，原版有阴影，但它们标了 noOcclusion
    // - 栅栏、台阶、楼梯、墙形状不完整，原版没有阴影，但它们是可遮挡方块
    //
    // 子类仍可重写以实现特例（泥巴、灵魂沙是 14/16 矮块却要阴影；雪层满层才要阴影）。
    return state.getCollisionShape().coversFullBlock() ? 0.2f : 1.0f;
}

i32 Block::getOpacity(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    (void)pos;
    // 默认实现对齐 vanilla BlockBehaviour#getLightBlock（本项目中"opacity"即 getLightBlock 语义）：
    // - 实体渲染方块（isSolidRender）-> 15
    // - 其余方块：显式设置的 opacity 优先（BlockProperties::opacity），
    //   未设置（默认 15 作为哨兵）时有流体 -> 1，否则 propagatesSkylightDown ? 0 : 1
    //
    // 15 分支的判定 = 实体渲染方块（vanilla BlockStateBase#solidRender）：
    //   canOcclude && isShapeFullBlock(occlusionShape)
    // 早期实现用「canOcclude && 材质不透明」近似，会把单层台阶、楼梯这类「材质不透明但形状
    // 不完整」的方块判成 15，而原版是 0——光照引擎据此让整格完全挡光，台阶后面永远比原版暗。
    // noOcclusion（canOcclude=false）方块已由 isSolidRender 内的 canOcclude 判定排除：
    // 铁栏杆（金属材质）、树叶、门等原版 lightBlock 分别为 0 或 1，而非 15。
    if (state.isSolidRender()) {
        return 15;
    }

    if (m_opacity != 15) {
        return m_opacity;
    }

    return propagatesSkylightDown(state, world, pos) ? 0 : 1;
}

bool Block::propagatesSkylightDown(const BlockState& state, IWorld* world, const BlockPos* pos) const
{
    (void)world;
    (void)pos;
    // 显式设置优先：注册时调用过 BlockProperties::propagatesSkylightDown() 的方块（如冰、玻璃、
    // 苍穹光透传方块）直接返回显式值，不走默认公式。
    if (m_propagatesSkylightDownExplicit) {
        return m_propagatesSkylightDown;
    }

    // 对齐 vanilla BlockBehaviour#propagatesSkylightDown 默认实现：
    //   !Block.isShapeFullBlock(state.getShape(EmptyBlockGetter, ZERO)) && state.getFluidState().isEmpty()
    // 即"形状非完整立方体 且 无流体"时透传天空光（propagatesSkylightDown=true）。
    //
    // Cubium 用 getOcclusionShape 近似 vanilla 的 getShape：vanilla 的 getShape 在
    // noCollision 方块上返回空形状（getCollisionShape 里的 hasCollision 短路），而 Cubium 的
    // getShape 默认返回 fullBlock、不看碰撞；getOcclusionShape 在透光/非碰撞方块上返回空
    // （如花草、栅栏），更接近 vanilla getShape 的光照语义。
    //
    // 注意这不是严格等价：把 getOcclusionShape 覆盖成非整格、而 getShape 仍是整格的方块
    // （如幽匿尖啸体）会与原版判定相反。这类方块必须自己重写 propagatesSkylightDown，
    // 见 SculkShriekerBlock。
    //
    // 流体判定：getFluidState 返回指针可能为 nullptr（无流体状态）或 EmptyFluid（isEmpty=true），
    // 两者均视为"无流体"→透传。含水方块（如含水栅栏）getFluidState 返回水→isEmpty=false→不透传，
    // 对齐 vanilla CrossCollisionBlock/WallBlock 的 !WATERLOGGED 语义。
    const CollisionShape& shape = getOcclusionShape(state);
    // 与 isSolidRender 用同一个精确判定（vanilla isShapeFullBlock），否则会出现
    // 「算作实体渲染 → lightBlock 15」与「又判为透传天空光」自相矛盾的组合。
    if (shape.coversFullBlock()) {
        return false;
    }
    const fluid::FluidState* fluidState = getFluidState(state);
    return fluidState == nullptr || fluidState->isEmpty();
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

void Block::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston)
{
    // 默认实现：空操作
    // 需要特殊初始化的方块应重写此方法
    (void)world;
    (void)pos;
    (void)state;
}

void Block::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state, bool movedByPiston)
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

const BlockState& Block::getStateForMovement(const BlockState& state, Direction pushDirection) const
{
    // 默认实现：无按推动方向的旋转语义，落地状态即被移动状态
    (void)pushDirection;
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

void Block::onBlockPlacedBy(
    IWorld& world, const BlockPos& pos, const BlockState& state, const Entity* placer, const ItemStack& stack)
{
    // 默认实现：空操作
    (void)world;
    (void)pos;
    (void)state;
    (void)placer;
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
        // 邻居形状更新导致当前状态成为空气（如垂根失去上方支撑）时，原版会实例化空气状态并
        // 继续以空邻居逐个更新（不返回）。Cubium 的 updatePostPlacement 直接返回空气状态，
        // 必须显式短路，否则空气的 updatePostPlacement 覆盖会把空气重新填回被推动方块的
        // 原状态，表现为"被推动方块落地后消失失败"。
        if (currentState.isAir()) {
            break;
        }
    }
    return currentState;
}

void Block::updateIndirectNeighbourShapes(
    const BlockState& state, IWorld& world, const BlockPos& pos, i32 flags, i32 recursionLeft, bool movedByPiston)
{
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(flags);
    MC_UNUSED(recursionLeft);
    MC_UNUSED(movedByPiston);
    // 默认实现为空（与 vanilla `BlockBehaviour#updateIndirectNeighbourShapes` 一致）。
}

void Block::updateNeighbourShapes(
    const BlockState& state, IWorld& world, const BlockPos& pos, i32 flags, i32 recursionLeft, bool movedByPiston)
{
    for (Direction direction : UPDATE_SHAPE_ORDER) {
        // 邻居的更新方向是「从邻居指向本方块」，即本方块朝向该邻居的反方向。
        world.notifyShapeChanged(
            Directions::opposite(direction), pos.offset(direction), pos, state, flags, recursionLeft, movedByPiston);
    }
}

void Block::executeShapeUpdate(Direction direction,
    const BlockState& sourceState,
    IWorld& world,
    const BlockPos& neighborPos,
    const BlockPos& sourcePos,
    i32 flags,
    i32 recursionLeft,
    bool movedByPiston)
{
    const BlockState* neighborState = world.getBlockState(neighborPos);
    if (neighborState == nullptr) {
        // 区块未加载：没有可更新的方块。
        return;
    }

    // UPDATE_SKIP_SHAPE_UPDATE_ON_WIRE：目标为红石线时跳过形状更新，避免红石网络在
    // 放置/破坏时产生 O(n²) 的形状重算风暴（对应 vanilla `NeighborUpdater.executeShapeUpdate`）。
    if ((flags & world::BlockUpdateFlags::UPDATE_SKIP_SHAPE_UPDATE_ON_WIRE) != 0 &&
        VanillaBlocks::REDSTONE_WIRE != nullptr && neighborState->is(VanillaBlocks::REDSTONE_WIRE)) {
        return;
    }

    Block& neighborBlock = neighborState->getBlockMutable();
    const BlockState updatedState =
        neighborBlock.updatePostPlacement(*neighborState, direction, sourceState, world, neighborPos, sourcePos);
    if (updatedState.stateId() == neighborState->stateId()) {
        return;
    }

    (void)movedByPiston;
    // 与 vanilla 一致：变空气走 destroyBlock（粒子/掉落/BLOCK_DESTROY 振动），否则写回新状态。
    // UPDATE_SUPPRESS_DROPS 由 updateOrDestroy 消费；形状递归预算继续向下传递。
    updateOrDestroy(*neighborState, updatedState, world, neighborPos, flags, recursionLeft);
}

void Block::updateOrDestroy(
    const BlockState& oldState, const BlockState& newState, IWorld& world, const BlockPos& pos, i32 flags)
{
    updateOrDestroy(oldState, newState, world, pos, flags, world::BlockUpdateFlags::DEFAULT_DESTROY_RECURSION);
}

void Block::updateOrDestroy(const BlockState& oldState,
    const BlockState& newState,
    IWorld& world,
    const BlockPos& pos,
    i32 flags,
    i32 recursionLeft)
{
    if (newState.stateId() == oldState.stateId()) {
        return;
    }

    if (newState.isAir()) {
        // 客户端不执行方块移除（方块变化由服务端下发）
        if (world.isClientSide()) {
            return;
        }

        // 视同摧毁：发破坏粒子、按需掉落、写回流体遗留方块并发送 BLOCK_DESTROY 振动事件。
        // UPDATE_SUPPRESS_DROPS 表示本条场景不产生掉落（如形状级联中的临时脱落）。
        const bool dropBlock = (flags & world::BlockUpdateFlags::UPDATE_SUPPRESS_DROPS) == 0;
        world.destroyBlock(pos, dropBlock, nullptr, recursionLeft);
        return;
    }

    // recursionLeft 是形状更新级联的深度预算，由变空气分支下传给 destroyBlock；
    // 非空气分支的后续级联由 ServerWorld::setBlockState 另行携带预算，此处无需递减。
    MC_UNUSED(recursionLeft);
    world.setBlockState(pos, &newState, flags & ~world::BlockUpdateFlags::UPDATE_SUPPRESS_DROPS);
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

BlockActionResult Block::useItemOn(ItemStack& stack,
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit)
{
    // 默认实现：不处理手持物品交互，请求以空手语义重试
    // （对齐 vanilla BlockBehaviour.useItemOn 默认返回 TRY_WITH_EMPTY_HAND）
    (void)stack;
    (void)state;
    (void)world;
    (void)pos;
    (void)player;
    (void)hand;
    (void)hit;
    return ActionResultType::TryEmptyHandInteraction;
}

BlockActionResult Block::useWithoutItem(
    const BlockState& state, IWorld& world, const BlockPos& pos, Player& player, const BlockRaycastResult& hit)
{
    // 默认实现：返回 Pass（本方块不响应空手交互）
    // （对齐 vanilla BlockBehaviour.useWithoutItem 默认返回 InteractionResult.PASS）
    (void)state;
    (void)world;
    (void)pos;
    (void)player;
    (void)hit;
    return ActionResultType::Pass;
}

std::optional<ContainerType> Block::getMenuProvider(const BlockState& state, IWorld& world, const BlockPos& pos) const
{
    // 默认实现：无界面
    (void)state;
    (void)world;
    (void)pos;
    return std::nullopt;
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
    // 原版 `BlockBehaviour#getPistonPushReaction` 返回方块登记时写下的 pushReaction；
    // Cubium 把它登记在材质上（`Material::m_pushReaction`），这里必须读材质而不是恒返回 Normal。
    // 恒返回 Normal 会让火把、树苗、铁轨等 `Destroy` 类方块被活塞「推着走」而不是被破坏
    // （由差分能力 piston_movement 查出）。
    (void)state;
    return material().getPushReaction();
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
    CollisionShape faceShape = shape.getFaceShape(direction);
    return faceShape.coversFullBlock();
}

bool Block::isFaceFull(const CollisionShape& shape, Direction direction)
{
    // 提取面投影并检查是否覆盖整个面
    CollisionShape faceShape = shape.getFaceShape(direction);
    return faceShape.coversFullBlock();
}

bool Block::canPlaceAt(const BlockItemUseContext& /*context*/) const
{
    // 默认：放置可行性交由 isValidPosition（canSurvive）在 BlockItem::canPlace 中判定。
    return true;
}

bool Block::isExceptionForConnection(const BlockState& state)
{
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
    // 播放破坏音效（按被破坏的状态取声音组：饰纹陶罐碎裂后改为碎裂声）
    const BlockSoundType& soundType = state.getSoundType();
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
    dropResources(world, pos, state, world.getBlockEntity(pos));
}

void Block::dropResources(IWorld& world, const BlockPos& pos, const BlockState& state, BlockEntity* blockEntity)
{
    // 仅在服务端生成掉落物品（lootTableManager 为空表示客户端）
    auto* lootTableManager = world.lootTableManager();
    if (lootTableManager == nullptr) {
        return;
    }

    const Block& block = state.getBlock();

    // 物品掉落受 block_drops 游戏规则门控。注意原版把该判定放在最底层的
    // `Block.popResource` / `popExperience`（1.21.11 为 block_drops），而不是放在
    // `Block.dropResources` 入口；因此这里只跳过物品掉落，绝不能提前 return，
    // 否则会把下方的 spawnAfterBreak 一并吞掉（原版 spawnAfterBreak 在生成掉落之后无条件调用，
    // 由各方块自行检查游戏规则——InfestedBlock 就在 spawnAfterBreak 内自己查 block_drops）。
    //
    // 该入口覆盖 /fill destroy、/setblock destroy、爆炸、下落方块等全部世界侧破坏路径；
    // 玩家挖掘路径走 server 侧 BlockDropHandler::generateDrops，那里另有一道同样的掉落守卫。
    if (world.getGameRules().getBoolean(world::gamerule::GameRuleKeys::BLOCK_DROPS)) {
        // 获取方块的掉落表。没有掉落表（或掉落表返回空）只意味着没有物品掉落，
        // 不影响 spawnAfterBreak。
        if (const loot::LootTable* lootTable = block.getLootTable(*lootTableManager)) {
            // 构建掉落上下文（无玩家、无工具，与 MC 原版 Block.dropResources 行为一致）
            // MC 使用 ServerLevel.random（世界共享随机），不创建新 Random 对象
            auto contextBuilder = loot::LootContextBuilder(world);

            // 设置方块状态和位置参数
            contextBuilder.withParameter(loot::LootParams::BLOCK_STATE, const_cast<BlockState*>(&state));
            contextBuilder.withParameter(loot::LootParams::BLOCK_POS, const_cast<BlockPos*>(&pos));

            // ORIGIN：方块掉落的来源位置取方块中心（Vec3.atCenterOf(pos) 的语义），
            // 供探险地图等按世界坐标搜索结构的掉落函数使用。
            contextBuilder.withOwnedValue(loot::LootParams::ORIGIN, pos.center());

            // BLOCK_ENTITY：容器类方块（箱子、熔炉等）的掉落表需要它复制名称与组件。
            // vanilla 通过 LootContextParams.BLOCK_ENTITY 传递，Cubium 以可选参数形式接入。
            if (blockEntity != nullptr) {
                contextBuilder.withParameter(loot::LootParams::BLOCK_ENTITY, blockEntity);
            }

            // 设置掉落表解析器和条件解析器
            contextBuilder.withLootTableResolver([lootTableManager](const std::string& id) -> const loot::LootTable* {
                return lootTableManager->getTable(id);
            });
            contextBuilder.withPredicateResolver(
                [lootTableManager](const std::string& id) -> const loot::LootCondition* {
                    return lootTableManager->getPredicate(id);
                });

            if (auto context = contextBuilder.build(loot::LootParameterSets::block())) {
                // 生成掉落物品并在世界中生成掉落物实体
                auto drops = lootTable->generate(*context);
                if (!drops.empty()) {
                    ItemDropHelper::spawnItemEntities(&world, pos, drops, world.getRandom());
                }
            }
        }
    }

    // 破坏附加效果（如虫蚀方块生成蠹虫、刷怪笼生成怪物）：无论是否有掉落表、掉落是否为空、
    // block_drops 是否关闭，都必须调用（原版 Block.dropResources 在掉落之后无条件调用）。
    block.spawnAfterBreak(world, pos, state, nullptr, false);
}

} // namespace mc
