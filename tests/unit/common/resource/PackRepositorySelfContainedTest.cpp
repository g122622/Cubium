/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "common/TempDirHelper.hpp"
#include "common/resource/repository/PackRepository.hpp"

#include <filesystem>
#include <fstream>

using namespace mc::resource;

namespace {

void writeTextFile(const std::filesystem::path& path, const std::string& text)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << text;
}

} // namespace

TEST(PackRepositorySelfContainedTest, ScanAndReadResourceFromFolderPack)
{
    const auto tempRoot = mc::test::makeUniqueTestDir("mc_resource_pack_repository_test");
    const auto packDir = tempRoot / "packA";

    // pack.mcmeta 是 PackRepository 判断"资源包目录"的关键文件
    writeTextFile(packDir / "pack.mcmeta", R"({"pack":{"pack_format":3,"description":"Test Pack"}})");
    writeTextFile(packDir / "assets/minecraft/test.txt", "hello");

    PackRepository list;
    auto scanResult = list.scanDirectory(tempRoot);
    ASSERT_TRUE(scanResult.success());
    EXPECT_EQ(scanResult.value(), 1u);
    EXPECT_EQ(list.packCount(), 1u);
    EXPECT_EQ(list.enabledPackCount(), 1u);

    // PackRepository 是按优先级遍历启用包读取资源
    // 路径相对于 PackType 根目录（如 assets/），不含 "assets/" 前缀
    auto readResult = list.readTextResource("minecraft/test.txt");
    ASSERT_TRUE(readResult.success());
    EXPECT_EQ(readResult.value(), "hello");

    // 清理临时目录
    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}
