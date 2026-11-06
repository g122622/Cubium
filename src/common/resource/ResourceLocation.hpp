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

#include "common/core/Types.hpp"
#include "common/resource/PackType.hpp"
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace mc::resource {

/**
 * @brief 资源位置标识符
 *
 * Minecraft标准资源格式: namespace:path
 * 例如: minecraft:textures/blocks/stone
 *       minecraft:models/block/cube_all
 */
class ResourceLocation {
public:
    ResourceLocation();
    explicit ResourceLocation(std::string_view fullPath);
    ResourceLocation(std::string namespace_, std::string path);

    // 解析资源路径
    [[nodiscard]] static ResourceLocation parse(std::string_view fullPath);

    /**
     * @brief 该字符能否出现在路径中，对应原版 `Identifier.validPathChar`
     *
     * 允许 `_ - a-z 0-9 / .`。注意**不含大写字母**，原版对路径大小写敏感且只认小写。
     */
    [[nodiscard]] static bool validPathChar(char value) noexcept
    {
        return value == '_' || value == '-' || (value >= 'a' && value <= 'z') || (value >= '0' && value <= '9') ||
            value == '/' || value == '.';
    }

    /**
     * @brief 该字符能否出现在命名空间中，对应原版 `Identifier.validNamespaceChar`
     *
     * 与路径的区别是**不允许 `/`**——命名空间只有一层。
     */
    [[nodiscard]] static bool validNamespaceChar(char value) noexcept
    {
        return value == '_' || value == '-' || (value >= 'a' && value <= 'z') || (value >= '0' && value <= '9') ||
            value == '.';
    }

    /**
     * @brief 路径是否全部由合法字符组成，对应原版 `Identifier.isValidPath`
     *
     * **空串返回 true**——原版只逐字符检查，没有非空校验。
     */
    [[nodiscard]] static bool isValidPath(std::string_view path) noexcept
    {
        for (const char value : path) {
            if (!validPathChar(value)) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief 命名空间是否全部由合法字符组成，对应原版 `Identifier.isValidNamespace`
     *
     * 同样**空串返回 true**。
     */
    [[nodiscard]] static bool isValidNamespace(std::string_view namespace_) noexcept
    {
        for (const char value : namespace_) {
            if (!validNamespaceChar(value)) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief 按分隔符拆分并校验，对应原版 `Identifier.tryBySeparator`
     *
     * 与 `parse` 的区别是**会做合法性校验**：任一字段含非法字符即返回 `std::nullopt`，
     * 而 `parse` 不做校验、总是返回一个对象。
     *
     * 拆分规则与原版逐条一致：
     * - 无分隔符：整串作路径，命名空间取 `minecraft`；
     * - 分隔符在首位（如 `":foo"`）：命名空间取 `minecraft`，其后作路径；
     * - 其余：分隔符前作命名空间、其后作路径。
     *
     * 注意路径/命名空间**允许为空**，原版也允许（`"ns:"` 与 `""` 都是合法的）。
     */
    [[nodiscard]] static std::optional<ResourceLocation> tryBySeparator(std::string_view text, char separator) noexcept
    {
        std::string namespace_;
        std::string path;
        const std::size_t position = text.find(separator);
        if (position != std::string_view::npos) {
            path = std::string(text.substr(position + 1));
            if (position == 0) {
                namespace_ = "minecraft";
            } else {
                namespace_ = std::string(text.substr(0, position));
            }
        } else {
            path = std::string(text);
            namespace_ = "minecraft";
        }

        if (!isValidPath(path) || !isValidNamespace(namespace_)) {
            return std::nullopt;
        }
        return ResourceLocation(std::move(namespace_), std::move(path));
    }

    // 获取命名空间
    [[nodiscard]] const std::string& namespace_() const noexcept
    {
        return m_namespace;
    }

    // 获取路径
    [[nodiscard]] const std::string& path() const noexcept
    {
        return m_path;
    }

    // 检查是否有效
    [[nodiscard]] bool isValid() const noexcept
    {
        return !m_namespace.empty() || !m_path.empty();
    }

    /**
     * @brief 语言键，对应原版 `Identifier.toLanguageKey`
     *
     * 把 `ns:path` 的冒号换成点。注意 `/` **不换**——翻译键里的路径分隔符保留原样。
     */
    [[nodiscard]] std::string toLanguageKey() const
    {
        return m_namespace + "." + m_path;
    }

    /**
     * @brief 短语言键，对应原版 `Identifier.toShortLanguageKey`
     *
     * 默认命名空间（`minecraft`）下省略前缀，否则与 `toLanguageKey` 相同。
     */
    [[nodiscard]] std::string toShortLanguageKey() const
    {
        return m_namespace == "minecraft" ? m_path : toLanguageKey();
    }

    /**
     * @brief 短字符串，对应原版 `Identifier.toShortString`
     *
     * 默认命名空间下只给路径，否则给 `ns:path`。
     */
    [[nodiscard]] std::string toShortString() const
    {
        return m_namespace == "minecraft" ? m_path : toString();
    }

    /**
     * @brief 可作文件名的形式，对应原版 `Identifier.toDebugFileName`
     *
     * `toDebugFileName` 会把 `/` 和 `:` 都换成 `_`，得到单段文件名。
     */
    [[nodiscard]] std::string toDebugFileName() const
    {
        std::string result = toString();
        for (char& value : result) {
            if (value == '/' || value == ':') {
                value = '_';
            }
        }
        return result;
    }

    // 转换为完整字符串 "namespace:path"
    [[nodiscard]] std::string toString() const;

    // 根据 PackType 转换为文件路径
    // ClientResources -> "assets/namespace/path"
    // ServerData -> "data/namespace/path"
    [[nodiscard]] std::string toFilePath(PackType type) const;

    // 根据 PackType 转换为文件路径（带扩展名）
    [[nodiscard]] std::string toFilePath(PackType type, std::string_view extension) const;

    // 比较
    [[nodiscard]] bool operator==(const ResourceLocation& other) const noexcept;
    [[nodiscard]] bool operator!=(const ResourceLocation& other) const noexcept;
    [[nodiscard]] bool operator<(const ResourceLocation& other) const noexcept;

    // 哈希
    [[nodiscard]] size_t hash() const noexcept;

private:
    std::string m_namespace;
    std::string m_path;
};

} // namespace mc::resource

// std::hash特化
namespace std {
template <>
struct hash<mc::resource::ResourceLocation> {
    size_t operator()(const mc::resource::ResourceLocation& loc) const noexcept
    {
        return loc.hash();
    }
};
} // namespace std

namespace mc {
using ResourceLocation = resource::ResourceLocation;
} // namespace mc
