#include "server/test/native/builtin/BuiltinNativeTests.hpp"

// GameTestHelper.hpp 提供 wrapNativeBody 模板定义（facade，GameTestHelper 完整）；MC_REGISTER_GAME_TEST 宏
// 展开调用之，须在宏展开前可见——本 TU 顶部 include 即满足（所有头先于宏展开处理）。
#include "common/test/framework/sequence/GameTestSequence.hpp" // startSequence() 返回值类型（需完整类型调 thenExecute/thenSucceed）
#include "common/test/native/GameTestMacros.hpp"               // MC_REGISTER_GAME_TEST
#include "server/test/facade/GameTestHelper.hpp"               // GameTestHelper（facade）+ wrapNativeBody 模板
#include "server/test/facade/GameTestRegistrar.hpp" // GameTestRegistrar（facade，宏依赖）

namespace mc::test {

// 样例测试体：空序列 thenSucceed 即通过（验证框架最小路径：注册→放置→tick→序列完成→succeed）
static void ExampleTests_alwaysSucceed(mc::test::GameTestHelper& helper)
{
    helper.startSequence().thenExecute([] { return mc::test::pass(); }).thenSucceed();
}

// 静态注册：maxTicks=20，required=true，结构名 gametest:empty_3x3
// 结构资源由 GameTestStructureBootstrap::ensureBuiltinStructureTemplates() 在服务端启动期程序化注入
// （3×3×3 全 air 模板，placeInWorld 立即成功）。TODO: 后续提供正式 .nbt 资源到资源包后可移除程序化兜底。
MC_REGISTER_GAME_TEST("ExampleTests", "alwaysSucceed", ExampleTests_alwaysSucceed)
    .structureName("gametest:empty_3x3")
    .maxTicks(20)
    .required(true)
    .registerTest();

void registerBuiltinNativeTests()
{
    // 注册经静态初始化完成；此函数仅作为显式触发点供 GameTestServer 调用，确保链接期保留样例 TU。
    // 静态初始化在 main 前已执行，此处 no-op；保留函数以便未来按需延迟注册。
}

} // namespace mc::test
