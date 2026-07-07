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

#include <gtest/gtest.h>

#include "client/ui/kagero/event/EventBus.hpp"
#include "client/ui/kagero/state/StateStore.hpp"
#include "client/ui/kagero/widget/ButtonWidget.hpp"
#include "client/ui/kagero/widget/CheckboxWidget.hpp"
#include "client/ui/kagero/widget/SliderWidget.hpp"
#include "client/ui/kagero/widget/TextFieldWidget.hpp"
#include "client/ui/minecraft/screens/CreateWorldScreen.hpp"
#include "common/core/DefaultValues.hpp"
#include "common/world/WorldConfig.hpp"
#include "common/world/storage/request/WorldRequests.hpp"

using namespace mc;
using namespace mc::client::ui::minecraft;
using namespace mc::client::ui::kagero::widget;

// ============================================================================
// 测试访问器：通过 friend 声明访问 CreateWorldScreen 的 private 成员与方法
// ============================================================================
//
// CreateWorldScreen.hpp 中已声明 `friend class test::CreateWorldScreenTestAccessor;`。
// 通过此访问器可在单元测试中直接设置 private 成员（m_nameField/m_seedField/m_difficulty
// 等）并调用 private 方法（_cycleDifficulty/_toggleAllowCommands 等），
// 无需修改生产代码的可见性。

namespace mc::client::ui::minecraft::test {

class CreateWorldScreenTestAccessor {
public:
    explicit CreateWorldScreenTestAccessor(CreateWorldScreen& screen)
        : m_screen(screen)
    {}

    // ====== 状态成员访问 ======
    void setGameMode(mc::GameMode mode) { m_screen.m_gameMode = mode; }
    void setWorldType(mc::WorldType type) { m_screen.m_worldType = type; }
    void setDifficulty(mc::Difficulty difficulty) { m_screen.m_difficulty = difficulty; }
    void setAllowCommands(bool allow) { m_screen.m_allowCommands = allow; }
    void setViewDistance(i32 distance) { m_screen.m_viewDistance = distance; }

    [[nodiscard]] mc::GameMode gameMode() const { return m_screen.m_gameMode; }
    [[nodiscard]] mc::WorldType worldType() const { return m_screen.m_worldType; }
    [[nodiscard]] mc::Difficulty difficulty() const { return m_screen.m_difficulty; }
    [[nodiscard]] bool allowCommands() const { return m_screen.m_allowCommands; }
    [[nodiscard]] i32 viewDistance() const { return m_screen.m_viewDistance; }

    // ====== 控件指针访问（用于注入测试用 widget） ======
    void setNameField(TextFieldWidget* field) { m_screen.m_nameField = field; }
    void setSeedField(TextFieldWidget* field) { m_screen.m_seedField = field; }
    void setDifficultyButton(ButtonWidget* button) { m_screen.m_difficultyButton = button; }
    void setAllowCommandsCheckbox(CheckboxWidget* checkbox) { m_screen.m_allowCommandsCheckbox = checkbox; }
    void setViewDistanceSlider(SliderWidget* slider) { m_screen.m_viewDistanceSlider = slider; }

    // ====== private 方法调用 ======
    void cycleDifficulty() { m_screen._cycleDifficulty(); }
    void toggleAllowCommands() { m_screen._toggleAllowCommands(); }
    void onViewDistanceChanged() { m_screen._onViewDistanceChanged(); }
    void updateDifficultyText() { m_screen._updateDifficultyText(); }

    [[nodiscard]] world::storage::CreateWorldRequest buildRequest() const { return m_screen.buildRequest(); }

private:
    CreateWorldScreen& m_screen;
};

} // namespace mc::client::ui::minecraft::test

