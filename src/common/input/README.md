# Input Module

输入系统，提供按键绑定和平台无关键码常量。

## 目录结构

```
src/common/input/
├── KeyBinding.hpp    # 按键绑定类 + Keys命名空间（平台无关键码常量）
└── KeyBinding.cpp    # 按键绑定实现
```

## 内部模块关系

```
Keys命名空间（键码常量）
       ↓
KeyBinding类（可重映射按键绑定）
       ↓
ClientSettings（初始化默认绑定）
```

- **Keys命名空间** 定义与GLFW兼容的平台无关键码常量，供UI组件和测试使用，避免直接依赖GLFW头文件
- **KeyBinding类** 管理可重映射的按键绑定，支持按键状态查询、分类、JSON序列化

## 上下游外部依赖关系

**被依赖方（谁使用了此模块）：**
- `client/ui/kagero/` - Widget组件（TextFieldWidget、SliderWidget、ScrollableWidget）和模板系统（BuiltinEvents）使用 `Keys::XXX` 常量替代硬编码键码
- `client/settings/ClientSettings.hpp` - 初始化默认按键绑定
- `client/input/InputManager.hpp` - 管理按键状态和回调
- 测试代码 - 使用 `Keys::XXX` 替代硬编码数值

**依赖方（此模块使用了谁）：**
- `common/core/Types.hpp` - 基础类型定义（i32等）

## 容易踩的坑

### Keys常量值与GLFW一致但不要直接包含GLFW

`Keys`命名空间中的常量值与GLFW键码完全相同，但不要因此引入`<GLFW/glfw3.h>`。Widget层应始终使用`Keys::XXX`，保持平台无关性。

### KeyAction和KeyMods在Types.hpp而非KeyBinding.hpp

`KeyAction`（Press/Release/Repeat）和`KeyMods`（Shift/Control/Alt/Super/CapsLock/NumLock）枚举定义在`client/ui/kagero/Types.hpp`中，不在本模块。这是因为它们属于UI事件系统，不属于通用输入层。
