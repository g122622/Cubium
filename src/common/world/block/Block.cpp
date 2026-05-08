#include "Block.hpp"
#include "BlockRegistry.hpp"
#include "BlockSoundType.hpp"
#include "Material.hpp"
#include "BlockPos.hpp"
#include "PlantType.hpp"
#include "../IWorld.hpp"
#include "../fluid/Fluid.hpp"
#include "../fluid/FluidRegistry.hpp"
#include "../fluid/fluids/EmptyFluid.hpp"
#include "../../entity/entities/player/Player.hpp"
#include "../../entity/core/Entity.hpp"
#include "../../item/core/ItemStack.hpp"
#include "../../item/context/ItemUseContext.hpp"
#include "../../item/context/BlockItemUseContext.hpp"
#include "../blockentity/BlockEntity.hpp"
#include "../../util/math/random/IRandom.hpp"
#include "../../util/math/Vector3.hpp"
#include "../../util/Direction.hpp"
#include "../../entity/loot/LootTable.hpp"
#include "../../entity/loot/LootConditions.hpp"
#include "../../sound/SoundCategory.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace mc {

// ============================================================================
// VoxelShapes
// ============================================================================

namespace {
    CollisionShape g_emptyShape = CollisionShape::empty();
    CollisionShape g_fullBlockShape = CollisionShape::fullBlock();
}

const CollisionShape& VoxelShapes::empty() {
    return g_emptyShape;
}

const CollisionShape& VoxelShapes::fullCube() {
    return g_fullBlockShape;
}

CollisionShape VoxelShapes::cube(f32 x1, f32 y1, f32 z1, f32 x2, f32 y2, f32 z2) {
    // 兼容两种写法：
    // 1) 像素坐标（0-16）- 与 Java 版 Block.makeCuboidShape 一致
    // 2) 归一化坐标（0-1）- 项目内已有大量此写法
    const f32 maxCoord = std::max({
        std::abs(x1), std::abs(y1), std::abs(z1),
        std::abs(x2), std::abs(y2), std::abs(z2)
    });

    if (maxCoord <= 1.0f) {
        return CollisionShape::box(x1, y1, z1, x2, y2, z2);
    }

    return CollisionShape::fromPixelBox(x1, y1, z1, x2, y2, z2);
}

// ============================================================================
// BlockState
// ============================================================================

BlockState::BlockState(const Block& block,
                       std::unordered_map<const IProperty*, size_t> values,
                       u32 stateId)
    : StateHolder<Block, BlockState>(&block, std::move(values), stateId) {
    cacheProperties();
}

void BlockState::cacheProperties() {
    // 缓存方块属性
    m_isSolid = m_owner->isSolid(*this);
    m_isOpaque = m_owner->isOpaque(*this);
    m_blocksMovement = m_owner->material().blocksMovement();
    m_isLiquid = m_owner->material().isLiquid();
    m_isFlammable = m_owner->material().isFlammable();
    m_lightLevel = m_owner->lightLevel();
    // 与 Java 版对齐：通过虚函数计算缓存值，确保子类重写生效。
    m_opacity = m_owner->getOpacity(*this, nullptr, nullptr);
    m_propagatesSkylightDown = m_owner->propagatesSkylightDown(*this, nullptr, nullptr);
    m_hardness = m_owner->hardness();
    m_resistance = m_owner->resistance();
    m_blockId = m_owner->blockId();
    m_harvestTool = m_owner->harvestTool();
    m_harvestLevel = m_owner->harvestLevel();
}

bool BlockState::isAir() const {
    return m_owner->isAir(*this);
}

const CollisionShape& BlockState::getCollisionShape() const {
    return m_owner->getCollisionShape(*this);
}

const CollisionShape& BlockState::getShape() const {
    return m_owner->getShape(*this);
}

const CollisionShape& BlockState::getOcclusionShape() const {
    return m_owner->getOcclusionShape(*this);
}

