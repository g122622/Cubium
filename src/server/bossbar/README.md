# BossBar 模块

Boss 栏系统，用于创建和管理自定义 Boss 血量条。

## 目录结构

```
bossbar/
├── BossInfo.hpp/cpp               # Boss 信息基类（颜色、样式枚举）
├── ServerBossInfo.hpp/cpp         # 服务端 Boss 信息（玩家管理）
├── CustomServerBossInfo.hpp/cpp   # 自定义服务端 Boss 信息（/bossbar 命令创建）
├── CustomServerBossInfoManager.hpp/cpp  # Boss 栏管理器
└── README.md
```

## 文件详解

### BossInfo.hpp/cpp

**职责**：定义 Boss 栏的核心属性和枚举类型。

**主要内容**：
- `BossInfoColor` 枚举：Boss 栏颜色（Pink, Blue, Red, Green, Yellow, Purple, White）
- `BossInfoOverlay` 枚举：Boss 栏样式（Progress, Notched6, Notched10, Notched12, Notched20）
- `BossInfo` 类：
  - UUID 唯一标识
  - 显示名称（ITextComponent）
  - 生命值百分比（0.0 ~ 1.0）
  - 颜色和样式
  - 标志位（变暗天空、播放 Boss 音乐、创建迷雾、可见性）

### ServerBossInfo.hpp/cpp

**职责**：扩展 BossInfo，添加服务端玩家管理功能。

**主要内容**：
- `ServerBossInfo` 类继承自 `BossInfo`
- 玩家可见性管理：
  - `addPlayer()` - 添加玩家到可见列表
  - `removePlayer()` - 从可见列表移除玩家
  - `removeAllPlayers()` - 清空可见列表
- 属性变更通知：
  - 覆写 `setName()`, `setPercent()`, `setColor()` 等方法
  - 属性变更时广播更新给可见玩家

### CustomServerBossInfo.hpp/cpp

**职责**：用于 /bossbar 命令创建的自定义 Boss 栏。

**主要内容**：
- `CustomServerBossInfo` 类继承自 `ServerBossInfo`
- 资源位置 ID（如 `minecraft:my_bossbar`）
- 数值管理：
  - `value()` / `setValue()` - 当前值
  - `max()` / `setMax()` - 最大值
  - 自动计算百分比
- 持久化：
  - `toNbt()` / `fromNbt()` - NBT 序列化
  - 玩家 UUID 集合（重连恢复）
- 玩家事件：
  - `onPlayerLogin()` - 玩家登录时恢复可见性
  - `onPlayerLogout()` - 玩家登出时清理

### CustomServerBossInfoManager.hpp/cpp

**职责**：管理所有自定义 Boss 栏。

**主要内容**：
- Boss 栏生命周期管理：
  - `create()` - 创建新 Boss 栏
  - `add()` - 添加到管理器
  - `remove()` - 移除 Boss 栏
  - `get()` - 通过 ID 获取
- 查询：
  - `getIds()` - 获取所有 ID
  - `getBossBars()` - 获取所有 Boss 栏
- 玩家事件：
  - `onPlayerLogin()` - 玩家登录
  - `onPlayerLogout()` - 玩家登出
- 网络同步：
  - `sendAddPacket()` - 发送添加包
  - `sendRemovePacket()` - 发送移除包
  - `broadcastUpdate()` - 广播更新
- 持久化：
  - `toNbt()` / `fromNbt()` - NBT 序列化

## 类继承关系

```
BossInfo (基类)
    │
    └── ServerBossInfo (服务端扩展)
            │
            └── CustomServerBossInfo (自定义 Boss 栏)
                    │
                    └── 由 CustomServerBossInfoManager 管理
```

## 模块职责

### 整体职责

1. **Boss 栏创建**：支持通过命令创建自定义 Boss 栏
2. **属性管理**：管理名称、颜色、样式、数值、可见性等属性
3. **玩家可见性**：管理哪些玩家可以看到 Boss 栏
4. **持久化存储**：Boss 栏数据可保存和恢复
5. **网络同步**：将 Boss 栏状态同步给客户端

