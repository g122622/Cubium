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
Vector3f WolfCollarLayer::getCollarColor(const ::mc::WolfEntity& entity) {
    u8 colorIndex = entity.getCollarColor();
    if (colorIndex < 16) {
        return COLLAR_COLORS[colorIndex];
    }
    return COLLAR_COLORS[14]; // 默认红色
}
```

## ArrowLayer

渲染生物身上附着的箭矢：
- 根据箭矢数量和位置渲染
- 支持不同角度

## HeldBlockLayer

渲染末影人持有的方块：
- 方块位于末影人头部附近
- 方块大小为 0.5x

## 参考

- MC 1.16.5 SaddleLayer
- MC 1.16.5 SheepWoolLayer
- MC 1.16.5 WolfCollarLayer
- MC 1.16.5 ArrowLayer/StuckInBodyLayer
- MC 1.16.5 HeldBlockLayer/EndermanLayer
