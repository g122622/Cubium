# 交互类方块实体 (Interactive Block Entities)

此目录包含交互类方块实体的实现。

## 目录结构

```
interactive/
├── EnchantingTableEntity.hpp/cpp  # 附魔台
├── EndGatewayEntity.hpp/cpp       # 末地折跃门
├── DispenserBlockEntity.hpp/cpp    # 发射器方块实体基类
├── DropperBlockEntity.hpp/cpp      # 投掷器方块实体（继承自DispenserBlockEntity）
├── PistonBlockEntity.hpp/cpp       # 活塞实体
├── SignEntity.hpp/cpp              # 告示牌
├── BannerEntity.hpp/cpp            # 旗帜
├── JukeboxEntity.hpp/cpp           # 唱片机
├── LecternEntity.hpp/cpp           # 讲台
└── README.md
```

## 文件详解

### EnchantingTableEntity.hpp/cpp

**职责**：附魔台方块实体。

**主要功能**：
- 附魔界面交互
- 书本动画
- 环绕粒子效果

**附魔力量计算**：

```
附魔台周围2格范围内:
- 水平距离 = 2（曼哈顿距离）
- 垂直距离 = 0 或 1

书架与附魔台之间必须是空气。

每个有效书架增加1点附魔力量，最大15点。
```

### EndGatewayEntity.hpp/cpp

**职责**：末地折跃门方块实体。

**主要功能**：
- 将实体从末地主岛传送到外岛（或返回）
- 传送冷却机制
- 自动寻找或生成出口传送门

**传送机制（MC 1.16.5）**：
- 实体进入折跃门后立即被传送
- 传送冷却 100 tick（触发后）
- 触发后冷却时间 40 tick
- 如果没有出口位置，会自动在约 1024 格外生成新传送门
- 每 2400 tick 自动触发冷却（用于外岛返回的传送门刷新）

**关键方法**：
- `teleportEntity()` - 传送实体
- `setExitPortal()` - 设置出口传送门位置
- `isSpawning()` - 检查是否正在生成
- `isCoolingDown()` - 检查是否在冷却中
- `getSpawnPercent()` - 获取生成进度（用于客户端动画）
- `getCooldownPercent()` - 获取冷却进度（用于客户端动画）

### DispenserBlockEntity.hpp/cpp

**职责**：发射器方块实体基类。

**主要功能**：
- 9格物品存储
- 红石信号触发发射
- 物品行为分发
- 战利品表支持

**MC 1.16.5 对齐**：
- addItemStack 返回 int（槽位索引或 -1），而非剩余物品堆
- getDispenseSlot 使用储水池采样算法确保等概率选择
- 支持战利品表填充（fillWithLoot方法）

### DropperBlockEntity.hpp/cpp

**职责**：投掷器方块实体，继承自 DispenserBlockEntity。

**与发射器的区别**：
- 投掷器只投掷物品，没有特殊行为
- 发射器对特定物品有特殊行为（如箭矢发射、火焰球等）
- 投掷器会尝试向相邻容器输出物品
- 显示名称不同：`container.dropper` vs `container.dispenser`

**关键方法**：
- `clone()` - 克隆投掷器实体
- 继承自 DispenserBlockEntity 的所有功能

### PistonBlockEntity.hpp/cpp

**职责**：活塞方块实体。

**主要功能**：
- 方块移动动画
- 推/拉逻辑
- 活塞头状态管理

**活塞移动流程**：

```
1. 红石信号触发 → PistonBlock.neighborChanged()
2. 计算推动链 → PistonBlock.extend() 或 retract()
3. 创建移动活塞方块 → 创建 PistonBlockEntity
4. 每 tick 更新进度 → tick() 中 progress += 0.5
5. 动画完成后 → 替换为最终方块
```

### SignEntity.hpp/cpp

**职责**：告示牌方块实体，存储文本并处理点击事件。

**主要功能**：
- 4行文本，每行最多15个字符
- 支持彩色文本（使用§代码）
- 支持富文本样式和点击事件
- 可编辑（右键点击）
- 木告示牌和荧石告示牌

