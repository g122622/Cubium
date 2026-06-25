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
 * IMPLIED, INCLUDING THE PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "client/ui/kagero/template/bindings/BuiltinEvents.hpp"
#include "client/ui/kagero/Types.hpp"
#include "common/input/KeyBinding.hpp"
#include <gtest/gtest.h>

using namespace mc::client::ui::kagero::tpl::bindings::event_utils;
using namespace mc;
using mc::client::ui::kagero::KeyMods;

// ============================================================================
// parseKeyCode 测试
// ============================================================================

TEST(ParseKeyCodeTest, UnknownKey)
{
    EXPECT_EQ(parseKeyCode("unknown"), Keys::Unknown);
    EXPECT_EQ(parseKeyCode("nonexistent_key"), Keys::Unknown);
}

TEST(ParseKeyCodeTest, PrintableKeys)
{
    EXPECT_EQ(parseKeyCode("space"), Keys::Space);
    EXPECT_EQ(parseKeyCode("apostrophe"), Keys::Apostrophe);
    EXPECT_EQ(parseKeyCode("comma"), Keys::Comma);
    EXPECT_EQ(parseKeyCode("minus"), Keys::Minus);
    EXPECT_EQ(parseKeyCode("period"), Keys::Period);
    EXPECT_EQ(parseKeyCode("slash"), Keys::Slash);
    EXPECT_EQ(parseKeyCode("semicolon"), Keys::Semicolon);
    EXPECT_EQ(parseKeyCode("equal"), Keys::Equal);
    EXPECT_EQ(parseKeyCode("left_bracket"), Keys::LeftBracket);
    EXPECT_EQ(parseKeyCode("backslash"), Keys::Backslash);
    EXPECT_EQ(parseKeyCode("right_bracket"), Keys::RightBracket);
    EXPECT_EQ(parseKeyCode("grave_accent"), Keys::GraveAccent);
}

TEST(ParseKeyCodeTest, LetterKeys)
{
    EXPECT_EQ(parseKeyCode("a"), Keys::A);
    EXPECT_EQ(parseKeyCode("z"), Keys::Z);
    EXPECT_EQ(parseKeyCode("A"), Keys::A);
    EXPECT_EQ(parseKeyCode("Z"), Keys::Z);
}

TEST(ParseKeyCodeTest, DigitKeys)
{
    EXPECT_EQ(parseKeyCode("0"), Keys::D0);
    EXPECT_EQ(parseKeyCode("9"), Keys::D9);
}

TEST(ParseKeyCodeTest, NavigationKeys)
{
    EXPECT_EQ(parseKeyCode("right"), Keys::Right);
    EXPECT_EQ(parseKeyCode("left"), Keys::Left);
    EXPECT_EQ(parseKeyCode("down"), Keys::Down);
    EXPECT_EQ(parseKeyCode("up"), Keys::Up);
    EXPECT_EQ(parseKeyCode("page_up"), Keys::PageUp);
    EXPECT_EQ(parseKeyCode("page_down"), Keys::PageDown);
    EXPECT_EQ(parseKeyCode("home"), Keys::Home);
    EXPECT_EQ(parseKeyCode("end"), Keys::End);
    EXPECT_EQ(parseKeyCode("insert"), Keys::Insert);
    EXPECT_EQ(parseKeyCode("delete"), Keys::Delete);
}

TEST(ParseKeyCodeTest, EditingKeys)
{
    EXPECT_EQ(parseKeyCode("enter"), Keys::Enter);
    EXPECT_EQ(parseKeyCode("tab"), Keys::Tab);
    EXPECT_EQ(parseKeyCode("backspace"), Keys::Backspace);
    EXPECT_EQ(parseKeyCode("escape"), Keys::Escape);
}

TEST(ParseKeyCodeTest, LockKeys)
{
    EXPECT_EQ(parseKeyCode("caps_lock"), Keys::CapsLock);
    EXPECT_EQ(parseKeyCode("scroll_lock"), Keys::ScrollLock);
    EXPECT_EQ(parseKeyCode("num_lock"), Keys::NumLock);
    EXPECT_EQ(parseKeyCode("print_screen"), Keys::PrintScreen);
    EXPECT_EQ(parseKeyCode("pause"), Keys::Pause);
}

