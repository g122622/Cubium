# 模块绑定框架

将C++类和函数绑定到JS模块的声明式框架。

- `IModuleBindingFactory` — 模块绑定工厂接口，注册模块的所有绑定
- `ModuleBinding` — 模块绑定，包含类、函数、枚举等
- `ClassBinding` — 类绑定，将C++类暴露到JS
- `ScriptClassBinding` — 脚本类绑定实现（QuickJS特化）
- `TypeConverter` — C++/JS类型转换注册表
- `ScriptObjectHandle` — 脚本对象句柄（强/弱引用）
