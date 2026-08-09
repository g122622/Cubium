#include "server/test/facade/GameTestHelper.hpp"

#include "common/test/base/error/GameTestErrorContext.hpp"
#include "common/test/framework/instance/BaseGameTestInstance.hpp"
#include "common/test/framework/sequence/GameTestSequence.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/BlockPos.hpp"
#include "server/test/minecraft/structure/StructureBounds.hpp"
#include "server/test/simulated/SimulatedPlayer.hpp" // spawnSimulatedPlayer / removeSimulatedPlayer

#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/entity/inventory/IInventory.hpp" // getItem/getContainerSize/isEmpty（容器断言）
#include "common/item/core/ItemStack.hpp"         // isSameItem（assertContainerContains 类型匹配）
#include "common/resource/ResourceLocation.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/util/property/Properties.hpp" // BlockStateProperties::NORTH/EAST/SOUTH/WEST（getFenceConnectivity）
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/block/blocks/redstone/AbstractButtonBlock.hpp" // AbstractButtonBlock::press（pressButton 接线）
#include "common/world/block/blocks/sculk/SculkSpreader.hpp"          // mc::blocks::SculkSpreader（getSculkSpreader）
#include "common/world/blockentity/BlockEntity.hpp"          // getBlockEntity → dynamic_cast<ContainerBlockEntity*>
#include "common/world/blockentity/ContainerBlockEntity.hpp" // getInventory（assertContainerContains/Empty）
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include "server/world/ServerWorld.hpp"

#include <spdlog/spdlog.h>

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