**点击事件处理** (MC 1.16.5 参考: SignTileEntity.executeCommand()):
- `RunCommand`: 服务端执行命令，权限级别为2（相当于OP）
- `SuggestCommand`: 客户端功能，将命令填入聊天输入框
- `OpenUrl`: 客户端功能，打开URL链接
- `CopyToClipboard`: 客户端功能，复制文本到剪贴板
- `OpenFile`: 出于安全原因不自动执行

**关键方法**：
- `getLine(i32 line)` - 获取指定行文本组件
- `setLine(i32 line, unique_ptr<ITextComponent> text)` - 设置文本组件
- `executeCommand(IWorld& world, Player& player)` - 执行点击事件命令
- `isEditable()` - 检查是否可编辑
- `setTextColor()` / `getTextColor()` - 文本颜色
- `isGlowing()` / `setGlowing()` - 发光状态

### BannerEntity.hpp/cpp

**职责**：旗帜方块实体，存储图案。

**主要功能**：
- 支持最多6层图案叠加
- 每层图案有类型和颜色
- 墙挂式和站立式两种形态
- 16种底色可选

**关键方法**：
- `addPattern(const BannerPattern& pattern)` - 添加图案
- `removeTopPattern()` - 移除顶层图案
- `clearPatterns()` - 清空图案
- `getBaseColor()` / `setBaseColor()` - 底色

### JukeboxEntity.hpp/cpp

**职责**：唱片机方块实体，播放音乐唱片。

**主要功能**：
- 1个槽位存放唱片
- 播放音乐时发射红石信号
- 可以被漏斗提取唱片

**关键方法**：
- `getRecord()` / `setRecord()` - 获取/设置唱片
- `hasRecord()` - 检查是否有唱片
- `startPlaying()` / `stopPlaying()` - 播放控制
- `isPlaying()` - 是否正在播放
- `getComparatorSignal()` - 红石比较器信号

### LecternEntity.hpp/cpp

**职责**：讲台方块实体，展示和阅读书本。

**主要功能**：
- 1个槽位存放书
- 支持书与笔、成书、附魔书
- 红石比较器输出当前页数
- 右键翻页

**关键方法**：
- `getBook()` / `setBook()` - 获取/设置书本
- `hasBook()` - 检查是否有书
- `getPage()` / `setPage()` - 当前页码
- `nextPage()` / `prevPage()` - 翻页
- `getTotalPages()` - 总页数
- `getComparatorSignal()` - 红石比较器信号

## 类继承关系

```
BlockEntity (基类)
│
├── EnchantingTableEntity (附魔台)
│
├── EndGatewayEntity (末地折跃门)
│
├── DispenserBlockEntity (发射器/投掷器)
│
├── PistonBlockEntity (活塞)
│
├── SignEntity (告示牌)
│
├── BannerEntity (旗帜)
│
├── JukeboxEntity (唱片机)
│   └── ContainerBlockEntity (容器基类)
│
└── LecternEntity (讲台)
```

## 依赖项

### 内部依赖
- `world/blockentity/BlockEntity.hpp` - 方块实体基类
- `world/blockentity/ContainerBlockEntity.hpp` - 容器基类
- `world/blockentity/core/SimpleInventory.hpp` - 简单背包
- `entity/inventory/IInventory.hpp` - 背包接口

### 外部依赖
- `<memory>` - 智能指针
- `<array>` - 静态数组
- `<string>` - 字符串

## 使用方法

### 创建告示牌

```cpp
// 创建告示牌
auto sign = std::make_unique<SignEntity>(BlockPos(0, 0, 0));

// 设置文本
sign->setLine(0, "Hello World!");
sign->setLine(1, "Line 2");
sign->setLine(2, "Line 3");
sign->setLine(3, "Line 4");

// 设置颜色
sign->setTextColor(0); // 黑色

// 设置发光（苂石告示牌）
sign->setGlowing(true);
```

### 创建旗帜

```cpp
// 创建旗帜
auto banner = std::make_unique<BannerEntity>(BlockPos(0, 0, 0));

// 设置底色
banner->setBaseColor(15); // 黑色

// 添加图案
BannerPattern pattern;
pattern.pattern = "stripe_bottom";
pattern.color = 0; // 白色
banner->addPattern(pattern);
```

