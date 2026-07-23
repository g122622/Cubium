# Screen 模块

`common/screen` 模块定义屏幕类型标识（`ScreenType` 枚举及其与资源 ID 的映射），供服务端菜单（如 `CraftingMenu`）标记自身所属的屏幕/界面类型，用于序列化与跨端共享。

## 目录结构

```
src/common/screen/
├── ScreenType.hpp     # 屏幕类型枚举声明
└── ScreenType.cpp     # 屏幕类型与资源ID映射实现
```

## 模块关系图

```
                    ┌─────────────────┐
                    │ ScreenType.hpp  │
                    │   (类型枚举)     │
                    └────────┬────────┘
                             │ 使用
                             ▼
┌──────────────────────────────────────────────────┐
│           server/menu/                            │
│  ┌─────────────────────────────────────────────┐ │
│  │   CraftingMenu                              │ │
│  │   - 服务端容器菜单                           │ │
│  │   - 关联ScreenType标识                      │ │
│  └─────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────┘
```

## 模块职责

1. **定义屏幕类型标识** - 提供屏幕类型的标准化枚举和序列化（`screenTypeToId` / `screenTypeFromId`）。
2. **解耦客户端与服务端** - 客户端和服务端通过 ScreenType 共享屏幕类型信息。

## 依赖项

| 依赖 | 说明 |
|------|------|
| `core/Types.hpp` | 基础类型定义（i32, u8, f32, String等） |
| `<string>` | std::string |
| `<unordered_map>` | ScreenType.cpp中使用 |

## 容易踩的坑

### 1. 资源ID格式

`screenTypeFromId` 支持两种格式：
- 完整格式：`minecraft:crafting_table`
- 简写格式：`crafting_table`

但 `screenTypeToId` 总是返回完整格式。若需要与其他系统对接，注意格式一致性。

### 2. ScreenType映射不完整

`idToTypeMap` 中的简写形式只覆盖了部分常用类型，不常用的屏幕类型（如`beacon`、`anvil`等）只能用完整格式解析。

## 相关文件

| 文件 | 说明 |
|------|------|
| `server/menu/CraftingMenu.hpp` | 服务端工作台菜单（使用ScreenType） |
| `client/ui/screen/ScreenManager.hpp` | 屏幕管理器（kagero 体系屏幕栈门面） |
| `client/ui/minecraft/widgets/ScreenStackWidget.hpp` | kagero UI屏幕栈组件 |
