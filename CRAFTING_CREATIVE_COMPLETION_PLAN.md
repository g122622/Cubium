# CraftingScreen 与创造模式背包补全计划

## 目标

以 Java 1.16.5 为对照，补全当前 `CraftingScreen`、玩家背包屏幕和创造模式物品库的交互闭环，做到：

- 生存模式下按 `E` 打开完整的玩家背包/2x2 合成界面
- 创造模式下按 `E` 打开完整的创造模式物品库界面
- 合成、拖拽、丢弃、快捷栏交换、创造模式选取/删除物品都能真正落到服务端
- 目录结构清晰，公共逻辑可复用，未来便于继续扩展其他容器屏幕

## 现状结论

基于当前仓库代码和本地 Java 1.16.5 源码，问题不是单一 UI 缺口，而是整条链路都不完整。

- `src/client/application/ClientApplication.cpp` 里按 `E` 目前无条件创建 `InventoryCraftingScreen`，没有像 Java 版那样根据创造模式切换到 `CreativeScreen`。
- `src/client/ui/screen/CraftingScreen.cpp` 只有渲染骨架，`renderTooltip()` 仍是 TODO，`onSlotClick()` 只保留了注释，没有实现合成结果、快捷操作、拖拽等真实交互。
- `src/client/ui/screen/AbstractContainerScreen.hpp` 里的默认点击逻辑只区分了非常粗糙的 `Pick/Pickup`，不够支撑 Java 版容器交互模型。
- `src/common/network/packet/ContainerPacketHandler.cpp` 的 `handleContainerClick()` 目前是空壳，容器点击没有真正落到服务端权威逻辑。
- `src/server/interaction/InventoryManager.cpp` 只负责服务端->客户端同步，没有创造模式所需的客户端->服务端库存修改通路。
- `src/common/screen/ScreenType.hpp` 已经预留了 `CreativeInventory`，但 C++ 侧没有对应实现。

## Java 对照

本次补全建议直接对照以下 Java 源码，不照搬架构，只借鉴交互与状态流转：

- `D:\Minecraft\MC研究\Minecraft1.16.5源码\net\minecraft\client\gui\screen\inventory\CraftingScreen.java`
- `D:\Minecraft\MC研究\Minecraft1.16.5源码\net\minecraft\client\gui\screen\inventory\InventoryScreen.java`
- `D:\Minecraft\MC研究\Minecraft1.16.5源码\net\minecraft\client\gui\screen\inventory\CreativeScreen.java`
- `D:\Minecraft\MC研究\Minecraft1.16.5源码\net\minecraft\client\gui\screen\inventory\CreativeCraftingListener.java`
- `D:\Minecraft\MC研究\Minecraft1.16.5源码\net\minecraft\client\gui\screen\inventory\ContainerScreen.java`

## 推荐目录结构

建议把现在堆在 `src/client/ui/screen` 下的实现拆成按功能分层的子目录，不要继续堆平铺文件。

```text
src/client/ui/screen/
  container/
    common/
      ContainerScreenInput.hpp/cpp
      ContainerSlotLayout.hpp/cpp
    crafting/
      CraftingScreen.hpp/cpp
      InventoryCraftingScreen.hpp/cpp
    creative/
      CreativeScreen.hpp/cpp
      CreativeInventoryModel.hpp/cpp
      CreativeInventoryListener.hpp/cpp
```

补充建议：

- `container/common` 放通用点击翻译、槽位布局、tooltip、拖拽分发。
- `container/crafting` 放工作台和玩家背包合成界面。
- `container/creative` 放创造模式物品库、搜索、分页、垃圾槽、快捷栏快照。
- 先保留现有 `client/ui/screen` 入口，后续再考虑向 `client/ui/minecraft/screens` 的新 UI 栈收敛，不要一次性双线重写。

## 分阶段补全

### 第一阶段：先把普通合成屏幕补到可用

- [ ] 把 `CraftingScreen` 与 `InventoryCraftingScreen` 拆清楚，避免一个文件塞两个概念。
- [ ] 补全 `renderTooltip()`，至少显示物品名和基础提示。
- [ ] 补全 `onSlotClick()` 的结果槽特殊处理，保证结果槽点击会走合成取出逻辑，而不是只做渲染层空转。
- [ ] 补全 result/grid/armor/offhand 的点击约束，避免错误槽位被当成普通槽处理。
- [ ] 把合成结果同步与原料消耗逻辑收口到明确的方法里，不要散落在屏幕里。
- [ ] 把 recipe book 的入口纳入计划，如果当前项目暂时没有完整 recipe book，就至少预留接口和布局位置。

### 第二阶段：重做容器输入模型

- [ ] 扩展 `AbstractContainerScreen`，让它能表达 Java 版容器交互，而不是只有简单左/右键点击。
- [ ] 统一支持 `Pick`、`PickAll`、`PickSome`、`Place`、`PlaceSome`、`Throw`、`ThrowAll`、`QuickMove`、`QuickCraft`、`Clone`、`Swap`。
- [ ] 补齐拖拽分发、快捷栏数字键交换、丢弃物品、双击合并等输入路径。
- [ ] 把槽位命中、空白区域点击、关闭窗口逻辑整理成独立 helper，降低后续 creative screen 复制代码量。
- [ ] 删除/收敛现有过度防御式空指针检查，只保留真正来自外部输入的边界保护。

