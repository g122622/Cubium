#include "server/test/facade/GameTestHelper.hpp"

#include "common/test/base/error/GameTestErrorContext.hpp"
#include "common/test/framework/instance/BaseGameTestInstance.hpp"
#include "common/test/framework/sequence/GameTestSequence.hpp"
#include "server/test/minecraft/structure/StructureBounds.hpp"
#include "server/test/simulated/SimulatedPlayer.hpp" // spawnSimulatedPlayer / removeSimulatedPlayer
#include "common/util/assert/AssertAll.hpp"
#include "common/world/block/BlockPos.hpp"

#include "common/entity/core/Entity.hpp"
#include "common/entity/core/EntityRegistry.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/AxisAlignedBB.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/Vector3.hpp"
#include "common/world/block/BlockRegistry.hpp"
#include "common/world/block/BlockState.hpp"
#include "common/world/gen/structure/StructureBoundingBox.hpp"
#include "server/world/ServerWorld.hpp"

#include <spdlog/spdlog.h>

#include <string>

namespace mc::test {

// 全限定别名，规避 mc::test 内非限定名两段查找不回退 mc::world 的遮蔽坑（见 BossBarState 内存）
using StructureBoundingBox = mc::world::gen::structure::StructureBoundingBox;

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
    const bool present = (actual != nullptr && expected != nullptr && *actual == *expected);
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
    // TODO: 经 BlockEntity/方块状态置 powered=true 并调度复位（ButtonBlock 体系就绪前 stub）
    (void)relativePos;
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

// === 4. 实体断言与 spawn ===

GameTestResult GameTestHelper::assertEntityPresent(
    const std::string& entityType, BlockPos relativePos, f32 searchDistance, bool isPresent)
{
    const BlockPos worldPos = worldBlockPosition(relativePos);
    const mc::Vector3 center = worldPos.toVector3();
    const auto found = m_world.getEntitiesInRange(center, searchDistance, nullptr);
    bool present = false;
    for (const auto* e : found) {
        if (e != nullptr && e->getTypeId() == entityType) {
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
        if (e != nullptr && e->getTypeId() == entityType) {
            present = true;
            break;
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
    // 以 position 为中心建 1x1x1 查询盒，intersects 判定接触
    const mc::Vector3 centerF = position.cast<f32>();
    const AxisAlignedBB box = AxisAlignedBB::fromPosition(centerF, 0.0f, 0.0f).grow(0.5f);
    const auto found = m_world.getEntitiesInAABB(box, nullptr);
    bool touching = false;
    for (const auto* e : found) {
        if (e != nullptr && e->getTypeId() == entityType && e->boundingBox().intersects(box)) {
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
    const auto* type = mc::entity::EntityRegistry::instance().getType(entityType);
    if (type == nullptr) {
        return GameTestError{
            GameTestErrorType::LevelStateModificationFailed, "Unknown entity type '{0}'", {entityType}};
    }
    auto entity = type->create(&m_world);
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

// === 私有 ===

const mc::BlockState* GameTestHelper::_resolveBlock(const std::string& blockType)
{
    // 容忍 "stone" 与 "minecraft:stone" 两种写法
    std::string full = blockType.find(':') == std::string::npos ? "minecraft:" + blockType : blockType;
    const auto* state = mc::BlockRegistry::instance().get(mc::ResourceLocation(full));
    if (state != nullptr) {
        return state;
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
