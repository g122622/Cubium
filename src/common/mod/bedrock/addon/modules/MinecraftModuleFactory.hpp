#pragma once

#include "common/mod/bedrock/addon/binding/IModuleBindingFactory.hpp"
#include "common/mod/bedrock/addon/core/ModuleDependency.hpp"
#include "common/mod/bedrock/addon/core/ModuleDescriptor.hpp"

namespace mc::mod::bedrock::addon {

/**
 * @brief @minecraft/server模块绑定工厂
 *
 * 注册@minecraft/server 2.x核心API到QuickJS上下文。
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
};

} // namespace mc::mod::bedrock::addon
