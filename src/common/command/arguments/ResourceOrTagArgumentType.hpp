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

#include "GameModeArgument.hpp"

#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace command {

/**
 * @brief 「资源或标签」参数（原版 {@code ResourceOrTagArgument} / {@code ResourceOrTagKeyArgument}）
 *
 * 形如 `#minecraft:logs`（标签）或 `minecraft:stone`（单个资源）。前者以 `#` 开头。
 * 命令树上的类型名由子类决定：
 * - `resource_or_tag`：值侧（/locate biome、/execute if biome 等）；
 * - `resource_or_tag_key`：键侧（/locate structure 等）。
 *
 * Cubium 侧不做注册表校验：语法合法即可，找不到资源/标签由调用方报错。
 */
class ParsedResourceOrTag {
public:
    ParsedResourceOrTag()
        : m_id("minecraft", "")
        , m_tag(false)
    {}

    ParsedResourceOrTag(ResourceLocation id, bool tag)
        : m_id(std::move(id))
        , m_tag(tag)
    {}

    [[nodiscard]] const ResourceLocation& id() const noexcept { return m_id; }
    [[nodiscard]] bool isTag() const noexcept { return m_tag; }

    /// 原样文本（标签带 `#`），用于直接按键查找。
    [[nodiscard]] std::string toString() const { return (m_tag ? "#" : "") + m_id.toString(); }

private:
    ResourceLocation m_id;
    bool m_tag;
};

/**
 * @brief 共用解析：可选的 `#` 前缀 + 资源位置。
 */
[[nodiscard]] inline ParsedResourceOrTag readResourceOrTag(StringReader& reader)
{
    bool tag = false;
    const i32 start = reader.getCursor();
    if (reader.canRead() && reader.peek() == '#') {
        tag = true;
        reader.skip();
    }
    try {
        return ParsedResourceOrTag(readResourceLocationId(reader), tag);
    }
    catch (const CommandException&) {
        reader.setCursor(start);
        throw;
    }
}

/// /locate biome、/execute if biome 等：resource_or_tag（值侧）。
class ResourceOrTagArgumentType : public ArgumentType<ParsedResourceOrTag> {
public:
    [[nodiscard]] ParsedResourceOrTag parse(StringReader& reader) override { return readResourceOrTag(reader); }

    [[nodiscard]] std::string getTypeName() const override { return "resource_or_tag"; }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"minecraft:plains", "#minecraft:is_forest", "plains"};
    }

    static std::shared_ptr<ResourceOrTagArgumentType> resourceOrTag()
    {
        return std::make_shared<ResourceOrTagArgumentType>();
    }
};

/// /locate structure 等：resource_or_tag_key（键侧）。
class ResourceOrTagKeyArgumentType : public ArgumentType<ParsedResourceOrTag> {
public:
    [[nodiscard]] ParsedResourceOrTag parse(StringReader& reader) override { return readResourceOrTag(reader); }

    [[nodiscard]] std::string getTypeName() const override { return "resource_or_tag_key"; }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"minecraft:village", "#minecraft:village", "village"};
    }

    static std::shared_ptr<ResourceOrTagKeyArgumentType> resourceOrTagKey()
    {
        return std::make_shared<ResourceOrTagKeyArgumentType>();
    }
};

} // namespace command
} // namespace mc