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
namespace server::gen {

// MushroomBlock 定义在 mc::blocks 中，在此引入以便使用
// MushroomBlock::BigMushroomGenerator 等符号时无需 blocks:: 前缀。
using namespace blocks;

/**
 * @brief server 侧巨型蘑菇生成器工厂
 *
 * 为棕色/红色蘑菇创建对应的 BigMushroomGenerator 回调。
 * 每个生成器内部创建对应的 BigMushroomFeatureConfig 并调用
 * BigMushroomFeature::place()。
 *
 * BigMushroomGenerator 签名为 void(IWorld&, const BlockPos&, math::IRandom&)，
 * 由 MushroomBlock 在 grow() 中通过 createFeatureRegion() 获取的
 * WorldGenRegion（以 IWorld 接口暴露）回调。lambda 内部将 IWorld&
 * static_cast 为 WorldGenRegion& 后调用 BigMushroomFeature::place()。
 *
 * 由于 common 层方块注册时无法引用 server gen 的 BigMushroomFeature，
 * 因此构造时传入空 generator，由 server 侧初始化阶段调用 injectAll()
 * 注入真实的 BigMushroomGenerator。
 */
struct ServerBigMushroomGenerators {
    /// 棕色巨型蘑菇生成器（平顶）
    static MushroomBlock::BigMushroomGenerator brownMushroom();

    /// 红色巨型蘑菇生成器（圆顶）
    static MushroomBlock::BigMushroomGenerator redMushroom();

    /**
     * @brief 向已注册的 MushroomBlock 注入真实 BigMushroomGenerator
     *
     * 在 RegistryBootstrap::initializeAll() 中，VanillaBlocks::initialize()
     * 完成所有方块注册后调用此方法。通过 VanillaBlocks 中的静态指针访问
     * 已注册的 MushroomBlock 实例，调用 setBigMushroomGenerator() 注入真实回调。
     */
    static void injectAll();
};

} // namespace server::gen
} // namespace mc
