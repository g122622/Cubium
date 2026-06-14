# 皮肤缓存管理器 (Skin Cache Manager)

皮肤文件的磁盘缓存管理，包含缓存索引、元数据持久化和默认皮肤提供。

## 目录结构

```
manager/
├── SkinCache.hpp            # 皮肤缓存类（磁盘文件 + 内存索引 + metadata.json 持久化）
├── SkinCache.cpp            # 缓存读写、过期清理、元数据序列化
├── SkinManager.hpp          # 皮肤管理器（主入口，协调缓存、加载、默认皮肤）
├── SkinManager.cpp          # 加载策略（缓存→下载→默认）
├── DefaultSkinProvider.hpp  # 默认皮肤提供者（Steve/Alex）
└── DefaultSkinProvider.cpp  # 基于 UUID 哈希选择默认皮肤
```

## 内部模块关系

```
SkinManager
├── SkinCache ──── metadata.json 持久化 + 磁盘文件缓存
├── DefaultSkinProvider ──── 内置 Steve/Alex 皮肤
└── ISkinLoader（通过 loadSkin 间接使用）
    ├── FileSkinLoader
    └── HttpSkinLoader
```

## 上下游外部依赖关系

### 上游依赖

- `common/skin/core` - SkinTextures、GameProfile、SkinTypes
- `common/skin/loader` - ISkinLoader、SkinLoadResult
- `common/skin/parser` - SkinMetadataParser
- `common/resource` - ResourceLocation
- `nlohmann-json` - 元数据 JSON 序列化
- `spdlog` - 日志

### 下游依赖

- `client/skin/ClientSkinManager` - 继承 SkinManager，添加 GPU 纹理管理

## 容易踩的坑

### 元数据持久化时间戳可移植性

SkinCache 的 `_saveMetadata`/`_loadMetadata` 使用 `file_time_type` 到 `chrono::seconds` 的转换。`file_time_type` 的 epoch 在不同平台上不同（Windows: 1601-01-01, Unix: 1970-01-01）。当前实现直接转换 `time_since_epoch`，跨平台迁移缓存目录时时间戳可能偏差。如需严格跨平台兼容，应改用 `last_write_time - clock::now()` 的相对差值。

### 缓存键哈希

缓存键使用 SHA-1 哈希（通过 FileSkinLoader/HttpSkinLoader 的 `_calculateHash` 生成），而非 Mojang 服务器使用的 SHA-256。目录结构为两级：`skins/<hash前2字符>/<完整hash>`。

### 线程安全

SkinCache 的所有公共方法通过 `m_entriesMutex` 保证线程安全。元数据读写仅在初始化/关闭时进行，不持有条目互斥锁。

### DefaultSkinProvider 尚未加载真实皮肤

`_loadBuiltinSkins()` 当前填充零像素数据，需要后续从资源包加载 Steve/Alex 皮肤纹理。
