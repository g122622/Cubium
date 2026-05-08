#pragma once

#include "../../Types.hpp"

namespace mc::client::ui::kagero::paint {

/**
 * @brief 字体接口
 */
class ITypeface {
public:
    virtual ~ITypeface() = default;

    [[nodiscard]] virtual const std::string& familyName() const = 0;
    [[nodiscard]] virtual bool isBold() const = 0;
    [[nodiscard]] virtual bool isItalic() const = 0;
};

} // namespace mc::client::ui::kagero::paint
