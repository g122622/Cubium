// 实体体积断言工具：基于 Dimension.getEntities 在指定体积内查询实体并断言存在/不存在。

import type { Test } from "@minecraft/server-gametest";

/**
 * 断言指定体积内不存在某类型实体。
 * 体积由 from 角点与 to 角点定义，查询时 volume 取两角点差值（基岩 getEntities 的 volume 是尺寸而非对角坐标）。
 */
export function assertEntityNotInVolume(test: Test, entityType: string, xFrom: number, yFrom: number, zFrom: number, xTo: number, yTo: number, zTo: number): void {
  const fromLoc = test.worldLocation({ x: xFrom, y: yFrom, z: zFrom });

  const entities = test.getDimension().getEntities({
    type: entityType,
    location: fromLoc,
    volume: {
      x: xTo - xFrom,
      y: yTo - yFrom,
      z: zTo - zFrom,
    },
  });

  test.assert(entities.length === 0, "Entity of type '" + entityType + "' found (" + entities.length + ")");
}

/**
 * 断言指定体积内存在至少一个某类型实体。语义同 assertEntityNotInVolume 取反。
 */
export function assertEntityInVolume(test: Test, entityType: string, xFrom: number, yFrom: number, zFrom: number, xTo: number, yTo: number, zTo: number): void {
  const fromLoc = test.worldLocation({ x: xFrom, y: yFrom, z: zFrom });

  const entities = test.getDimension().getEntities({
    type: entityType,
    location: fromLoc,
    volume: {
      x: xTo - xFrom,
      y: yTo - yFrom,
      z: zTo - zFrom,
    },
  });

  test.assert(entities.length > 0, "Entity of type '" + entityType + "' was not found (" + entities.length + ")");
}
