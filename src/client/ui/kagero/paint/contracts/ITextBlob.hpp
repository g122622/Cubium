#pragma once

#include "../../Types.hpp"

namespace mc::client::ui::kagero::paint {

class ITypeface;

/**
 * @brief 文本块接口
 */
class ITextBlob {
public:
    virtual ~ITextBlob() = default;

    [[nodiscard]] virtual const std::string& text() const = 0;
    [[nodiscard]] virtual const ITypeface& typeface() const = 0;
    [[nodiscard]] virtual f32 textSize() const = 0;
};

} // namespace mc::client::ui::kagero::paint
