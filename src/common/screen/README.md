# Screen 模块

`common/screen` 模块定义了游戏屏幕系统的核心接口和类型，为客户端UI屏幕提供统一的抽象层。

## 目录结构

```
src/common/screen/
├── IScreen.hpp        # 屏幕接口定义
├── ScreenType.hpp     # 屏幕类型枚举声明
└── ScreenType.cpp     # 屏幕类型与资源ID映射实现
```

## 文件详解

### IScreen.hpp

**职责**：定义所有客户端UI屏幕的基类接口。

**主要内容**：
- `IScreen` 抽象类，定义屏幕的完整生命周期和交互方法

**屏幕生命周期**：
1. **创建** - 通过工厂方法创建
2. **初始化** - `init()` 设置UI元素
3. **显示** - 显示给玩家
4. **交互** - 玩家点击/按键/滚轮等
5. **关闭** - `onClose()` 清理资源

**核心方法**：

| 方法 | 说明 |
|------|------|
| `init()` | 初始化屏幕，设置UI元素和状态 |
| `render(mouseX, mouseY, partialTick)` | 渲染屏幕内容 |
| `onClick(mouseX, mouseY, button)` | 处理鼠标点击 |
| `onRelease(mouseX, mouseY, button)` | 处理鼠标释放（可选实现） |
| `onDrag(mouseX, mouseY, deltaX, deltaY)` | 处理鼠标拖动（可选实现） |
| `onScroll(mouseX, mouseY, delta)` | 处理鼠标滚轮（可选实现） |
| `onKey(key, scanCode, action, mods)` | 处理键盘按键 |
| `onChar(codePoint)` | 处理字符输入（可选实现） |
| `onClose()` | 屏幕关闭时调用，清理资源 |
| `isPauseScreen()` | 返回是否暂停游戏 |
| `getTitle()` | 获取屏幕标题 |
| `shouldRenderBackground()` | 是否渲染背景暗化 |
| `onResize(width, height)` | 屏幕尺寸改变时调用 |
| `tick(dt)` | 每帧更新 |

### ScreenType.hpp

**职责**：定义所有可打开的屏幕/界面类型枚举。

**枚举类型**：`ScreenType`

**屏幕类型分类**：

| 分类 | 类型 |
|------|------|
| **玩家背包** | `Inventory`, `CreativeInventory` |
| **容器** | `Chest`, `DoubleChest`, `ShulkerBox`, `Barrel` |
| **工作台** | `CraftingTable`, `Furnace`, `BlastFurnace`, `Smoker`, `Anvil`, `Grindstone`, `Stonecutter`, `SmithingTable`, `Loom`, `CartographyTable`, `BrewingStand`, `EnchantingScreen` |
| **红石** | `Dispenser`, `Dropper`, `Hopper`, `Beacon` |
| **其他** | `Sign`, `CommandBlock`, `StructureBlock`, `JigsawBlock`, `Bed` |

**辅助函数**：
- `screenTypeToId(ScreenType)` - 获取屏幕类型的资源位置ID
- `screenTypeFromId(const String&)` - 从资源位置ID解析屏幕类型

### ScreenType.cpp

**职责**：实现屏幕类型与资源ID的双向映射。

**实现细节**：
- 使用 `std::unordered_map` 存储双向映射
- 支持完整的资源ID格式（如 `minecraft:crafting_table`）
- 支持简写格式（如 `crafting_table`、`furnace`）

## 模块关系图

```
                    ┌─────────────────┐
                    │   IScreen.hpp   │
                    │   (接口定义)     │
                    └────────┬────────┘
                             │ 继承
                             ▼
┌──────────────────────────────────────────────────┐
│           client/ui/screen/                       │
│  ┌─────────────────────────────────────────────┐ │
│  │   AbstractContainerScreen<Menu>             │ │
│  │   - 模板基类，继承IScreen                    │ │
│  │   - 管理容器菜单的客户端屏幕                 │ │
│  └─────────────────────────────────────────────┘ │
│  ┌─────────────────────────────────────────────┐ │
│  │   CraftingScreen                            │ │
│  │   - 具体屏幕实现                             │ │
│  └─────────────────────────────────────────────┘ │
│  ┌─────────────────────────────────────────────┐ │
│  │   ScreenManager                             │ │
│  │   - 管理屏幕栈                               │ │
│  │   - 使用IScreen指针                          │ │
│  └─────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────┘

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

### 整体职责

1. **定义屏幕抽象接口** - 为所有客户端UI屏幕提供统一的交互接口
2. **定义屏幕类型标识** - 提供屏幕类型的标准化枚举和序列化
3. **解耦客户端与服务端** - 客户端和服务端通过ScreenType共享屏幕类型信息

### 输入

- 鼠标事件（点击、释放、拖动、滚轮）
- 键盘事件（按键、字符输入）
- 屏幕尺寸变化事件
- 帧更新事件（tick）

### 输出

- 渲染结果（通过render方法）
- 事件处理结果（布尔值表示是否消费事件）
- 屏幕状态（是否暂停游戏、是否渲染背景）

## 依赖项

| 依赖 | 说明 |
|------|------|
| `core/Types.hpp` | 基础类型定义（i32, u8, f32, String等） |
| `<string>` | std::string |
| `<unordered_map>` | ScreenType.cpp中使用 |

## 使用方法

### 创建自定义屏幕

```cpp
#include "screen/IScreen.hpp"

