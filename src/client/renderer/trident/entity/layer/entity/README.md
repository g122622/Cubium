# 实体特性层渲染器

本目录包含实体特性相关的层渲染器。

## 文件说明

| 文件 | 描述 |
|------|------|
| `SaddleLayer.hpp/cpp` | 鞍层渲染器 |
| `SheepWoolLayer.hpp/cpp` | 羊毛层渲染器 |
| `WolfCollarLayer.hpp/cpp` | 狼项圈层渲染器 |
| `ArrowLayer.hpp/cpp` | 箭矢附着层渲染器 |
| `HeldBlockLayer.hpp/cpp` | 持有方块层渲染器（末影人） |
| `VillagerLayer.hpp` | 村民多层纹理层渲染器 |

## SaddleLayer

渲染可骑乘实体上的鞍：
- 马、驴、骡
- 猪

## SheepWoolLayer

渲染羊的羊毛：
- 支持染色羊毛（16种颜色）
- 剪毛后不渲染
- **彩虹羊检测**：名称为 "jeb_" 的羊会循环显示彩虹色羊毛

### 彩虹羊实现

```cpp
template<typename TEntity, typename TModel>
bool SheepWoolLayer<TEntity, TModel>::isRainbowSheep(const TEntity& entity) {
    // MC 1.16.5: 检查实体是否有自定义名称 "jeb_"
    if constexpr (std::is_base_of_v<::mc::Entity, TEntity>) {
        if (entity.hasCustomName()) {
            std::string name = entity.customNameText();
            return name == "jeb_" || name == "jeb";
        }
    }
    return false;
}
```

彩虹羊颜色每 2 tick 变化一次，循环 16 种颜色。

## WolfCollarLayer

渲染驯服狼（狗）的项圈：
- 只有驯服的狼才显示项圈（使用 `entity.isTamed()` 检测）
- 项圈颜色从 `entity.getCollarColor()` 获取（0-15，对应16种染料颜色）
- 默认颜色为红色（索引 14）

### 驯服检测

```cpp
bool WolfCollarLayer::shouldRender(const ::mc::WolfEntity& entity) const {
    // 只有驯服的狼才显示项圈
    return entity.isTamed();
}
```

### 项圈颜色

```cpp
Vector3f WolfCollarLayer::_getCollarColor(const ::mc::WolfEntity& entity) {
    u8 colorIndex = entity.getCollarColor();
    if (colorIndex < 16) {
        return COLLAR_COLORS[colorIndex];
    }
    return COLLAR_COLORS[14]; // 默认红色
}
```

## ArrowLayer

渲染生物身上附着的箭矢：
- 根据 `LivingEntity::getArrowCount()` 获取箭矢数量
- 最多渲染 10 支箭矢，随机分布在实体身上
- 箭矢位置和旋转基于实体 ID 确定性随机
- 支持不同角度

### 实现细节

```cpp
template<typename TEntity, typename TModel>
void ArrowLayer<TEntity, TModel>::render(TEntity& entity, ...):
    // 从 LivingEntity 获取箭矢数量
    i32 arrowCount = 0;
    if constexpr (std::is_base_of_v<::mc::LivingEntity, TEntity>) {
        arrowCount = entity.getArrowCount();
    }

    // 最多渲染 10 支箭矢
    arrowCount = std::min(arrowCount, 10);

    // 确定性随机分布
    math::Random rng(entity.id());
    for (i32 i = 0; i < arrowCount; ++i) {
        // 基于实体 ID 的确定性随机位置和旋转
        // ...
    }
}
```

## HeldBlockLayer

渲染末影人持有的方块：
- 使用编译时类型检查 (`if constexpr` + `std::is_base_of_v`) 检测 `EndermanEntity`
- 从 `EndermanEntity::getHeldBlockState()` 获取持有的方块状态
- 从 `EndermanEntity::isHoldingBlock()` 判断是否应该渲染
- 方块位于末影人头部附近（Y偏移 0.6875）
- 方块大小为 0.5x
- **颜色渲染**：使用 `ChunkMesher::getDefaultBlockTintColor()` 获取方块默认着色颜色
  - 参考 MC 1.16.5 `BlockColors.getColor(state, null, null, 0)`
  - 末影人持有方块时没有世界/位置信息，因此使用默认颜色
  - 支持的方块：草方块（默认草色）、树叶（云杉/桦树固定颜色，其他默认叶色）、水（默认水色）等

