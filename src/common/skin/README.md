# 皮肤系统 (Skin System)

玩家皮肤系统，负责皮肤数据的存储、加载、缓存和渲染集成。

## 目录结构

```
skin/
├── core/                       # 核心类型定义
│   ├── SkinTypes.hpp           # 皮肤类型枚举（Default/Slim）和工具函数
│   ├── SkinTypes.cpp
│   ├── SkinTextures.hpp        # 皮肤纹理数据结构（皮肤/披风/鞘翅URL和位置）
│   ├── SkinTextures.cpp
│   ├── GameProfile.hpp         # 玩家档案（UUID、名称、textures属性）
│   └── GameProfile.cpp
├── manager/                    # 管理器
│   ├── SkinManager.hpp         # 皮肤管理器（主入口，协调缓存、加载、默认皮肤）
│   ├── SkinManager.cpp
│   ├── SkinCache.hpp           # 皮肤缓存（磁盘+内存索引）
│   ├── SkinCache.cpp
│   ├── DefaultSkinProvider.hpp # 默认皮肤提供者（Steve/Alex）
│   └── DefaultSkinProvider.cpp
├── loader/                     # 加载器
│   ├── SkinLoader.hpp          # 皮肤加载器接口（ISkinLoader）
│   ├── SkinLoader.cpp
│   ├── FileSkinLoader.hpp      # 本地文件加载
│   ├── FileSkinLoader.cpp
│   ├── HttpSkinLoader.hpp      # HTTP下载加载（textures.minecraft.net）
│   └── HttpSkinLoader.cpp
├── parser/                     # 解析器
│   ├── SkinMetadataParser.hpp  # textures属性的Base64 JSON解析
│   └── SkinMetadataParser.cpp
└── network/                    # 网络同步
    ├── PlayerSkinInfo.hpp      # 客户端玩家皮肤信息（加载状态、纹理引用、部件可见性）
    ├── PlayerSkinInfo.cpp
    ├── SkinPackets.hpp         # PlayerListItemPacket（玩家列表+皮肤属性）
    └── SkinPackets.cpp
```

## 内部模块关系

```
                    ┌─────────────────┐
                    │  SkinManager    │ ◄── 主入口，协调各组件
                    └────────┬────────┘
                             │
        ┌────────────────────┼────────────────────┐
        ▼                    ▼                    ▼
┌───────────────┐   ┌────────────────┐   ┌─────────────────────┐
│  SkinCache    │   │ DefaultSkin    │   │ ISkinLoader         │
│  (磁盘缓存)    │   │ Provider       │   │ ├─ FileSkinLoader   │
└───────────────┘   │ (Steve/Alex)   │   │ └─ HttpSkinLoader   │
                    └────────────────┘   └─────────────────────┘
                             │                    │
                             ▼                    ▼
                    ┌─────────────────────────────────┐
                    │ SkinMetadataParser              │
                    │ (解析textures属性的Base64 JSON)  │
                    └─────────────────────────────────┘
                             │
                             ▼
                    ┌─────────────────────────────────┐
                    │ PlayerSkinInfo                  │
                    │ (客户端皮肤状态、纹理引用)        │
                    └─────────────────────────────────┘
```

- **core**: 基础类型定义，被所有其他模块依赖
- **manager**: 核心管理逻辑，依赖 core、loader、parser
- **loader**: 加载器实现，依赖 core
- **parser**: 解析器，依赖 core
- **network**: 网络同步类型，依赖 core

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

- `common/core` - 基础类型（Result、Types）
- `common/resource` - ResourceLocation
- `common/network` - 网络包序列化
- `common/util/text` - ITextComponent 文本组件
- `common/entity/entities/player` - PlayerModelPart 枚举
- `spdlog` - 日志
- `nlohmann-json` - JSON解析

### 下游依赖（依赖本模块）

- `client/skin/ClientSkinManager` - 客户端皮肤管理器
- `client/network/NetworkClient` - 处理玩家列表包
- `client/application/ClientApplication` - 初始化皮肤系统

## 容易踩的坑

### UUID 字节序

GameProfile 中的 UUID 是 **big-endian**（大端序），与 Java 版一致。但网络传输和部分内部存储可能使用小端序，转换时需特别注意字节序问题。使用 `GameProfile::parseUUID()` 可自动处理带/不带连字符的 UUID 字符串。

### 皮肤类型判断

皮肤类型由 textures.minecraft.net URL 中的哈希决定，或由 `getDefaultSkinTypeForUUID()` 根据 UUID 哈希最低位判断。Steve/Alex 的选择逻辑是 `(hashCode & 1) == 1 → Alex`。

### displayName JSON 格式

PlayerListEntry 的 displayName 字段存储的是 **ITextComponent 序列化后的 JSON 字符串**，而非原始文本。设置时使用 `setDisplayName(ITextComponent)`，读取时使用 `getDisplayNameAsText()` 解析回 ITextComponent。

### 缓存路径结构

皮肤缓存使用 **两级目录结构**：`skins/<hash前2字符>/<完整hash>`，避免单个目录文件过多。清理缓存时需遍历子目录。

### 线程安全

- **SkinManager**: 所有公共方法线程安全
- **SkinCache**: 所有公共方法线程安全
- **PlayerSkinInfo**: 加载状态使用 atomic，纹理访问需要外部同步

### 异步加载

HttpSkinLoader 使用异步下载，回调可能在非主线程执行。如果回调中需要更新 UI 或渲染资源，需要切换到主线程。

## 命名空间

所有类型位于 `mc::skin` 命名空间。