class MyScreen : public mc::IScreen {
public:
    void init() override {
        // 初始化UI元素
    }

    void render(i32 mouseX, i32 mouseY, f32 partialTick) override {
        // 渲染屏幕内容
    }

    bool onClick(i32 mouseX, i32 mouseY, i32 button) override {
        // 处理点击
        return true; // 事件已处理
    }

    bool onKey(i32 key, i32 scanCode, i32 action, i32 mods) override {
        if (key == 256 && action == 1) { // ESC
            onClose();
            return true;
        }
        return false;
    }

    void onClose() override {
        // 清理资源
    }

    String getTitle() const override {
        return "My Screen";
    }
};
```

### 使用ScreenType

```cpp
#include "screen/ScreenType.hpp"

// 获取屏幕类型的资源ID
mc::String id = mc::screenTypeToId(mc::ScreenType::CraftingTable);
// 结果: "minecraft:crafting_table"

// 从资源ID解析屏幕类型
mc::ScreenType type = mc::screenTypeFromId("minecraft:furnace");
// 结果: ScreenType::Furnace

// 简写格式也支持
mc::ScreenType type2 = mc::screenTypeFromId("hopper");
// 结果: ScreenType::Hopper
```

### 服务端关联屏幕类型

```cpp
// 服务端菜单类中使用ScreenType标识
class CraftingMenu : public AbstractContainerMenu {
public:
    CraftingMenu(/* ... */)
        : m_screenType(ScreenType::CraftingTable) {}

    ScreenType getScreenType() const { return m_screenType; }

private:
    ScreenType m_screenType;
};
```

## 容易踩的坑

### 1. 鼠标按钮编码

鼠标按钮参数使用GLFW编码：
- `0` = 左键
- `1` = 右键
- `2` = 中键

不要与Minecraft协议中的按钮编码混淆。

### 2. 键盘按键编码

键盘按键使用GLFW键码：
- `256` = ESC键
- `69` = E键

实现时需要引用GLFW常量或硬编码值。

### 3. 屏幕暂停行为

默认 `isPauseScreen()` 返回 `false`。菜单类屏幕应重写返回 `true`，容器类屏幕返回 `false`：

```cpp
// 菜单屏幕
bool isPauseScreen() const override { return true; }

// 容器屏幕（背包、箱子等）
bool isPauseScreen() const override { return false; }
```

### 4. 资源ID格式

`screenTypeFromId` 支持两种格式：
- 完整格式：`minecraft:crafting_table`
- 简写格式：`crafting_table`

但 `screenTypeToId` 总是返回完整格式。若需要与其他系统对接，注意格式一致性。

### 5. 事件消费机制

所有事件处理方法返回 `bool`：
- `true` = 事件已消费，不再传递
- `false` = 事件未消费，继续传递

未消费的事件可能被父级处理或传递到游戏世界。

### 6. 纯虚函数必须实现

`IScreen` 中的纯虚函数必须由派生类实现：
- `init()`
- `render()`
- `onClick()`
- `onKey()`
- `onClose()`

其他方法有默认实现，可根据需要重写。

### 7. 屏幕尺寸初始化

在 `render()` 前必须调用 `setScreenSize()` 或 `onResize()`，否则居中计算可能不正确。

## 测试用例

当前模块暂无专用单元测试。测试覆盖范围：

| 测试位置 | 测试内容 |
|---------|---------|
| `tests/ui/kagero/template/template_test.cpp` | Kagero UI模板系统的`<screen>`标签解析（非本模块测试） |

**建议添加的测试**：
1. `ScreenType` 枚举值完整性测试
2. `screenTypeToId` / `screenTypeFromId` 双向映射测试
3. `screenTypeFromId` 对无效ID的处理测试
4. `IScreen` 默认方法行为测试

## 扩展指南

### 添加新的屏幕类型

1. 在 `ScreenType` 枚举中添加新类型（在 `Count` 之前）：
```cpp
enum class ScreenType : u8 {
    // ...
    NewScreen,    // 新增
    Count
};
```

2. 在 `ScreenType.cpp` 中添加映射：
```cpp
const std::unordered_map<ScreenType, String> typeToIdMap = {
    // ...
    {ScreenType::NewScreen, "minecraft:new_screen"},
};

const std::unordered_map<String, ScreenType> idToTypeMap = {
    // ...
    {"minecraft:new_screen", ScreenType::NewScreen},
    {"new_screen", ScreenType::NewScreen},  // 可选：添加简写
};
```

3. 创建对应的屏幕实现（在 `client/ui/screen/` 目录下）。

## 相关文件

| 文件 | 说明 |
|------|------|
| `client/ui/screen/ScreenManager.hpp` | 屏幕管理器，管理屏幕栈 |
| `client/ui/screen/AbstractContainerScreen.hpp` | 容器屏幕模板基类 |
| `client/ui/screen/CraftingScreen.hpp` | 工作台屏幕实现 |
| `client/ui/minecraft/widgets/ScreenStackWidget.hpp` | Kagero UI屏幕栈组件 |
| `server/menu/CraftingMenu.hpp` | 服务端工作台菜单（使用ScreenType） |
