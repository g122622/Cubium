# 模块绑定框架

将C++类和函数绑定到JS模块的声明式框架。所有组件通过`IScriptBindingContext`抽象接口操作，不依赖具体引擎API。

## 目录结构树

```
binding/
├── IModuleBindingFactory.hpp      # 模块绑定工厂接口，每个JS模块对应一个工厂
├── IScriptBindingContext.hpp      # 脚本绑定上下文抽象接口（核心）
├── ScriptCallbackHolder.hpp       # JS函数引用的生命周期管理包装
├── ScriptClassBinding.hpp         # 类绑定工具（ScriptObjectRegistry/NativeModuleBuilder/ClassRegistrar）
├── ScriptClassBinding.cpp         # 类绑定工具实现
├── ScriptHandleRegistry.hpp/cpp  # 按实际对象身份管理非拥有实体句柄的失效
└── README.md
```

## 内部模块关系

```
IScriptBindingContext (抽象接口)
        │
        ├── ScriptObjectRegistry ──→ 使用ctx进行C++/JS对象包装/解包
        │
        ├── NativeModuleBuilder ──→ 使用ctx构建模块、导出类
        │
        ├── ClassRegistrar ──→ 使用ctx注册方法/属性，使用Registry包装对象
        │
        └── ScriptCallbackHolder ──→ 使用ctx管理函数引用的retain/release
```

## 上下游外部依赖关系

### 被以下模块依赖
- `modules/MinecraftModuleFactory` — 注册@minecraft/server模块绑定
- `modules/ScriptEventBinding` — 注册脚本事件绑定
- `modules/ScriptCustomComponentBinding` — 注册自定义组件绑定
- `modules/types/ScriptVec2/ScriptVec3/ScriptColor` — 脚本类型转换
- `engine/quickjs/QuickJSBindingContext` — IScriptBindingContext的QuickJS实现
- `engine/quickjs/QuickJSContext/QuickJSEngine` — 通过QuickJSBindingContext使用

### 依赖以下模块
- `core/ModuleDependency`、`core/ModuleDescriptor` — 模块描述符和依赖
- `core/IScriptContext` — 脚本上下文抽象（前向声明）
- `common/core/Types.hpp` — 基本类型定义（i32, u64, f64等）

## 容易踩的坑

1. **void*句柄所有权约定**：`IScriptBindingContext`的所有`void*`值句柄有明确的所有权语义——创建方法返回的句柄拥有引用所有权；`getProperty`返回的句柄拥有引用所有权（调用者负责release）；`setProperty`消耗传入value的引用所有权（调用者不再需要release value）。忘记release会导致内存泄漏，重复release会导致崩溃。

2. **ScriptCallbackHolder构造时机**：必须传入有效的函数值（已通过`isFunction`检查），否则持有器为无效状态。构造后需要释放原始句柄（因为Holder会自己做retain）。

3. **owned对象的生命周期**：通过`ScriptObjectRegistry::wrap`包装的owned对象会由JS GC自动delete。如果C++对象由游戏管理（非owned），务必在C++对象销毁时调用`invalidate`防止悬空指针。

4. **ClassRegistrar模板参数**：`ClassRegistrar<T>`的`wrap`方法使用`typeid(T).name()`作为类型名，确保T是多态类型或具有正确的type_info。

5. **实体编号不是进程级身份**：不同维度独立分配 `EntityInstanceId`，未入册的临时实体也可能同号。脚本句柄必须按实际实体对象地址登记和失效；按编号分组会错误清空其他仍存活实体的句柄。单对象 `invalidate` 可能先清空 `ptr`，GC 注销须通过 `ObjectData` 本身定位记录。

6. **实体访问线程**：包装、解包和实体生命周期操作在所属游戏线程执行，脚本 tick 与世界 tick 顺序调用。注册表互斥锁仅保护容器，不赋予裸实体指针跨线程寿命保证。

7. **子对象的所有者**：背包容器的 opaque 指向背包，不是玩家本身；须显式保存所属实体地址，按实体销毁统一使句柄失效。实体及直接指向实体的组件可以复用实体地址，所有权移交不改变句柄所属实体。
