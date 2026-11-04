/**
 * `block_pathfindable` 的已知缺口。
 *
 * 目前为空：`decorated_pot` 与 `sniffer_egg` 原先只走基类的「碰撞形状不完整即可通行」
 * 默认判据，与原版的恒 `false` 不一致；两者已补上 `isPathfindable` 覆写。
 *
 * 注意 `knownDifferences` 的 `key` 只支持精确匹配或 `*`，不支持前缀通配。
 */

/** 受影响的单元（`<状态>#<寻路类型>`），全部字段为 `pathfindable`。 */
export const KNOWN_PATHFINDABLE_GAPS: readonly { key: string; field: string; reason: string }[] = [];