CollisionShape BlockState::getFaceOcclusionShape(Direction direction) const {
    return m_owner->getFaceOcclusionShape(*this, direction);
}

bool BlockState::hasOpaqueCollisionShape() const {
    // 如果方块不透明且有碰撞，则有不透明碰撞形状
    // 参考: net.minecraft.block.AbstractBlock.AbstractBlockState#hasOpaqueCollisionShape
    return m_isOpaque && m_owner->material().blocksMovement();
}

float BlockState::getAmbientOcclusionLightValue() const {
    // 如果方块有不透明碰撞形状，返回0.2（产生阴影）
    // 否则返回1.0（透明方块如玻璃、树叶不产生阴影）
    // 参考: net.minecraft.block.AbstractBlock.AbstractBlockState#getAmbientOcclusionLightValue
    return hasOpaqueCollisionShape() ? 0.2f : 1.0f;
}

bool BlockState::isSolidSide(IWorld& world, const BlockPos& pos, Direction side) const {
    return m_owner->isSolidSide(*this, world, pos, side);
}

bool BlockState::isOpaqueCube(IWorld& world, const BlockPos& pos) const {
    // 如果方块是固体的且有不透明碰撞形状，则为不透明完整方块
    MC_UNUSED(world);
    MC_UNUSED(pos);
    return m_isSolid && m_isOpaque && hasOpaqueCollisionShape();
}

const ResourceLocation& BlockState::blockLocation() const {
    return m_owner->blockLocation();
}

const fluid::FluidState* BlockState::getFluidState() const {
    return m_owner->getFluidState(*this);
}

const Material& BlockState::getMaterial() const {
    return m_owner->material();
}

const BlockSoundType& BlockState::getSoundType() const {
    return m_owner->getSoundType();
}

std::string BlockState::toModelKey() const {
    if (m_values.empty()) {
        return "";
    }

    // 按属性名排序，确保模型键稳定且与资源系统缓存键一致
    // 直接拼接字符串，避免 ostringstream 的格式化和分配开销。
    std::vector<std::pair<const IProperty*, size_t>> sortedValues;
    sortedValues.reserve(m_values.size());
    for (const auto& entry : m_values) {
        sortedValues.emplace_back(entry.first, entry.second);
    }

    std::sort(sortedValues.begin(), sortedValues.end(),
        [](const auto& a, const auto& b) {
            return a.first->name() < b.first->name();
        });

    std::string result;
    result.reserve(sortedValues.size() * 16);
    bool first = true;
    for (const auto& [prop, valueIndex] : sortedValues) {
        if (!first) {
            result.push_back(',');
        }
        result += prop->name();
        result.push_back('=');
        result += prop->valueToString(valueIndex);
        first = false;
    }
    return result;
}

std::string BlockState::ownerName() const {
    return m_owner->toString();
}

u8 BlockState::getHarvestTool() const {
    return m_harvestTool;
}

i32 BlockState::getHarvestLevel() const {
    return m_harvestLevel;
}

bool BlockState::isToolEffective(u8 toolType, i32 harvestLevel) const {
    // 检查工具类型是否匹配
    if (m_harvestTool != toolType) {
        return false;
    }
    // 检查工具等级是否足够
    return harvestLevel >= m_harvestLevel;
}

bool BlockState::requiresTool() const {
    return m_owner->requiresTool();
}

bool BlockState::isStickyBlock() const {
    return m_owner->isStickyBlock(*this);
}

bool BlockState::canStickTo(const BlockState& other) const {
    return m_owner->canStickTo(*this, other);
}

// ============================================================================
// BlockProperties
// ============================================================================

