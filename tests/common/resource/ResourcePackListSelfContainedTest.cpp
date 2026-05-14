#include <gtest/gtest.h>

#include "common/resource/ResourcePackList.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>

using namespace mc;

namespace {

std::filesystem::path makeUniqueTempDir()
{
    const auto base = std::filesystem::temp_directory_path();
    const auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const auto dir = base / ("mc_resource_pack_list_test_" + std::to_string(static_cast<long long>(now)));
    std::filesystem::create_directories(dir);
    return dir;
}

void writeTextFile(const std::filesystem::path& path, const std::string& text)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream file(path, std::ios::binary);
    file << text;
}

} // namespace

TEST(ResourcePackListSelfContainedTest, ScanAndReadResourceFromFolderPack)
{
    const auto tempRoot = makeUniqueTempDir();
    const auto packDir = tempRoot / "packA";

    // pack.mcmeta 是 ResourcePackList 判断“资源包目录”的关键文件
    writeTextFile(packDir / "pack.mcmeta", R"({"pack":{"pack_format":3,"description":"Test Pack"}})");
    writeTextFile(packDir / "assets/minecraft/test.txt", "hello");

    ResourcePackList list;
    auto scanResult = list.scanDirectory(tempRoot);
    ASSERT_TRUE(scanResult.success());
    EXPECT_EQ(scanResult.value(), 1u);
    EXPECT_EQ(list.packCount(), 1u);
    EXPECT_EQ(list.enabledPackCount(), 1u);

    // ResourcePackList 是按优先级遍历启用包读取资源
    auto readResult = list.readTextResource("assets/minecraft/test.txt");
    ASSERT_TRUE(readResult.success());
    EXPECT_EQ(readResult.value(), "hello");

    // 清理临时目录
    std::error_code ec;
    std::filesystem::remove_all(tempRoot, ec);
}
