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
#include <cstddef>
#include <filesystem>
#include <string>

namespace mc::world::storage {

/**
 * @brief 世界名称清理与去重工具
 *
 * 提供世界名称的合法性检查、清理和冲突解决功能：
 * - 替换非法文件名字符为 '_'
 * - Windows 保留名处理
 * - 冲突时追加 " (1)"、"(2)" 等
 * - 限制长度
 */
class WorldNameSanitizer {
public:
    /**
     * @brief 非法文件名字符（适用于 Windows/Linux/macOS）
     *
     * 包括: / \ : * ? " < > | 以及控制字符
     */
    static const char* ILLEGAL_CHARS;

    /**
     * @brief Windows 保留名称列表
     *
     * CON, PRN, AUX, NUL, COM1-COM9, LPT1-LPT9
     */
    static bool isReservedName(const std::string& name) noexcept;

    /**
     * @brief 清理世界名称为合法文件名
     *
     * - 替换非法字符为 '_'
     * - 处理 Windows 保留名
     * - 去除首尾空格和点
     *
     * @param name 用户输入的世界名称
     * @return 清理后的名称
     */
    static std::string sanitizeName(const std::string& name) noexcept;

    /**
     * @brief 在给定目录中查找可用的世界目录名
     *
     * 如果请求名称已存在，则尝试追加 " (1)"、"(2)" 等，
     * 直到找到一个不存在的目录名。
     *
     * @param savesDir 存档根目录
     * @param requestedName 请求的目录名（已清理）
     * @return 可用的目录名（不含路径）
     */
    static Result<std::string> findAvailableLevelId(
        const std::filesystem::path& savesDir, const std::string& requestedName);

    /**
     * @brief 检查世界目录名是否可用
     *
     * 尝试创建并立即删除目录来验证。
     *
     * @param savesDir 存档根目录
     * @param levelId 目录名
     * @return 可用返回 true，不可用或错误返回 false
     */
    static bool isLevelIdAvailable(const std::filesystem::path& savesDir, const std::string& levelId) noexcept;

    /**
     * @brief 从显示名生成默认目录名
     *
     * 当用户未指定目录名时，使用显示名作为基础，
     * 清理后检查冲突。
     *
     * @param displayName 世界显示名
     * @return 清理后的目录名基础
     */
    static std::string generateLevelIdFromDisplayName(const std::string& displayName) noexcept;

private:
    /// 控制字符的最大值（ASCII 0-31 为控制字符）
    static constexpr u8 CONTROL_CHAR_MAX = 31;

    /// 文件名最大长度（预留扩展名空间）
    static constexpr std::size_t MAX_FILENAME_LENGTH = 240;

    /**
     * @brief 解析名称中的序号
     *
     * 如果名称形如 "Name (3)"，提取 "Name" 和序号 3。
     *
     * @param name 待解析的名称
     * @param baseName 输出：基础名称
     * @param number 输出：序号
     * @return 是否成功解析出序号
     */
    static bool _parseExistingNameWithNumber(const std::string& name, std::string& baseName, i32& number) noexcept;
};

} // namespace mc::world::storage
