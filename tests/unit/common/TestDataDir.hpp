/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/ sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

// 仓库内测试素材（testdata）的定位工具。
//
// 与 GameDirectory::defaultDirectory() 那套「读 ~/minecraft_reborn/ 下的真实数据包」不同：
// 那类素材位于机器相关的外部路径，缺失时测试只能 GTEST_SKIP，等于在 CI/他人机器上不跑。
// 本文件的素材随仓库分发，因此缺失即故障，调用方应直接断言失败而不是跳过。
//
// 只负责定位 tests/unit/testdata/ 这个根；素材内部如何分类（worlds/、nbt/、……）由各
// 素材自己的夹具表达，新增素材种类无需改动本文件。

#pragma once

#include <filesystem>
#include <string>
#include <string_view>

// 素材路径由测试目标注入的编译期宏拼接。缺少该宏说明构建配置被改动过，此处直接编译期报错，
// 避免悄悄退化成依赖工作目录的相对路径（在不同 CWD 下跑会得到难以定位的假失败）。
#ifndef MC_SOURCE_DIR
#error "MC_SOURCE_DIR 未定义：测试素材定位依赖 tests/unit/CMakeLists.txt 的 target_compile_definitions"
#endif

namespace mc::test {

/**
 * @brief 测试素材根目录
 *
 * @return <仓库根>/tests/unit/testdata
 */
inline std::filesystem::path testDataRoot()
{
    return std::filesystem::path(MC_SOURCE_DIR) / "tests" / "unit" / "testdata";
}

/**
 * @brief 取素材根下的路径（不校验存在）
 *
 * @param relative 相对素材根的路径，如 "worlds/java-anvil-1.21.11"
 * @return 绝对路径
 */
inline std::filesystem::path testDataPath(std::string_view relative)
{
    return testDataRoot() / std::filesystem::path(relative);
}

/**
 * @brief 判断素材根下的路径是否存在
 *
 * @param relative 相对素材根的路径
 * @return 存在返回 true
 */
inline bool testDataExists(std::string_view relative)
{
    std::error_code ec;
    return std::filesystem::exists(testDataPath(relative), ec);
}

} // namespace mc::test
