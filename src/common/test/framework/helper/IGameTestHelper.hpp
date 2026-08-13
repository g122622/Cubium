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
class ItemStack;
} // namespace mc

namespace mc::blocks {
class SculkSpreader;
} // namespace mc::blocks

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
 * @brief 栅栏连接性值对象（对齐基岩 `FenceConnectivity`）。
 *
 * `getFenceConnectivity(pos)` 返回此结构：该位置栅栏在四个水平方向上的连接状态。
 * 脚本绑定层据此组装 JS `FenceConnectivity` 值对象（{north,east,south,west} 四 bool，
 * 原型由 ScriptGameTestTypes 注册作 instanceof 锚点）。
 */
struct FenceConnectivity {
    bool north = false;
    bool east = false;
    bool south = false;
    bool west = false;
};

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

    // 容器/排列/流体断言与操作（批次4 补齐，对齐基岩 Test 类官方 JS API）。
    /// 断言 pos 处容器（如箱子）含指定物品栈（按物品类型匹配，至少 1 个）。底层 IInventory 就绪。
    [[nodiscard]] virtual GameTestResult assertContainerContains(
        const mc::ItemStack& itemStack, BlockPos relativePos) = 0;
    /// 断言 pos 处容器为空。底层 IInventory::isEmpty 就绪。
    [[nodiscard]] virtual GameTestResult assertContainerEmpty(BlockPos relativePos) = 0;
    /// 按 BlockPermutation（C++ 侧为 BlockState）设 pos 方块，对齐基岩 setBlockPermutation。
    [[nodiscard]] virtual GameTestResult setBlockPermutation(
        const mc::BlockState& permutation, BlockPos relativePos) = 0;
    /// 设 pos 处流体容器（如炼药锅）的流体类型。底层 ILiquidContainer 写入体系未就绪，stub。
    [[nodiscard]] virtual GameTestResult setFluidContainer(BlockPos relativePos, const std::string& fluidType) = 0;
    /// 触发方块内部事件（对齐基岩 triggerInternalBlockEvent）。依赖方块事件体系未就绪，stub。
    virtual void triggerInternalBlockEvent(BlockPos relativePos, const std::string& eventName) = 0;
    /// 测试多方块传播：从 fromFace 向 direction 传播（对齐基岩 spreadFromFaceTowardDirection，用于 sculk/苔藓等）。
    /// 依赖 MultifaceSpreader 接线体系未就绪，stub。
    virtual void spreadFromFaceTowardDirection(BlockPos relativePos, Direction fromFace, Direction direction) = 0;

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
    /// 杀死指定实体（对齐基岩 /kill 语义，走伤害致死链路，触发完整死亡流程：die → deathTime 倒计时 → remove）。
    /// 与 killAllEntities 的 discard（静默移除不掉落、不触发死亡）不同，killEntity 让实体经死亡链路移除，
    /// 从而触发"死亡时"行为（如史莱姆分裂、僵尸增援、掉落物/经验）。基岩 Test 类无此 API，此为项目测试设施。
    [[nodiscard]] virtual GameTestResult killEntity(mc::Entity& entity) = 0;

    // spawn 返回 Entity*（失败时 error 携带在出参或 variant）。此处用 std::variant 语义：
    // 返回 Entity*；失败时由调用方先检查单独的 lastError()。为简化，spawn 返回 GameTestResult，
    // 生成的实体经 out 参数回传。
    [[nodiscard]] virtual GameTestResult spawnEntity(
        const std::string& entityType, BlockPos relativePos, mc::Entity*& outEntity) = 0;
    [[nodiscard]] virtual GameTestResult spawnItemAt(
        const std::string& itemType, const mc::math::Vector3d& position, mc::Entity*& outEntity) = 0;

    // 实体 spawn 变体与实体状态断言（批次4 补齐，对齐基岩 Test 类官方 JS API）。
    /// 在世界绝对 Vector3 位置生成实体（spawn 的浮点位置变体，对齐基岩 spawnAtLocation）。
    [[nodiscard]] virtual GameTestResult spawnAtLocation(
        const std::string& entityType, const mc::math::Vector3d& position, mc::Entity*& outEntity) = 0;
    /// 生成无 AI 行为的实体（对齐基岩 spawnWithoutBehaviors，供 walkTo 等可预测行为测试）。
    /// 依赖行为移除体系未就绪，stub（当前等同 spawnEntity，TODO）。
    [[nodiscard]] virtual GameTestResult spawnWithoutBehaviors(
        const std::string& entityType, BlockPos relativePos, mc::Entity*& outEntity) = 0;
    /// 生成无 AI 行为的实体（spawnAtLocation 的无行为变体，对齐基岩 spawnWithoutBehaviorsAtLocation）。stub。
    [[nodiscard]] virtual GameTestResult spawnWithoutBehaviorsAtLocation(
        const std::string& entityType, const mc::math::Vector3d& position, mc::Entity*& outEntity) = 0;
    /// 断言 pos 处实体装备指定护甲槽/护甲名/数据值。依赖装备槽体系未就绪，stub。
    [[nodiscard]] virtual GameTestResult assertEntityHasArmor(const std::string& entityType,
        i32 armorSlot,
        const std::string& armorName,
        i32 armorData,
        BlockPos relativePos,
        bool hasArmor) = 0;
    /// 断言 pos 处实体含指定组件。依赖实体组件体系未就绪，stub。
    [[nodiscard]] virtual GameTestResult assertEntityHasComponent(
        const std::string& entityType, const std::string& componentId, BlockPos relativePos, bool hasComponent) = 0;
    /// 断言 pos 处实体满足 predicate。predicate 接 Entity&，返回 false 即断言失败。依赖实体查询体系 stub。
    [[nodiscard]] virtual GameTestResult assertEntityState(
        BlockPos relativePos, const std::string& entityType, std::function<bool(const mc::Entity&)> predicate) = 0;
    /// 断言实体能否寻路到达 pos。依赖 PathNavigator（硬依赖 dynamic_cast<MobEntity*>）未就绪，stub。
    [[nodiscard]] virtual GameTestResult assertCanReachLocation(
        mc::Entity& entity, BlockPos relativePos, bool canReach) = 0;
    /// 模拟实体跳跃事件（对齐基岩 onPlayerJump）。依赖跳跃事件体系未就绪，stub。
    virtual void onPlayerJump(mc::Entity& entity, i32 jumpAmount) = 0;
    /// 设可爆炸实体（TNT 等）的引信时长。依赖实体 fuse 体系未就绪，stub。
    virtual void setTntFuse(mc::Entity& entity, i32 fuseLength) = 0;

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
    /// 每 tick 检查 pos 处实体是否含指定组件，满足时标记成功（对齐基岩 succeedWhenEntityHasComponent）。
    /// 依赖实体组件体系未就绪，stub（注册空 succeed 条件，TODO 组件查询做实后补）。
    virtual void succeedWhenEntityHasComponent(
        const std::string& entityType, const std::string& componentId, BlockPos relativePos, bool hasComponent) = 0;

    // === 7. SimulatedPlayer ===
    [[nodiscard]] virtual GameTestResult spawnSimulatedPlayer(
        const std::string& name, BlockPos relativePos, mc::GameMode gameMode, SimulatedPlayer*& outPlayer) = 0;
    virtual void removeSimulatedPlayer(SimulatedPlayer& player) = 0;

    // === 8. 查询 ===
    [[nodiscard]] virtual const mc::BlockState* getBlock(BlockPos relativePos) const = 0;
    /// 取 pos 处栅栏的连接性（四方向 bool）。底层 BlockState NORTH/EAST/SOUTH/WEST 属性就绪。
    [[nodiscard]] virtual FenceConnectivity getFenceConnectivity(BlockPos relativePos) const = 0;
    /// 取 pos 处的幽匿扩散器。项目无 SculkCatalystBlockEntity（spreader 载体缺失），返回新建空 spreader
    /// 快照（maxCharge 做实，cursors 空）。TODO: SculkCatalystBlockEntity 实现后取真实 spreader。
    [[nodiscard]] virtual mc::blocks::SculkSpreader* getSculkSpreader(BlockPos relativePos) const = 0;
    [[nodiscard]] virtual mc::IWorld& world() noexcept = 0;

    // === 9. 工具 ===
    virtual void print(const std::string& text) = 0;
    [[nodiscard]] virtual GameTestError generateErrorWithContext(
        GameTestErrorType type, std::string message, BlockPos relativePos) const = 0;

    // === 10. 异步轮询 ===
    /**
     * @brief 每 tick 轮询 `testFn` 直到通过（返回 nullopt），随后调 `doneFn` 完成收尾。
     *
     * 对齐基岩版 `BaseGameTestHelper::until`：内部经 `runAtTickTime` 在后续每 tick 重试 `testFn`，
     * `testFn` 返回非 nullopt 即继续等待（不立即失败），直到其返回 nullopt 触发 `doneFn`。
     * `doneFn` 返回非 nullopt 则测试失败。超时由测试 `maxTicks` 兜底（轮询未通过即超时 fail）。
     *
     * @param testFn 轮询条件（nullopt=条件满足）。每 tick 调用直到通过。
     * @param doneFn 条件满足后的收尾回调（nullopt=成功完成收尾）。
     */
    virtual void until(std::function<GameTestResult()> testFn, std::function<GameTestResult()> doneFn) = 0;

    // TODO: idle(tickDelay) — JS Promise 语义（await helper.idle(n) 暂停测试体 n tick），
    // 需 C++↔JS 事件总线桥接后实现。C++ 原生测试用 startSequence().thenIdle(n) / runAfterDelay(n, fn) 替代。
};

} // namespace mc::test