TEST(ParseKeyCodeTest, FunctionKeys)
{
    EXPECT_EQ(parseKeyCode("f1"), Keys::F1);
    EXPECT_EQ(parseKeyCode("f2"), Keys::F2);
    EXPECT_EQ(parseKeyCode("f3"), Keys::F3);
    EXPECT_EQ(parseKeyCode("f4"), Keys::F4);
    EXPECT_EQ(parseKeyCode("f5"), Keys::F5);
    EXPECT_EQ(parseKeyCode("f6"), Keys::F6);
    EXPECT_EQ(parseKeyCode("f7"), Keys::F7);
    EXPECT_EQ(parseKeyCode("f8"), Keys::F8);
    EXPECT_EQ(parseKeyCode("f9"), Keys::F9);
    EXPECT_EQ(parseKeyCode("f10"), Keys::F10);
    EXPECT_EQ(parseKeyCode("f11"), Keys::F11);
    EXPECT_EQ(parseKeyCode("f12"), Keys::F12);
    EXPECT_EQ(parseKeyCode("f13"), Keys::F13);
    EXPECT_EQ(parseKeyCode("f14"), Keys::F14);
    EXPECT_EQ(parseKeyCode("f15"), Keys::F15);
    EXPECT_EQ(parseKeyCode("f16"), Keys::F16);
    EXPECT_EQ(parseKeyCode("f17"), Keys::F17);
    EXPECT_EQ(parseKeyCode("f18"), Keys::F18);
    EXPECT_EQ(parseKeyCode("f19"), Keys::F19);
    EXPECT_EQ(parseKeyCode("f20"), Keys::F20);
    EXPECT_EQ(parseKeyCode("f21"), Keys::F21);
    EXPECT_EQ(parseKeyCode("f22"), Keys::F22);
    EXPECT_EQ(parseKeyCode("f23"), Keys::F23);
    EXPECT_EQ(parseKeyCode("f24"), Keys::F24);
    EXPECT_EQ(parseKeyCode("f25"), Keys::F25);
}

TEST(ParseKeyCodeTest, NumpadKeys)
{
    EXPECT_EQ(parseKeyCode("kp_0"), Keys::KP_0);
    EXPECT_EQ(parseKeyCode("kp_1"), Keys::KP_1);
    EXPECT_EQ(parseKeyCode("kp_2"), Keys::KP_2);
    EXPECT_EQ(parseKeyCode("kp_3"), Keys::KP_3);
    EXPECT_EQ(parseKeyCode("kp_4"), Keys::KP_4);
    EXPECT_EQ(parseKeyCode("kp_5"), Keys::KP_5);
    EXPECT_EQ(parseKeyCode("kp_6"), Keys::KP_6);
    EXPECT_EQ(parseKeyCode("kp_7"), Keys::KP_7);
    EXPECT_EQ(parseKeyCode("kp_8"), Keys::KP_8);
    EXPECT_EQ(parseKeyCode("kp_9"), Keys::KP_9);
    EXPECT_EQ(parseKeyCode("kp_decimal"), Keys::KP_Decimal);
    EXPECT_EQ(parseKeyCode("kp_divide"), Keys::KP_Divide);
    EXPECT_EQ(parseKeyCode("kp_multiply"), Keys::KP_Multiply);
    EXPECT_EQ(parseKeyCode("kp_subtract"), Keys::KP_Subtract);
    EXPECT_EQ(parseKeyCode("kp_add"), Keys::KP_Add);
    EXPECT_EQ(parseKeyCode("kp_enter"), Keys::KP_Enter);
    EXPECT_EQ(parseKeyCode("kp_equal"), Keys::KP_Equal);
}

TEST(ParseKeyCodeTest, ModifierKeys)
{
    EXPECT_EQ(parseKeyCode("left_shift"), Keys::LeftShift);
    EXPECT_EQ(parseKeyCode("left_control"), Keys::LeftControl);
    EXPECT_EQ(parseKeyCode("left_alt"), Keys::LeftAlt);
    EXPECT_EQ(parseKeyCode("left_super"), Keys::LeftSuper);
    EXPECT_EQ(parseKeyCode("right_shift"), Keys::RightShift);
    EXPECT_EQ(parseKeyCode("right_control"), Keys::RightControl);
    EXPECT_EQ(parseKeyCode("right_alt"), Keys::RightAlt);
    EXPECT_EQ(parseKeyCode("right_super"), Keys::RightSuper);
}

TEST(ParseKeyCodeTest, SpecialKeys)
{
    EXPECT_EQ(parseKeyCode("world_1"), Keys::World1);
    EXPECT_EQ(parseKeyCode("world_2"), Keys::World2);
    EXPECT_EQ(parseKeyCode("menu"), Keys::Menu);
}

