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

#include "common/mod/bedrock/addon/binding/IModuleBindingFactory.hpp"
#include "common/mod/bedrock/addon/core/ModuleDependency.hpp"
#include "common/mod/bedrock/addon/core/ModuleDescriptor.hpp"
#include "common/mod/bedrock/addon/modules/ScriptEventBinding.hpp"

#include <string>
#include <vector>

namespace mc::mod::bedrock::addon {

class ScriptScheduler;
class ScriptEventBus;
class IScriptBindingContext;

/**
 * @brief @minecraft/server模块绑定工厂
 *
 * 注册@minecraft/server 2.x核心API到脚本上下文。
 * 提供World、System、Dimension、Entity、Player、Block、ItemStack等类的JS绑定。
 *
 * 绑定模式：
 * 1. MinecraftModuleFactory::registerBindings() 创建NativeModuleBuilder
 * 2. 注册类（exportClass）和常量（exportConst）
 * 3. ClassRegistrar在类原型上添加方法和属性
 * 4. finalize()完成模块注册
 */
class MinecraftModuleFactory : public IModuleBindingFactory {
public:
    MinecraftModuleFactory() = default;
    ~MinecraftModuleFactory() override = default;

    [[nodiscard]] std::string name() const override { return "@minecraft/server"; }
    [[nodiscard]] std::string uuid() const override { return "b26a4d4c-afdf-4690-88f8-931846312678"; }
    [[nodiscard]] std::vector<ModuleVersion> supportedVersions() const override;
    [[nodiscard]] std::vector<ModuleDependency> dependencies(const ModuleVersion& version) const override;
    bool registerBindings(IScriptContext& context) override;

    void setScheduler(ScriptScheduler* scheduler);
    void setEventSignals(const std::vector<EventSignalInfo>& signals);
    void setEventBus(ScriptEventBus* eventBus);

private:
    ScriptScheduler* m_scheduler = nullptr;
    ScriptEventBus* m_eventBus = nullptr;
    std::vector<EventSignalInfo> m_eventSignals;
};

} // namespace mc::mod::bedrock::addon