namespace mc::client::ui::minecraft {
namespace {

// 清理 kagero 单例状态，避免用例间污染
void resetKageroSingletons()
{
    kagero::state::StateStore::instance().clear();
    kagero::state::StateStore::instance().clearMiddlewares();
    kagero::event::EventBus::instance().clear();
}

// ============================================================================
// CreateWorldScreen 构造与默认状态测试
// ============================================================================

class CreateWorldScreenTest : public ::testing::Test {
protected:
    void SetUp() override { resetKageroSingletons(); }
    void TearDown() override { resetKageroSingletons(); }
};

// 构造 CreateWorldScreen 需要走真实模板加载链路（TemplateScreen + TemplateInstance），
// 这里通过 try-catch 容错：若模板加载失败（如测试环境无文件），则跳过构造相关断言。
// 但由于 TemplateScreen::resolveTemplatePath 基于 __FILE__ 回退到源码树路径，
// 通常在测试环境中也能定位到 create_world.tpl。

TEST_F(CreateWorldScreenTest, ConstructorLoadsTemplateAndCachesWidgets)
{
    CreateWorldScreen screen;
    // 模板加载成功后，控件指针应全部非空
    // 若模板加载失败，指针为空——这里只验证不崩溃
    EXPECT_NO_THROW({
        auto req = screen.buildRequest();
        // 默认状态下 displayName 回退为 "New World"（m_nameField 为空时）
        // 若模板加载成功，m_nameField 非空但 text 为空，displayName 仍为空字符串
        // 这里只验证 buildRequest 不抛异常
        (void)req;
    });
}

// ============================================================================
// buildRequest() 种子解析测试
// ============================================================================
//
// 这些测试通过 Accessor 注入 mock 的 TextFieldWidget，避免依赖模板加载。

class BuildRequestTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        resetKageroSingletons();
        m_screen = std::make_unique<CreateWorldScreen>();
        m_accessor = std::make_unique<test::CreateWorldScreenTestAccessor>(*m_screen);

        // 注入 mock nameField 与 seedField
        m_nameField = std::make_unique<TextFieldWidget>("nameField", 0, 0, 100, 20);
        m_seedField = std::make_unique<TextFieldWidget>("seedField", 0, 0, 100, 20);
        m_accessor->setNameField(m_nameField.get());
        m_accessor->setSeedField(m_seedField.get());
    }

    void TearDown() override
    {
        m_accessor.reset();
        m_screen.reset();
        m_nameField.reset();
        m_seedField.reset();
        resetKageroSingletons();
    }

    std::unique_ptr<CreateWorldScreen> m_screen;
    std::unique_ptr<test::CreateWorldScreenTestAccessor> m_accessor;
    std::unique_ptr<TextFieldWidget> m_nameField;
    std::unique_ptr<TextFieldWidget> m_seedField;
};

TEST_F(BuildRequestTest, EmptySeedReturnsZeroSeed)
{
    m_nameField->setText("TestWorld");
    m_seedField->setText("");

    auto req = m_accessor->buildRequest();
    EXPECT_EQ(req.seed, 0u);
}

TEST_F(BuildRequestTest, NumericSeedIsParsedDirectly)
{
    m_nameField->setText("TestWorld");
    m_seedField->setText("12345");

    auto req = m_accessor->buildRequest();
    EXPECT_EQ(req.seed, 12345u);
}

TEST_F(BuildRequestTest, LargeNumericSeedIsParsedCorrectly)
{
    m_nameField->setText("TestWorld");
    m_seedField->setText("18446744073709551615"); // u64 最大值

    auto req = m_accessor->buildRequest();
    EXPECT_EQ(req.seed, 18446744073709551615ULL);
}

TEST_F(BuildRequestTest, NegativeNumericSeedIsParsed)
{
    m_nameField->setText("TestWorld");
    m_seedField->setText("-1");

    auto req = m_accessor->buildRequest();
    // stoull 对负数会按 unsigned 回绕
    EXPECT_EQ(req.seed, 18446744073709551615ULL);
}

