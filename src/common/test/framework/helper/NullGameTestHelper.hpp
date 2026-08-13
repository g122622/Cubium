#pragma once

#include "common/test/base/coords/TestTransform.hpp"
#include "common/test/framework/helper/IGameTestHelper.hpp"

#include <memory>

namespace mc::test {

// 前向声明：startSequence 返回 GameTestSequence&，持 unique_ptr<GameTestSequence> 成员需完整类型仅在 .cpp。
class GameTestSequence;

/**
 * @brief 空实现的测试助手。
 *
 * 对齐基岩版 `NullGameTestHelper`：所有方法空操作/返回通过结果，用于框架自身的 headless 单元测试
 *（不依赖真实 `ServerWorld`）。`GameTestSequence`/`BaseGameTestInstance` 的状态机逻辑可借此在不接触
 * 世界的情况下验证。
 *
 * 状态查询返回安全默认值（`isCompleted`=false、`currentTick`=0 等），坐标变换返回原值（无旋转）。
 * `world()` 不应被调用（null helper 无世界），调用即断言失败暴露错误。
 */
class NullGameTestHelper final : public IGameTestHelper {
public:
    NullGameTestHelper();
    ~NullGameTestHelper() override; // 需完整类型 GameTestSequence 析构，定义在 .cpp

    // 1. 生命周期与状态
    void startExecution() override {}
    void succeed() override {}
    void fail(GameTestError /*error*/) override {}
    [[nodiscard]] bool isCompleted() const noexcept override { return false; }
    [[nodiscard]] bool isCleaningUp() const noexcept override { return false; }
    [[nodiscard]] i32 currentTick() const noexcept override { return 0; }
    [[nodiscard]] i32 maxTicks() const noexcept override { return 100; }
    [[nodiscard]] Rotation rotation() const noexcept override { return Rotation::None; }
    [[nodiscard]] const TestTransform& transform() const noexcept override { return m_transform; }

    // 2. 序列与调度
    [[nodiscard]] GameTestSequence& startSequence() override;
    void runAtTickTime(i32 /*tick*/, std::function<GameTestResult()> /*fn*/) override {}
    void runAfterDelay(i32 /*delay*/, std::function<GameTestResult()> /*fn*/) override {}
    void runOnFinish(std::function<GameTestResult()> /*fn*/) override {}

