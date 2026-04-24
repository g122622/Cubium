# 实体特性层渲染器

本目录包含实体特性相关的层渲染器。

## 文件说明

| 文件 | 描述 |
|------|------|
| `SaddleLayer.hpp/cpp` | 鞍层渲染器 |
| `SheepWoolLayer.hpp/cpp` | 羊毛层渲染器 |
| `ArrowLayer.hpp/cpp` | 箭矢附着层渲染器 |

## SaddleLayer

渲染可骑乘实体上的鞍：
- 马、驴、骡
- 猪

## SheepWoolLayer

渲染羊的羊毛：
- 支持染色羊毛
- 剪毛后不渲染

## ArrowLayer

渲染生物身上附着的箭矢：
- 根据箭矢数量和位置渲染
- 支持不同角度

## 参考

- MC 1.16.5 SaddleLayer
- MC 1.16.5 SheepWoolLayer
- MC 1.16.5 ArrowLayer/StuckInBodyLayer
