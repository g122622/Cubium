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
#include "common/command/arguments/ArgumentType.hpp"
#include "common/resource/ResourceLocation.hpp"

#include <string>
#include <utility>
#include <vector>

namespace mc::command {

/**
 * @brief 单值资源参数（原版 {@code ResourceArgument}，命令树里叫 {@code resource}）
 *
 * 取值绑定到某个注册表（构造时给注册表键，参与网络序列化），语法是单个资源位置；
 * 需要 `#tag` 语法时用 {@code ResourceOrTagArgument}。
 */
class ResourceArgumentType : public ArgumentType<ResourceLocation> {
public:
    explicit ResourceArgumentType(std::string registryKey, std::string typeName)
        : m_registryKey(std::move(registryKey))
        , m_typeName(std::move(typeName))
    {}

    [[nodiscard]] ResourceLocation parse(StringReader& reader) override
    {
        return readResourceLocationId(reader);
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return m_typeName;
    }

    /// 网络序列化：原版 ResourceArgument.Info 写注册表键，客户端据此定位取值注册表。
    [[nodiscard]] nlohmann::json serializeMetadata() const override
    {
        return nlohmann::json{{"registry", m_registryKey}};
    }

    [[nodiscard]] const std::string& registryKey() const noexcept
    {
        return m_registryKey;
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"minecraft:zombie", "zombie", "minecraft:pig"};
    }

private:
    /// 该参数所属的注册表键，参与网络序列化。
    std::string m_registryKey;
    /// 命令树里的类型名；原版部分注册表（战利品表）有专属类型名。
    std::string m_typeName;
};

/**
 * @brief 单值注册表键参数（原版 {@code ResourceKeyArgument}，命令树里叫 {@code resource_key}）
 *
 * 与 {@link ResourceArgumentType} 同形，只是类型名不同：原版用它表示「按键查找注册表条目」
 * 的参数（/advancement 的进度、/recipe 的配方），网络编码同样只写注册表键。
 */
class ResourceKeyArgumentType : public ArgumentType<ResourceLocation> {
public:
    explicit ResourceKeyArgumentType(std::string registryKey)
        : m_registryKey(std::move(registryKey))
    {}

    [[nodiscard]] ResourceLocation parse(StringReader& reader) override
    {
        return readResourceLocationId(reader);
    }

    [[nodiscard]] std::string getTypeName() const override
    {
        return "resource_key";
    }

    [[nodiscard]] nlohmann::json serializeMetadata() const override
    {
        return nlohmann::json{{"registry", m_registryKey}};
    }

    [[nodiscard]] const std::string& registryKey() const noexcept
    {
        return m_registryKey;
    }

    [[nodiscard]] std::vector<std::string> getExamples() const override
    {
        return {"minecraft:story/root", "story/root"};
    }

private:
    /// 该参数所属的注册表键，参与网络序列化。
    std::string m_registryKey;
};

} // namespace mc::command
