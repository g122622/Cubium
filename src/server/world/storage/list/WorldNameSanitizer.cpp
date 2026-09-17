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

#include "WorldNameSanitizer.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace mc::world::storage {

const char* WorldNameSanitizer::ILLEGAL_CHARS = "/\\:*?\"<>|";

bool WorldNameSanitizer::isReservedName(const std::string& name) noexcept
{
    // Windows 保留名
    static const char* reservedNames[] = {"CON",
        "PRN",
        "AUX",
        "NUL",
        "COM1",
        "COM2",
        "COM3",
        "COM4",
        "COM5",
        "COM6",
        "COM7",
        "COM8",
        "COM9",
        "LPT1",
        "LPT2",
        "LPT3",
        "LPT4",
        "LPT5",
        "LPT6",
        "LPT7",
        "LPT8",
        "LPT9"};

    std::string upperName = name;
    std::transform(upperName.begin(), upperName.end(), upperName.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });

    // 检查是否有扩展名，如果有则去除
    size_t dotPos = upperName.find('.');
    if (dotPos != std::string::npos) {
        upperName = upperName.substr(0, dotPos);
    }

    for (const char* reserved : reservedNames) {
        if (upperName == reserved) {
            return true;
        }
    }
    return false;
}

std::string WorldNameSanitizer::sanitizeName(const std::string& name) noexcept
{
    if (name.empty()) {
        return "World";
    }

    std::string result = name;

    // 替换非法字符为 '_'
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
        // 检查是否为非法字符
        if (std::strchr(ILLEGAL_CHARS, static_cast<char>(c)) != nullptr) {
            return '_';
        }
        // 控制字符
        if (c <= CONTROL_CHAR_MAX) {
            return '_';
        }
        return static_cast<char>(c);
    });

    // 去除首尾空格和点
    size_t start = result.find_first_not_of(" .");
    if (start == std::string::npos) {
        return "World";
    }
    size_t end = result.find_last_not_of(" .");
    result = result.substr(start, end - start + 1);

    // 处理 Windows 保留名
    if (isReservedName(result)) {
        result = "_" + result + "_";
    }

    // 限制长度（预留扩展名空间）
    if (result.length() > MAX_FILENAME_LENGTH) {
        result = result.substr(0, MAX_FILENAME_LENGTH);
    }

    return result;
}

bool WorldNameSanitizer::_parseExistingNameWithNumber(
    const std::string& name, std::string& baseName, i32& number) noexcept
{
    // 匹配模式 "Name (N)" 其中 N 是数字
    if (name.length() < 4) {
        return false;
    }

    size_t spacePos = name.find(" (");
    if (spacePos == std::string::npos) {
        return false;
    }

    if (name.back() != ')') {
        return false;
    }

    // 提取括号内的数字
    std::string numStr = name.substr(spacePos + 2, name.length() - spacePos - 3);

    // 检查是否全为数字
    for (char c : numStr) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            return false;
        }
    }

    try {
        number = std::stoi(numStr);
        baseName = name.substr(0, spacePos);
        return true;
    }
    catch (...) {
        return false;
    }
}

Result<std::string> WorldNameSanitizer::findAvailableLevelId(
    const std::filesystem::path& savesDir, const std::string& requestedName)
{
    std::string baseName = sanitizeName(requestedName);
    if (baseName.empty()) {
        baseName = "World";
    }

    std::string candidate = baseName;

    // 首先检查基础名称是否可用
    std::error_code ec;
    if (!std::filesystem::exists(savesDir / candidate, ec)) {
        return candidate;
    }

    // 解析现有名称，找到最大序号
    i32 maxNumber = 0;
    bool hasExistingNumber = _parseExistingNameWithNumber(baseName, baseName, maxNumber);

    // 遍历目录查找冲突并确定下一个可用序号
    std::vector<i32> usedNumbers;
    bool baseExists = false;

    for (const auto& entry : std::filesystem::directory_iterator(savesDir, ec)) {
        if (!entry.is_directory()) {
            continue;
        }

        std::string dirName = entry.path().filename().string();

        if (dirName == baseName) {
            baseExists = true;
            continue;
        }

        std::string existingBase;
        i32 existingNumber;
        if (_parseExistingNameWithNumber(dirName, existingBase, existingNumber)) {
            if (existingBase == baseName) {
                usedNumbers.push_back(existingNumber);
            }
        }
    }

    // 如果基础名称不存在，直接使用
    if (!baseExists) {
        return baseName;
    }

    // 找到最小未使用的序号
    std::sort(usedNumbers.begin(), usedNumbers.end());
    i32 nextNumber = 1;
    for (i32 num : usedNumbers) {
        if (num == nextNumber) {
            nextNumber++;
        } else if (num > nextNumber) {
            break;
        }
    }

    // 格式化新名称
    std::ostringstream oss;
    oss << baseName << " (" << nextNumber << ")";
    return oss.str();
}

bool WorldNameSanitizer::isLevelIdAvailable(const std::filesystem::path& savesDir, const std::string& levelId) noexcept
{
    std::error_code ec;
    std::filesystem::path worldDir = savesDir / levelId;

    // 如果目录不存在，尝试创建并删除
    if (!std::filesystem::exists(worldDir, ec)) {
        if (std::filesystem::create_directory(worldDir, ec)) {
            std::filesystem::remove(worldDir, ec);
            return true;
        }
        return false;
    }
    return false;
}

std::string WorldNameSanitizer::generateLevelIdFromDisplayName(const std::string& displayName) noexcept
{
    return sanitizeName(displayName);
}

} // namespace mc::world::storage
