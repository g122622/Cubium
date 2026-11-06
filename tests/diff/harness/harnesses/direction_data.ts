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
/**
 * direction_data.ts — `Direction` 数据值编码与按轴取分量的输入构造。
 *
 * 三个方法都是纯查表，但涉及**两套互不相同的编码**，是最容易混淆的地方：
 *
 * | 方向   | get3DDataValue | get2DDataValue |
 * |--------|----------------|----------------|
 * | DOWN   | 0              | 未定义 (-1)    |
 * | UP     | 1              | 未定义 (-1)    |
 * | NORTH  | 2              | 2              |
 * | SOUTH  | 3              | 0              |
 * | WEST   | 4              | 1              |
 * | EAST   | 5              | 3              |
 *
 * 三维值等于枚举序号；二维值按「South=0、West=1、North=2、East=3」重排，
 * 用于天空光与朝向相关的水平编码。**两者不能互相替代。**
 *
 * 单元格式：`<mode>|<direction>` 或 `<mode>|<direction>|<x>,<y>,<z>`。
 */

/** 该能力的一个单元。 */
export interface DirectionDataUnit {
    readonly unit: string;
}

/** 单元字符串原样透传。 */
export function directionDataInput(unit: string): DirectionDataUnit {
    return { unit };
}