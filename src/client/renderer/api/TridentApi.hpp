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

// 网格
#include "mesh/MeshData.hpp"

// 渲染引擎
#include "IRenderEngine.hpp"

// 注意：此命名空间别名可能导致冲突，已移除
// 如需使用，请在代码中使用完整命名空间 mc::client::renderer::api
