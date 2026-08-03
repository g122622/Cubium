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
 */

#pragma once

#include "FluidStatus.hpp"
#include "common/core/Types.hpp"

namespace mc::world::gen::aquifer {

/**
 * @brief 创建全局流体选择器（对齐 MC 1.21.11 NoiseBasedChunkGenerator.createFluidPicker）
 *
 * 这是原版对所有维度（主世界/下界/末地/洞穴/浮岛）统一使用的唯一流体选择器，
 * 仅读取 noise_settings 的 sea_level 与 default_fluid 两个字段——故本身即数据驱动，
 * 无需按维度分支。原版逻辑：
 *   y < min(-54, seaLevel) → FluidStatus{-54, LAVA}
 *   否则                    → FluidStatus{seaLevel, defaultFluid}
 *
 * 各维度零行为差（已逐维度核对 minY）：
 * - 主世界 minY=-64, seaLevel=63, defaultFluid=水：y<-54 熔岩, 否则 {63,水}
 * - 下界   minY=0,  seaLevel=32, defaultFluid=熔岩：y∈[0,128] 永不 < -54 → 全 {32,熔岩}
 * - 末地   minY=0,  seaLevel=0,  defaultFluid=空气：永不 < -54 → {0,空气}, at(y)→AIR
 *
 * @param seaLevel 海平面高度（noise_settings.sea_level）
 * @param defaultFluid 默认流体方块状态（noise_settings.default_fluid）
 * @return 流体选择器
 */
[[nodiscard]] FluidPicker createFluidPicker(i32 seaLevel, const BlockState* defaultFluid);

} // namespace mc::world::gen::aquifer
