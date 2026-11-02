# 差分能力：`map_decoration_type`

> 由 `tests/diff/README.md` 拆分而来，记录该能力的实现要点、踩坑与验证结论。
> 跨能力共性规律见 [`_patterns.md`](./_patterns.md)。

第 178 个能力，42/43 一致 + 1 处已登记差异。**探针调用 Cubium 的
`world::map::decorationTypeToString` / `decorationTypeFromString`**。

本能力暴露并修复了 **1 处真实缺陷**。

## 发现的缺陷：5 个村庄的序列化名用了纹理名

### 修复前

```cpp
// MapDecoration.cpp（修复前）
case DecorationType::DESERT_VILLAGE:
    return "desert_village";      // ← 纹理名
case DecorationType::PLAINS_VILLAGE:
    return "plains_village";
case DecorationType::SAVANNA_VILLAGE:
    return "savanna_village";
case DecorationType::SNOWY_VILLAGE:
    return "snowy_village";
case DecorationType::TAIGA_VILLAGE:
    return "taiga_village";
```

### 差异实测（6 处）

| 单元 | 原版 | Cubium（修复前） |
|---|---|---|
| `toString\|DESERT_VILLAGE` | **`village_desert`** | `desert_village` |
| `toString\|PLAINS_VILLAGE` | **`village_plains`** | `plains_village` |
| `toString\|SAVANNA_VILLAGE` | **`village_savanna`** | `savanna_village` |
| `toString\|SNOWY_VILLAGE` | **`village_snowy`** | `snowy_village` |
| `toString\|TAIGA_VILLAGE` | **`village_taiga`** | `taiga_village` |
| `fromString\|desert_village` | **`null`** | `DESERT_VILLAGE` |

### 根因：原版 `register` 有两个名字参数

```java
public static final Holder<MapDecorationType> DESERT_VILLAGE =
    register("village_desert", "desert_village", true, MapColor.COLOR_LIGHT_GRAY.col, false, true);
//        ^^^^^^^^^^^^^^^^^  ^^^^^^^^^^^^^^^^
//        注册名（序列化用）  纹理 id
```

**第一个参数是序列化名，第二个是纹理路径。** 两者恰好相反。

Cubium 的枚举名是 `DESERT_VILLAGE`，注释写「Java: village_desert」，但
`decorationTypeToString` 输出的是**纹理名** `desert_village`。

### 为什么这是缺陷

地图装饰会写入存档 NBT 与网络包：

```java
// 原版 MapItemSavedData 的 NBT 格式
"decorations": [{"type": "minecraft:village_desert", ...}]
```

Cubium 写出 `desert_village`，**Java 版客户端读不到这个类型**（原版只认注册名）。
本项目的客户端已停止维护、仅支持 Java 版客户端连接，因此这是**实质互通缺陷**。

### 修复

```cpp
case DecorationType::DESERT_VILLAGE:
    return "village_desert";      // ← 注册名
case DecorationType::PLAINS_VILLAGE:
    return "village_plains";
case DecorationType::SAVANNA_VILLAGE:
    return "village_savanna";
case DecorationType::SNOWY_VILLAGE:
    return "village_snowy";
case DecorationType::TAIGA_VILLAGE:
    return "village_taiga";
```

`fromString` **保留**纹理名兼容（见下方已知差异）。

### 修复后

```
[1/1] map_decoration_type          通过      9.6s  42/43 一致；1 处已登记的已知差异
```

5 处差异消失，`mc_tests` 全量构建通过（退出码 0）。

## 核心发现：两套命名规则互相矛盾

| 常量 | 注册名（**序列化**） | 纹理名 |
|---|---|---|
| `WOODLAND_MANSION` | **`mansion`** | `woodland_mansion` |
| `OCEAN_MONUMENT` | **`monument`** | `ocean_monument` |
| `WHITE_BANNER` | `banner_white` | `white_banner` |
| `DESERT_VILLAGE` | `village_desert` | `desert_village` |

- **府邸/神殿**：注册名是**短名**（`mansion`），纹理是长名（`woodland_mansion`）
- **旗帜**：注册名是**前缀式**（`banner_white`），纹理是后缀式（`white_banner`）
- **村庄**：注册名是**前缀式**（`village_desert`），纹理是后缀式（`desert_village`）