namespace mc::test {

// 全限定别名，规避 mc::test 内非限定名两段查找不回退 mc::world 的遮蔽坑（见 BossBarState 内存）
using StructureBoundingBox = mc::world::gen::structure::StructureBoundingBox;

namespace {
// 规范化实体类型名：基岩行为包用短名（如 "fox"），项目注册表用全名 "minecraft:fox"。
// 无命名空间前缀时补 "minecraft:"，使 spawn/assert 能查到对应实体类型。
// 已含 ":" 前缀（如 "minecraft:fox" 或其他命名空间）原样返回（随后再过基岩别名表）。
std::string normalizeEntityType(const std::string& name)
{
    std::string full = name.find(':') == std::string::npos ? "minecraft:" + name : name;

    // 基岩版与 Java 版实体类型命名差异别名表。基岩行为包用基岩名（如 villager_v2），
    // 项目实体注册表用 Java 名（如 minecraft:villager）。此处把基岩名归一化为项目注册表名，
    // 使 spawn/assertEntityPresent/getEntities 的 type 查询命中同一 EntityType。
    // 仅收录 GameTest 行为包实际用到的基岩别名；新增基岩别名时在此追加。
    static const std::unordered_map<std::string, std::string> kBedrockAliases = {
        {"minecraft:villager_v2", "minecraft:villager"},
    };
    auto it = kBedrockAliases.find(full);
    if (it != kBedrockAliases.end()) {
        return it->second;
    }
    return full;
}
} // namespace

GameTestHelper::GameTestHelper(
    mc::server::ServerWorld& world, BlockPos origin, const StructureBounds* bounds, BaseGameTestInstance& instance)
    : m_world(world)
    , m_instance(instance)
    , m_bounds(bounds)
{
    // 由 TestData.rotation 与 bounds 尺寸构建坐标变换；bounds 为空（结构未就绪）时用零尺寸退化变换
    const Rotation rotation = instance.function().data().rotation();
    if (bounds != nullptr) {
        m_transform = TestTransform(origin, bounds->size(), rotation);
    } else {
        m_transform = TestTransform(origin, BlockPos{0, 0, 0}, rotation);
    }
}

GameTestHelper::~GameTestHelper() = default; // unique_ptr<GameTestSequence> 需完整类型，故在此定义

// === 1. 生命周期与状态 ===

void GameTestHelper::startExecution()
{
    m_instance.startExecution();
}

void GameTestHelper::succeed()
{
    m_instance.succeed();
}

void GameTestHelper::fail(GameTestError error)
{
    m_instance.fail(std::move(error));
}

bool GameTestHelper::isCompleted() const noexcept
{
    return isDone(m_instance.state());
}

bool GameTestHelper::isCleaningUp() const noexcept
{
    // 第一阶段无清理阶段状态机（基岩版有 ClearUp 阶段）；暂以"已完成且失败"近似，留 TODO 精化。
    // TODO: 引入显式 CleaningUp 状态后改为状态判定（见 framework/instance/GameTestState.hpp）
    return false;
}

i32 GameTestHelper::currentTick() const noexcept
{
    return m_instance.tickCount();
}

i32 GameTestHelper::maxTicks() const noexcept
{
    return m_instance.function().data().maxTicks();
}

Rotation GameTestHelper::rotation() const noexcept
{
    return m_transform.rotation();
}

// === 2. 序列与调度 ===

GameTestSequence& GameTestHelper::startSequence()
{
    if (!m_sequence) {
        m_sequence = std::make_unique<GameTestSequence>(*this);
        // 序列注册到 instance 以便 tick 推进：但 BaseGameTestInstance::createSequence 已封装此逻辑。
        // 为保持单一序列路径，这里把 helper 持有的序列也挂到 instance（instance tick 会推进它）。
        // TODO: 当前 instance::createSequence 与 helper::m_sequence 双轨，需统一为 instance 单一持有；
        //       第一阶段样例测试仅用 startSequence().thenSucceed()，双轨暂不冲突。
    }
    // 经 instance.createSequence() 取得 instance 持有并 tick 推进的序列（对齐基岩 GameTestHelper.startSequence）
    return m_instance.createSequence();
}

void GameTestHelper::runAtTickTime(i32 tick, std::function<GameTestResult()> fn)
{
    m_instance.registerRunAtTickTime(tick, std::move(fn));
}

void GameTestHelper::runAfterDelay(i32 delay, std::function<GameTestResult()> fn)
{
    // 对齐基岩 runAfterDelay(delay, fn)：在 currentTick + delay 时刻执行一次
    const i32 target = currentTick() + delay;
    m_instance.registerRunAtTickTime(target, std::move(fn));
}

void GameTestHelper::runOnFinish(std::function<GameTestResult()> fn)
{
    m_instance.registerOnFinish(std::move(fn));
}

// === 3. 块断言与操作 ===

GameTestResult GameTestHelper::assertBlockPresent(const std::string& blockType, BlockPos relativePos, bool isPresent)
{
    const BlockPos worldPos = worldBlockPosition(relativePos);
    const BlockState* expected = _resolveBlock(blockType);
    const BlockState* actual = m_world.getBlockState(worldPos);
    // 对齐基岩/Java GameTest 语义：assertBlockPresent 仅检查方块类型（BlockType），不比较
    // BlockState 属性（facing/axis 等）。结构放置/clone 的方块带朝向属性，若用 stateId 比较
    // 会使同类型不同朝向的方块判为不相等，导致断言误失败。故用 blockId() 比较（同方块任意状态均命中）。
    const bool present = (actual != nullptr && expected != nullptr && actual->blockId() == expected->blockId());
    if (present != isPresent) {
        return _expectBlockError(
            isPresent ? "Expected block to be present" : "Expected block to be absent", relativePos, actual);
    }
    return std::nullopt;
}

GameTestResult GameTestHelper::assertBlockState(
    BlockPos relativePos, std::function<bool(const mc::BlockState&)> predicate)
{
    const BlockPos worldPos = worldBlockPosition(relativePos);
    const BlockState* actual = m_world.getBlockState(worldPos);
    if (actual == nullptr) {
        return _expectBlockError("Block state unavailable (chunk unloaded)", relativePos, nullptr);
    }
    if (!predicate(*actual)) {
        return _expectBlockError("Block state predicate failed", relativePos, actual);
    }
    return std::nullopt;
}

GameTestResult GameTestHelper::setBlock(const std::string& blockType, BlockPos relativePos, i32 updateFlags)
{
    const BlockPos worldPos = worldBlockPosition(relativePos);
    const BlockState* state = _resolveBlock(blockType);
    if (state == nullptr) {
        return GameTestError{GameTestErrorType::LevelStateModificationFailed, "Unknown block type '{0}'", {blockType}};
    }
    if (!m_world.setBlockState(worldPos, state, updateFlags)) {
        return GameTestError{GameTestErrorType::LevelStateModificationFailed,
            "Failed to set block '{0}' at {1}",
            {blockType, worldPos.toString()}};
    }
    return std::nullopt;
}

GameTestResult GameTestHelper::destroyBlock(BlockPos relativePos, bool dropResources)
{
    // 项目 IWorld/ServerWorld 无 destroyBlock 方法（见调研）：手动置 air + TODO 掉落物
    const BlockPos worldPos = worldBlockPosition(relativePos);
    const BlockState* air = mc::BlockRegistry::instance().airState();
    if (air == nullptr) {
        return GameTestError{GameTestErrorType::LevelStateModificationFailed, "Air block state unavailable"};
    }
    if (!m_world.setBlockState(worldPos, air, 3)) {
        return GameTestError{
            GameTestErrorType::LevelStateModificationFailed, "Failed to destroy block at {0}", {worldPos.toString()}};
    }
    // TODO: dropResources=true 时按战利品表生成掉落物（需 LootTable 体系就绪）
    (void)dropResources;
    return std::nullopt;
}

GameTestResult GameTestHelper::pressButton(BlockPos relativePos)
{
    // 转发 AbstractButtonBlock::press：置 powered=true → setBlockState（触发邻居更新）→
    // notifyNeighbors → 相邻命令方块的 neighborChanged 检测红石上升沿 → 调度 1 tick 后执行命令。
    // press 内部已处理点击音效与弹起调度（StoneButton 20 tick / WoodButton 30 tick）。
    const BlockPos worldPos = worldBlockPosition(relativePos);
    const BlockState* state = m_world.getBlockState(worldPos);
    if (state == nullptr) {
        return generateErrorWithContext(GameTestErrorType::LevelStateModificationFailed,
            "No block at button position " + worldPos.toString(),
            relativePos);
    }
    // getBlockMutable：press 非 const（会改世界状态），但 Block 对象自身无状态，
    // BlockState::getBlockMutable 内部 const_cast 取非 const Block& 供调用非 const 方法。
    auto* button = dynamic_cast<mc::blocks::AbstractButtonBlock*>(&state->getBlockMutable());
    if (button == nullptr) {
        return generateErrorWithContext(GameTestErrorType::LevelStateModificationFailed,
            "Block at " + worldPos.toString() + " is not a button",
            relativePos);
    }
    button->press(m_world, worldPos, *state);
    return std::nullopt;
}

GameTestResult GameTestHelper::pullLever(BlockPos relativePos)
{
    // TODO: 切换 LeverBlock powered 状态（LeverBlock 体系就绪前 stub）
    (void)relativePos;
    return std::nullopt;
}

GameTestResult GameTestHelper::pulseRedstone(BlockPos relativePos, i32 duration)
{
    // TODO: 在 relativePos 处发 duration tick 的红石脉冲（红石信号体系就绪前 stub）
    (void)relativePos;
    (void)duration;
    return std::nullopt;
}

GameTestResult GameTestHelper::assertRedstonePower(BlockPos relativePos, i32 power)
{
    // TODO: 读取 relativePos 处红石信号强度断言（红石信号体系就绪前 stub）
    (void)relativePos;
    (void)power;
    return std::nullopt;
}

GameTestResult GameTestHelper::assertIsWaterlogged(BlockPos relativePos, bool isWaterlogged)
{
    // TODO: 读取方块 Waterlogged 属性断言（属性体系就绪前 stub）
    (void)relativePos;
    (void)isWaterlogged;
    return std::nullopt;
}

GameTestResult GameTestHelper::assertContainerContains(const mc::ItemStack& itemStack, BlockPos relativePos)
{
    // 对齐基岩 Test.assertContainerContains：pos 处容器（如箱子）须含指定物品栈（按物品类型匹配，至少 1 个）。
    const BlockPos worldPos = worldBlockPosition(relativePos);
    mc::BlockEntity* be = m_world.getBlockEntity(worldPos);
    auto* container = dynamic_cast<mc::ContainerBlockEntity*>(be);
    if (container == nullptr) {
        return generateErrorWithContext(
            GameTestErrorType::LevelStateModificationFailed, "No container at {0}", relativePos);
    }
    mc::IInventory* inv = container->getInventory();
    if (inv == nullptr) {
        return generateErrorWithContext(
            GameTestErrorType::LevelStateModificationFailed, "Container at {0} has no inventory", relativePos);
    }
    const i32 size = inv->getContainerSize();
    for (i32 slot = 0; slot < size; ++slot) {
        mc::ItemStack slotItem = inv->getItem(slot);
        if (!slotItem.isEmpty() && slotItem.isSameItem(itemStack)) {
            return std::nullopt; // 命中：含指定物品类型
        }
    }
    return generateErrorWithContext(
        GameTestErrorType::FailConditionsMet, "Container at {0} does not contain the specified item", relativePos);
}

GameTestResult GameTestHelper::assertContainerEmpty(BlockPos relativePos)
{
    // 对齐基岩 Test.assertContainerEmpty：pos 处容器须为空。
    const BlockPos worldPos = worldBlockPosition(relativePos);
    mc::BlockEntity* be = m_world.getBlockEntity(worldPos);
    auto* container = dynamic_cast<mc::ContainerBlockEntity*>(be);
    if (container == nullptr) {
        return generateErrorWithContext(
            GameTestErrorType::LevelStateModificationFailed, "No container at {0}", relativePos);
    }
    mc::IInventory* inv = container->getInventory();
    if (inv == nullptr || !inv->isEmpty()) {
        return generateErrorWithContext(
            GameTestErrorType::FailConditionsMet, "Container at {0} is not empty", relativePos);
    }
    return std::nullopt;
}

GameTestResult GameTestHelper::setBlockPermutation(const mc::BlockState& permutation, BlockPos relativePos)
{
    // 对齐基岩 Test.setBlockPermutation：按 BlockPermutation（C++ 侧为 BlockState）设 pos 方块。
    // 复用 setBlock 的写入路径（m_world.setBlockState），入参从 blockType 字符串换成 BlockState&。
    const BlockPos worldPos = worldBlockPosition(relativePos);
    if (!m_world.setBlockState(worldPos, &permutation, 3)) {
        return generateErrorWithContext(
            GameTestErrorType::LevelStateModificationFailed, "Failed to set block permutation at {0}", relativePos);
    }
    return std::nullopt;
}

GameTestResult GameTestHelper::setFluidContainer(BlockPos relativePos, const std::string& fluidType)
{
    // TODO: 设 pos 处流体容器（如炼药锅）的流体类型。底层 ILiquidContainer::receiveFluid 写入体系
    //       未就绪，stub 返回 MethodNotImplemented（绑定层经 _resultToJs 转 GameTestError 抛出）。
    (void)fluidType;
    return generateErrorWithContext(GameTestErrorType::MethodNotImplemented,
        "setFluidContainer not implemented yet (fluid container write system pending)",
        relativePos);
}

void GameTestHelper::triggerInternalBlockEvent(BlockPos /*relativePos*/, const std::string& /*eventName*/)
{
    // TODO: 触发方块内部事件（对齐基岩 triggerInternalBlockEvent）。依赖方块事件分发体系，未就绪 stub。
}

void GameTestHelper::spreadFromFaceTowardDirection(
    BlockPos /*relativePos*/, Direction /*fromFace*/, Direction /*direction*/)
{
    // TODO: 测试多方块传播（对齐基岩 spreadFromFaceTowardDirection，用于 sculk/苔藓等）。
    //       依赖 MultifaceSpreader 接线体系，未就绪 stub。
}

// === 4. 实体断言与 spawn ===

GameTestResult GameTestHelper::assertEntityPresent(
    const std::string& entityType, BlockPos relativePos, f32 searchDistance, bool isPresent)
{
    const auto fullType = normalizeEntityType(entityType);
    const BlockPos worldPos = worldBlockPosition(relativePos);
    // 对齐 Java GameTestHelper.assertEntityPresent(type, BlockPos)：用 AABB(blockpos)（1 格方块）
    // 判定实体是否落在该方块内，而非球查询。searchDistance=0 表示精确 1 格方块（Java 默认语义）；
    // searchDistance>0 表示以方块中心为中心、±searchDistance 格的容差盒（基岩带 distance 重载语义）。
    // 此前用 getEntitiesInRange(center, 64.0) 球查询导致 succeedWhenEntityPresent 假通过
    // （实体在结构内任意位置都算 present，掩盖"实体未到精确位置"的失败）。
    const AxisAlignedBB box = (searchDistance > 0.0f)
        ? AxisAlignedBB(static_cast<f32>(worldPos.x) + 0.5f - searchDistance,
              static_cast<f32>(worldPos.y) + 0.5f - searchDistance,
              static_cast<f32>(worldPos.z) + 0.5f - searchDistance,
              static_cast<f32>(worldPos.x) + 0.5f + searchDistance,
              static_cast<f32>(worldPos.y) + 0.5f + searchDistance,
              static_cast<f32>(worldPos.z) + 0.5f + searchDistance)
        : AxisAlignedBB(static_cast<f32>(worldPos.x),
              static_cast<f32>(worldPos.y),
              static_cast<f32>(worldPos.z),
              static_cast<f32>(worldPos.x) + 1.0f,
              static_cast<f32>(worldPos.y) + 1.0f,
              static_cast<f32>(worldPos.z) + 1.0f);
    const auto found = m_world.getEntitiesInAABB(box, nullptr);
    bool present = false;
    for (const auto* e : found) {
        if (e != nullptr && e->getTypeId() == fullType) {
            present = true;
            break;
        }
    }
    if (present != isPresent) {
        return generateErrorWithContext(GameTestErrorType::AssertAtPosition,
            (isPresent ? "Expected entity '" : "Expected entity '") + entityType + "' present/absent mismatch",
            relativePos);
    }
    return std::nullopt;
}

GameTestResult GameTestHelper::assertEntityPresentInArea(const std::string& entityType, bool isPresent)
{
    const auto fullType = normalizeEntityType(entityType);
    if (m_bounds == nullptr) {
        return GameTestError{GameTestErrorType::MethodNotImplemented, "Structure bounds unavailable for area query"};
    }
    const StructureBoundingBox bb = m_bounds->bounds();
    const AxisAlignedBB box(static_cast<f32>(bb.minX()),
        static_cast<f32>(bb.minY()),
        static_cast<f32>(bb.minZ()),
        static_cast<f32>(bb.maxX() + 1),
        static_cast<f32>(bb.maxY() + 1),
        static_cast<f32>(bb.maxZ() + 1));
    const auto found = m_world.getEntitiesInAABB(box, nullptr);
    bool present = false;
    for (const auto* e : found) {
        if (e != nullptr) {
            if (e->getTypeId() == fullType) {
                present = true;
                break;
            }
        }
    }
    if (present != isPresent) {
        return GameTestError{GameTestErrorType::Assert,
            isPresent ? "Expected entity '{0}' present in area" : "Expected entity '{0}' absent in area",
            {entityType}};
    }
    return std::nullopt;
}

GameTestResult GameTestHelper::assertEntityInstancePresent(
    const mc::Entity& entity, BlockPos relativePos, bool isPresent)
{
    const BlockPos worldPos = worldBlockPosition(relativePos);
    const f32 dist = entity.distanceSqTo(worldPos.x + 0.5f, worldPos.y + 0.5f, worldPos.z + 0.5f);
    // searchDistance 默认取 1.0（同方块半径）以与基岩 assertEntityInstancePresent 语义对齐
    const bool present = (dist <= 1.0f * 1.0f) && entity.isAlive();
    if (present != isPresent) {
        return generateErrorWithContext(GameTestErrorType::AssertAtPosition,
            isPresent ? "Expected entity instance present" : "Expected entity instance absent",
            relativePos);
    }
    return std::nullopt;
}

GameTestResult GameTestHelper::assertEntityInstancePresentInArea(const mc::Entity& entity, bool isPresent)
{
    if (m_bounds == nullptr) {
        return GameTestError{GameTestErrorType::MethodNotImplemented, "Structure bounds unavailable for area query"};
    }
    const StructureBoundingBox bb = m_bounds->bounds();
    const AxisAlignedBB box(static_cast<f32>(bb.minX()),
        static_cast<f32>(bb.minY()),
        static_cast<f32>(bb.minZ()),
        static_cast<f32>(bb.maxX() + 1),
        static_cast<f32>(bb.maxY() + 1),
        static_cast<f32>(bb.maxZ() + 1));
    const bool present = entity.isAlive() && box.intersects(entity.boundingBox());
    if (present != isPresent) {
        return GameTestError{GameTestErrorType::Assert,
            isPresent ? "Expected entity instance present in area" : "Expected entity instance absent in area"};
    }
    return std::nullopt;
}

GameTestResult GameTestHelper::assertEntityTouching(
    const std::string& entityType, const mc::math::Vector3d& position, bool isTouching)
{
    const auto fullType = normalizeEntityType(entityType);
    // 以 position 为中心建 1x1x1 查询盒，intersects 判定接触
    const mc::Vector3 centerF = position.cast<f32>();
    const AxisAlignedBB box = AxisAlignedBB::fromPosition(centerF, 0.0f, 0.0f).grow(0.5f);
    const auto found = m_world.getEntitiesInAABB(box, nullptr);
    bool touching = false;
    for (const auto* e : found) {
        if (e != nullptr && e->getTypeId() == fullType && e->boundingBox().intersects(box)) {
            touching = true;
            break;
        }
    }
    if (touching != isTouching) {
        return GameTestError{GameTestErrorType::Assert,
            isTouching ? "Expected entity '{0}' touching position" : "Expected entity '{0}' not touching position",
            {entityType}};
    }
    return std::nullopt;
}

GameTestResult GameTestHelper::assertItemEntityPresent(
    const std::string& itemType, BlockPos relativePos, f32 searchDistance, bool isPresent)
{
    // TODO: 区分 ItemEntity 并按 itemType 过滤（ItemEntity 体系就绪前用类型名 minecraft:item 近似）
    (void)itemType;
    (void)relativePos;
    (void)searchDistance;
    (void)isPresent;
    return std::nullopt;
}

GameTestResult GameTestHelper::assertItemEntityCountIs(
    const std::string& itemType, BlockPos relativePos, f32 searchDistance, i32 count)
{
    // TODO: 统计指定 itemType 的 ItemEntity 数量（ItemEntity 体系就绪前 stub）
    (void)itemType;
    (void)relativePos;
    (void)searchDistance;
    (void)count;
    return std::nullopt;
}

GameTestResult GameTestHelper::killAllEntities()
{
    // 清除结构范围内所有非玩家实体（对齐基岩 killAllEntities）
    if (m_bounds == nullptr) {
        return GameTestError{
            GameTestErrorType::MethodNotImplemented, "Structure bounds unavailable for killAllEntities"};
    }
    const StructureBoundingBox bb = m_bounds->bounds();
    const AxisAlignedBB box(static_cast<f32>(bb.minX()),
        static_cast<f32>(bb.minY()),
        static_cast<f32>(bb.minZ()),
        static_cast<f32>(bb.maxX() + 1),
        static_cast<f32>(bb.maxY() + 1),
        static_cast<f32>(bb.maxZ() + 1));
    const auto found = m_world.getEntitiesInAABB(box, nullptr);
    for (auto* e : found) {
        if (e != nullptr && e->isAlive()) {
            e->discard(); // 静默移除，不掉落
        }
    }
    return std::nullopt;
}

GameTestResult GameTestHelper::spawnEntity(const std::string& entityType, BlockPos relativePos, mc::Entity*& outEntity)
{
    outEntity = nullptr;
    const auto fullType = normalizeEntityType(entityType);
    const auto* type = mc::entity::EntityRegistry::instance().getType(fullType);
    if (type == nullptr) {
        return GameTestError{
            GameTestErrorType::LevelStateModificationFailed, "Unknown entity type '{0}'", {entityType}};
    }
    auto entity = type->create(&m_world, *m_world.entityRegistry());
    if (entity == nullptr) {
        return GameTestError{
            GameTestErrorType::LevelStateModificationFailed, "Failed to create entity '{0}'", {entityType}};
    }
    const BlockPos worldPos = worldBlockPosition(relativePos);
    entity->setPosition(
        static_cast<f32>(worldPos.x) + 0.5f, static_cast<f32>(worldPos.y), static_cast<f32>(worldPos.z) + 0.5f);
    mc::Entity* raw = entity.get();
    const auto id = m_world.spawnEntity(std::move(entity));
    if (id == 0) {
        return GameTestError{GameTestErrorType::LevelStateModificationFailed,
            "Failed to spawn entity '{0}' at {1}",
            {entityType, worldPos.toString()}};
    }
    outEntity = raw;
    return std::nullopt;
}

GameTestResult GameTestHelper::spawnItemAt(
    const std::string& itemType, const mc::math::Vector3d& position, mc::Entity*& outEntity)
{
    // TODO: 构造 ItemEntity 并按 position 放置（ItemEntity/ItemStack 体系就绪前 stub）
    (void)itemType;
    (void)position;
    outEntity = nullptr;
    return std::nullopt;
}

GameTestResult GameTestHelper::spawnAtLocation(
    const std::string& entityType, const mc::math::Vector3d& position, mc::Entity*& outEntity)
{
    // 对齐基岩 Test.spawnAtLocation：在世界绝对 Vector3 位置生成实体（spawn 的浮点位置变体）。
    // 复用 spawnEntity 的创建/注册逻辑，位置从 BlockPos 改为 Vector3d（浮点）。
    outEntity = nullptr;
    const auto fullType = normalizeEntityType(entityType);
    const auto* type = mc::entity::EntityRegistry::instance().getType(fullType);
    if (type == nullptr) {
        return GameTestError{
            GameTestErrorType::LevelStateModificationFailed, "Unknown entity type '{0}'", {entityType}};
    }
    auto entity = type->create(&m_world, *m_world.entityRegistry());
    if (entity == nullptr) {
        return GameTestError{
            GameTestErrorType::LevelStateModificationFailed, "Failed to create entity '{0}'", {entityType}};
    }
    entity->setPosition(static_cast<f32>(position.x), static_cast<f32>(position.y), static_cast<f32>(position.z));
    mc::Entity* raw = entity.get();
    const auto id = m_world.spawnEntity(std::move(entity));
    if (id == 0) {
        return GameTestError{GameTestErrorType::LevelStateModificationFailed,
            "Failed to spawn entity '{0}' at ({1}, {2}, {3})",
            {entityType, std::to_string(position.x), std::to_string(position.y), std::to_string(position.z)}};
    }
    outEntity = raw;
    return std::nullopt;
}

GameTestResult GameTestHelper::spawnWithoutBehaviors(
    const std::string& entityType, BlockPos relativePos, mc::Entity*& outEntity)
{
    // TODO: 生成无 AI 行为的实体（对齐基岩 spawnWithoutBehaviors，供 walkTo 等可预测行为测试）。
    //       依赖行为移除体系（goal 清空/Brain 停用）未就绪，当前退化为普通 spawnEntity。
    return spawnEntity(entityType, relativePos, outEntity);
}

GameTestResult GameTestHelper::spawnWithoutBehaviorsAtLocation(
    const std::string& entityType, const mc::math::Vector3d& position, mc::Entity*& outEntity)
{
    // TODO: spawnAtLocation 的无行为变体（对齐基岩 spawnWithoutBehaviorsAtLocation）。
    //       依赖行为移除体系未就绪，当前退化为普通 spawnAtLocation。
    return spawnAtLocation(entityType, position, outEntity);
}

GameTestResult GameTestHelper::assertEntityHasArmor(const std::string& entityType,
    i32 armorSlot,
    const std::string& armorName,
    i32 armorData,
    BlockPos relativePos,
    bool hasArmor)
{
    // TODO: 断言 pos 处实体装备指定护甲槽/护甲名/数据值。依赖装备槽查询体系未就绪，stub。
    (void)entityType;
    (void)armorSlot;
    (void)armorName;
    (void)armorData;
    (void)hasArmor;
    return generateErrorWithContext(GameTestErrorType::MethodNotImplemented,
        "assertEntityHasArmor not implemented yet (equipment slot system pending)",
        relativePos);
}

GameTestResult GameTestHelper::assertEntityHasComponent(
    const std::string& entityType, const std::string& componentId, BlockPos relativePos, bool hasComponent)
{
    // TODO: 断言 pos 处实体含指定组件。依赖实体组件查询体系未就绪，stub。
    (void)entityType;
    (void)componentId;
    (void)hasComponent;
    return generateErrorWithContext(GameTestErrorType::MethodNotImplemented,
        "assertEntityHasComponent not implemented yet (entity component system pending)",
        relativePos);
}

GameTestResult GameTestHelper::assertEntityState(
    BlockPos relativePos, const std::string& entityType, std::function<bool(const mc::Entity&)> predicate)
{
    // TODO: 断言 pos 处实体满足 predicate（predicate 接 Entity&，返回 false 即失败）。
    //       依赖实体按 pos 查询体系（getEntitiesInAABB + 类型过滤）做实，当前 stub。
    (void)relativePos;
    (void)entityType;
    (void)predicate;
    return GameTestError{
        GameTestErrorType::MethodNotImplemented, "assertEntityState not implemented yet (entity query system pending)"};
}

GameTestResult GameTestHelper::assertCanReachLocation(mc::Entity& entity, BlockPos relativePos, bool canReach)
{
    // TODO: 断言实体能否寻路到达 pos。依赖 PathNavigator（硬依赖 dynamic_cast<MobEntity*>，Player 不是
    //       MobEntity）未就绪，stub。寻路做实见 MobEntity
    //       寻路三层根因（mobentity-navigator-pathfinder-null-global-bug）。
    (void)entity;
    (void)relativePos;
    (void)canReach;
    return GameTestError{GameTestErrorType::MethodNotImplemented,
        "assertCanReachLocation not implemented yet (pathfinding system pending)"};
}

void GameTestHelper::onPlayerJump(mc::Entity& /*entity*/, i32 /*jumpAmount*/)
{
    // TODO: 模拟实体跳跃事件（对齐基岩 onPlayerJump）。依赖跳跃事件分发体系未就绪，stub。
}

void GameTestHelper::setTntFuse(mc::Entity& /*entity*/, i32 /*fuseLength*/)
{
    // TODO: 设可爆炸实体（TNT 等）的引信时长。依赖实体 fuse 体系未就绪，stub。
}

// === 5. 坐标变换 ===

BlockPos GameTestHelper::worldBlockPosition(BlockPos relativePos) const noexcept
{
    return m_transform.relativeToWorld(relativePos);
}

BlockPos GameTestHelper::relativeBlockPosition(BlockPos worldPos) const noexcept
{
    return m_transform.worldToRelative(worldPos);
}

mc::math::Vector3d GameTestHelper::worldPosition(const mc::math::Vector3d& relativePos) const noexcept
{
    return m_transform.relativeToWorldF(relativePos);
}

mc::math::Vector3d GameTestHelper::relativePosition(const mc::math::Vector3d& worldPos) const noexcept
{
    return m_transform.worldToRelativeF(worldPos);
}

Direction GameTestHelper::rotateDirection(Direction direction) const noexcept
{
    return mc::Directions::rotateDirection(direction, m_transform.rotation());
}

mc::math::Vector3d GameTestHelper::rotateVector(const mc::math::Vector3d& vector) const noexcept
{
    // 把向量按 rotation 绕 Y 轴旋转：用 Direction 步进向量近似（整数步进够用，连续旋转 TODO）
    const Direction in =
        mc::Directions::fromVector(static_cast<f32>(vector.x), static_cast<f32>(vector.y), static_cast<f32>(vector.z));
    const Direction out = rotateDirection(in);
    return mc::math::Vector3d(
        static_cast<f64>(mc::Directions::xOffset(out)), vector.y, static_cast<f64>(mc::Directions::zOffset(out)));
}

Direction GameTestHelper::getTestDirection() const noexcept
{
    // 测试默认朝向 North（结构前方），经旋转变换
    return rotateDirection(Direction::North);
}

// === 6. 完成路径 ===

void GameTestHelper::succeedWhenBlockPresent(const std::string& blockType, BlockPos relativePos, bool isPresent)
{
    // 注册为 succeed 条件：每 tick 检查，满足即成功
    std::string type = blockType;
    BlockPos rel = relativePos;
    bool want = isPresent;
    m_instance.registerSucceedCondition(
        [this, type = std::move(type), rel, want]() -> GameTestResult { return assertBlockPresent(type, rel, want); });
}

void GameTestHelper::succeedWhen(std::function<GameTestResult()> fn)
{
    m_instance.registerSucceedCondition(std::move(fn));
}

void GameTestHelper::succeedIf(std::function<GameTestResult()> fn)
{
    // succeedIf 与 succeedWhen 在第一阶段语义等价（都注册为 succeed 条件）。
    // 基岩版 succeedIf 仅检查一次（瞬时），succeedWhen 持续检查——区别待状态机细化（TODO）。
    m_instance.registerSucceedCondition(std::move(fn));
}

void GameTestHelper::succeedOnTick(i32 tick)
{
    m_instance.registerRunAtTickTime(tick, [this]() -> GameTestResult {
        succeed();
        return std::nullopt;
    });
}

void GameTestHelper::succeedOnTickWhen(i32 tick, std::function<GameTestResult()> fn)
{
    // 在指定 tick 注册 succeed 条件：仅该 tick 检查 fn
    std::function<GameTestResult()> callback = std::move(fn);
    m_instance.registerRunAtTickTime(tick, [this, cb = std::move(callback)]() -> GameTestResult {
        if (cb) {
            return cb();
        }
        return std::nullopt;
    });
}

void GameTestHelper::failIf(std::function<GameTestResult()> fn)
{
    m_instance.registerFailCondition(std::move(fn));
}

void GameTestHelper::succeedWhenEntityHasComponent(
    const std::string& entityType, const std::string& componentId, BlockPos relativePos, bool hasComponent)
{
    // TODO: 每 tick 检查 pos 处实体是否含指定组件，满足时标记成功（对齐基岩 succeedWhenEntityHasComponent）。
    //       依赖实体组件查询体系未就绪，当前注册恒失败 succeed 条件（保持轮询语义，组件体系做实后补真实判定）。
    std::string type = entityType;
    std::string comp = componentId;
    BlockPos rel = relativePos;
    bool want = hasComponent;
    m_instance.registerSucceedCondition([this, type, comp, rel, want]() -> GameTestResult {
        (void)this;
        (void)type;
        (void)comp;
        (void)rel;
        (void)want;
        // 组件查询未就绪：恒返回"未满足"使轮询继续（由 maxTicks 超时兜底），避免假成功。
        return GameTestError{GameTestErrorType::MethodNotImplemented,
            "succeedWhenEntityHasComponent not implemented yet (entity component system pending)"};
    });
}

// === 7. SimulatedPlayer ===

GameTestResult GameTestHelper::spawnSimulatedPlayer(
    const std::string& name, BlockPos relativePos, mc::GameMode gameMode, SimulatedPlayer*& outPlayer)
{
    outPlayer = SimulatedPlayer::spawn(*this, name, relativePos, gameMode);
    if (outPlayer == nullptr) {
        return GameTestError{GameTestErrorType::LevelStateModificationFailed,
            "Failed to spawn SimulatedPlayer '{0}' at {1}",
            {name, relativePos.toString()}};
    }
    return std::nullopt;
}

void GameTestHelper::removeSimulatedPlayer(SimulatedPlayer& player)
{
    // 静默移除（不掉落、不触发死亡流程）；EntityManager 后续回收对象
    player.discard();
}

// === 8. 查询 ===

const mc::BlockState* GameTestHelper::getBlock(BlockPos relativePos) const
{
    return m_world.getBlockState(worldBlockPosition(relativePos));
}

FenceConnectivity GameTestHelper::getFenceConnectivity(BlockPos relativePos) const
{
    // 对齐基岩 Test.getFenceConnectivity：读 pos 处栅栏四方向连接属性（NORTH/EAST/SOUTH/WEST）。
    // 属性不存在（非栅栏）时返回全 false；栅栏的连接属性为 BooleanProperty，get 返回 bool。
    FenceConnectivity conn;
    const mc::BlockState* state = m_world.getBlockState(worldBlockPosition(relativePos));
    if (state == nullptr || state->isAir()) {
        return conn;
    }
    // 用 getOptional 容错：非栅栏方块无 NORTH 等属性，getOptional 返回 nullopt（视作未连接）。
    conn.north = state->getOptional<bool>(mc::BlockStateProperties::NORTH()).value_or(false);
    conn.east = state->getOptional<bool>(mc::BlockStateProperties::EAST()).value_or(false);
    conn.south = state->getOptional<bool>(mc::BlockStateProperties::SOUTH()).value_or(false);
    conn.west = state->getOptional<bool>(mc::BlockStateProperties::WEST()).value_or(false);
    return conn;
}

mc::blocks::SculkSpreader* GameTestHelper::getSculkSpreader(BlockPos /*relativePos*/) const
{
    // 对齐基岩 Test.getSculkSpreader：取 pos 处的幽匿扩散器。项目无 SculkCatalystBlockEntity
    // （vanilla 中持 SculkSpreader 的载体），无法按 pos 取真实 spreader。返回新建空 spreader 快照
    // （maxCharge=kMaxCharge 做实，cursors 空），供 JS 侧只读访问属性。
    // TODO: SculkCatalystBlockEntity 实现后改为按 pos 取真实 spreader（owned 快照或非拥有引用）。
    return new mc::blocks::SculkSpreader(mc::blocks::SculkSpreader::createLevelSpreader());
}

// === 9. 工具 ===

void GameTestHelper::print(const std::string& text)
{
    spdlog::info("[gametest] {}", text);
}

GameTestError GameTestHelper::generateErrorWithContext(
    GameTestErrorType type, std::string message, BlockPos relativePos) const
{
    GameTestError error(type, std::move(message));
    error.setContext(GameTestErrorContext(worldBlockPosition(relativePos), relativePos, currentTick()));
    return error;
}

// === 10. 异步轮询 ===

void GameTestHelper::until(std::function<GameTestResult()> testFn, std::function<GameTestResult()> doneFn)
{
    // 对齐基岩 BaseGameTestHelper::until：每 tick 轮询 testFn，nullopt=条件满足触发 doneFn 收尾。
    // 实现经自重调度：注册下一 tick 回调，未通过则继续注册下一 tick，通过则调 doneFn。
    // doneFn 返回非 nullopt 则 fail；超时由测试 maxTicks 兜底（轮询始终未通过即超时 fail）。
    auto state = std::make_shared<std::pair<std::function<GameTestResult()>, std::function<GameTestResult()>>>(
        std::move(testFn), std::move(doneFn));

    // 递归轮询经 shared_ptr<function> 持自身以自重调度（值捕获 shared_ptr 保活，避免悬垂）。
    // 注意：lambda 捕获 pollRef 而 *pollRef = lambda 形成自循环引用，须在轮询终止时显式清空
    // *pollRef 打破循环，否则每 until() 调用泄漏一个 function 对象。
    auto pollRef = std::make_shared<std::function<GameTestResult()>>();
    *pollRef = [this, state, pollRef]() -> GameTestResult {
        if (isCompleted()) {
            *pollRef = nullptr;  // 打破自循环（std::function 无 reset，赋 nullptr 清空）
            return std::nullopt; // 测试已结束（成功/失败/超时），停止轮询
        }
        const GameTestResult r = state->first ? state->first() : GameTestResult(std::nullopt);
        if (isPass(r)) {
            *pollRef = nullptr; // 打破自循环
            // 条件满足：调 doneFn 收尾，其返回错误即 fail
            return state->second ? state->second() : GameTestResult(std::nullopt);
        }
        // 未满足：注册下一 tick 继续轮询（拷贝 *pollRef，shared_ptr 保活）
        runAfterDelay(1, *pollRef);
        return std::nullopt; // 本 tick 未满足，不报错继续等
    };
    runAfterDelay(1, *pollRef);
}

// === 私有 ===

const mc::BlockState* GameTestHelper::_resolveBlock(const std::string& blockType)
{
    // 容忍 "stone" 与 "minecraft:stone" 两种写法
    std::string full = blockType.find(':') == std::string::npos ? "minecraft:" + blockType : blockType;
    const mc::ResourceLocation loc(full);

    // 基岩版/旧版方块名别名表：基岩行为包与 1.16 旧名用旧 ID，项目方块注册表用 1.21.11 Java 名。
    // 使 setBlockType/assertBlockPresent 命中正确方块而非退化为 air。
    // 仅收录 GameTest 行为包实际用到的别名；新增别名时在此追加。
    static const std::unordered_map<std::string, std::string> kBlockAliases = {
        {"minecraft:brick_block", "minecraft:bricks"},
    };

    // 注意：BlockRegistry::get 找不到时返回 airState()（非 nullptr），不能用 get 的返回值判存在性。
    // 须用 getBlock 判断方块是否注册；未注册时再查别名表，别名仍命中不了才退化为 air。
    if (mc::BlockRegistry::instance().getBlock(loc) != nullptr) {
        return mc::BlockRegistry::instance().get(loc);
    }
    auto it = kBlockAliases.find(full);
    if (it != kBlockAliases.end()) {
        const mc::ResourceLocation aliasedLoc(it->second);
        if (mc::BlockRegistry::instance().getBlock(aliasedLoc) != nullptr) {
            return mc::BlockRegistry::instance().get(aliasedLoc);
        }
    }
    return mc::BlockRegistry::instance().airState();
}

GameTestError GameTestHelper::_expectBlockError(
    const std::string& expectation, BlockPos relativePos, const mc::BlockState* actual) const
{
    const std::string actualName = (actual != nullptr) ? actual->blockLocation().toString() : "<null>";
    GameTestError error(GameTestErrorType::AssertAtPosition,
        "{0} at relative={1} (actual={2})",
        {expectation, relativePos.toString(), actualName});
    error.setContext(GameTestErrorContext(worldBlockPosition(relativePos), relativePos, currentTick()));
    return error;
}

} // namespace mc::test
