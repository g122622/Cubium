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
 * LIABILITY, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include "common/world/block/blocks/vegetation/MushroomBlock.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 巨型蘑菇生成器工厂
 *
 * 为棕色/红色蘑菇创建对应的 BigMushroomGenerator 回调。
 * 每个生成器内部创建对应的 BigMushroomFeatureConfig 并调用
 * BigMushroomFeature::place()。
 *
 * BigMushroomGenerator 签名为 void(WorldGenRegion&, const BlockPos&, math::Random&)，
 * 可直接传给 MushroomBlock 构造函数。
 *
 * wiki tech_蘑菇.txt#巨型蘑菇（:84-87）：骨粉生成巨型蘑菇需满足：
 *   - 下方为有效地面（泥土/砂土/草方块/菌丝体/灰化土/菌岩）
 *   - 生长空间充足（长宽各7格，高6-8格）
 *   - 蘑菇不含雪
 * wiki tech_巨型蘑菇.txt#生成（:55）：玩家或发射器对蘑菇使用骨粉时，
 *   若满足上述条件，蘑菇有40%的概率生长成巨型蘑菇。
 */
struct BigMushroomGenerators {
    /// 棕色巨型蘑菇生成器（平顶）
    static MushroomBlock::BigMushroomGenerator brownMushroom();

    /// 红色巨型蘑菇生成器（圆顶）
    static MushroomBlock::BigMushroomGenerator redMushroom();
};

} // namespace blocks
} // namespace mc
