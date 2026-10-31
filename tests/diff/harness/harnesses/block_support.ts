/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/*
 * block_support.ts — 方块支撑判定的底座清单与输入构造。
 *
 * 单元是「方块」（oracle 的 `list` 枚举全部方块，按默认状态评估），每个单元配一组固定底座，
 * 逐底座比较「能否站在它上面」。
 *
 * ## 底座清单为什么这样选
 *
 * 能力的关键是**区分**两种支撑语义：
 *   - `isSolid()`  —— 「有碰撞、不是流体」（原版 `calculateSolid`：碰撞盒体积 ≥ 0.729 或 Y ≥ 1.0）；
 *     站立告示牌、压力板用的是这一档；
 *   - `isFaceSturdy(FULL)` —— 「这一面铺满整格」；按钮、火把、灯笼、珊瑚等用的是这一档。
 *
 * 清单里两组底座各司其职：
 *   1. **完整方块**（泥土/石头/草方块/圆石/沙/沙砾/黏土/耕地/下界岩/末地石/雪块/泥）——
 *      两档都满足，用于验证「正常情况必须通过」；
 *   2. **不满足这两档的方块**（台阶、楼梯、栅栏、地毯、漏斗、箱子）——一档或两档不满足，
 *      用于验证「不该通过时必须拒绝」；
 *   3. **液体与空气**——必须失败。
 *
 * ## 已知缺口：玻璃 / 树叶 / 冰一类「有整格碰撞但被标为非固体」
 *
 * 原版这些方块的 `legacySolid` 为 true（碰撞盒是整格），Cubium 在注册时调用
 * `notSolid()` 把它们标成了非固体，因此凡是用 `isSolid()` 判支撑的方块
 * （告示牌、压力板……）都会判它们撑不住。修法需要让 `m_isSolid` 按原版
 * `calculateSolid` 从碰撞形状推导，而不是靠注册时手写 `notSolid()`——这会牵动
 * 全项目 234 处 `notSolid()` 调用点，属于独立议题，**不在本能力里放开**。
 * 因此底座清单刻意不包含玻璃、树叶、冰：它们只会把这一处全局偏差重复记 100+ 次，
 * 淹没有价值的信号。TODO: 对齐 `calculateSolid` 后把这三种底座加回清单。
 */

/**
 * 底座清单。
 *
 * 名称取原版方块 ID，两侧都用命令语法解析，因此比较的是同一族方块。
 */
export const SUPPORT_BASES: readonly string[] = [
    // 完整方块：两档支撑都满足
    "minecraft:dirt",
    "minecraft:stone",
    "minecraft:grass_block[snowy=false]",
    "minecraft:cobblestone",
    "minecraft:sand",
    "minecraft:gravel",
    "minecraft:clay",
    "minecraft:farmland[moisture=0]",
    "minecraft:netherrack",
    "minecraft:end_stone",
    "minecraft:snow_block",
    "minecraft:mud",
    "minecraft:moss_block",
    // 不完整方块：`isFaceSturdy(FULL)` 不成立
    "minecraft:oak_slab[type=bottom,waterlogged=false]",
    "minecraft:oak_slab[type=top,waterlogged=false]",
    "minecraft:oak_stairs[facing=north,half=bottom,shape=straight,waterlogged=false]",
    "minecraft:oak_fence[east=false,north=false,south=false,west=false,waterlogged=false]",
    "minecraft:white_carpet",
    "minecraft:hopper[facing=down,enabled=true]",
    "minecraft:chest[facing=north,type=single,waterlogged=false]",
    // 液体
    "minecraft:water[level=0]",
    "minecraft:lava[level=0]",
    // 空气：悬空
    "minecraft:air",
];

/** 全部单元（方块 ID，由 oracle 的 `list` 枚举；这里只提供输入构造）。 */
export function blockSupportInput(unit: string): { block: string; bases: string[] } {
    return { block: unit, bases: [...SUPPORT_BASES] };
}