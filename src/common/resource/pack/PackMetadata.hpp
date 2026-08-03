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
#include <string>
#include <string_view>

namespace mc::resource {

/**
 * @brief 资源包元数据
 *
 * 解析pack.mcmeta文件
 */
class PackMetadata {
public:
    PackMetadata() = default;

    /**
     * @brief 构造函数
     * @param packFormat pack_format 版本号
     * @param description 描述文本
     */
    PackMetadata(i32 packFormat, std::string description);

    /**
     * @brief 从JSON字符串解析资源包元数据
     * @param jsonContent JSON格式的pack.mcmeta内容
     * @return 解析成功返回PackMetadata，失败返回错误
     */
    [[nodiscard]] static Result<PackMetadata> parse(std::string_view jsonContent);

    /**
     * @brief 从文件解析资源包元数据
     * @param filePath pack.mcmeta文件路径
     * @return 解析成功返回PackMetadata，失败返回错误
     */
    [[nodiscard]] static Result<PackMetadata> parseFile(std::string_view filePath);

    /**
     * @brief 获取pack_format版本号
     * @return pack_format版本
     */
    [[nodiscard]] i32 packFormat() const noexcept { return m_packFormat; }

    /**
     * @brief 获取描述文本
     * @return 描述文本
     */
    [[nodiscard]] const std::string& description() const noexcept { return m_description; }

    /**
     * @brief 验证版本兼容性
     * @param minFormat 最低支持版本
     * @param maxFormat 最高支持版本
     * @return 版本在范围内返回true
     */
    [[nodiscard]] bool isCompatible(i32 minFormat, i32 maxFormat) const noexcept;

private:
    i32 m_packFormat = 0;
    std::string m_description;
};

} // namespace mc::resource

namespace mc {
using PackMetadata = resource::PackMetadata;
} // namespace mc
