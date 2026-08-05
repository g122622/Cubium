#pragma once

#include "common/core/Types.hpp"
#include "common/test/base/coords/TestTransform.hpp"
#include "common/test/base/error/GameTestResult.hpp"
#include "common/util/Direction.hpp" // Rotation / Direction
#include "common/world/block/BlockPos.hpp"

#include <functional>
#include <optional>
#include <string>

namespace mc {
class BlockState;
class Entity;
class IWorld;
} // namespace mc

namespace mc::math {
template <typename>
class Vector3;
using Vector3d = Vector3<f64>;
} // namespace mc::math

namespace mc::test {

// 前向声明：避免 helper/ 与 sequence/ 互引成环。GameTestSequence 在 framework/sequence/ 定义。
class GameTestSequence;
class SimulatedPlayer;

/**
 * @brief 测试助手纯虚接口。
 *
 * 对齐基岩版 `BaseGameTestHelper`：框架核心（`BaseGameTestInstance`/`GameTestSequence`）经此接口持有助手，
 * 不依赖具体 `ServerWorld` 实现，可单元测试（`NullGameTestHelper`）。门面 `GameTestHelper`（`facade/`）
 * 是具体实现，直接绑 `ServerWorld&`。
 *
 * 全部断言/操作方法返回 `GameTestResult`（错误即值，nullopt=通过），无异常。
 *
 * 方法分 9 类（对齐基岩 `BaseGameTestHelper` ~55 虚方法 + JS `Test` 独有补集，见校正 10）：
 * 1. 生命周期与状态
 * 2. 序列与调度
 * 3. 块断言与操作
 * 4. 实体断言与 spawn
 * 5. 坐标变换
 * 6. 完成路径（succeedWhen/succeedIf/failIf）
 * 7. SimulatedPlayer
 * 8. 查询
 * 9. 工具
 *
 * 第一阶段实现"无异步依赖"者；`idle`/`until`（Promise 语义）留 TODO stub。
 */
class IGameTestHelper {
public:
    virtual ~IGameTestHelper() = default;

    // === 1. 生命周期与状态 ===
    virtual void startExecution() = 0;
    virtual void succeed() = 0;
    virtual void fail(GameTestError error) = 0;
    [[nodiscard]] virtual bool isCompleted() const noexcept = 0;
    [[nodiscard]] virtual bool isCleaningUp() const noexcept = 0;
    [[nodiscard]] virtual i32 currentTick() const noexcept = 0;
    [[nodiscard]] virtual i32 maxTicks() const noexcept = 0;
    [[nodiscard]] virtual Rotation rotation() const noexcept = 0;
    [[nodiscard]] virtual const TestTransform& transform() const noexcept = 0;

    // === 2. 序列与调度 ===
    [[nodiscard]] virtual GameTestSequence& startSequence() = 0;
    virtual void runAtTickTime(i32 tick, std::function<GameTestResult()> fn) = 0;
    virtual void runAfterDelay(i32 delay, std::function<GameTestResult()> fn) = 0;
    virtual void runOnFinish(std::function<GameTestResult()> fn) = 0;

    // === 3. 块断言与操作 ===
    [[nodiscard]] virtual GameTestResult assertBlockPresent(
        const std::string& blockType, BlockPos relativePos, bool isPresent) = 0;
    [[nodiscard]] virtual GameTestResult assertBlockState(
        BlockPos relativePos, std::function<bool(const mc::BlockState&)> predicate) = 0;
    [[nodiscard]] virtual GameTestResult setBlock(
        const std::string& blockType, BlockPos relativePos, i32 updateFlags) = 0;
    [[nodiscard]] virtual GameTestResult destroyBlock(BlockPos relativePos, bool dropResources) = 0;
    [[nodiscard]] virtual GameTestResult pressButton(BlockPos relativePos) = 0;
    [[nodiscard]] virtual GameTestResult pullLever(BlockPos relativePos) = 0;
    [[nodiscard]] virtual GameTestResult pulseRedstone(BlockPos relativePos, i32 duration) = 0;
    [[nodiscard]] virtual GameTestResult assertRedstonePower(BlockPos relativePos, i32 power) = 0;
    [[nodiscard]] virtual GameTestResult assertIsWaterlogged(BlockPos relativePos, bool isWaterlogged) = 0;

