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

#include "common/world/block/blocks/vegetation/SaplingBlock.hpp"

namespace mc {
namespace server::gen {

/**
 * @brief server 侧树木生成器工厂
 *
 * 为每种树苗类型创建对应的 TreeGenerator 回调。
 * 每个生成器内部创建对应的 TreeFeatureConfig 并调用 TreeFeature::place()。
 *
 * TreeGenerator 签名为 void(IWorld&, const BlockPos&, math::Random&)，
 * 可直接传给 SaplingBlock 构造函数。lambda 内部将 IWorld& static_cast
 * 为 WorldGenRegion& 后调用 TreeFeature::place()。
 *
 * 由于 common 层方块注册时无法引用 server gen 的 TreeFeature，
 * SaplingBlock 构造时传入空 generator，由 RegistryBootstrap 在
 * VanillaBlocks::initialize() 之后调用 injectAll() 注入真实 generator。
 */
struct ServerTreeGenerators {
    /// 橡树生成器
    static SaplingBlock::TreeGenerator oakTree();

    /// 白桦树生成器
    static SaplingBlock::TreeGenerator birchTree();

    /// 云杉树生成器
    static SaplingBlock::TreeGenerator spruceTree();

    /// 丛林树生成器
    static SaplingBlock::TreeGenerator jungleTree();

    /// 金合欢树生成器
    static SaplingBlock::TreeGenerator acaciaTree();

    /// 深色橡树生成器
    static SaplingBlock::TreeGenerator darkOakTree();

    /// 樱花树生成器
    static SaplingBlock::TreeGenerator cherryTree();

    /// 苍白橡树生成器
    static SaplingBlock::TreeGenerator paleOakTree();

    /// 杜鹃树生成器（用于杜鹃方块与开花杜鹃方块的骨粉生长）
    static SaplingBlock::TreeGenerator azaleaTree();

    /**
     * @brief 向已注册的 SaplingBlock 注入真实 TreeGenerator
     *
     * 在 RegistryBootstrap::initializeAll() 中，VanillaBlocks::initialize()
     * 完成所有方块注册后调用此方法。通过 VanillaBlocks 中的静态指针访问
     * 已注册的 SaplingBlock 实例，调用 setTreeGenerator() 注入真实回调。
     */
    static void injectAll();
};

} // namespace server::gen
} // namespace mc
