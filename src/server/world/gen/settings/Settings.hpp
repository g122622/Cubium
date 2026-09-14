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

#pragma once

/// @file Settings.hpp
/// @brief 世界生成设置聚合头文件
///
/// 本文件聚合了所有世界生成相关的设置类型，方便外部模块统一引用。
/// 包含：
/// - DimensionSettings：维度设置
/// - NoiseSettings：噪声设置
/// - ScalingSettings：缩放设置
/// - SlideSettings：滑动设置
/// - FlatLayerInfo：平坦层信息
/// - FlatLevelGeneratorSettings：平坦世界生成设置

#include "DimensionSettings.hpp"
#include "FlatLayerInfo.hpp"
#include "FlatLevelGeneratorSettings.hpp"
#include "NoiseSettings.hpp"
#include "ScalingSettings.hpp"
#include "SlideSettings.hpp"
