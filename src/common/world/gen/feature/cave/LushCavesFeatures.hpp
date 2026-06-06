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

/**
 * @file LushCavesFeatures.hpp
 * @brief 繁茂洞穴特征工厂
 *
 * 创建和注册所有繁茂洞穴特征：
 * - 苔藓地面贴片 (LushCavesVegetation)
 * - 苔藓天花板贴片 (LushCavesCeilingVegetation)
 * - 洞穴藤蔓 (CaveVines)
 * - 黏土池 (LushCavesClay)
 * - 杜鹃树根系统 (RootedAzaleaTree)
 * - 孢子花 (SporeBlossom)
 * - 经典藤蔓 (ClassicVines)
 * 及其子特征。
 *
 * 参考: net.minecraft.data.worldgen.features.CaveFeatures
 */

#include <memory>
#include <vector>

namespace mc {

class ConfiguredFeatureBase;

/**
 * @brief 繁茂洞穴特征工厂
 *
 * 负责创建所有繁茂洞穴的配置化特征实例。
 * 调用 initialize() 后通过 getAllFeaturesAndClear() 获取所有权。
 */
struct LushCavesFeatures {
    /// 初始化所有繁茂洞穴特征
    static void initialize();

    /// 获取所有特征并清空内部存储
    [[nodiscard]] static std::vector<std::unique_ptr<ConfiguredFeatureBase>> getAllFeaturesAndClear();

private:
    // 子特征（被主特征引用，不直接添加到群系）
    static std::unique_ptr<ConfiguredFeatureBase> createMossVegetation();
    static std::unique_ptr<ConfiguredFeatureBase> createCaveVineInMoss();
    static std::unique_ptr<ConfiguredFeatureBase> createClayWithDripleaves();
    static std::unique_ptr<ConfiguredFeatureBase> createClayPoolWithDripleaves();
    static std::unique_ptr<ConfiguredFeatureBase> createSmallDripleaf();
    static std::unique_ptr<ConfiguredFeatureBase> createBigDripleafNorth();
    static std::unique_ptr<ConfiguredFeatureBase> createBigDripleafSouth();
    static std::unique_ptr<ConfiguredFeatureBase> createBigDripleafWest();
    static std::unique_ptr<ConfiguredFeatureBase> createBigDripleafEast();
    static std::unique_ptr<ConfiguredFeatureBase> createDripleaf();
    static std::unique_ptr<ConfiguredFeatureBase> createAzaleaTree();

    // 主特征（直接添加到群系）
    static std::unique_ptr<ConfiguredFeatureBase> createLushCavesVegetation();
    static std::unique_ptr<ConfiguredFeatureBase> createLushCavesCeilingVegetation();
    static std::unique_ptr<ConfiguredFeatureBase> createCaveVines();
    static std::unique_ptr<ConfiguredFeatureBase> createLushCavesClay();
    static std::unique_ptr<ConfiguredFeatureBase> createRootedAzaleaTree();
    static std::unique_ptr<ConfiguredFeatureBase> createSporeBlossom();
    static std::unique_ptr<ConfiguredFeatureBase> createClassicVines();

    static std::vector<std::unique_ptr<ConfiguredFeatureBase>> s_features;
};

} // namespace mc
