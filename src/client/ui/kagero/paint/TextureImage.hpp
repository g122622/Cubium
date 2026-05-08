#pragma once

#include "contracts/IImage.hpp"
#include <vulkan/vulkan.h>

namespace mc::client::ui::kagero::paint {

/**
 * @brief 纹理图像实现
 *
 * 实现 IImage 接口，持有 Vulkan 纹理资源引用。
 * 用于在画布上绘制纹理图像。
 *
 * 注意：此类不拥有纹理资源，只是持有引用。
 * 纹理的生命周期由外部（如 GuiTextureAtlas）管理。
 *
 * ## 多图集支持
 *
 * 通过 atlasSlot 字段支持多图集纹理选择：
 * - 槽位 0: 字体纹理
 * - 槽位 1: 物品纹理图集
 * - 槽位 2+: GUI纹理图集（icons、widgets等）
 */
class TextureImage final : public IImage {
public:
    /// 默认着色颜色（白色，全不透明）
    static constexpr u32 DEFAULT_TINT = 0xFFFFFFFF;

    /**
     * @brief 构造函数
     * @param imageView Vulkan 图像视图
     * @param sampler Vulkan 采样器
     * @param width 图像宽度（像素）
     * @param height 图像高度（像素）
     * @param u0 纹理坐标 U0
     * @param v0 纹理坐标 V0
     * @param u1 纹理坐标 U1
     * @param v1 纹理坐标 V1
     * @param atlasSlot 图集槽位ID（默认1=物品图集）
     * @param debugName 调试名称
     */
    TextureImage(VkImageView imageView, VkSampler sampler,
                 i32 width, i32 height,
                 f32 u0 = 0.0f, f32 v0 = 0.0f, f32 u1 = 1.0f, f32 v1 = 1.0f,
                 u8 atlasSlot = 1,
                 std::string debugName = std::string());

    ~TextureImage() override = default;

    // 禁止拷贝
    TextureImage(const TextureImage&) = delete;
    TextureImage& operator=(const TextureImage&) = delete;

    // 允许移动
    TextureImage(TextureImage&&) noexcept = default;
    TextureImage& operator=(TextureImage&&) noexcept = default;

    // ==================== IImage 接口实现 ====================

    [[nodiscard]] i32 width() const override { return m_width; }
    [[nodiscard]] i32 height() const override { return m_height; }
    [[nodiscard]] ImageFormat format() const override { return m_format; }
    [[nodiscard]] const std::string& debugName() const override { return m_debugName; }

    // ==================== 纹理访问 ====================

    /**
     * @brief 获取 Vulkan 图像视图
     */
    [[nodiscard]] VkImageView imageView() const { return m_imageView; }

    /**
     * @brief 获取 Vulkan 采样器
     */
    [[nodiscard]] VkSampler sampler() const { return m_sampler; }

    /**
     * @brief 获取纹理坐标 U0
     */
    [[nodiscard]] f32 u0() const { return m_u0; }

    /**
     * @brief 获取纹理坐标 V0
     */
    [[nodiscard]] f32 v0() const { return m_v0; }

    /**
     * @brief 获取纹理坐标 U1
     */
    [[nodiscard]] f32 u1() const { return m_u1; }

    /**
     * @brief 获取纹理坐标 V1
     */
    [[nodiscard]] f32 v1() const { return m_v1; }

    /**
     * @brief 获取图集槽位ID
     */
    [[nodiscard]] u8 atlasSlot() const { return m_atlasSlot; }

    /**
     * @brief 设置图集槽位ID
     * @param slot 新的槽位ID
     */
    void setAtlasSlot(u8 slot) { m_atlasSlot = slot; }

    /**
     * @brief 检查纹理是否有效
     */
    [[nodiscard]] bool isValid() const { return m_imageView != VK_NULL_HANDLE; }

private:
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;
    i32 m_width = 0;
    i32 m_height = 0;
    f32 m_u0 = 0.0f;
    f32 m_v0 = 0.0f;
    f32 m_u1 = 1.0f;
    f32 m_v1 = 1.0f;
    u8 m_atlasSlot = 1;  ///< 默认使用物品图集槽位
    ImageFormat m_format = ImageFormat::RGBA8;
    std::string m_debugName;
};

} // namespace mc::client::ui::kagero::paint
