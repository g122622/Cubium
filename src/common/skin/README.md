# 皮肤系统 (Skin System)

玩家皮肤系统，负责皮肤数据的存储、加载、缓存和渲染集成。

## 目录结构

```
skin/
├── core/                       # 核心类型定义
│   ├── SkinTypes.hpp           # 皮肤类型枚举、18种默认皮肤变体、SignatureState 签名状态枚举和工具函数
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
│   ├── DefaultSkinProvider.hpp # 默认皮肤提供者（18种默认皮肤）
│   └── DefaultSkinProvider.cpp
├── loader/                     # 加载器
│   ├── SkinLoader.hpp          # 皮肤加载器接口（ISkinLoader）
│   ├── SkinLoader.cpp
│   ├── FileSkinLoader.hpp      # 本地文件加载
│   ├── FileSkinLoader.cpp
│   ├── HttpSkinLoader.hpp      # HTTP下载加载（textures.minecraft.net）
│   └── HttpSkinLoader.cpp
├── parser/                     # 解析器
│   ├── SkinMetadataParser.hpp  # textures属性的Base64 JSON解析和签名验证
│   └── SkinMetadataParser.cpp
└── network/                    # 网络同步
    ├── PlayerSkinInfo.hpp      # 客户端玩家皮肤信息（加载状态、纹理引用、签名安全状态、部件可见性）
    ├── PlayerSkinInfo.cpp
    ├── SkinPackets.hpp         # PlayerListEntry 结构 + PlayerListAction 枚举 + 工厂（作为 IR PlayerInfoUpdate/Remove 的逻辑载荷）
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
└───────────────┘   │ (18种默认皮肤)  │   │ └─ HttpSkinLoader   │
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

## 默认皮肤选择算法

MC 1.21.1 有 **18 种默认皮肤**（9 slim + 9 wide），通过 UUID 哈希选择：

```
索引 0-8:  slim (alex, ari, efe, kai, makena, noor, steve, sunny, zuri)
索引 9-17: wide (alex, ari, efe, kai, makena, noor, steve, sunny, zuri)
```

选择算法与 MC Java 版 `DefaultPlayerSkin.get(UUID)` 一致：

```
index = Math.floorMod(UUID.hashCode(), 18)
```

- `getDefaultSkinVariantForUUID(uuid)` — 返回完整的 `DefaultSkinVariant`（名称、类型、纹理路径）
- `getDefaultSkinTypeForUUID(uuid)` — 委托给上述函数，返回 `SkinType`
- `getCanonicalDefaultSkin()` — 返回 slim/steve（索引 6），即无 UUID 上下文时的规范回退

纹理路径格式：`minecraft:textures/entity/player/{slim|wide}/{name}.png`

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

- `common/core` - 基础类型（Result、Types）
- `common/resource` - ResourceLocation
- `common/network` - 网络包序列化
- `common/util/text` - ITextComponent 文本组件
- `common/util/crypto` - SHA-1 哈希（缓存键生成）
- `common/entity/entities/player` - PlayerModelPart 枚举
- `spdlog` - 日志
- `nlohmann-json` - JSON解析
- `stb_image` / `stb_image_write` - PNG 解码/编码

### 下游依赖（依赖本模块）

- `client/skin/ClientSkinManager` - 客户端皮肤管理器
- `client/network/ClientPlayVisitor` - 处理 IR PlayerInfoUpdate/Remove（玩家列表+皮肤属性）
- `client/application/ClientApplication` - 初始化皮肤系统

## 容易踩的坑

### UUID 字节序

GameProfile 中的 UUID 是 **big-endian**（大端序），与 Java 版一致。但网络传输和部分内部存储可能使用小端序，转换时需特别注意字节序问题。使用 `GameProfile::parseUUID()` 可自动处理带/不带连字符的 UUID 字符串。

### 皮肤类型判断

皮肤类型由 `getDefaultSkinTypeForUUID()` 通过 18 种默认皮肤变体选择算法确定，该算法委托给 `getDefaultSkinVariantForUUID()`，使用 `floorMod(UUID.hashCode(), 18)` 选择索引，再返回对应变体的 `skinType`。**注意**：旧的 `(hashCode & 1)` 二值算法已弃用，它与 18 皮肤算法对同一 UUID 可能返回矛盾的 SkinType。

### displayName JSON 格式

PlayerListEntry 的 displayName 字段存储的是 **ITextComponent 序列化后的 JSON 字符串**，而非原始文本。设置时使用 `setDisplayName(ITextComponent)`，读取时使用 `getDisplayNameAsText()` 解析回 ITextComponent。

### 缓存路径结构

皮肤缓存使用 **两级目录结构**：`skins/<hash前2字符>/<完整hash>`，避免单个目录文件过多。清理缓存时需遍历子目录。

### 缓存哈希算法

皮肤缓存键使用 **SHA-1** 哈希（`util/crypto/Sha1`）计算，生成 40 字符十六进制字符串。Mojang 皮肤服务器的纹理 URL 中包含的哈希为 SHA-256 格式（64 字符），但本地缓存键使用 SHA-1 是为了与 SkinCache 的目录结构兼容。

### 元数据持久化

SkinCache 在初始化时从 `metadata.json` 加载缓存条目信息，在关闭时和析构时保存。元数据包含 hash、location、fileSize、lastAccess、lastModified 字段。如果元数据文件不存在或解析失败，会回退到文件扫描模式。

### 线程安全

- **SkinManager**: 所有公共方法线程安全
- **SkinCache**: 所有公共方法线程安全
- **PlayerSkinInfo**: 加载状态使用 atomic，纹理访问需要外部同步

### 签名验证

- **SignatureState** 枚举定义在 `core/SkinTypes.hpp`（不在 `parser/SkinMetadataParser.hpp`），避免网络层反向依赖解析层
- **getSignatureState()**: 当前因项目未集成 RSA 加密库，有签名的属性降级为 `Unsigned`（而非 `Invalid`），避免误判有效签名导致皮肤不可用
- **verifySignature()**: 基于 `getSignatureState()` 实现，`Unsigned` 和 `Signed` 视为有效，`Invalid` 视为无效
- 签名验证数据是 `property.value` 的原始 ASCII 字节（不是 Base64 解码后内容），与 MC Java 版 authlib `Property.isSignatureValid()` 一致
- 完整的 RSA-SHA1 验证流程已记录在 `SkinMetadataParser.cpp` 的 TODO 注释中

### 异步加载

FileSkinLoader 和 HttpSkinLoader 支持异步加载（`loadAsync`），通过注入的 `UniversalWorkerPool` 实现真正的异步执行。回调在 worker 线程触发，如果回调中需要更新 UI 或渲染资源，需要切换到主线程。未注入线程池时，`loadAsync` 降级为同步执行后立即回调。

## 命名空间

所有类型位于 `mc::skin` 命名空间。