    // 3. 块断言与操作 —— 全部返回通过
    [[nodiscard]] GameTestResult assertBlockPresent(
        const std::string& /*blockType*/, BlockPos /*relativePos*/, bool /*isPresent*/) override
    {
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult assertBlockState(
        BlockPos /*relativePos*/, std::function<bool(const mc::BlockState&)> /*predicate*/) override
    {
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult setBlock(
        const std::string& /*blockType*/, BlockPos /*relativePos*/, i32 /*updateFlags*/) override
    {
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult destroyBlock(BlockPos /*relativePos*/, bool /*dropResources*/) override
    {
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult pressButton(BlockPos /*relativePos*/) override { return std::nullopt; }
    [[nodiscard]] GameTestResult pullLever(BlockPos /*relativePos*/) override { return std::nullopt; }
    [[nodiscard]] GameTestResult pulseRedstone(BlockPos /*relativePos*/, i32 /*duration*/) override
    {
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult assertRedstonePower(BlockPos /*relativePos*/, i32 /*power*/) override
    {
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult assertIsWaterlogged(BlockPos /*relativePos*/, bool /*isWaterlogged*/) override
    {
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult assertContainerContains(
        const mc::ItemStack& /*itemStack*/, BlockPos /*relativePos*/) override
    {
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult assertContainerEmpty(BlockPos /*relativePos*/) override { return std::nullopt; }
    [[nodiscard]] GameTestResult setBlockPermutation(
        const mc::BlockState& /*permutation*/, BlockPos /*relativePos*/) override
    {
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult setFluidContainer(BlockPos /*relativePos*/, const std::string& /*fluidType*/) override
    {
        return std::nullopt;
    }
    void triggerInternalBlockEvent(BlockPos /*relativePos*/, const std::string& /*eventName*/) override {}
    void spreadFromFaceTowardDirection(
        BlockPos /*relativePos*/, Direction /*fromFace*/, Direction /*direction*/) override
    {}

    // 4. 实体断言与 spawn
    [[nodiscard]] GameTestResult assertEntityPresent(const std::string& /*entityType*/,
        BlockPos /*relativePos*/,
        f32 /*searchDistance*/,
        bool /*isPresent*/) override
    {
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult assertEntityPresentInArea(
        const std::string& /*entityType*/, bool /*isPresent*/) override
    {
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult assertEntityInstancePresent(
        const mc::Entity& /*entity*/, BlockPos /*relativePos*/, bool /*isPresent*/) override
    {
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult assertEntityInstancePresentInArea(
        const mc::Entity& /*entity*/, bool /*isPresent*/) override
    {
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult assertEntityTouching(
        const std::string& /*entityType*/, const mc::math::Vector3d& /*position*/, bool /*isTouching*/) override
    {
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult assertItemEntityPresent(
        const std::string& /*itemType*/, BlockPos /*relativePos*/, f32 /*searchDistance*/, bool /*isPresent*/) override
    {
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult assertItemEntityCountIs(
        const std::string& /*itemType*/, BlockPos /*relativePos*/, f32 /*searchDistance*/, i32 /*count*/) override
    {
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult killAllEntities() override { return std::nullopt; }
    [[nodiscard]] GameTestResult killEntity(mc::Entity& /*entity*/) override { return std::nullopt; }
    [[nodiscard]] GameTestResult spawnEntity(
        const std::string& /*entityType*/, BlockPos /*relativePos*/, mc::Entity*& outEntity) override
    {
        outEntity = nullptr;
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult spawnItemAt(
        const std::string& /*itemType*/, const mc::math::Vector3d& /*position*/, mc::Entity*& outEntity) override
    {
        outEntity = nullptr;
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult spawnAtLocation(
        const std::string& /*entityType*/, const mc::math::Vector3d& /*position*/, mc::Entity*& outEntity) override
    {
        outEntity = nullptr;
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult spawnWithoutBehaviors(
        const std::string& /*entityType*/, BlockPos /*relativePos*/, mc::Entity*& outEntity) override
    {
        outEntity = nullptr;
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult spawnWithoutBehaviorsAtLocation(
        const std::string& /*entityType*/, const mc::math::Vector3d& /*position*/, mc::Entity*& outEntity) override
    {
        outEntity = nullptr;
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult assertEntityHasArmor(const std::string& /*entityType*/,
        i32 /*armorSlot*/,
        const std::string& /*armorName*/,
        i32 /*armorData*/,
        BlockPos /*relativePos*/,
        bool /*hasArmor*/) override
    {
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult assertEntityHasComponent(const std::string& /*entityType*/,
        const std::string& /*componentId*/,
        BlockPos /*relativePos*/,
        bool /*hasComponent*/) override
    {
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult assertEntityState(BlockPos /*relativePos*/,
        const std::string& /*entityType*/,
        std::function<bool(const mc::Entity&)> /*predicate*/) override
    {
        return std::nullopt;
    }
    [[nodiscard]] GameTestResult assertCanReachLocation(
        mc::Entity& /*entity*/, BlockPos /*relativePos*/, bool /*canReach*/) override
    {
        return std::nullopt;
    }
    void onPlayerJump(mc::Entity& /*entity*/, i32 /*jumpAmount*/) override {}
    void setTntFuse(mc::Entity& /*entity*/, i32 /*fuseLength*/) override {}

    // 5. 坐标变换 —— 无旋转，原样返回
    [[nodiscard]] BlockPos worldBlockPosition(BlockPos relativePos) const noexcept override { return relativePos; }
    [[nodiscard]] BlockPos relativeBlockPosition(BlockPos worldPos) const noexcept override { return worldPos; }
    [[nodiscard]] mc::math::Vector3d worldPosition(const mc::math::Vector3d& relativePos) const noexcept override
    {
        return relativePos;
    }
    [[nodiscard]] mc::math::Vector3d relativePosition(const mc::math::Vector3d& worldPos) const noexcept override
    {
        return worldPos;
    }
    [[nodiscard]] Direction rotateDirection(Direction direction) const noexcept override { return direction; }
    [[nodiscard]] mc::math::Vector3d rotateVector(const mc::math::Vector3d& vector) const noexcept override
    {
        return vector;
    }
    [[nodiscard]] Direction getTestDirection() const noexcept override { return Direction::North; }

    // 6. 完成路径 —— 空操作
    void succeedWhenBlockPresent(
        const std::string& /*blockType*/, BlockPos /*relativePos*/, bool /*isPresent*/) override
    {}
    void succeedWhen(std::function<GameTestResult()> /*fn*/) override {}
    void succeedIf(std::function<GameTestResult()> /*fn*/) override {}
    void succeedOnTick(i32 /*tick*/) override {}
    void succeedOnTickWhen(i32 /*tick*/, std::function<GameTestResult()> /*fn*/) override {}
    void failIf(std::function<GameTestResult()> /*fn*/) override {}
    void succeedWhenEntityHasComponent(const std::string& /*entityType*/,
        const std::string& /*componentId*/,
        BlockPos /*relativePos*/,
        bool /*hasComponent*/) override
    {}

    // 7. SimulatedPlayer
    [[nodiscard]] GameTestResult spawnSimulatedPlayer(const std::string& /*name*/,
        BlockPos /*relativePos*/,
        mc::GameMode /*gameMode*/,
        SimulatedPlayer*& outPlayer) override
    {
        outPlayer = nullptr;
        return std::nullopt;
    }
    void removeSimulatedPlayer(SimulatedPlayer& /*player*/) override {}

    // 8. 查询
    [[nodiscard]] const mc::BlockState* getBlock(BlockPos /*relativePos*/) const override { return nullptr; }
    [[nodiscard]] FenceConnectivity getFenceConnectivity(BlockPos /*relativePos*/) const override { return {}; }
    [[nodiscard]] mc::blocks::SculkSpreader* getSculkSpreader(BlockPos /*relativePos*/) const override
    {
        return nullptr;
    }
    [[nodiscard]] mc::IWorld& world() noexcept override;

    // 9. 工具
    void print(const std::string& /*text*/) override {}
    [[nodiscard]] GameTestError generateErrorWithContext(
        GameTestErrorType type, std::string message, BlockPos /*relativePos*/) const override
    {
        return GameTestError{type, std::move(message)};
    }

    // 10. 异步轮询（NullGameTestHelper 无实例状态机，until 为 no-op）
    void until(std::function<GameTestResult()> /*testFn*/, std::function<GameTestResult()> /*doneFn*/) override {}

private:
    TestTransform m_transform;
    std::unique_ptr<GameTestSequence> m_sequence; // 懒构造，startSequence 首次调用时建
};

} // namespace mc::test
