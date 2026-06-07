# 客户端皮肤系统 (Client Skin System)

客户端皮肤系统，负责 GPU 纹理管理和渲染集成。

## 目录结构

```
client/skin/
├── ClientSkinManager.hpp   # 客户端皮肤管理器（主入口）
├── ClientSkinManager.cpp   # GPU纹理上传、EntityTextureAtlas集成
└── README.md
```

## 内部模块关系

本目录只有一个核心组件 `ClientSkinManager`，它是对 `common/skin::SkinManager` 的客户端扩展：

```
ClientSkinManager
    │
    ├── 封装 SkinManager（common层）
    │       └── 玩家皮肤信息缓存、默认皮肤、异步加载
    │
    └── 管理 EntityTextureAtlas
            └── GPU纹理图集、TextureRegion分配
```

## 上下游外部依赖关系

### 上游依赖（本模块依赖）

- `common/skin` - 皮肤核心功能（SkinManager、SkinCache、GameProfile、PlayerSkinInfo、SkinTextures、SkinType）
- `client/renderer/trident/entity/pipeline` - EntityTextureAtlas（实体纹理图集）、TextureRegion
- `common/resource` - ResourceLocation
- `common/core` - Result、Error、基础类型
- Vulkan - GPU资源（VkDevice、VkPhysicalDevice、VkCommandPool、VkQueue）
- stb_image - PNG解码

### 下游依赖（谁依赖本模块）

- 暂无使用者（模块已完成但尚未集成到渲染管线）

## 核心组件

### ClientSkinManager

客户端皮肤管理器，扩展 `SkinManager` 添加：
- GPU 纹理资源管理（初始化时接收 Vulkan 资源）
- EntityTextureAtlas 集成（皮肤上传到图集）
- TextureRegion 提供给渲染器使用
- 默认皮肤（Steve/Alex）预加载到图集

## 命名空间

所有类型位于 `mc::client::skin` 命名空间。

## 容易踩的坑

- `ClientSkinManager` 必须在 `TridentEngine` 销毁前调用 `shutdown()` 或直接 `reset()`，否则 `EntityTextureAtlas` 可能在失效设备上执行 Vulkan 销毁。
- 初始化流程中即便皮肤管理器返回失败，也可能留下部分已创建的 GPU 资源，回滚路径要按同样顺序先释放皮肤管理器再销毁渲染器。
- `registerPlayerSkin` 后需要调用 `rebuildAtlas()` 才能更新纹理区域引用，否则 `getSkinRegion` 返回的可能仍是旧引用或 nullptr。
- `m_pendingSkins` 和 `m_device` 字段已定义但尚未使用，异步皮肤上传功能待实现。