**同一个 `register` 调用里，两个名字的顺序在府邸和村庄之间是相反的。**

**Cubium 的 `mansion` / `monument` 是对的**（读源码时我一度以为名称有误，核对后确认短名才是注册名），
只有村庄错了。

## 核心发现：地图装饰在 1.21.11 已不是枚举

原版已改为**注册表对象**：

```java
public record MapDecorationType(
    Identifier assetId, boolean hasMapColor, int mapColor, boolean trackCount, boolean explorationMapElement) {}
```

序列化名来自注册表键 `ResourceKey<MapDecorationType>`，而不是枚举名。

- **`mansion`** / **`monument`** 由 `register` 的第一个参数显式给出
- 其余项恰好与枚举常量名的小写一致（如 `banner_white` ↔ `WHITE_BANNER`）

**只有村庄这一组，注册名与常量名的词序不同**——这是最容易出错的地方。

## 踩坑一：`ResourceKey.location()` 在 1.21.11 已改名

```java
// 旧版本
holder.unwrapKey().orElseThrow().location().getPath()

// 1.21.11
holder.unwrapKey().orElseThrow().identifier().getPath()
```

编译报 `cannot find symbol: method location()`。用 `javap` 查 `ResourceKey` 的公开方法列表确认
新名字是 `identifier()`。

## 踩坑二：静态初始化早于 bootstrap（第四次踩坑）

`BY_NAME` 用 `static { ... }` 块填充，导致：

```
Exception in thread "main" java.lang.ExceptionInInitializerError
Caused by: java.lang.IllegalArgumentException: Not bootstrapped (called from registry minecraft:game_event)
    at net.minecraft.core.registries.BuiltInRegistries.internalRegister(BuiltInRegistries.java:292)
```

**`MapDecorationTypes` 的静态初始化会触碰 `BuiltInRegistries`**，而 `static` 块在 `main` 之前执行，
此时 bootstrap 还没跑。

修复：改为**惰性初始化**，在 `main` 里 bootstrap 之后再填充。

```java
private static final Map<String, Holder<MapDecorationType>> BY_NAME = new LinkedHashMap<>();

private static void initRegistry() {
    if (!BY_NAME.isEmpty()) {
        return;
    }
    BY_NAME.put("PLAYER", MapDecorationTypes.PLAYER);
    // ...
}

public static void main(String[] args) throws IOException {
    net.minecraft.SharedConstants.tryDetectVersion();
    net.minecraft.server.Bootstrap.bootStrap();
    initRegistry();
    // ...
}
```

**教训：凡是要访问注册表的 oracle，`static` 块一律不能用，必须惰性初始化。**

## 已登记的已知差异（1 处）

| 单元 | 原版 | Cubium | 原因 |
|---|---|---|---|
| `fromString\|desert_village` | `null` | `DESERT_VILLAGE` | Cubium 额外接受纹理名以兼容旧数据包 JSON |

`decorationTypeFromString` 的注释写明「同时接受 registry key 与 assetId」，
唯一调用方是 `LootSerializers.cpp:1550`（读数据包 JSON）。这是**有意的向后兼容**，
且修复后 `toString` 已输出正确的注册名，**不会写出错误数据**。

## 与其它能力的关系

| 能力 | 关系 |
|---|---|
| [`game_type`](./game_type.md) | 同为「小枚举 + 名字映射」，且都是 `ZERO` 越界 |
| [`render_type_strings`](./render_type_strings.md) | 同为字符串双向映射 |
| [`map_color`](./mth_color.md) | 地图颜色计算 |
| [`loot_table`](./loot_table.md) | 调用 `decorationTypeFromString` 的数据包解析 |

## 未覆盖范围

- `MapDecorationType` 的 `hasMapColor` / `mapColor` / `trackCount` / `explorationMapElement` 字段
- `MapDecoration` 的 NBT 读写（`fromNbt` / `toNbt`）
- 装饰在地图上的渲染与旋转
- 注册表 `minecraft:map_decoration_type` 的完整内容（本能力只覆盖 35 个内置项）