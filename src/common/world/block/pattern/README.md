# 3D 方块模式匹配（block/pattern）

在真实世界中按“字符画 + 方块状态谓词”识别三维多方块结构。对应 MC Java
`net.minecraft.world.level.block.state.pattern` 包。

## 目录结构树

```text
pattern/
├── BlockInWorld.hpp/cpp         # 世界方块引用：惰性状态/方块实体、未加载区块判定
├── BlockPattern.hpp/cpp         # 三维谓词模式、候选搜索、24 种正交朝向、坐标旋转
├── BlockPatternBuilder.hpp/cpp  # aisle/where 流式构建，把字符画编译成谓词数组
├── BlockPatternCache.hpp/cpp    # 一次 find/matches 全程共享的位置→BlockInWorld 缓存
└── README.md
```

## 内部模块关系

`BlockPatternBuilder` 把 `aisle` 字符串编译成 `BlockPattern` 的三维谓词数组；
`BlockPattern::find` 创建 `BlockPatternCache`，遍历候选起点与 `(forwards, up)` 正交朝向，
逐格通过 `BlockInWorld` 惰性读取世界；匹配成功后 `BlockPatternMatch` 持有同一个缓存，
`getBlock()` 返回匹配时刻的方块快照。

## 上下游外部依赖关系

依赖 `IWorld`、`BlockState`、`BlockRegistry`、`BlockPos`、`Direction`。使用方：

- `EndPortalFrameBlock` + `EnderEyeItem`：12 框架围成后填充末地传送门
- `EndDragonFight`：出口讲台定位与重生清理
- `CarvedPumpkinBlock`：雪 / 铁 / 铜傀儡建造
- `WitherSkullBlock`：凋灵召唤

## 容易踩的坑

1. **搜索范围以 pos 为下界**：`find(pos)` 的候选起点是
   `[pos, pos + (max-1)]³`（`max = max(width, height, depth)`），不是以 `pos` 为中心。
   结构位于 `pos` 的负方向时需要调用方自己前移锚点。
2. **区块/空气语义**：`loadChunks=false` 时未加载区块返回 `nullptr` 且不缓存；
   区块已加载但位置为空时返回非 null 的**空气状态**（原版 `getBlockState` 对空气也是非 null）。
   `hasState([]{return true;})` 因此能匹配空气。
3. **缓存快照**：`BlockPatternMatch` 持有匹配期间的缓存，`getBlock()` 返回匹配时刻的状态；
   匹配成功后世界变化不会影响已取得的 `BlockInWorld`。缓存按位置去重，一次 `find` 中
   同一世界坐标最多查询一次。
4. **清除方块前先取 stateId**：测试世界中的 `BlockState` 可能是会被 `setBlockState` 销毁的拷贝；
   `CarvedPumpkinBlock::clearPatternBlocks` 已在写方块前把 `stateId` 取成值。
5. **遍历顺序对齐原版**：候选起点按 `BlockPos.betweenClosed` 语义（x 最快、y 次之、z 最慢），
   谓词按 `width(i) → height(j) → depth(k)` 求值；多个匹配同时存在时首个匹配的选取依赖该顺序。
6. **非法输入一律断言**：`BlockPatternBuilder` 的尺寸不一致/未绑定字符、`translateAndRotate`
   的非正交 `(forwards, up)` 均使用 `MC_ASSERT_RELEASE_MSG`，不是异常。空格 `' '` 默认匹配任意方块。
7. **朝向共 24 种**：`forwards` 6 选 1，`up` 从剩余 4 个非平行/非相反方向中选 1。