### 第三阶段：补齐服务端容器点击处理

- [ ] 完整实现 `src/common/network/packet/ContainerPacketHandler.cpp` 的 `handleContainerClick()`。
- [ ] 让服务端真正调用当前打开的 menu，而不是只做 packet 解码。
- [ ] 把 `ClickAction` 与 `ClickType` 的映射整理成单独表，避免散落在客户端和服务端两边各写一份。
- [ ] 补上容器内容回填和单槽更新的同步路径，保证客户端屏幕和服务端 menu 状态一致。
- [ ] 为工作台、箱子、熔炉等现有容器复用这条通路，不要只为 crafting 特判。

### 第四阶段：实现创造模式物品库

- [ ] 新建 `CreativeScreen`，不要把创造模式硬塞进普通合成屏幕。
- [ ] 复刻 Java 版关键能力：分类页、搜索框、滚动条、分页、垃圾槽、临时展示槽、热bar 快照。
- [ ] 让创造模式界面使用本地模型而不是服务端容器菜单，行为上对齐 Java 版。
- [ ] 新建 `CreativeInventoryModel`，负责物品列表构建、搜索过滤、分页、滚动窗口与槽位映射。
- [ ] 新建 `CreativeInventoryListener` 或同等角色，专门处理创造模式下的背包同步通知。
- [ ] 支持从物品库直接选取物品、复制整组、删除物品、拖拽放置、快捷栏回填。
- [ ] 处理 `CreativeScreen` 中的特殊槽位语义，尤其是垃圾槽和暂存槽，不要混进普通容器逻辑。

### 第五阶段：补齐创造模式的同步协议

- [ ] 新增一个明确的客户端->服务端创造模式库存修改通路，不能继续依赖纯 `PlayerInventoryPacket` 的单向同步。
- [ ] 设计并实现单独的 packet，或者明确扩展现有容器 packet 语义，支持创造模式选取、删除、快捷栏覆盖等动作。
- [ ] 服务端需要校验当前玩家是否处于创造模式，再决定是否接受这些操作。
- [ ] 服务端接受修改后，要同步玩家背包回客户端，避免客户端本地状态和服务端权威状态分裂。
- [ ] 如果某些操作只在客户端生效，也要显式记录原因，不要默默吞掉。

### 第六阶段：按游戏模式切换入口

- [ ] 修改 `src/client/application/ClientApplication.cpp` 的 `E` 键路径，依据 `PlayerAbilities.creativeMode` / `Player::gameMode()` 切换打开不同 screen。
- [ ] 创造模式下，优先打开 `CreativeScreen`。
- [ ] 生存模式下，打开 `InventoryCraftingScreen`。
- [ ] 游戏模式变化后如果当前屏幕不匹配，要做收口切换，避免界面和模式状态不一致。
- [ ] 保持 `ScreenType::CreativeInventory` 的路由完整，不要只是 enum 留空。

## 代码规范要求

- 所有新增和修改的方法前都要写简体中文 Doxygen 注释，说明用法、前置条件和容易踩坑的地方。
- 新增代码要使用明确命名空间隔离，建议至少做到 `mc::client::ui::screen::container::crafting` 与 `mc::client::ui::screen::container::creative` 这种粒度。
- 不要把所有 screen 文件继续堆在单目录，必须按功能拆到子目录。
- 只在真正的外部输入边界做防御，不要用大量 `nullptr` 检查掩盖结构性缺陷。
- 编译 warning 要清零，尤其是 `unused parameter`、`signed/unsigned`、`missing override`、`dead branch`、`temporary copy` 这类常见问题。

## 测试计划

### 单测

- [ ] `CraftingInventory` 的槽位映射、清空、边界、回调保持覆盖。
- [ ] `CraftingMenu` 的结果槽取出、原料消耗、shift-click 移动、距离有效性继续补强。
- [ ] 新增创造模式模型单测，覆盖搜索过滤、分页、滚动、垃圾槽行为。
- [ ] 新增 packet 单测，覆盖创造模式库存修改包的序列化与反序列化。
- [ ] 新增容器点击处理单测，覆盖普通点击、快捷栏交换、丢弃、创造模式选取等关键分支。

### 集成验证

- [ ] 生存模式按 `E` 打开普通背包/合成界面。
- [ ] 创造模式按 `E` 打开创造模式物品库。
- [ ] 在创造模式里可从物品库选取物品、删除物品、覆盖快捷栏。
- [ ] 合成结果点击会正确取出并消耗原料。
- [ ] 关闭屏幕后，服务端与客户端背包状态一致。

## 验收标准

- `E` 键行为与 Java 1.16.5 对齐。
- 创造模式物品库具备可用的选取、删除、搜索、分页、滚动能力。
- 合成屏幕不再是“只会画槽位”的半成品。
- 容器点击有完整客户端->服务端闭环。
- 新增代码都有 doc 注释、命名空间清晰、目录层次清楚。
- 相关编译 warning 清零。

## 实施顺序建议

1. 先补 `AbstractContainerScreen` 和 `ContainerPacketHandler`，打通最底层交互。
2. 再修 `CraftingScreen` / `InventoryCraftingScreen`，让普通合成可用。
3. 然后新增 `CreativeScreen` 与创造模式同步通路。
4. 最后回到 `ClientApplication.cpp` 接入按键分流和模式切换。

