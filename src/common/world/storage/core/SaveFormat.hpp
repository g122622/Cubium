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

#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <filesystem>
#include <string>

namespace mc::world::storage {

/**
 * @brief 存档格式枚举
 *
 * 标识存档的物理存储格式，用于选择对应的后端读取器。
 */
enum class SaveFormat : u8 {
    /// 项目自有 RocksDB 格式（读写）
    Native,
    /// Java 版 Anvil 区域文件格式（只读）
    JavaAnvil,
    /// 基岩版 LevelDB 格式（只读）
    BedrockLDB,
};

/**
 * @brief 存档格式信息
 *
 * 包含检测到的存档格式、版本号等元数据。
 */
struct SaveFormatInfo {
    /// 检测到的存档格式
    SaveFormat format = SaveFormat::Native;

    /// 格式可读名称，如 "Java 1.16.5"、"Bedrock 1.21"、"Native"
    std::string formatName;

    /// 存档的数据版本号（Java 版 DataVersion / 基岩版协议版本 / Native 项目版本）
    i32 dataVersion = 0;

    /// 是否以只读方式打开（Java/Bedrock 始终为 true）
    bool readonly = false;
};

/**
 * @brief 存档格式检测器
 *
 * 根据目录结构和文件内容自动检测存档格式。
 * 检测优先级：
 * 1. 检查是否存在 region/ 目录及 .mca 文件 → JavaAnvil
 * 2. 检查 db/ 目录中是否包含 LevelDB 特征文件 → BedrockLDB
 * 3. 检查 db/ 目录中是否包含 RocksDB 特征文件 → Native
 * 4. 检查 level.dat 文件格式进行最终判定
 */
class SaveFormatDetector {
public:
    /**
     * @brief 检测指定目录的存档格式
     *
     * @param worldDir 存档目录路径
     * @return 格式信息，失败返回错误
     */
    static Result<SaveFormatInfo> detect(const std::filesystem::path& worldDir);

private:
    /// 检查目录是否包含 RocksDB 特征文件（OPTIONS-、*.sst、CURRENT）
    static bool hasRocksDb(const std::filesystem::path& dbDir);

    /// 检查目录是否包含 Anvil 区域文件（region/*.mca）
    static bool hasAnvilRegion(const std::filesystem::path& worldDir);

    /// 检查目录是否包含 Bedrock LevelDB 特征文件（*.ldb、LOG 文件且无 OPTIONS-）
    static bool hasBedrockDb(const std::filesystem::path& dbDir);

    /// 从 Java level.dat 读取版本信息
    static Result<SaveFormatInfo> detectJavaVersion(const std::filesystem::path& worldDir);

    /// 从 Bedrock level.dat 读取版本信息
    static Result<SaveFormatInfo> detectBedrockVersion(const std::filesystem::path& worldDir);
};

} // namespace mc::world::storage
