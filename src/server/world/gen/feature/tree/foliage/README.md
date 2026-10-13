# 树冠放置器

## 目录结构树

```text
foliage/
├── FoliagePlacer.hpp/cpp            # 树冠尺寸采样、逐层放置与叶片状态选择
├── BlobFoliagePlacer.hpp/cpp        # 橡树和白桦的球形树冠
├── FancyFoliagePlacer.hpp/cpp       # 精美橡树的多层圆形树冠
├── AcaciaFoliagePlacer.hpp/cpp      # 金合欢树冠
├── BushFoliagePlacer.hpp/cpp        # 灌木树冠
├── CherryFoliagePlacer.hpp/cpp      # 樱花树冠与垂叶
├── DarkOakFoliagePlacer.hpp/cpp     # 深色橡树树冠
├── JungleFoliagePlacer.hpp/cpp      # 丛林木树冠
├── MegaPineFoliagePlacer.hpp/cpp    # 巨型松树树冠
├── PineFoliagePlacer.hpp/cpp        # 松树树冠
├── SpruceFoliagePlacer.hpp/cpp      # 云杉树冠
├── RandomSpreadFoliagePlacer.hpp/cpp # 按提供者散布树叶
└── FoliagePlacers.hpp/cpp           # 放置器聚合入口
```

## 内部模块关系

各具体放置器继承 `FoliagePlacer`，消费树干返回的 `FoliagePosition`。`TreeFeature` 先采样 `FoliageShape`，空间检查及树干生成后再逐附着点放置树冠。高度、半径每棵树采样一次，偏移每附着点采样一次。

## 上下游外部依赖关系

上游依赖世界生成区域、树干附着点、整数提供者、方块状态提供者和随机源。下游由 `TreeFeature` 与数据包特征加载器构造及调用。

## 容易踩的坑

- 树冠只能覆盖空气和 `REPLACEABLE_BY_TREES` 标签成员，且不得覆盖持久树叶。坐标收集与最终落位须使用同一条件，避免遗漏新植被或覆盖玩家树叶。

- 逐层跳过规则的 Y 是该层相对附着点的高度，不能恒传零。
- `nextInt(2)` 上界排他；`nextInt(0, 2)` 的闭区间包含三个值，会改变角落密度和随机流。
- 树冠尺寸在空间检查前采样，失败的树也必须消耗这部分随机数；不得在每个分支重新采样尺寸。
- 精美橡树的树冠高度固定，首末层半径不扩张，中间层扩张一格；圆形裁剪使用距离平方。
- `placeFoliage` 显式接受预采样尺寸和可空的状态提供者；旧签名已由仓库内调用方统一迁移，不保留兼容重载。
- 即使兜底叶片方块为空，也必须收集坐标，使独立状态提供者能放置树冠。
