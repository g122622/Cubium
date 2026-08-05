#pragma once

#include "common/test/base/coords/TestTransform.hpp"
#include "common/test/base/error/GameTestError.hpp"
#include "common/test/base/error/GameTestResult.hpp"
#include "common/test/framework/helper/IGameTestHelper.hpp"
#include "common/test/native/NativeGameTestFunction.hpp" // NativeGameTestFunction::TestBody（wrapNativeBody 返回类型）
#include "common/util/Direction.hpp"                     // Rotation / Direction
#include "common/world/block/BlockPos.hpp"
#include "server/world/ServerWorld.hpp" // ServerWorld 公有继承 IWorld；world() 内联返回 IWorld& 需二者完整类型

#include <memory>
#include <utility> // std::move（wrapNativeBody）

namespace mc::test {

class BaseGameTestInstance;
class GameTestSequence;
class StructureBounds;

/**
 * @brief 测试体门面：测试作者唯一接触的测试体 API（~65 方法，全错误即值）。
 *
 * 对齐基岩版 `BaseGameTestHelper`（~55 虚方法）+ JS `Test` 类独有补集（见校正 10）。直接绑 `ServerWorld&`，
 * 实现 `IGameTestHelper` 接口，并回指所属 `BaseGameTestInstance` 以：
 * 1. 注册 `runAtTickTime`/`runAfterDelay`/`runOnFinish`/`succeedIf`/`failIf`/`succeedWhen` 调度回调；
 * 2. 读取 `currentTick`/`maxTicks`/`rotation`/`isCompleted` 等运行期状态。
 *
 * 构造采用单阶段（对齐基岩 `BaseGameTestHelper(BaseGameTestInstance&)`）：由 `MinecraftGameTestHelperProvider`
 * 在创建时同时传入 `ServerWorld&`、结构原点、`StructureBounds*` 与 `BaseGameTestInstance&`。
 *
 * 方法分 9 类（见 `IGameTestHelper`）：生命周期与状态 / 序列与调度 / 块断言与操作 / 实体断言与 spawn /
 * 坐标变换 / 完成路径 / SimulatedPlayer / 查询 / 工具。第一阶段实现"无异步依赖"者；
 * `idle`/`until`（JS Promise 语义）留 TODO stub。
 *
 * 门面纪律：外部测试作者仅经此门面（与 `GameTestRegistrar`）操作测试；内部 `IGameTestHelper`/`NullGameTestHelper`
 * 不对外。本类放 `facade/`（顶层聚合），不暴露 `ServerWorld&` 引用细节给 `mc_test` 库（其仅含 base/framework/native）。
 */
class GameTestHelper final : public IGameTestHelper {
public:
    GameTestHelper(
        mc::server::ServerWorld& world, BlockPos origin, const StructureBounds* bounds, BaseGameTestInstance& instance);
    ~GameTestHelper() override; // 需完整类型 GameTestSequence 析构，定义在 .cpp

    GameTestHelper(const GameTestHelper&) = delete;
    GameTestHelper& operator=(const GameTestHelper&) = delete;
    GameTestHelper(GameTestHelper&&) = delete;
    GameTestHelper& operator=(GameTestHelper&&) = delete;

    // === 1. 生命周期与状态 ===
    void startExecution() override;
    void succeed() override;
    void fail(GameTestError error) override;
    [[nodiscard]] bool isCompleted() const noexcept override;
    [[nodiscard]] bool isCleaningUp() const noexcept override;
    [[nodiscard]] i32 currentTick() const noexcept override;
    [[nodiscard]] i32 maxTicks() const noexcept override;
    [[nodiscard]] Rotation rotation() const noexcept override;
    [[nodiscard]] const TestTransform& transform() const noexcept override { return m_transform; }

    // === 2. 序列与调度 ===
    [[nodiscard]] GameTestSequence& startSequence() override;
    void runAtTickTime(i32 tick, std::function<GameTestResult()> fn) override;
    void runAfterDelay(i32 delay, std::function<GameTestResult()> fn) override;
    void runOnFinish(std::function<GameTestResult()> fn) override;

    // === 3. 块断言与操作 ===
    [[nodiscard]] GameTestResult assertBlockPresent(
        const std::string& blockType, BlockPos relativePos, bool isPresent) override;
    [[nodiscard]] GameTestResult assertBlockState(
        BlockPos relativePos, std::function<bool(const mc::BlockState&)> predicate) override;
    [[nodiscard]] GameTestResult setBlock(const std::string& blockType, BlockPos relativePos, i32 updateFlags) override;
    [[nodiscard]] GameTestResult destroyBlock(BlockPos relativePos, bool dropResources) override;
    [[nodiscard]] GameTestResult pressButton(BlockPos relativePos) override;
    [[nodiscard]] GameTestResult pullLever(BlockPos relativePos) override;
    [[nodiscard]] GameTestResult pulseRedstone(BlockPos relativePos, i32 duration) override;
    [[nodiscard]] GameTestResult assertRedstonePower(BlockPos relativePos, i32 power) override;
    [[nodiscard]] GameTestResult assertIsWaterlogged(BlockPos relativePos, bool isWaterlogged) override;

