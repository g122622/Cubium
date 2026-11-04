/**
 * `block_pathfindable` 的已知缺口：Cubium 中**不存在**对应实现的两个方块。
 *
 * 这两类方块不是「寻路判定写错了」，而是整个方块类都没实现：
 *
 * - `decorated_pot`（40 个单元）：Cubium 无 `DecoratedPotBlock`。原版显式
 *   `isPathfindable` 返回 `false`。
 * - `sniffer_egg`（6 个单元）：Cubium 无 `SnifferEggBlock`。原版同样返回 `false`。
 *
 * 两者当前在 Cubium 侧落到 `Block` 基类的默认实现（按碰撞形状判定），于是
 * 形状不完整的状态被判为可通行，与原版的恒 `false` 不一致。
 *
 * 注意 `knownDifferences` 的 `key` 只支持精确匹配或 `*`，不支持前缀通配，
 * 所以这里把 46 个受影响的单元逐一列出。
 *
 * TODO: 实现 `DecoratedPotBlock` 与 `SnifferEggBlock` 时删除本文件的对应条目。
 */

/** 受影响的单元（`<状态>#<寻路类型>`），全部字段为 `pathfindable`。 */
export const KNOWN_PATHFINDABLE_GAPS: readonly { key: string; field: string; reason: string }[] = [
    "minecraft:decorated_pot[cracked=false,facing=east,waterlogged=false]#air",
    "minecraft:decorated_pot[cracked=false,facing=east,waterlogged=false]#land",
    "minecraft:decorated_pot[cracked=false,facing=east,waterlogged=true]#air",
    "minecraft:decorated_pot[cracked=false,facing=east,waterlogged=true]#land",
    "minecraft:decorated_pot[cracked=false,facing=east,waterlogged=true]#water",
    "minecraft:decorated_pot[cracked=false,facing=north,waterlogged=false]#air",
    "minecraft:decorated_pot[cracked=false,facing=north,waterlogged=false]#land",
    "minecraft:decorated_pot[cracked=false,facing=north,waterlogged=true]#air",
    "minecraft:decorated_pot[cracked=false,facing=north,waterlogged=true]#land",
    "minecraft:decorated_pot[cracked=false,facing=north,waterlogged=true]#water",
    "minecraft:decorated_pot[cracked=false,facing=south,waterlogged=false]#air",
    "minecraft:decorated_pot[cracked=false,facing=south,waterlogged=false]#land",
    "minecraft:decorated_pot[cracked=false,facing=south,waterlogged=true]#air",
    "minecraft:decorated_pot[cracked=false,facing=south,waterlogged=true]#land",
    "minecraft:decorated_pot[cracked=false,facing=south,waterlogged=true]#water",
    "minecraft:decorated_pot[cracked=false,facing=west,waterlogged=false]#air",
    "minecraft:decorated_pot[cracked=false,facing=west,waterlogged=false]#land",
    "minecraft:decorated_pot[cracked=false,facing=west,waterlogged=true]#air",
    "minecraft:decorated_pot[cracked=false,facing=west,waterlogged=true]#land",
    "minecraft:decorated_pot[cracked=false,facing=west,waterlogged=true]#water",
    "minecraft:decorated_pot[cracked=true,facing=east,waterlogged=false]#air",
    "minecraft:decorated_pot[cracked=true,facing=east,waterlogged=false]#land",
    "minecraft:decorated_pot[cracked=true,facing=east,waterlogged=true]#air",
    "minecraft:decorated_pot[cracked=true,facing=east,waterlogged=true]#land",
    "minecraft:decorated_pot[cracked=true,facing=east,waterlogged=true]#water",
    "minecraft:decorated_pot[cracked=true,facing=north,waterlogged=false]#air",
    "minecraft:decorated_pot[cracked=true,facing=north,waterlogged=false]#land",
    "minecraft:decorated_pot[cracked=true,facing=north,waterlogged=true]#air",
    "minecraft:decorated_pot[cracked=true,facing=north,waterlogged=true]#land",
    "minecraft:decorated_pot[cracked=true,facing=north,waterlogged=true]#water",
    "minecraft:decorated_pot[cracked=true,facing=south,waterlogged=false]#air",
    "minecraft:decorated_pot[cracked=true,facing=south,waterlogged=false]#land",
    "minecraft:decorated_pot[cracked=true,facing=south,waterlogged=true]#air",
    "minecraft:decorated_pot[cracked=true,facing=south,waterlogged=true]#land",
    "minecraft:decorated_pot[cracked=true,facing=south,waterlogged=true]#water",
    "minecraft:decorated_pot[cracked=true,facing=west,waterlogged=false]#air",
    "minecraft:decorated_pot[cracked=true,facing=west,waterlogged=false]#land",
    "minecraft:decorated_pot[cracked=true,facing=west,waterlogged=true]#air",
    "minecraft:decorated_pot[cracked=true,facing=west,waterlogged=true]#land",
    "minecraft:decorated_pot[cracked=true,facing=west,waterlogged=true]#water",
    "minecraft:sniffer_egg[hatch=0]#air",
    "minecraft:sniffer_egg[hatch=0]#land",
    "minecraft:sniffer_egg[hatch=1]#air",
    "minecraft:sniffer_egg[hatch=1]#land",
    "minecraft:sniffer_egg[hatch=2]#air",
    "minecraft:sniffer_egg[hatch=2]#land",
].map((key) => ({
    key,
    field: "pathfindable",
    reason: key.startsWith("minecraft:decorated_pot")
        ? "Cubium 无 DecoratedPotBlock，落到基类形状判据；原版显式返回 false"
        : "Cubium 无 SnifferEggBlock，落到基类形状判据；原版显式返回 false",
}));
