# 树木装饰器

## 目录结构

```text
decorator/
├── TreeDecorator.*         # 执行上下文、条件查询与配置解析
├── PlaceOnGroundDecorator.* # 在实心地面上放置落叶等装饰
├── AttachToLogsDecorator.*  # 按方向附着在原木周围
├── TrunkVineDecorator.*     # 树干藤蔓
└── SimpleTreeDecorators.*   # 蜂巢、树叶藤蔓、地面改造等装饰
```

## 内部模块关系

各装饰器共用 `TreeDecoratorContext` 的世界读写、随机源与树干/树叶/树根坐标。配置解析器构造具体装饰器，树木特征按配置顺序执行。

## 上下游外部依赖关系

依赖生成区域、方块状态与标签、状态提供者和随机源；由树木及倒木特征调用。

## 容易踩的坑

条件必须读取方块的当前状态，不能因本轮写过该位置就忽略谓词；否则落叶会成为错误的支撑物，并改变后续随机抽取。装饰器与树干/树冠共用随机序列，取点及条件检查顺序会影响后续树木位置。

生成期 `WorldGenRegion::setBlockState` 会**同步**为带方块实体的方块创建方块实体，因此装饰器可以在 `setBlock` 之后立刻用 `TreeDecoratorContext::getBlockEntity` 取到它并写入数据（`BeehiveDecorator` 往蜂巢里塞蜜蜂就是这条路径）。注意「取不到方块实体」时仍须照数消耗随机数——方块实体只是写入目标，随机流位置由原版调用顺序决定，少一次 `nextInt` 会让后续装饰器取点整体错开。