### 创建唱片机

```cpp
// 创建唱片机
auto jukebox = std::make_unique<JukeboxEntity>(BlockPos(0, 0, 0));

// 放入唱片
jukebox->setRecord(ItemStack(Items::MUSIC_DISC_13, 1));

// 开始播放
jukebox->startPlaying(world);

// 检查状态
if (jukebox->isPlaying()) {
    // 正在播放
}
```

### 创建讲台

```cpp
// 创建讲台
auto lectern = std::make_unique<LecternEntity>(BlockPos(0, 0, 0));

// 放入书本
lectern->setBook(ItemStack(Items::WRITTEN_BOOK, 1));

// 翻页
lectern->nextPage();

// 获取红石信号
i32 signal = lectern->getComparatorSignal();
```

## 实现状态

### 附魔台 (EnchantingTableEntity)

| 功能 | 状态 |
|------|------|
| 附魔力量计算 | ✅ 完成 |
| 书架检测 | ✅ 完成（待 VanillaBlocks::BOOKSHELF） |
| 自定义名称 | ✅ 完成 |
| 序列化/反序列化 | ✅ 完成 |
| 书本动画 | ✅ 框架完成 |
| 玩家靠近检测 | ⏳ TODO |
| GUI打开 | ⏳ TODO |

### 活塞 (PistonBlockEntity)

| 功能 | 状态 |
|------|------|
| 方块实体基础结构 | ✅ 完成 |
| 动画进度管理 | ✅ 完成 |
| 序列化/反序列化 | ✅ 完成 |
| 渲染偏移计算 | ✅ 完成 |
| 实体推动 | ⏳ 框架完成（待实体系统完善） |
| 方块状态保存/恢复 | ⏳ TODO（待 BlockState 序列化） |
| 蜂蜜块特殊处理 | ⏳ TODO |

### 告示牌 (SignEntity)

| 功能 | 状态 |
|------|------|
| 文本存储 | ✅ 完成 |
| 颜色支持 | ✅ 完成 |
| 发光状态 | ✅ 完成 |
| 序列化/反序列化 | ✅ 完成 |
| 编辑状态管理 | ✅ 完成 |
| 点击命令执行 | ✅ 完成 |
| RunCommand 服务端执行 | ✅ 完成 |
| SuggestCommand/OpenUrl/CopyToClipboard | ✅ 完成（客户端功能标记） |

### 旗帜 (BannerEntity)

| 功能 | 状态 |
|------|------|
| 图案存储 | ✅ 完成 |
| 底色支持 | ✅ 完成 |
| 序列化/反序列化 | ✅ 完成 |

### 唱片机 (JukeboxEntity)

| 功能 | 状态 |
|------|------|
| 唱片存储 | ✅ 完成 |
| 播放状态 | ✅ 完成 |
| 红石信号 | ✅ 完成 |
| 序列化/反序列化 | ✅ 完成 |
| 音乐播放 | ⏳ TODO |

### 讲台 (LecternEntity)

| 功能 | 状态 |
|------|------|
| 书本存储 | ✅ 完成 |
| 翻页功能 | ✅ 完成 |
| 红石信号 | ✅ 完成 |
| 序列化/反序列化 | ✅ 完成 |

## 测试用例

测试文件位于 `tests/common/world/blockentity/`：

- `EnchantingTableEntityTest.cpp` - 附魔台测试
- `PistonBlockEntityTest.cpp` - 活塞测试
- `SignEntityTest.cpp` - 告示牌基础功能测试（文本存储、序列化、克隆）
- `SignEntityCommandTest.cpp` - 告示牌点击命令执行测试（RunCommand、客户端功能标记）
- `BannerEntityTest.cpp` - 旗帜测试
- `JukeboxEntityTest.cpp` - 唱片机测试
- `LecternEntityTest.cpp` - 讲台测试

告示牌方块交互测试位于 `tests/common/world/block/blocks/SignBlockTest.cpp`：
- AbstractSignBlock::onBlockActivated 触发 SignEntity::executeCommand
- 无方块实体时的行为
- 非 SignEntity 方块实体的行为
- WoodType 属性测试
- StandingSignBlock/WallSignBlock 状态属性测试
