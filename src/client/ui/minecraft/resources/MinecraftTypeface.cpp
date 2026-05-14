#include "MinecraftTypeface.hpp"

namespace mc::client::ui::minecraft {

MinecraftTypeface::MinecraftTypeface(std::unique_ptr<kagero::paint::ITypeface> typeface)
    : m_typeface(std::move(typeface))
{}

const kagero::paint::ITypeface* MinecraftTypeface::get() const
{
    return m_typeface.get();
}

} // namespace mc::client::ui::minecraft
