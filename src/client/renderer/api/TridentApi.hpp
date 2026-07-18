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

/**
 * @file TridentApi.hpp
 * @brief Trident 渲染引擎 API 统一头文件
 *
 * 包含所有平台无关的渲染接口定义。
 * 这些接口为不同渲染后端（Vulkan、OpenGL、DirectX等）提供统一的抽象。
 */

// 基础类型
#include "BlendMode.hpp"
#include "CompareOp.hpp"
#include "CullMode.hpp"
#include "Types.hpp"

// 缓冲区
#include "buffer/IBuffer.hpp"

// 纹理
#include "texture/ITexture.hpp"
#include "texture/ITextureAtlas.hpp"
#include "texture/TextureRegion.hpp"

// 管线
#include "pipeline/IPipeline.hpp"
#include "pipeline/RenderState.hpp"
#include "pipeline/RenderType.hpp"

// 相机
#include "camera/CameraConfig.hpp"
#include "camera/ICamera.hpp"

// 渲染引擎
#include "IRenderEngine.hpp"

// 注意：此命名空间别名可能导致冲突，已移除
// 如需使用，请在代码中使用完整命名空间 mc::client::renderer::api
