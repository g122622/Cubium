// 方块建造工具：在 GameTest 结构内批量放置方块构成墙体/实心长方体。
// 这些函数接收 GameTest 的 Test 对象，用 test.setBlockType 按 helper 相对坐标放置。

import type { Test } from "@minecraft/server-gametest";

/**
 * 用指定方块填满一个实心长方体区域（含边界）。
 * 坐标为 helper 相对坐标，下界与上界均包含。
 */
export function fillBlock(test: Test, blockType: string, xFrom: number, yFrom: number, zFrom: number, xTo: number, yTo: number, zTo: number): void {
  for (let i = xFrom; i <= xTo; i++) {
    for (let j = yFrom; j <= yTo; j++) {
      for (let k = zFrom; k <= zTo; k++) {
        test.setBlockType(blockType, { x: i, y: j, z: k });
      }
    }
  }
}

/**
 * 沿长方体四周围一圈空心墙（六面中除顶底外的四面，实心填满每面）。
 * 即在 z=zFrom 与 z=zTo 两个面各填满 x×y，再在 x=xFrom 与 x=xTo 两个面各填满（z 排除已填的边界）。
 */
export function addFourWalls(test: Test, blockType: string, xFrom: number, yFrom: number, zFrom: number, xTo: number, yTo: number, zTo: number): void {
  for (let xCoord = xFrom; xCoord <= xTo; xCoord++) {
    for (let yCoord = yFrom; yCoord <= yTo; yCoord++) {
      test.setBlockType(blockType, { x: xCoord, y: yCoord, z: zFrom });
      test.setBlockType(blockType, { x: xCoord, y: yCoord, z: zTo });
    }
  }

  for (let zCoord = zFrom + 1; zCoord < zTo; zCoord++) {
    for (let yCoord = yFrom; yCoord <= yTo; yCoord++) {
      test.setBlockType(blockType, { x: xFrom, y: yCoord, z: zCoord });
      test.setBlockType(blockType, { x: xTo, y: yCoord, z: zCoord });
    }
  }
}

/**
 * 与 addFourWalls 类似，但四角不放置（带缺口墙体），用于测试实体能否穿过缺口。
 * z=zFrom/z=zTo 面只填 x∈(xFrom,xTo) 区间，x=xFrom/x=xTo 面只填 z∈(zFrom,zTo) 区间。
 */
export function addFourNotchedWalls(test: Test, blockType: string, xFrom: number, yFrom: number, zFrom: number, xTo: number, yTo: number, zTo: number): void {
  for (let xCoord = xFrom + 1; xCoord < xTo; xCoord++) {
    for (let yCoord = yFrom; yCoord <= yTo; yCoord++) {
      test.setBlockType(blockType, { x: xCoord, y: yCoord, z: zFrom });
      test.setBlockType(blockType, { x: xCoord, y: yCoord, z: zTo });
    }
  }

  for (let zCoord = zFrom + 1; zCoord < zTo; zCoord++) {
    for (let yCoord = yFrom; yCoord <= yTo; yCoord++) {
      test.setBlockType(blockType, { x: xFrom, y: yCoord, z: zCoord });
      test.setBlockType(blockType, { x: xTo, y: yCoord, z: zCoord });
    }
  }
}