    // === 4. 实体断言与 spawn ===
    [[nodiscard]] GameTestResult assertEntityPresent(
        const std::string& entityType, BlockPos relativePos, f32 searchDistance, bool isPresent) override;
    [[nodiscard]] GameTestResult assertEntityPresentInArea(const std::string& entityType, bool isPresent) override;
    [[nodiscard]] GameTestResult assertEntityInstancePresent(
        const mc::Entity& entity, BlockPos relativePos, bool isPresent) override;
    [[nodiscard]] GameTestResult assertEntityInstancePresentInArea(const mc::Entity& entity, bool isPresent) override;
    [[nodiscard]] GameTestResult assertEntityTouching(
        const std::string& entityType, const mc::math::Vector3d& position, bool isTouching) override;
    [[nodiscard]] GameTestResult assertItemEntityPresent(
        const std::string& itemType, BlockPos relativePos, f32 searchDistance, bool isPresent) override;
    [[nodiscard]] GameTestResult assertItemEntityCountIs(
        const std::string& itemType, BlockPos relativePos, f32 searchDistance, i32 count) override;
    [[nodiscard]] GameTestResult killAllEntities() override;
    [[nodiscard]] GameTestResult spawnEntity(
        const std::string& entityType, BlockPos relativePos, mc::Entity*& outEntity) override;
    [[nodiscard]] GameTestResult spawnItemAt(
        const std::string& itemType, const mc::math::Vector3d& position, mc::Entity*& outEntity) override;

    // === 5. 坐标变换 ===
    [[nodiscard]] BlockPos worldBlockPosition(BlockPos relativePos) const noexcept override;
    [[nodiscard]] BlockPos relativeBlockPosition(BlockPos worldPos) const noexcept override;
    [[nodiscard]] mc::math::Vector3d worldPosition(const mc::math::Vector3d& relativePos) const noexcept override;
    [[nodiscard]] mc::math::Vector3d relativePosition(const mc::math::Vector3d& worldPos) const noexcept override;
    [[nodiscard]] Direction rotateDirection(Direction direction) const noexcept override;
    [[nodiscard]] mc::math::Vector3d rotateVector(const mc::math::Vector3d& vector) const noexcept override;
    [[nodiscard]] Direction getTestDirection() const noexcept override;

    // === 6. 完成路径 ===
    void succeedWhenBlockPresent(const std::string& blockType, BlockPos relativePos, bool isPresent) override;
    void succeedWhen(std::function<GameTestResult()> fn) override;
    void succeedIf(std::function<GameTestResult()> fn) override;
    void succeedOnTick(i32 tick) override;
    void succeedOnTickWhen(i32 tick, std::function<GameTestResult()> fn) override;
    void failIf(std::function<GameTestResult()> fn) override;

    // === 7. SimulatedPlayer ===
    [[nodiscard]] GameTestResult spawnSimulatedPlayer(
        const std::string& name, BlockPos relativePos, mc::GameMode gameMode, SimulatedPlayer*& outPlayer) override;
    void removeSimulatedPlayer(SimulatedPlayer& player) override;

    // === 8. 查询 ===
    [[nodiscard]] const mc::BlockState* getBlock(BlockPos relativePos) const override;
    [[nodiscard]] mc::IWorld& world() noexcept override { return m_world; }

    // === 9. 工具 ===
    void print(const std::string& text) override;
    [[nodiscard]] GameTestError generateErrorWithContext(
        GameTestErrorType type, std::string message, BlockPos relativePos) const override;

private:
    /**
     * @brief 经 `BlockRegistry` 按名查方块默认状态，查不到回 air。
     *
     * @param blockType 形如 `"minecraft:stone"` 或 `"stone"`（无命名空间按 minecraft 处理）。
     * @return 默认 `BlockState*`；查不到返 `airState()`（保证非空，便于 setBlock）。
     */
    [[nodiscard]] static const mc::BlockState* _resolveBlock(const std::string& blockType);

    /**
     * @brief 生成"期望/实际不符"的断言错误，附世界绝对方块坐标上下文。
     */
    [[nodiscard]] GameTestError _expectBlockError(
        const std::string& expectation, BlockPos relativePos, const mc::BlockState* actual) const;

    mc::server::ServerWorld& m_world;
    BaseGameTestInstance& m_instance;
    TestTransform m_transform;
    const StructureBounds* m_bounds;              // 非拥有，供 area 实体查询算包围盒（nullable：结构未就绪）
    std::unique_ptr<GameTestSequence> m_sequence; // 懒构造，startSequence 首次调用时建
};

/**
 * @brief 把作者编写的 `void(GameTestHelper&)` 测试体包装为 `GameTestResult(IGameTestHelper&)` 适配闭包。
 *
 * `GameTestHelper` 实现 `IGameTestHelper`，故 `static_cast<GameTestHelper&>(helper)` 安全（此模板定义在 facade 头，
 * GameTestHelper 完整可见）。作者体返回 void（成功即不报错），包装器在体正常返回后返回 `pass()`；体可经
 * `helper.fail(...)` 主动失败，此时体仍正常返回（fail 已设状态），包装器返回 pass——失败由 instance 状态机捕获。
 *
 * 模板定义须在此（facade 头）而非 mc_test 库的 `GameTestMacros.hpp`，因向派生类引用转换需 GameTestHelper 完整类型，
 * 而 mc_test 库不可依赖 facade/server 类型。`MC_REGISTER_GAME_TEST` 宏经此头取得模板定义。
 */
template <typename Body>
inline NativeGameTestFunction::TestBody wrapNativeBody(Body body)
{
    return [body = std::move(body)](IGameTestHelper& helper) -> GameTestResult {
        body(static_cast<GameTestHelper&>(helper));
        return mc::test::pass();
    };
}

} // namespace mc::test
