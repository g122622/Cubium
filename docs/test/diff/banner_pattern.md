# 差分能力：`banner_pattern`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

`banner_pattern` 是第 68 个能力，43 项全部一致。它有一个特殊的处理方式值得记录。

原版 1.21.11 已把旗帜图案改成数据包驱动的注册表：

```java
public record BannerPattern(Identifier assetId, String translationKey)
// Registries.BANNER_PATTERN，数据包 data/<ns>/banner_pattern/*.json
```

Cubium 侧却仍是写死的枚举：

```cpp
enum class BannerPatternType : u8 {
    Base = 0, SquareBottomLeft = 1, ...   // 43 个固定值
};
```

如果探针去读那个枚举，比对就退化成「枚举与枚举自己比」——数据包里改一个 `asset_id`，
Cubium 不跟随，这个能力也发现不了。所以这里**绕开 Cubium 的注册表，直接解析数据包
JSON**，与原版 oracle 走同一份数据源。

代价是：本能力当前只能验证「数据包内容与解析口径」，无法覆盖「注册表装载顺序」
「运行时覆盖」这类偏差。已在探针头部记 TODO：待 Cubium 迁移到数据包驱动注册表后，
探针应改回读注册表。

这也是一次有意为之的取舍：**先让能力跑起来并锁住数据基线**，而不是等 Cubium 完成
架构迁移。43 项全部一致意味着至少确认了「数据包里的 43 个图案能被正确解析并与原版
一一对应」，这是一个真实的、可回归的保证。