### 类型安全的实现

```cpp
template<typename TEntity>
const ::mc::BlockState* HeldBlockLayer<TEntity>::getHeldBlock(const TEntity& entity) const {
    // 使用编译时类型检查：只有 EndermanEntity 有手持方块功能
    // 参考 MC 1.16.5: EndermanEntity.getHeldBlockState()
    if constexpr (std::is_base_of_v<::mc::EndermanEntity, TEntity>) {
        return entity.getHeldBlockState();
    }
    return nullptr;
}

template<typename TEntity>
bool HeldBlockLayer<TEntity>::shouldRender(const TEntity& entity) const {
    if constexpr (std::is_base_of_v<::mc::EndermanEntity, TEntity>) {
        return entity.isHoldingBlock();
    }
    return false;
}
```

### 显式实例化

```cpp
template class HeldBlockLayer<::mc::LivingEntity>;
template class HeldBlockLayer<::mc::EndermanEntity>;
```

## VillagerLayer

渲染村民多层纹理，参考 MC 1.16.5 VillagerLevelPendantLayer：

### 多层纹理结构

村民纹理由多层叠加组成：
1. **基础纹理** (`villager.png`) - 由主渲染器渲染，包含身体和头部基础
2. **类型层** (`type/{type}.png`) - 根据生物群系叠加不同外观
3. **职业层** (`profession/{profession}.png`) - 根据职业叠加装备和服饰
4. **等级徽章层** (`profession_level/{badge}.png`) - 显示交易等级徽章

### 渲染规则（MC 1.16.5）

- **类型层**：始终渲染（非隐身时）
- **职业层**：职业 != NONE 且 非儿童 时渲染
- **等级徽章层**：职业 != NONE 且 职业 != NITWIT 且 非儿童 时渲染

### 类型映射

| 枚举值 | 类型名称 |
|--------|----------|
| Desert (0) | desert |
| Jungle (1) | jungle |
| Plains (2) | plains |
| Savanna (3) | savanna |
| Snow (4) | snow |
| Swamp (5) | swamp |
| Taiga (6) | taiga |

### 职业映射

| 枚举值 | 职业名称 |
|--------|----------|
| None (0) | none |
| Armorer (1) | armorer |
| Butcher (2) | butcher |
| ... | ... |
| Weaponsmith (14) | weaponsmith |

### 等级徽章映射

| 等级 | 徽章名称 |
|------|----------|
| 1 | stone (新手) |
| 2 | iron (学徒) |
| 3 | gold (老手) |
| 4 | emerald (专家) |
| 5 | diamond (大师) |

### 实现架构

```cpp
template<typename TEntity, typename TModel>
class VillagerLayer : public layer::core::LayerRenderer<TEntity> {
public:
    void renderPipeline(TEntity& entity, VkCommandBuffer cmd,
        const AnimationContext& context, EntityPipeline& pipeline) override;

    void setTextureAtlas(const pipeline::EntityTextureAtlas* atlas);

private:
    // 静态网格缓存（按纹理路径索引）
    static std::unordered_map<std::string, std::unique_ptr<pipeline::EntityMesh>> s_meshCache;
    static std::shared_mutex s_meshCacheMutex;

    pipeline::EntityMesh* getOrCreateMeshForTexture(
        EntityPipeline& pipeline, TModel& model, const ResourceLocation& textureLoc);
};
```

### UV重映射

VillagerLayer 使用纹理图集UV重映射实现多层纹理：

1. 模型生成网格时使用局部UV坐标（0-1范围）
2. 每层渲染时，根据纹理图集中的位置重映射UV坐标
3. 使用静态缓存避免重复创建网格

```cpp
void remapUVs(std::vector<ModelVertex>& vertices, const TextureRegion& region) {
    const f64 du = region.u1 - region.u0;
    const f64 dv = region.v1 - region.v0;
    for (auto& vertex : vertices) {
        vertex.texCoord.x = region.u0 + vertex.texCoord.x * du;
        vertex.texCoord.y = region.v0 + vertex.texCoord.y * dv;
    }
}
```

## 参考

- MC 1.16.5 SaddleLayer
- MC 1.16.5 SheepWoolLayer
- MC 1.16.5 WolfCollarLayer
- MC 1.16.5 ArrowLayer/StuckInBodyLayer
- MC 1.16.5 HeldBlockLayer/EndermanLayer
- MC 1.16.5 VillagerLevelPendantLayer
