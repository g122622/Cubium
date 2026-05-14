#pragma once

#include "../../Types.hpp"

namespace mc::client::ui::kagero::paint {

enum class ImageFormat : u8 { R8, RG8, RGB8, RGBA8, BGRA8 };

/**
 * @brief 图像接口
 */
class IImage {
public:
    virtual ~IImage() = default;

    [[nodiscard]] virtual i32 width() const = 0;
    [[nodiscard]] virtual i32 height() const = 0;
    [[nodiscard]] virtual ImageFormat format() const = 0;
    [[nodiscard]] virtual const std::string& debugName() const = 0;
};

} // namespace mc::client::ui::kagero::paint
