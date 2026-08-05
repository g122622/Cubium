#pragma once

namespace mc::test {

/**
 * @brief 注册内置原生样例测试。
 *
 * 提供 1 个最小样例 `ExampleTests::alwaysSucceed`（空序列即通过），供 `GameTestServer` 自检与
 * 测试用例参考。由 `GameTestServer::initialize`（1F）调用，幂等。
 *
 * 样例经 `MC_REGISTER_GAME_TEST` 宏注册；测试体见 `.cpp`。
 */
void registerBuiltinNativeTests();

} // namespace mc::test
