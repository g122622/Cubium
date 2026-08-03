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

#include "client/renderer/api/Types.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"

namespace mc::client::renderer::api {

/**
 * @brief 纹理格式
 */
enum class TextureFormat : u8 {
    R8_UNORM,           // 单通道 8 位无符号归一化
    R8G8_UNORM,         // 双通道 8 位无符号归一化
    R8G8B8_UNORM,       // 三通道 8 位无符号归一化
    R8G8B8A8_UNORM,     // 四通道 8 位无符号归一化
    R8G8B8_SRGB,        // 三通道 8 位 sRGB
    R8G8B8A8_SRGB,      // 四通道 8 位 sRGB
    R16_FLOAT,          // 单通道 16 位浮点
    R16G16_FLOAT,       // 双通道 16 位浮点
    R16G16B16A16_FLOAT, // 四通道 16 位浮点
    R32_FLOAT,          // 单通道 32 位浮点
    R32G32_FLOAT,       // 双通道 32 位浮点
    R32G32B32A32_FLOAT, // 四通道 32 位浮点
    D16_UNORM,          // 深度 16 位
    D24_UNORM_S8_UINT,  // 深度 24 位 + 模板 8 位
    D32_FLOAT,          // 深度 32 位浮点
    BC1_RGB_SRGB,       // 压缩格式
    BC2_SRGB,
    BC3_SRGB,
    BC4_UNORM,
    BC5_UNORM,
    BC7_SRGB
};

/**
 * @brief 纹理过滤模式
 */
enum class TextureFilter : u8 {
    Nearest,              // 最近邻
    Linear,               // 线性
    NearestMipmapNearest, // 最近邻 + 最近邻 mipmap
    LinearMipmapNearest,  // 线性 + 最近邻 mipmap
    NearestMipmapLinear,  // 最近邻 + 线性 mipmap
    LinearMipmapLinear    // 线性 + 线性 mipmap (三线性)
};

/**
 * @brief 纹理寻址模式
 */
enum class TextureAddressMode : u8 {
    Repeat,           // 重复
    MirroredRepeat,   // 镜像重复
    ClampToEdge,      // 边缘夹紧
    ClampToBorder,    // 边界夹紧
    MirrorClampToEdge // 镜像边缘夹紧
};

/**
 * @brief 纹理描述
 */
struct TextureDesc {
    u32 width = 0;
    u32 height = 0;
    u32 depth = 1;
    u32 mipLevels = 1;
    u32 arrayLayers = 1;
    TextureFormat format = TextureFormat::R8G8B8A8_SRGB;
    TextureFilter magFilter = TextureFilter::Linear;
    TextureFilter minFilter = TextureFilter::LinearMipmapLinear;
    TextureAddressMode addressModeU = TextureAddressMode::Repeat;
    TextureAddressMode addressModeV = TextureAddressMode::Repeat;
    TextureAddressMode addressModeW = TextureAddressMode::Repeat;
    bool generateMipmaps = false;
};

/**
 * @brief 纹理接口
 *
 * 平台无关的纹理抽象接口。
 */
class ITexture {
public:
    virtual ~ITexture() = default;

    /**
     * @brief 销毁纹理
     */
    virtual void destroy() = 0;

    /**
     * @brief 获取纹理宽度
     */
    [[nodiscard]] virtual u32 width() const = 0;

    /**
     * @brief 获取纹理高度
     */
    [[nodiscard]] virtual u32 height() const = 0;

    /**
     * @brief 获取纹理深度
     */
    [[nodiscard]] virtual u32 depth() const = 0;

    /**
     * @brief 获取 mip 层级数
     */
    [[nodiscard]] virtual u32 mipLevels() const = 0;

    /**
     * @brief 获取纹理格式
     */
    [[nodiscard]] virtual TextureFormat format() const = 0;

    /**
     * @brief 检查纹理是否有效
     */
    [[nodiscard]] virtual bool isValid() const = 0;

    /**
     * @brief 上传数据到纹理
     * @param data 数据指针
     * @param size 数据大小
     * @param level mip 层级
     * @return 成功或错误
     */
    [[nodiscard]] virtual Result<void> upload(const void* data, u64 size, u32 level = 0) = 0;

    /**
     * @brief 上传数据到纹理的指定区域
     * @param data 数据指针
     * @param size 数据大小（字节）
     * @param offsetX 目标区域 X 偏移（像素）
     * @param offsetY 目标区域 Y 偏移（像素）
     * @param width 目标区域宽度（像素）
     * @param height 目标区域高度（像素）
     * @param level mip 层级
     * @param rowLength 源数据行长度（像素），0 表示使用 width
     * @return 成功或错误
     *
     * 此方法用于更新纹理的子区域，适用于动画纹理等场景。
     */
    [[nodiscard]] virtual Result<void> uploadRegion(const void* data,
        u64 size,
        u32 offsetX,
        u32 offsetY,
        u32 width,
        u32 height,
        u32 level = 0,
        u32 rowLength = 0) = 0;

    /**
     * @brief 绑定到指定绑定槽
     * @param binding 绑定槽索引
     */
    virtual void bind(u32 binding) = 0;
};

} // namespace mc::client::renderer::api