TEST(ParseKeyCodeTest, CaseInsensitive)
{
    EXPECT_EQ(parseKeyCode("ENTER"), Keys::Enter);
    EXPECT_EQ(parseKeyCode("Enter"), Keys::Enter);
    EXPECT_EQ(parseKeyCode("BACKSPACE"), Keys::Backspace);
    EXPECT_EQ(parseKeyCode("Left"), Keys::Left);
    EXPECT_EQ(parseKeyCode("RIGHT"), Keys::Right);
    EXPECT_EQ(parseKeyCode("F1"), Keys::F1);
    EXPECT_EQ(parseKeyCode("KP_ENTER"), Keys::KP_Enter);
}

// ============================================================================
// parseKeyMods 测试
// ============================================================================

TEST(ParseKeyModsTest, NoModifier)
{
    EXPECT_EQ(parseKeyMods(""), 0);
    EXPECT_EQ(parseKeyMods("none"), 0);
}

TEST(ParseKeyModsTest, SingleModifiers)
{
    EXPECT_EQ(parseKeyMods("shift"), static_cast<i32>(KeyMods::Shift));
    EXPECT_EQ(parseKeyMods("ctrl"), static_cast<i32>(KeyMods::Control));
    EXPECT_EQ(parseKeyMods("alt"), static_cast<i32>(KeyMods::Alt));
    EXPECT_EQ(parseKeyMods("super"), static_cast<i32>(KeyMods::Super));
    EXPECT_EQ(parseKeyMods("caps"), static_cast<i32>(KeyMods::CapsLock));
    EXPECT_EQ(parseKeyMods("num"), static_cast<i32>(KeyMods::NumLock));
}

TEST(ParseKeyModsTest, ControlAliases)
{
    EXPECT_EQ(parseKeyMods("control"), static_cast<i32>(KeyMods::Control));
    EXPECT_EQ(parseKeyMods("meta"), static_cast<i32>(KeyMods::Super));
}

TEST(ParseKeyModsTest, CombinedModifiers)
{
    EXPECT_EQ(parseKeyMods("shift+ctrl"), static_cast<i32>(KeyMods::Shift) | static_cast<i32>(KeyMods::Control));
    EXPECT_EQ(parseKeyMods("ctrl+alt"), static_cast<i32>(KeyMods::Control) | static_cast<i32>(KeyMods::Alt));
    EXPECT_EQ(parseKeyMods("shift+alt+ctrl"),
        static_cast<i32>(KeyMods::Shift) | static_cast<i32>(KeyMods::Alt) | static_cast<i32>(KeyMods::Control));
    EXPECT_EQ(parseKeyMods("shift+caps"), static_cast<i32>(KeyMods::Shift) | static_cast<i32>(KeyMods::CapsLock));
}

TEST(ParseKeyModsTest, CaseInsensitive)
{
    EXPECT_EQ(parseKeyMods("SHIFT"), static_cast<i32>(KeyMods::Shift));
    EXPECT_EQ(parseKeyMods("Ctrl"), static_cast<i32>(KeyMods::Control));
    EXPECT_EQ(parseKeyMods("ALT"), static_cast<i32>(KeyMods::Alt));
}

// ============================================================================
// parseMouseButton 测试
// ============================================================================

TEST(ParseMouseButtonTest, NamedButtons)
{
    EXPECT_EQ(parseMouseButton("left"), 0);
    EXPECT_EQ(parseMouseButton("right"), 1);
    EXPECT_EQ(parseMouseButton("middle"), 2);
}

TEST(ParseMouseButtonTest, NumericButtons)
{
    EXPECT_EQ(parseMouseButton("0"), 0);
    EXPECT_EQ(parseMouseButton("1"), 1);
    EXPECT_EQ(parseMouseButton("2"), 2);
    EXPECT_EQ(parseMouseButton("3"), 3);
    EXPECT_EQ(parseMouseButton("4"), 4);
}

TEST(ParseMouseButtonTest, ExtendedButtons)
{
    EXPECT_EQ(parseMouseButton("button4"), 3);
    EXPECT_EQ(parseMouseButton("button5"), 4);
}

TEST(ParseMouseButtonTest, UnknownButton)
{
    EXPECT_EQ(parseMouseButton("unknown"), 0);
    EXPECT_EQ(parseMouseButton(""), 0);
}
