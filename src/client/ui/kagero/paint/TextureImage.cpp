#include "TextureImage.hpp"

namespace mc::client::ui::kagero::paint {

TextureImage::TextureImage(VkImageView imageView, VkSampler sampler,
                           i32 width, i32 height,
                           f32 u0, f32 v0, f32 u1, f32 v1,
                           u8 atlasSlot,
                           std::string debugName)
    : m_imageView(imageView)
    , m_sampler(sampler)
    , m_width(width)
    , m_height(height)
    , m_u0(u0)
    , m_v0(v0)
    , m_u1(u1)
    , m_v1(v1)
    , m_atlasSlot(atlasSlot)
    , m_debugName(std::move(debugName)) {
}

} // namespace mc::client::ui::kagero::paint
