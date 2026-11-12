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
#include "common/util/text/ITextComponent.hpp"
#include <memory>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>

namespace mc::text {

/**
 * @brief NBT 数据源类型
 *
 * 对应原版 `net.minecraft.network.chat.contents.data.DataSources` 的三种数据源。
 */
enum class NbtDataSourceKind : u8 {
    Block,   // 方块实体：{"block":"x y z"}
    Entity,  // 实体：{"entity":"@p"}
    Storage, // 命令存储：{"storage":"namespace:path"}
};

/**
 * @brief NBT 路径组件
 *
 * 对应原版 `NbtContents`：从方块实体/实体/命令存储中取 NBT 路径的值并显示。
 * JSON 形态：
 * `{"nbt":"Inventory[0].id","interpret":true,"separator":{...},"entity":"@p"}`。
 *
 * 解析 NBT 需要服务器上下文；服务端只透传，`getUnformattedText()` 无上下文时返回空串。
 */
class NbtTextComponent : public BaseTextComponent {
public:
    /**
     * @brief 构造 NBT 组件
     * @param path NBT 路径（Mojangson 路径语法）
     * @param interpret 是否把取到的字符串再按文本组件解析
     * @param dataSourceKind 数据源类型
     * @param dataSourceValue 数据源文本（方块坐标 / 实体选择器 / 存储资源位置）
     */
    NbtTextComponent(std::string path, bool interpret, NbtDataSourceKind dataSourceKind, std::string dataSourceValue)
        : m_path(std::move(path))
        , m_interpret(interpret)
        , m_dataSourceKind(dataSourceKind)
        , m_dataSourceValue(std::move(dataSourceValue))
    {}

    [[nodiscard]] std::string getUnformattedText() const override
    {
        std::string result;
        for (const auto& sibling : m_siblings) {
            result += sibling->getUnformattedText();
        }
        return result;
    }

    [[nodiscard]] std::string getFormattedText() const override
    {
        std::string result = getStyleCodes(m_style);
        for (const auto& sibling : m_siblings) {
            result += sibling->getFormattedText();
        }
        return result;
    }

    [[nodiscard]] std::unique_ptr<ITextComponent> deepCopy() const override
    {
        auto copy = std::make_unique<NbtTextComponent>(m_path, m_interpret, m_dataSourceKind, m_dataSourceValue);
        if (m_separator != nullptr) {
            copy->m_separator = m_separator->deepCopy();
        }
        copyBaseTo(*copy);
        return copy;
    }

    [[nodiscard]] std::unique_ptr<ITextComponent> shallowCopy() const override
    {
        auto copy = std::make_unique<NbtTextComponent>(m_path, m_interpret, m_dataSourceKind, m_dataSourceValue);
        if (m_separator != nullptr) {
            copy->m_separator = m_separator->deepCopy();
        }
        copy->setStyle(m_style);
        return copy;
    }

    [[nodiscard]] nlohmann::json toJson() const override
    {
        nlohmann::json json = m_style.toJson();
        json["nbt"] = m_path;
        if (m_interpret) {
            json["interpret"] = true;
        }
        if (m_separator != nullptr) {
            json["separator"] = m_separator->toJson();
        }
        json[dataSourceKey()] = m_dataSourceValue;
        appendSiblingsToJson(json);
        return json;
    }

    [[nodiscard]] const std::string& path() const noexcept
    {
        return m_path;
    }

    [[nodiscard]] bool isInterpreting() const noexcept
    {
        return m_interpret;
    }

    [[nodiscard]] NbtDataSourceKind dataSourceKind() const noexcept
    {
        return m_dataSourceKind;
    }

    [[nodiscard]] const std::string& dataSourceValue() const noexcept
    {
        return m_dataSourceValue;
    }

    [[nodiscard]] const std::unique_ptr<ITextComponent>& separator() const noexcept
    {
        return m_separator;
    }

    /**
     * @brief 设置多个结果之间的分隔组件
     * @param separator 分隔组件（所有权转移）
     */
    void setSeparator(std::unique_ptr<ITextComponent> separator)
    {
        m_separator = std::move(separator);
    }

    /**
     * @brief 数据源类型对应的 JSON/NBT 键名
     */
    [[nodiscard]] const char* dataSourceKey() const noexcept
    {
        switch (m_dataSourceKind) {
            case NbtDataSourceKind::Block:
                return "block";
            case NbtDataSourceKind::Entity:
                return "entity";
            case NbtDataSourceKind::Storage:
                return "storage";
        }
        return "storage";
    }

private:
    std::string m_path;
    bool m_interpret;
    NbtDataSourceKind m_dataSourceKind;
    std::string m_dataSourceValue;
    std::unique_ptr<ITextComponent> m_separator;
};

} // namespace mc::text