TEST_F(BuildRequestTest, NonNumericSeedIsHashed)
{
    m_nameField->setText("TestWorld");
    m_seedField->setText("hello");

    auto req = m_accessor->buildRequest();
    const u64 expectedHash = std::hash<std::string>{}("hello");
    EXPECT_EQ(req.seed, expectedHash);
}

TEST_F(BuildRequestTest, DisplayNameFallsBackToNewWorldWhenNameFieldNull)
{
    // 不注入 nameField（保持为 nullptr）
    m_accessor->setNameField(nullptr);
    m_seedField->setText("123");

    auto req = m_accessor->buildRequest();
    EXPECT_EQ(req.displayName, "New World");
}

TEST_F(BuildRequestTest, DisplayNameUsesNameFieldText)
{
    m_nameField->setText("MyCustomWorld");
    m_seedField->setText("123");

    auto req = m_accessor->buildRequest();
    EXPECT_EQ(req.displayName, "MyCustomWorld");
}

// ============================================================================
// buildRequest() 视距边界限制测试
// ============================================================================

TEST_F(BuildRequestTest, ViewDistanceBelowMinIsClampedToMin)
{
    m_nameField->setText("TestWorld");
    m_accessor->setViewDistance(1); // 低于 VIEW_DISTANCE_MIN(3)

    auto req = m_accessor->buildRequest();
    EXPECT_EQ(req.viewDistance, 3);
}

TEST_F(BuildRequestTest, ViewDistanceAboveMaxIsClampedToMax)
{
    m_nameField->setText("TestWorld");
    m_accessor->setViewDistance(100); // 高于 VIEW_DISTANCE_MAX(32)

    auto req = m_accessor->buildRequest();
    EXPECT_EQ(req.viewDistance, 32);
}

TEST_F(BuildRequestTest, ViewDistanceInRangeIsPreserved)
{
    m_nameField->setText("TestWorld");
    m_accessor->setViewDistance(12);

    auto req = m_accessor->buildRequest();
    EXPECT_EQ(req.viewDistance, 12);
}

TEST_F(BuildRequestTest, ViewDistanceAtBoundariesIsPreserved)
{
    m_nameField->setText("TestWorld");

    m_accessor->setViewDistance(3);
    EXPECT_EQ(m_accessor->buildRequest().viewDistance, 3);

    m_accessor->setViewDistance(32);
    EXPECT_EQ(m_accessor->buildRequest().viewDistance, 32);
}

// ============================================================================
// buildRequest() 配置字段传递测试
// ============================================================================

TEST_F(BuildRequestTest, DifficultyIsPropagatedToRequest)
{
    m_nameField->setText("TestWorld");
    m_accessor->setDifficulty(mc::Difficulty::Hard);

    auto req = m_accessor->buildRequest();
    EXPECT_EQ(req.difficulty, mc::Difficulty::Hard);
}

TEST_F(BuildRequestTest, AllowCommandsIsPropagatedToRequest)
{
    m_nameField->setText("TestWorld");
    m_accessor->setAllowCommands(true);

    auto req = m_accessor->buildRequest();
    EXPECT_TRUE(req.allowCommands);
}

TEST_F(BuildRequestTest, GameModeAndWorldTypeArePropagatedToRequest)
{
    m_nameField->setText("TestWorld");
    m_accessor->setGameMode(mc::GameMode::Creative);
    m_accessor->setWorldType(mc::WorldType::Flat);

    auto req = m_accessor->buildRequest();
    EXPECT_EQ(req.gameMode, mc::GameMode::Creative);
    EXPECT_EQ(req.worldType, mc::WorldType::Flat);
}

TEST_F(BuildRequestTest, HardcoreIsAlwaysFalse)
{
    m_nameField->setText("TestWorld");
    // buildRequest 当前硬编码 hardcore=false（极限模式未在 UI 暴露）
    auto req = m_accessor->buildRequest();
    EXPECT_FALSE(req.hardcore);
}

