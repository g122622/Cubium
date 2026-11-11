#include <gtest/gtest.h>

#include "common/core/Types.hpp"
#include "server/world/gen/density/ast/AstNodes.hpp"
#include "server/world/gen/density/ast/BytecodeGen.hpp"
#include "server/world/gen/density/ast/CompiledDensityFunction.hpp"
#include "server/world/gen/density/ast/OptoPasses.hpp"

#include <cmath>
#include <memory>

using namespace mc;
using namespace mc::world::gen::density::ast;

namespace {

/// JIT 启用平台判定（与 DensityJitCompiler.cpp 的 MC_DENSITY_JIT_ENABLED 一致）
#if (defined(_WIN32) || defined(__linux__)) && (defined(__x86_64__) || defined(_M_X64))
inline constexpr bool kJitEnabled = true;
#else
inline constexpr bool kJitEnabled = false;
#endif

/// 把 AST 编译成求值器。必须先过 OptoPasses：直接把未优化的 AST 交给
/// BytecodeGen::compile 不会得到正确结果（ClampNode 会退化成它的输入）。
std::shared_ptr<CompiledDensityFunction> compileTree(AstNode::Ptr root)
{
    AstNode::Ptr optimized = OptoPasses::optimize(std::move(root));
    return BytecodeGen::compile(optimized, -1.0, 1.0);
}

} // namespace

// clamp(y, lo, hi) 在 y 的不同取值上：低于下界夹到 lo、区间内原样、高于上界夹到 hi。
// 用 y 坐标（CoordinateNode）作输入而不是常量——常量会被 OptoPasses 折叠掉，
// 根本走不到 JIT 的 Clamp 指令，测试就成了空转。
TEST(DensityJitClampTest, JitMatchesInterpreterAcrossClampBranches)
{
    if (!kJitEnabled) {
        GTEST_SKIP() << "density JIT not enabled on this platform (non x64)";
    }

    // clamp(y, -0.5, 0.5)：y<0 时落在下界、y==0 时在区间内、y>0 时落在上界。
    AstNode::Ptr root = std::make_shared<ClampNode>(std::make_shared<CoordinateNode>(Axis::Y), -0.5, 0.5);

    auto compiled = compileTree(std::move(root));
    ASSERT_NE(compiled, nullptr);
    ASSERT_TRUE(compiled->hasJitCompiled()) << "expected JIT to compile this tree";

    for (const i32 y : {-64, -10, -5, -1, 0, 1, 5, 10, 64, 320}) {
        const f64 viaJit = compiled->eval(0, y, 0);
        const f64 viaInterpreter = compiled->evalInterpreter(0, y, 0);
        EXPECT_DOUBLE_EQ(viaJit, viaInterpreter) << "y=" << y;
    }

    // 三个分支的边界语义直接断言：
    EXPECT_DOUBLE_EQ(compiled->eval(0, -5, 0), -0.5) << "below lower bound must clamp to lo";
    EXPECT_DOUBLE_EQ(compiled->eval(0, 0, 0), 0.0) << "inside range must pass through";
    EXPECT_DOUBLE_EQ(compiled->eval(0, 5, 0), 0.5) << "above upper bound must clamp to hi";
}