### 输入

- 命令创建请求（/bossbar add）
- 属性修改请求（/bossbar set）
- 玩家登录/登出事件

### 输出

- Boss 栏状态同步包
- 持久化数据（NBT）

## 依赖项

### 外部依赖
- `<memory>` - 智能指针
- `<set>`, `<vector>`, `<unordered_map>` - 容器

### 内部依赖
- `common/core/Types.hpp` - 基础类型
- `common/resource/ResourceLocation.hpp` - 资源位置
- `common/util/text/ITextComponent.hpp` - 文本组件
- `common/util/nbt/Nbt.hpp` - NBT 序列化
- `server/application/IServer.hpp` - 服务器接口
- `server/player/ServerPlayer.hpp` - 服务端玩家

## 使用方法

### 创建 Boss 栏

```cpp
#include "server/bossbar/CustomServerBossInfoManager.hpp"
#include "common/util/text/StringTextComponent.hpp"

// 创建管理器
mc::server::CustomServerBossInfoManager manager(server);

// 创建 Boss 栏
auto name = std::make_unique<mc::text::StringTextComponent>("My Boss Bar");
auto bossInfo = manager.create(
    mc::ResourceLocation("minecraft:my_bossbar"),
    std::move(name)
);

// 设置属性
bossInfo->setColor(mc::server::BossInfoColor::Red);
bossInfo->setOverlay(mc::server::BossInfoOverlay::Notched10);
bossInfo->setValue(50);
bossInfo->setMax(100);
```

### 玩家可见性

```cpp
// 添加玩家
bossInfo->addPlayer(player);

// 移除玩家
bossInfo->removePlayer(player);

// 设置玩家列表
std::vector<mc::ServerPlayer*> players = {player1, player2};
bossInfo->setPlayers(players);
```

### 持久化

```cpp
// 保存
nbt::tags::compound_tag data = manager.toNbt();

// 加载
manager.fromNbt(data);
```

## 容易踩的坑

### 1. UUID 生成

当前实现使用资源位置 ID 的哈希值作为 UUID。对于生产环境，应考虑使用真正的 UUID 生成器。

```cpp
// 当前实现
u64 uuid = std::hash<std::string>{}(id.toString());

// 更好的实现（如果需要真正的 UUID）
// u64 uuid = generateRandomUuid();
```

### 2. 网络包尚未实现

`sendAddPacket()`, `sendRemovePacket()`, `broadcastUpdate()` 方法当前仅标记数据为脏。
需要实现 `BossInfoPacket` 后才能进行真正的网络同步。

### 3. 玩家登出处理

`onPlayerLogout()` 不发送网络包，因为玩家已经断开连接。只清理内存中的可见性状态。

### 4. 线程安全

`CustomServerBossInfoManager` 不是线程安全的。如果需要在多线程环境使用，需要添加互斥锁保护。

## 测试用例

测试文件位于 `tests/server/bossbar/` 目录：

| 文件 | 测试内容 |
|------|----------|
| `BossInfoTest.cpp` | 颜色/样式枚举、属性设置 |
| `CustomServerBossInfoTest.cpp` | 数值管理、持久化、玩家管理 |
| `CustomServerBossInfoManagerTest.cpp` | 创建/删除/查询、持久化 |

运行测试：
```powershell
./build/bin/Release/mc_tests.exe --gtest_filter="BossBar*"
```

## 参考

- MC 1.16.5 `net.minecraft.world.BossInfo`
- MC 1.16.5 `net.minecraft.world.server.ServerBossInfo`
- MC 1.16.5 `net.minecraft.server.CustomServerBossInfo`
- MC 1.16.5 `net.minecraft.server.CustomServerBossInfoManager`
- MC 1.16.5 `net.minecraft.command.impl.BossBarCommand`