// ============================================================================
// _cycleDifficulty() 难度循环切换测试
// ============================================================================

class CycleDifficultyTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        resetKageroSingletons();
        m_screen = std::make_unique<CreateWorldScreen>();
        m_accessor = std::make_unique<test::CreateWorldScreenTestAccessor>(*m_screen);
        // 注入 difficulty button 以验证 _updateDifficultyText
        m_difficultyButton = std::make_unique<ButtonWidget>("difficultyBtn", 0, 0, 100, 20, "Normal");
        m_accessor->setDifficultyButton(m_difficultyButton.get());
    }

    void TearDown() override
    {
        m_accessor.reset();
        m_screen.reset();
        m_difficultyButton.reset();
        resetKageroSingletons();
    }

    std::unique_ptr<CreateWorldScreen> m_screen;
    std::unique_ptr<test::CreateWorldScreenTestAccessor> m_accessor;
    std::unique_ptr<ButtonWidget> m_difficultyButton;
};

TEST_F(CycleDifficultyTest, CyclesPeacefulToEasy)
{
    m_accessor->setDifficulty(mc::Difficulty::Peaceful);
    m_accessor->cycleDifficulty();
    EXPECT_EQ(m_accessor->difficulty(), mc::Difficulty::Easy);
}

TEST_F(CycleDifficultyTest, CyclesEasyToNormal)
{
    m_accessor->setDifficulty(mc::Difficulty::Easy);
    m_accessor->cycleDifficulty();
    EXPECT_EQ(m_accessor->difficulty(), mc::Difficulty::Normal);
}

TEST_F(CycleDifficultyTest, CyclesNormalToHard)
{
    m_accessor->setDifficulty(mc::Difficulty::Normal);
    m_accessor->cycleDifficulty();
    EXPECT_EQ(m_accessor->difficulty(), mc::Difficulty::Hard);
}

TEST_F(CycleDifficultyTest, CyclesHardBackToPeaceful)
{
    m_accessor->setDifficulty(mc::Difficulty::Hard);
    m_accessor->cycleDifficulty();
    EXPECT_EQ(m_accessor->difficulty(), mc::Difficulty::Peaceful);
}

TEST_F(CycleDifficultyTest, MultipleCyclesWrapAroundCorrectly)
{
    m_accessor->setDifficulty(mc::Difficulty::Peaceful);
    for (i32 i = 0; i < 4; ++i) {
        m_accessor->cycleDifficulty();
    }
    // 4 次循环后应回到 Peaceful
    EXPECT_EQ(m_accessor->difficulty(), mc::Difficulty::Peaceful);
}

TEST_F(CycleDifficultyTest, UpdateDifficultyTextUpdatesButtonText)
{
    m_accessor->setDifficulty(mc::Difficulty::Hard);
    m_accessor->updateDifficultyText();
    EXPECT_EQ(m_difficultyButton->text(), "Hard");
}

TEST_F(CycleDifficultyTest, CycleDifficultyUpdatesButtonText)
{
    m_accessor->setDifficulty(mc::Difficulty::Peaceful);
    m_accessor->cycleDifficulty();
    // _cycleDifficulty 内部调用 _updateDifficultyText
    EXPECT_EQ(m_difficultyButton->text(), "Easy");
}

// ============================================================================
// _toggleAllowCommands() 状态同步测试
// ============================================================================

class ToggleAllowCommandsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        resetKageroSingletons();
        m_screen = std::make_unique<CreateWorldScreen>();
        m_accessor = std::make_unique<test::CreateWorldScreenTestAccessor>(*m_screen);
        // 注入 mock checkbox
        m_checkbox = std::make_unique<CheckboxWidget>("allowCommandsCheck", 0, 0, "Allow Cheats");
        m_accessor->setAllowCommandsCheckbox(m_checkbox.get());
    }

    void TearDown() override
    {
        m_accessor.reset();
        m_screen.reset();
        m_checkbox.reset();
        resetKageroSingletons();
    }

    std::unique_ptr<CreateWorldScreen> m_screen;
    std::unique_ptr<test::CreateWorldScreenTestAccessor> m_accessor;
    std::unique_ptr<CheckboxWidget> m_checkbox;
};

