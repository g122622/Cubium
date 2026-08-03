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

#include "common/mod/bedrock/addon/core/IScriptEngine.hpp"
#include "common/mod/bedrock/addon/core/ScriptData.hpp"
#include "common/mod/bedrock/addon/pack/BehaviorPack.hpp"

#include <memory>
#include <optional>
#include <string>

namespace mc::mod::bedrock::addon {

/**
 * @brief 脚本包源
 *
 * 从BehaviorPack中读取脚本文件，实现IDependencyLoader接口。
 * 每个ScriptPlugin持有一个ScriptPluginSource实例来加载其脚本代码。
 */
class ScriptPluginSource : public IDependencyLoader {
public:
    /**
     * @brief 构造函数
     * @param pack 行为包引用
     */
    explicit ScriptPluginSource(const BehaviorPack& pack);

    /**
     * @brief 从行为包中加载脚本模块
     * @param moduleName 模块名称（如"@minecraft/server"或相对路径）
     * @return 脚本数据，未找到返回std::nullopt
     */
    [[nodiscard]] std::optional<ScriptData> loadScript(const std::string& moduleName) override;

    /**
     * @brief 读取入口脚本
     * @return 入口脚本的源代码和路径
     */
    [[nodiscard]] std::optional<ScriptData> loadEntryPoint() const;

    /**
     * @brief 获取关联的行为包
     */
    [[nodiscard]] const BehaviorPack& pack() const;

private:
    const BehaviorPack& m_pack;
};

} // namespace mc::mod::bedrock::addon
