# 射线与射线检测

`Ray` 射线值类型，以及沿射线做方块 / 流体检测的 DDA 实现。

## 目录结构树

```text
ray/
├── Ray.hpp        # 射线值类型：origin + direction，at(t) 取点，fromAngles 由俯仰/偏航构造
├── Raycast.hpp    # RaycastContext（射线 + 最大距离）与两个检测入口
└── Raycast.cpp    # DDA 遍历实现
```

## 内部模块关系

`Ray` 只依赖 `Vector3` 与角度换算。`RaycastContext` 把射线与最大检测距离绑在一起，
并提供 `endPosition()`。`raycastBlocks` 用 DDA 沿射线逐格推进，对每个候选方块取
`shape` 做精确包围盒相交；`raycastFluidSource` 复用同一套推进逻辑，但只对流体源判定
碰撞形状。两者都返回 `BlockRaycastResult`（`m_hitPos` 命中点 / `m_blockPos` 方块坐标 /
`m_face` 命中面 / `m_distance` 距离）。

## 上下游外部依赖关系

上游依赖 `Vector3`、`MathUtils`（角度换算）、`IWorld`（读方块与流体状态）、
`BlockRaycastResult` 与方块 `shape` 系统。下游是玩家视线选取（破坏/放置/交互）、
实体视线判定、流体源检测等。

## 容易踩的坑

- **`Ray::direction` 必须是单位向量**。`at(t)` 直接算 `origin + direction * t`，
  没有内部归一化；传非单位向量会让 `t` 与真实距离不再对应，最大距离判断随之失真。
  用 `fromAngles` 构造时它内部已做 `normalized()`，自己直接构造则需要自行归一化。
- **`fromAngles` 的角度语义是 MC 约定**：`pitch` 正为向下看、`yaw` 正为向左转，
  且内部做了 `-pitch` / `-yaw + PI/2` 的换算。传数学坐标系的角度会得到错误方向。
- **未加载区块视为空气并继续推进**（`raycastBlocks` 第 5 条算法说明）。所以射线
  在区块边界处可能"穿过"尚未加载的方块——需要"区块未加载就不命中"的语义时，
  调用方要自己判断。
- 射线终点做了**微小偏移**以避免恰好落在方块边界时的判定抖动；自己写替代实现时
  要一并处理，否则会出现随机漏判或重复命中。
- **`RaycastContext` 必须显式传检测距离**，没有默认值。项目规范禁止给函数参数设默认值，
  这两个默认值（构造参数与成员初始化各一处）已在清理时移除；**同时移除了默认构造函数**——
  它会让 `maxDistance` 保持未初始化，是真实隐患而非便利。全仓没有默认构造
  `RaycastContext` 的调用点。
- `raycastFluidSource` 只判**流体源**：遇到流动流体会立即终止但不产生命中。
  这与"流体也挡住视线"是两回事，不要混用两个入口的语义。