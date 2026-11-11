# 区块基础类型的值类型

区块系统共用的三个纯头文件值类型：区块坐标、区段坐标、区块标识符。
它们只做坐标运算与打包，不持有任何数据，可自由拷贝。

## 目录结构树

```text
base/
├── ChunkPos.hpp      # 区块坐标 (x, z)，含 BlockPos/Vector3 转换与算术运算
├── SectionPos.hpp    # 区段坐标 (x, y, z)，含从 BlockPos 派生的工厂
└── ChunkId.hpp       # 区块标识符，把 (x, z, dimension) 压进一个 u64
```

## 内部模块关系

`ChunkPos` 与 `SectionPos` 是独立的坐标值类型，`ChunkId` 依赖 `ChunkPos`
（`chunkPos()` 会解包出坐标）。三个类型都在 `mc::world::chunk` 命名空间下定义，
并在文件末尾各自给出一个同名别名（`mc::ChunkPos` 等）与 `std::hash` 特化，
因此可直接作为 `std::unordered_map` / `std::unordered_set` 的键。

三者都是只读值语义：拷贝即得独立值，不含指针或引用。

## 上下游外部依赖关系

上游只依赖 `Types.hpp` 的 `ChunkCoord` / `i32` / `u64`、`BlockPos`（`common/world/BlockPos.hpp`）
与 `Vector3`。下游覆盖区块存储、加载票据、生成调度、光照、实体追踪等几乎所有
按区块组织的子系统。

## 容易踩的坑

- **`ChunkId` 的位布局是 `(dim << 48) | (dx << 24) | dz`**，打包时会做掩码截断：
  `dim` 取低 **16** 位、`dx` / `dz` 各取低 **24** 位（拆包时按最高位做符号扩展，
  因此坐标的有效范围是 24 位有符号，约 ±838 万）。**超出范围的坐标会被静默截断，
  不会有任何报错**。跨维度比较必须用 `ChunkId` 而不能只用 `ChunkPos`，
  否则不同维度的同名区块会被当成同一个。
- **`ChunkPos` 与 `SectionPos` 是不同层级**：前者是区块（16×16 的水平单位），
  后者是 16×16×16 的区段（还要带 Y）。二者不能直接互换。
  从 `BlockPos` 构造时要走 `ChunkPos(pos)` 或 `SectionPos::fromBlockPos(pos)`，
  不要自己算移位——一旦 `CHUNK_SHIFT` 之类常量调整，手算的地方会静默错。
- **`SectionPos` 的 Y 是区段序号而非方块 Y**。`SectionPos::fromBlockPos` 内部已用
  `blockY >> SECTION_SHIFT` 折算；直接用方块 Y 构造会得到越界值。
- 三个类型都有**默认构造函数**（零值），且 `ChunkPos` 还提供 `zero()`。
  零值是合法坐标（区块 `(0,0)`、区段 `(0,0,0)`），**不能用"是否为零"判断"是否未设置"**；
  需要表达"无坐标"请用 `std::optional`。
- 坐标运算（`operator+` / `-`）只对 `ChunkPos` 提供；`SectionPos` 没有对应重载，
  不要假定两者行为一致。