namespace {
    /**
     * @brief 根据材质获取默认的声音类型
     *
     * 参考 Java 版 net.minecraft.block.SoundType 和 Material 的对应关系
     */
    const BlockSoundType& getDefaultSoundType(const Material& material) {
        // 木头材质 -> 木头声音
        if (&material == &Material::WOOD || &material == &Material::NETHER_WOOD) {
            return BlockSoundTypes::WOOD;
        }
        // 泥土材质 -> 泥土声音
        if (&material == &Material::EARTH) {
            return BlockSoundTypes::DIRT;
        }
        // 草材质 -> 草声音
        if (&material == &Material::PLANT || &material == &Material::REPLACEABLE_PLANT
            || &material == &Material::LEAVES || &material == &Material::TALL_PLANTS
            || &material == &Material::OCEAN_PLANT || &material == &Material::SEA_GRASS
            || &material == &Material::MOSS) {
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
}

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
    , m_soundType(&getDefaultSoundType(material)) {
}

BlockProperties& BlockProperties::hardness(f32 value) {
    m_hardness = value;
    return *this;
}

BlockProperties& BlockProperties::resistance(f32 value) {
    m_resistance = value;
    return *this;
}

BlockProperties& BlockProperties::lightLevel(u8 level) {
    m_lightLevel = level > 15 ? 15 : level;
    return *this;
}

BlockProperties& BlockProperties::noCollision() {
    m_hasCollision = false;
    return *this;
}

BlockProperties& BlockProperties::notSolid() {
    m_isSolid = false;
    return *this;
}

BlockProperties& BlockProperties::requiresTool() {
    m_requiresTool = true;
    return *this;
}

BlockProperties& BlockProperties::flammable(bool value) {
    m_isFlammable = value;
    return *this;
}

BlockProperties& BlockProperties::replaceable() {
    m_isReplaceable = true;
    return *this;
}

BlockProperties& BlockProperties::strength(f32 value) {
    m_hardness = value;
    m_resistance = value;
    return *this;
}

BlockProperties& BlockProperties::opacity(i32 value) {
    m_opacity = value < 0 ? 0 : (value > 15 ? 15 : value);
    return *this;
}

BlockProperties& BlockProperties::propagatesSkylightDown(bool value) {
    m_propagatesSkylightDown = value;
    return *this;
}

BlockProperties& BlockProperties::harvestTool(u8 toolType) {
    m_harvestTool = toolType;
    return *this;
}

BlockProperties& BlockProperties::harvestLevel(i32 level) {
    m_harvestLevel = level < 0 ? 0 : level;
    return *this;
}

// ============================================================================
// Block
// ============================================================================

Block* Block::getBlock(u32 blockId) {
    return BlockRegistry::instance().getBlock(blockId);
}

Block* Block::getBlock(const ResourceLocation& id) {
    return BlockRegistry::instance().getBlock(id);
}

BlockState* Block::getBlockState(u32 stateId) {
    return BlockRegistry::instance().getBlockState(stateId);
}

void Block::forEachBlock(std::function<void(Block&)> callback) {
    BlockRegistry::instance().forEachBlock(std::move(callback));
}

void Block::forEachBlockState(std::function<void(const BlockState&)> callback) {
    BlockRegistry::instance().forEachBlockState(std::move(callback));
}

Block::Block(BlockProperties properties)
    : m_material(properties.m_material)
    , m_hardness(properties.m_hardness)
    , m_resistance(properties.m_resistance)
    , m_lightLevel(properties.m_lightLevel)
    , m_opacity(properties.m_opacity)
    , m_hasCollision(properties.m_hasCollision)
    , m_isFlammable(properties.m_isFlammable)
    , m_propagatesSkylightDown(properties.m_propagatesSkylightDown)
    , m_requiresTool(properties.m_requiresTool)
    , m_isReplaceable(properties.m_isReplaceable)
    , m_ticksRandomly(properties.m_ticksRandomly)
    , m_harvestTool(properties.m_harvestTool)
    , m_harvestLevel(properties.m_harvestLevel)
    , m_lootTableId(properties.m_lootTableId)
    , m_soundType(properties.m_soundType)
    , m_slipperiness(properties.m_slipperiness)
    , m_speedFactor(properties.m_speedFactor)
    , m_jumpFactor(properties.m_jumpFactor) {
    // 所有方块都必须至少拥有一个基础状态。
    // 这与 Java 版 StateContainer 行为一致，可避免遗漏 createBlockState()
    // 时在注册阶段出现空指针崩溃。
    auto container = StateContainer<Block, BlockState>::Builder(*this)
        .create([](const Block& block, std::unordered_map<const IProperty*, size_t> values, u32 id) {
            return std::make_unique<BlockState>(block, std::move(values), id);
        });
    createBlockState(std::move(container));
}

void Block::createBlockState(std::unique_ptr<StateContainer<Block, BlockState>> container) {
    m_stateContainer = std::move(container);
    m_defaultState = &m_stateContainer->baseState();
}

void Block::setDefaultState(const BlockState& state) {
    m_defaultState = &state;
}

// ============================================================================
// 实体交互
// ============================================================================

void Block::onLanded(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const {
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    // 默认实现：Y速度归零
    // MC 1.16.5: entityIn.setMotion(entityIn.getMotion().mul(1.0D, 0.0D, 1.0D));
    entity.setVelocity(entity.velocity().x, 0.0f, entity.velocity().z);
}

void Block::onEntityWalk(const BlockState& state, IWorld& world, const BlockPos& pos, Entity& entity) const {
    MC_UNUSED(state);
    MC_UNUSED(world);
    MC_UNUSED(pos);
    MC_UNUSED(entity);
    // 默认实现：无操作
    // 子类可以重写此方法实现特殊行为（如岩浆块造成伤害、岩浆方块产生气泡等）
}

const CollisionShape& Block::getShape(const BlockState& state) const {
    (void)state;
    return VoxelShapes::fullCube();
}

const CollisionShape& Block::getCollisionShape(const BlockState& state) const {
    if (!m_hasCollision) {
        return VoxelShapes::empty();
    }
    return getShape(state);
}

const CollisionShape& Block::getOcclusionShape(const BlockState& state) const {
    return getShape(state);
}

CollisionShape Block::getFaceOcclusionShape(const BlockState& state, Direction direction) const {
    // 默认实现：如果遮挡形状是完整方块，返回完整方块
    // 否则返回遮挡形状在指定方向的切片
    const CollisionShape& occlusion = getOcclusionShape(state);
    if (occlusion.isFullBlock()) {
        return CollisionShape::fullBlock();
    }
    // 对于非完整方块，返回遮挡形状
    // TODO: 实现 CollisionShape 的面切片方法
    return occlusion;
}

bool Block::useShapeForLightOcclusion(const BlockState& state) const {
    (void)state;
    // 默认不使用形状进行光照遮挡
    // 台阶、楼梯、栅栏等非完整方块需要覆盖此方法返回 true
    return false;
}

bool Block::isAir(const BlockState& state) const {
    (void)state;
    // 基类默认返回 false
    // AirBlock 会覆盖此方法返回 true
    return false;
}

bool Block::isSolid(const BlockState& state) const {
    (void)state;
    return m_material->isSolid();
}

bool Block::isOpaque(const BlockState& state) const {
    (void)state;
    return m_material->isOpaque();
}

i32 Block::getOpacity(const BlockState& state, IWorld* world, const BlockPos* pos) const {
    (void)world;
    (void)pos;
    // 默认实现对齐 Java 1.16.5：
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

bool Block::propagatesSkylightDown(const BlockState& state, IWorld* world, const BlockPos* pos) const {
    (void)state;
    (void)world;
    (void)pos;
    // 默认返回属性值
    return m_propagatesSkylightDown;
}

const fluid::FluidState* Block::getFluidState(const BlockState& state) const {
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

void Block::tick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    // 默认实现：空操作
    // 需要tick行为的方块应重写此方法
    (void)world;
    (void)pos;
    (void)state;
    (void)random;
}

void Block::randomTick(IWorld& world, const BlockPos& pos, BlockState& state, math::IRandom& random) {
    // MC 1.16.5 行为：默认调用 tick()
    // 参考: net.minecraft.block.AbstractBlock.randomTick()
    // 如果方块设置了 ticksRandomly = true 但没有重写 randomTick()，
    // 则在随机刻时执行 tick() 方法
    tick(world, pos, state, random);
}

void Block::neighborChanged(IWorld& world, const BlockPos& pos,
                             Block& neighborBlock, const BlockPos& neighborPos,
                             bool isMoving) {
    // 默认实现：空操作
    // 需要响应邻居变化的方块应重写此方法
    (void)world;
    (void)pos;
    (void)neighborBlock;
    (void)neighborPos;
    (void)isMoving;
}

void Block::onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 默认实现：空操作
    // 需要特殊初始化的方块应重写此方法
    (void)world;
    (void)pos;
    (void)state;
}

void Block::onBlockRemoved(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 默认实现：空操作
    // 需要特殊清理的方块应重写此方法
    (void)world;
    (void)pos;
    (void)state;
}

bool Block::isSolidSide(const BlockState& state, IWorld& world, const BlockPos& pos, Direction side) const {
    // 参考 MC 1.16.5: Block.isSolidSide
    // 冰块特殊处理：冰块的侧面不被认为是实体面（用于流体流动判断）
    if (*m_material == Material::ICE) {
        return false;
    }
    // 默认实现：检查方块是否为固体且有碰撞
    (void)state;
    (void)world;
    (void)pos;
    (void)side;
    return m_material->isSolid() && m_hasCollision;
}

const BlockState& Block::rotate(const BlockState& state, Rotation rotation) const {
    // 默认实现：不旋转，返回原状态
    (void)rotation;
    return state;
}

const BlockState& Block::mirror(const BlockState& state, Mirror mirror) const {
    // 默认实现：不镜像，返回原状态
    (void)mirror;
    return state;
}

const loot::LootTable* Block::getLootTable(const loot::LootTableManager& manager) const {
    if (m_lootTableId.empty()) {
        return nullptr;
    }
    return manager.getTable(m_lootTableId);
}

// ============================================================================
// 新增虚方法默认实现
// ============================================================================

void Block::fillStateContainer(StateContainer<Block, BlockState>& container) {
    // 默认实现：无属性
    (void)container;
}

const BlockState& Block::getDefaultState() const {
    return *m_defaultState;
}

BlockState Block::getStateForPlacement(BlockItemUseContext& context) {
    // 默认实现：返回默认状态
    (void)context;
    return defaultState();
}

void Block::onBlockPlacedBy(IWorld& world, const BlockPos& pos, const BlockState& state) {
    // 默认实现：空操作
    (void)world;
    (void)pos;
    (void)state;
}

BlockState Block::updatePostPlacement(
    const BlockState& state,
    Direction facing,
    const BlockState& facingState,
    IWorld& world,
    const BlockPos& currentPos,
    const BlockPos& facingPos) {
    // 默认实现：返回原状态
    (void)facing;
    (void)facingState;
    (void)world;
    (void)currentPos;
    (void)facingPos;
    return state;
}

bool Block::isValidPosition(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos) const {
    // 默认实现：总是有效
    (void)state;
    (void)world;
    (void)pos;
    return true;
}

bool Block::isReplaceable(
    const BlockState& state,
    BlockItemUseContext& context) const {
    // 默认实现：使用 BlockProperties 的 isReplaceable 值
    (void)context;
    return m_isReplaceable;
}

bool Block::canSustainPlant(
    const BlockState& state,
    IBlockReader& world,
    const BlockPos& pos,
    Direction facing,
    const IPlantable& plant) const {
    // 默认实现：不支持任何植物
    // 子类（如草方块、泥土、耕地等）应重写此方法
    (void)state;
    (void)world;
    (void)pos;
    (void)facing;
    (void)plant;
    return false;
}

ActionResultType Block::onBlockActivated(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Player& player,
    Hand hand,
    const BlockRaycastResult& hit) {
    // 默认实现：返回 Pass（未处理）
    (void)state;
    (void)world;
    (void)pos;
    (void)player;
    (void)hand;
    (void)hit;
    return ActionResultType::Pass;
}

std::unique_ptr<BlockEntity> Block::createBlockEntity(const BlockPos& pos) {
    // 默认实现：无方块实体
    (void)pos;
    return nullptr;
}

i32 Block::getComparatorInputOverride(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos) const {
    // 默认实现：无比较器输入覆盖
    (void)state;
    (void)world;
    (void)pos;
    return 0;
}

Material::PushReaction Block::getPushReaction(const BlockState& state) const {
    // 默认实现：正常推动
    (void)state;
    return Material::PushReaction::Normal;
}

// ============================================================================
// 静态辅助方法
// ============================================================================

bool Block::shouldSideBeRendered(
    const BlockState& state,
    IWorld& world,
    const BlockPos& pos,
    Direction face) {

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

bool Block::hasSolidSideOnTop(IWorld& world, const BlockPos& pos) {
    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr) {
        return false;
    }

    // 参考 MC 1.16.5: Block.hasSolidSideOnTop
    // 检查顶面是否为实体面
    return state->isSolidSide(world, pos, Direction::Up);
}

bool Block::hasEnoughSolidSide(
    IWorld& world,
    const BlockPos& pos,
    Direction direction) {

    const BlockState* state = world.getBlockState(pos);
    if (state == nullptr || state->isAir()) {
        return false;
    }

    // 参考 MC 1.16.5: Block.hasEnoughSolidSide
    // 检查指定方向是否有足够大的固体面
    return state->isSolidSide(world, pos, direction);
}

bool Block::doesSideFillSquare(const CollisionShape& shape, Direction direction) {
    // 参考 MC 1.16.5: Block.doesSideFillSquare
    // 如果形状是完整方块，则填充整个面
    if (shape.isFullBlock()) {
        return true;
    }

    // 检查形状在指定面上的投影是否填充整个面
    // 对于非完整方块，需要检查投影面积
    // 简化实现：非完整方块的面不填充方形
    return false;
}

// ============================================================================
// 攻击和交互
// ============================================================================

void Block::harvestBlock(
    IWorld& world,
    Player& player,
    const BlockPos& pos,
    const BlockState& state,
    BlockEntity* blockEntity,
    const ItemStack* stack) {

    // 参考 MC 1.16.5: Block.harvestBlock
    // 播放破坏音效
    const BlockSoundType& soundType = state.owner().getSoundType();
    world.playSound(
        soundType.getBreakSound(),
        sound::SoundCategory::Blocks,
        pos.center(),
        (soundType.getVolume() + 1.0f) / 2.0f,
        soundType.getPitch() * 0.8f
    );

    // 掉落物由 BlockDropHandler 处理
    MC_UNUSED(player);
    MC_UNUSED(blockEntity);
    MC_UNUSED(stack);
}

f32 Block::getPlayerRelativeBlockHardness(
    Player& player,
    IBlockReader& world,
    const BlockPos& pos,
    const BlockState& state) const {

    // 参考 MC 1.16.5: Block.getPlayerRelativeBlockHardness
    // 基础挖掘速度 = 1 / (hardness * 30) 对于硬度 > 0
    // 创造模式：瞬间破坏

    MC_UNUSED(player);
    MC_UNUSED(world);
    MC_UNUSED(pos);

    f32 hardness = state.hardness();
    if (hardness <= 0.0f) {
        // 硬度为0的方块（如空气、水）瞬间破坏
        return 1.0f;
    }

    // TODO: 获取玩家的挖掘速度倍率
    // f32 digSpeed = player.getDigSpeed(state, pos);
    // bool canHarvest = !state.requiresTool() || player.canHarvestBlock(state);
    //
    // if (canHarvest) {
    //     return digSpeed / hardness / 30.0f;
    // } else {
    //     return digSpeed / hardness / 30.0f / 100.0f;
    // }

    // 简化实现：返回基础挖掘速度
    return 1.0f / (hardness * 30.0f);
}

} // namespace mc
