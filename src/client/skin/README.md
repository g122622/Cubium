# 客户端皮肤系统 (Client Skin System)

客户端皮肤系统，负责 GPU 纹理管理和渲染集成。

## 目录结构

```
client/skin/
├── ClientSkinManager.hpp       # 客户端皮肤管理器
├── ClientSkinManager.cpp       # GPU纹理管理、渲染系统集成
├── SkinTextureManager.hpp      # 皮肤纹理管理器
├── SkinTextureManager.cpp      # 纹理上传、图集管理
└── README.md
```

## 核心组件

### ClientSkinManager

客户端皮肤管理器，扩展 `SkinManager` 添加：
- GPU 纹理资源管理
- EntityTextureAtlas 集成
- PlayerRenderer 纹理绑定

### SkinTextureManager

纹理管理器，职责：
- 上传皮肤PNG到GPU
- 管理纹理图集
- 提供 TextureRegion 给渲染器

## 与渲染系统集成

### PlayerRenderer 集成

```cpp
// 在 PlayerRenderer 中使用皮肤
void PlayerRenderer::render(Entity& entity, f64 partialTicks) {
    auto& player = static_cast<Player&>(entity);
    auto uuid = player.uuid();
    
    // 从 ClientSkinManager 获取皮肤纹理
    auto* region = clientSkinManager.getSkinRegion(uuid);
    if (region) {
        setSkinTexture(region);
    }
    
    // 获取皮肤类型
    auto skinInfo = clientSkinManager.getPlayerInfo(uuid);
    if (skinInfo && skinInfo->getSkinType() == SkinType::Slim) {
        m_model.setSlimArms(true);
    }
}
```

### CapeLayer 集成

```cpp
void CapeLayer::render(Player& player, ...) {
    auto uuid = player.uuid();
    auto* capeRegion = clientSkinManager.getCapeRegion(uuid);
    
    if (capeRegion && player.isWearing(PlayerModelPart::Cape)) {
        // 使用自定义披风纹理渲染
        renderCape(capeRegion);
    }
}
```

## 命名空间

所有类型位于 `mc::client::skin` 命名空间。

## 使用示例

```cpp
#include "client/skin/ClientSkinManager.hpp"

// 初始化
mc::client::skin::ClientSkinManager skinManager;
skinManager.initialize(device, physicalDevice, commandPool, graphicsQueue);

// 注册玩家皮肤
auto result = skinManager.registerPlayerSkin(profile);

// 在渲染时获取纹理区域
const TextureRegion* skinRegion = skinManager.getSkinRegion(uuid);
const TextureRegion* capeRegion = skinManager.getCapeRegion(uuid);

// 默认皮肤
const TextureRegion* steveRegion = skinManager.getSteveSkinRegion();
const TextureRegion* alexRegion = skinManager.getAlexSkinRegion();
```

## 依赖

- `common/skin` - 皮肤核心功能
- `client/renderer/trident/entity/pipeline/EntityTextureAtlas.hpp` - 实体纹理图集
- Vulkan - GPU资源