    // === 4. 实体断言与 spawn ===
    [[nodiscard]] virtual GameTestResult assertEntityPresent(
        const std::string& entityType, BlockPos relativePos, f32 searchDistance, bool isPresent) = 0;
    [[nodiscard]] virtual GameTestResult assertEntityPresentInArea(const std::string& entityType, bool isPresent) = 0;
    [[nodiscard]] virtual GameTestResult assertEntityInstancePresent(
        const mc::Entity& entity, BlockPos relativePos, bool isPresent) = 0;
    [[nodiscard]] virtual GameTestResult assertEntityInstancePresentInArea(
        const mc::Entity& entity, bool isPresent) = 0;
    [[nodiscard]] virtual GameTestResult assertEntityTouching(
        const std::string& entityType, const mc::math::Vector3d& position, bool isTouching) = 0;
    [[nodiscard]] virtual GameTestResult assertItemEntityPresent(
        const std::string& itemType, BlockPos relativePos, f32 searchDistance, bool isPresent) = 0;
    [[nodiscard]] virtual GameTestResult assertItemEntityCountIs(
        const std::string& itemType, BlockPos relativePos, f32 searchDistance, i32 count) = 0;
    [[nodiscard]] virtual GameTestResult killAllEntities() = 0;

    // spawn 返回 Entity*（失败时 error 携带在出参或 variant）。此处用 std::variant 语义：
    // 返回 Entity*；失败时由调用方先检查单独的 lastError()。为简化，spawn 返回 GameTestResult，
    // 生成的实体经 out 参数回传。
    [[nodiscard]] virtual GameTestResult spawnEntity(
        const std::string& entityType, BlockPos relativePos, mc::Entity*& outEntity) = 0;
    [[nodiscard]] virtual GameTestResult spawnItemAt(
        const std::string& itemType, const mc::math::Vector3d& position, mc::Entity*& outEntity) = 0;

    // === 5. 坐标变换 ===
    [[nodiscard]] virtual BlockPos worldBlockPosition(BlockPos relativePos) const noexcept = 0;
    [[nodiscard]] virtual BlockPos relativeBlockPosition(BlockPos worldPos) const noexcept = 0;
    [[nodiscard]] virtual mc::math::Vector3d worldPosition(const mc::math::Vector3d& relativePos) const noexcept = 0;
    [[nodiscard]] virtual mc::math::Vector3d relativePosition(const mc::math::Vector3d& worldPos) const noexcept = 0;
    [[nodiscard]] virtual Direction rotateDirection(Direction direction) const noexcept = 0;
    [[nodiscard]] virtual mc::math::Vector3d rotateVector(const mc::math::Vector3d& vector) const noexcept = 0;
    [[nodiscard]] virtual Direction getTestDirection() const noexcept = 0;

    // === 6. 完成路径 ===
    virtual void succeedWhenBlockPresent(const std::string& blockType, BlockPos relativePos, bool isPresent) = 0;
    virtual void succeedWhen(std::function<GameTestResult()> fn) = 0;
    virtual void succeedIf(std::function<GameTestResult()> fn) = 0;
    virtual void succeedOnTick(i32 tick) = 0;
    virtual void succeedOnTickWhen(i32 tick, std::function<GameTestResult()> fn) = 0;
    virtual void failIf(std::function<GameTestResult()> fn) = 0;

    // === 7. SimulatedPlayer ===
    [[nodiscard]] virtual GameTestResult spawnSimulatedPlayer(
        const std::string& name, BlockPos relativePos, mc::GameMode gameMode, SimulatedPlayer*& outPlayer) = 0;
    virtual void removeSimulatedPlayer(SimulatedPlayer& player) = 0;

    // === 8. 查询 ===
    [[nodiscard]] virtual const mc::BlockState* getBlock(BlockPos relativePos) const = 0;
    [[nodiscard]] virtual mc::IWorld& world() noexcept = 0;

    // === 9. 工具 ===
    virtual void print(const std::string& text) = 0;
    [[nodiscard]] virtual GameTestError generateErrorWithContext(
        GameTestErrorType type, std::string message, BlockPos relativePos) const = 0;

    // TODO: idle(tickDelay) / until(fn) — JS Promise 异步断言，待 C++↔JS 事件总线桥接后实现。
    // 第一阶段脚本绑定层用 thenExecute/thenWaitAfter 模拟，留 stub。
};

} // namespace mc::test
