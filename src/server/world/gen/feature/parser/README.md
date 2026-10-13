# 特征配置解析器

## 目录结构

```text
parser/
├── BlockStateParser.*         # 方块名称和属性解析
├── FluidStateParser.*         # 流体名称和属性解析
├── BlockStateProviderParser.* # 方块状态提供者解析
├── BlockPredicateParser.*     # 方块放置谓词解析
├── RuleTestParser.*           # 规则匹配条件解析
├── TrunkPlacerParser.*        # 树干类型及高度配置解析
├── FoliagePlacerParser.*      # 树冠类型及尺寸配置解析
└── FeatureSizeParser.*        # 空间检查尺寸和裁剪高度解析
```

## 内部模块关系

各解析器返回对应配置对象；状态提供者复用方块状态解析，特征工厂组合树干、树冠和尺寸配置。

## 上下游外部依赖关系

上游由 `FeatureTypeRegistry` 和配置化特征加载器调用；下游依赖方块、流体注册表、整数提供者和各特征配置类型。

## 容易踩的坑

- 树冠的 radius 和 offset 是整数提供者；裸整数须解析为常量提供者，不能引入额外随机数抽取。
- 树干基础高度和最小裁剪高度须传递给树木放置逻辑，不能被旧配置的固定高度下限覆盖。
- 状态属性须使用注册表中的规范对象；未知类型、方块或属性应通过 Result 返回错误。
