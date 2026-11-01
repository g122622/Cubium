# 差分能力：`entity_attributes`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

生物属性不能简单拿 oracle 输出当基准：原版不少生物在「创建」时改写属性，
`DefaultAttributes` 的默认供应器与实体实例并不相同（史莱姆尺寸 1 只有 1 点血，
而默认供应器是 20）。所以这条 harness 的基准是
`harness/harnesses/entity_attributes_reference.ts`：

- 表里 73 个生物的期望值取自原版 `DefaultAttributes`（`javap -c` 读字节码常量，
  与 `Minecraft wiki` 一致），是**属性工厂的默认值**；
- `RUNTIME_DIVERGENT`（史莱姆/岩浆怪/幻翼）的运行时值以尺寸/状态改写，oracle 只报
  `missing`，基准仍取参考表；
- 表外的生物（例如 Cubium 有、原版没有的类型）仍由 oracle 的默认供应器兜底；
- 只比较两侧都建模的 6 个属性（生命/移速/跟随/攻击/护甲/击退抗性）；原版别的属性
  （`SPAWN_REINFORCEMENTS_CHANCE`、`ATTACK_KNOCKBACK`……）Cubium 尚未建模，不混入比对；
- 表面数据可用 `node tests/diff/tools/export_entity_attributes_reference.ts` 用 oracle
  重新导出（不带 `--write` 只打印，便于 diff；带 `--write` 只改数据段，注释与
  `RUNTIME_DIVERGENT` 原样保留）。`RUNTIME_DIVERGENT` 与实现缺口清单里的静态条目在
  oracle 侧没有输出，导出时沿用旧值。