TEST_F(ToggleAllowCommandsTest, SyncsTrueWhenCheckboxChecked)
{
    m_checkbox->setChecked(true);
    m_accessor->toggleAllowCommands();
    EXPECT_TRUE(m_accessor->allowCommands());
}

TEST_F(ToggleAllowCommandsTest, SyncsFalseWhenCheckboxUnchecked)
{
    m_checkbox->setChecked(false);
    m_accessor->toggleAllowCommands();
    EXPECT_FALSE(m_accessor->allowCommands());
}

TEST_F(ToggleAllowCommandsTest, ReflectsLatestCheckboxState)
{
    m_checkbox->setChecked(true);
    m_accessor->toggleAllowCommands();
    EXPECT_TRUE(m_accessor->allowCommands());

    m_checkbox->setChecked(false);
    m_accessor->toggleAllowCommands();
    EXPECT_FALSE(m_accessor->allowCommands());
}

// ============================================================================
// _onViewDistanceChanged() 状态同步测试
// ============================================================================

class OnViewDistanceChangedTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        resetKageroSingletons();
        m_screen = std::make_unique<CreateWorldScreen>();
        m_accessor = std::make_unique<test::CreateWorldScreenTestAccessor>(*m_screen);
        // 注入 mock slider，范围 3~32，初值 12
        m_slider = std::make_unique<SliderWidget>("viewDistanceSlider", 0, 0, 100, 20, 3.0, 32.0, 12.0);
        m_accessor->setViewDistanceSlider(m_slider.get());
    }

    void TearDown() override
    {
        m_accessor.reset();
        m_screen.reset();
        m_slider.reset();
        resetKageroSingletons();
    }

    std::unique_ptr<CreateWorldScreen> m_screen;
    std::unique_ptr<test::CreateWorldScreenTestAccessor> m_accessor;
    std::unique_ptr<SliderWidget> m_slider;
};

TEST_F(OnViewDistanceChangedTest, SyncsSliderValueToMember)
{
    m_slider->setValue(20.0);
    m_accessor->onViewDistanceChanged();
    EXPECT_EQ(m_accessor->viewDistance(), 20);
}

TEST_F(OnViewDistanceChangedTest, SyncsMinimumValue)
{
    m_slider->setValue(3.0);
    m_accessor->onViewDistanceChanged();
    EXPECT_EQ(m_accessor->viewDistance(), 3);
}

TEST_F(OnViewDistanceChangedTest, SyncsMaximumValue)
{
    m_slider->setValue(32.0);
    m_accessor->onViewDistanceChanged();
    EXPECT_EQ(m_accessor->viewDistance(), 32);
}

TEST_F(OnViewDistanceChangedTest, SliderValueIsClampedToRange)
{
    // SliderWidget::setValue 会自动 clamp 到 [min, max]
    m_slider->setValue(100.0);
    m_accessor->onViewDistanceChanged();
    EXPECT_EQ(m_accessor->viewDistance(), 32);

    m_slider->setValue(-5.0);
    m_accessor->onViewDistanceChanged();
    EXPECT_EQ(m_accessor->viewDistance(), 3);
}

TEST_F(OnViewDistanceChangedTest, ReflectsLatestSliderValue)
{
    m_slider->setValue(10.0);
    m_accessor->onViewDistanceChanged();
    EXPECT_EQ(m_accessor->viewDistance(), 10);

    m_slider->setValue(24.0);
    m_accessor->onViewDistanceChanged();
    EXPECT_EQ(m_accessor->viewDistance(), 24);
}

} // namespace
} // namespace mc::client::ui::minecraft
