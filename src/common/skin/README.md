# 皮肤系统 (Skin System)

玩家皮肤系统，负责皮肤数据的存储、加载、缓存和渲染集成。

## 目录结构

```
skin/
├── core/                       # 核心类型定义
│   ├── SkinTypes.hpp           # 皮肤类型枚举和工具函数
│   ├── SkinTypes.cpp
│   ├── SkinTextures.hpp        # 皮肤纹理数据结构
│   ├── SkinTextures.cpp
│   ├── GameProfile.hpp         # 玩家档案（UUID、名称、属性）
│   └── GameProfile.cpp
├── manager/                    # 管理器
│   ├── SkinManager.hpp         # 皮肤管理器（主入口）
│   ├── SkinManager.cpp
│   ├── SkinCache.hpp           # 皮肤缓存（磁盘+内存）
│   ├── SkinCache.cpp
│   ├── DefaultSkinProvider.hpp # 默认皮肤提供者（Steve/Alex）
│   └── DefaultSkinProvider.cpp
├── loader/                     # 加载器
│   ├── SkinLoader.hpp          # 皮肤加载器接口
│   ├── SkinLoader.cpp
│   ├── FileSkinLoader.hpp      # 本地文件加载
│   ├── FileSkinLoader.cpp
│   ├── HttpSkinLoader.hpp      # HTTP下载加载
│   └── HttpSkinLoader.cpp
├── parser/                     # 解析器
│   ├── SkinMetadataParser.hpp  # 皮肤元数据解析
│   └── SkinMetadataParser.cpp
└── network/                    # 网络同步
    ├── PlayerSkinInfo.hpp      # 客户端玩家皮肤信息
    ├── PlayerSkinInfo.cpp
    ├── SkinPackets.hpp         # 皮肤相关网络包
    └── SkinPackets.cpp
```

## 核心组件

### SkinTypes

皮肤类型定义：
- `SkinType` 枚举：`Default`（Steve宽手臂）、`Slim`（Alex窄手臂）
- `parseSkinType()` - 解析字符串为皮肤类型
- `getDefaultSkinTypeForUUID()` - 根据UUID确定默认皮肤类型

### GameProfile

玩家游戏档案，存储：
- UUID（16字节）
- 玩家名称
- 属性列表（包含 textures 属性）

### SkinTextures

皮肤纹理集合，管理：
- 皮肤纹理位置
- 披风纹理位置
- 鞘翅纹理位置
- 皮肤类型

### SkinManager

核心管理器，职责：
- 玩家皮肤信息缓存
- 异步皮肤加载
- 默认皮肤提供
- 纹理生命周期管理

### SkinCache

磁盘缓存管理：
- 皮肤文件缓存到磁盘
- 哈希索引
- 过期清理

### PlayerSkinInfo

客户端玩家皮肤信息：
- 加载状态追踪
- 纹理资源引用
- 部件可见性

## 数据流

```
服务端 PlayerListItemPacket
         │
         ▼
客户端解析 GameProfile
         │
         ▼
提取 textures 属性（Base64 JSON）
         │
         ▼
解析为 SkinTextures（URL、类型）
         │
         ├─→ 缓存命中 → 直接加载
         │
         └─→ 缓存未命中 → 异步下载 → 缓存 → 加载
                                  │
                                  ▼
                         上传到 GPU 纹理图集
                                  │
                                  ▼
                         通知 PlayerRenderer 更新
```

## 使用示例

```cpp
#include "common/skin/manager/SkinManager.hpp"
#include "common/skin/network/PlayerSkinInfo.hpp"

// 创建皮肤管理器
mc::skin::SkinManager skinManager("./cache/skins");
skinManager.initialize();

// 加载玩家皮肤
mc::skin::GameProfile profile(uuid, "PlayerName");
profile.addProperty({"textures", base64Data});

auto skinInfo = skinManager.getOrCreatePlayerInfo(profile);

// 获取皮肤纹理
auto skinLocation = skinInfo->getSkinLocation();
auto skinType = skinInfo->getSkinType();

// 渲染时使用
if (skinInfo->isLoaded()) {
    renderer.setSkinTexture(skinRegion);
}
```

## 命名空间

所有类型位于 `mc::skin` 命名空间。

## 依赖

- `common/core` - 基础类型
- `common/resource` - ResourceLocation
- `common/network` - 网络包序列化
- `spdlog` - 日志
- `nlohmann-json` - JSON解析

## 测试

- `tests/common/skin/test_skin_types.cpp`
- `tests/common/skin/test_game_profile.cpp`
- `tests/common/skin/test_skin_cache.cpp`
- `tests/common/skin/test_skin_packets.cpp`
- `tests/common/skin/test_skin_manager.cpp`
