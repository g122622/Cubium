#pragma once

#include "common/core/Types.hpp"
#include "common/util/Direction.hpp" // Rotation
#include "common/world/block/BlockPos.hpp"

#include <vector>

namespace mc::test {

// 前向声明：BaseGameTestFunction 在 framework/function/，1B 阶段定义。
// base/ 不得依赖 framework/，故此处仅前向声明用于容器，实参由调用方提供。
class BaseGameTestFunction;

/**
 * @brief GameTest 运行期参数。
 *
 * 对齐基岩版 `TestParameters`：与 `TestData`（注册期元数据）分离——`TestData` 描述"测试是什么"，
 * `TestParameters` 描述"这一轮怎么跑"（在哪个坐标、是否失败即停、重复几次、每行几个、旋转几度）。
 *
 * 由 `GameTestServer`/`GameTestCommand` 在启动运行时构造，传给 `GameTestRunner`。
 * 字段：
 * - `testPos`：测试网格的起始绝对方块坐标（`StructureGridSpawner` 在此基础上按列/行间距铺开）。
 * - `stopOnFailure`：某测试失败后是否立即停止整批（对齐基岩，对应 `RetryOptions.haltOnFailure` 的批次级语义）。
 * - `stopOtherTestsOnFailure`：某测试失败后是否停止其他并行测试。
 * - `repeatCount`：每个测试重复运行次数（`--verify` 压测用，对齐 Java `rotateAndMultiply` 的 ×100）。
 * - `testsPerRow`：每行测试数（默认 8，对齐 Java `DEFAULT_TESTS_PER_ROW`）。
 * - `maxTestsPerBatch`：每批最大测试数（默认 50，对齐 Java `MAX_TESTS_PER_BATCH`）。
 * - `rotation`：本轮施加的额外旋转（`--verify` 时遍历 4 旋转各跑）。
 * - `testFunctions`：本轮要跑的测试函数列表（已应用 `--tests` 通配符筛选）。
 */
class TestParameters {
public:
    TestParameters() noexcept = default;

    [[nodiscard]] const BlockPos& testPos() const noexcept { return m_testPos; }
    [[nodiscard]] bool stopOnFailure() const noexcept { return m_stopOnFailure; }
    [[nodiscard]] bool stopOtherTestsOnFailure() const noexcept { return m_stopOtherTestsOnFailure; }
    [[nodiscard]] i32 repeatCount() const noexcept { return m_repeatCount; }
    [[nodiscard]] i32 testsPerRow() const noexcept { return m_testsPerRow; }
    [[nodiscard]] i32 maxTestsPerBatch() const noexcept { return m_maxTestsPerBatch; }
    [[nodiscard]] Rotation rotation() const noexcept { return m_rotation; }
    [[nodiscard]] const std::vector<BaseGameTestFunction*>& testFunctions() const noexcept { return m_testFunctions; }

    TestParameters& setTestPos(BlockPos pos) noexcept
    {
        m_testPos = pos;
        return *this;
    }
    TestParameters& setStopOnFailure(bool stop) noexcept
    {
        m_stopOnFailure = stop;
        return *this;
    }
    TestParameters& setStopOtherTestsOnFailure(bool stop) noexcept
    {
        m_stopOtherTestsOnFailure = stop;
        return *this;
    }
    TestParameters& setRepeatCount(i32 count) noexcept
    {
        m_repeatCount = count;
        return *this;
    }
    TestParameters& setTestsPerRow(i32 count) noexcept
    {
        m_testsPerRow = count;
        return *this;
    }
    TestParameters& setMaxTestsPerBatch(i32 count) noexcept
    {
        m_maxTestsPerBatch = count;
        return *this;
    }
    TestParameters& setRotation(Rotation rotation) noexcept
    {
        m_rotation = rotation;
        return *this;
    }
    TestParameters& setTestFunctions(std::vector<BaseGameTestFunction*> functions)
    {
        m_testFunctions = std::move(functions);
        return *this;
    }

private:
    BlockPos m_testPos;
    bool m_stopOnFailure = false;
    bool m_stopOtherTestsOnFailure = false;
    i32 m_repeatCount = 1;
    i32 m_testsPerRow = 8;
    i32 m_maxTestsPerBatch = 50;
    Rotation m_rotation = Rotation::None;
    std::vector<BaseGameTestFunction*> m_testFunctions;
};

} // namespace mc::test
