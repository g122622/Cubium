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

/**
 * @file test_command_dispatcher.cpp
 * @brief Command framework tests
 */

#include "common/command/CommandContext.hpp"
#include "common/command/CommandDispatcher.hpp"
#include "common/command/CommandNode.hpp"
#include "common/command/CommandResult.hpp"
#include "common/command/ICommandSource.hpp"
#include "common/command/StringReader.hpp"
#include "common/command/arguments/ArgumentType.hpp"
#include "common/command/exceptions/CommandExceptions.hpp"
#include "common/command/suggestions/Suggestions.hpp"
#include "common/world/entity/EntityManager.hpp"
#include "server/application/IServer.hpp"
#include "server/command/CommandRegistry.hpp"
#include "server/command/ServerCommandSource.hpp"
#include "server/core/ConnectionManager.hpp"
#include "server/core/GameModeManager.hpp"
#include "server/core/KeepAliveManager.hpp"
#include "server/core/PlayerManager.hpp"
#include "server/core/PositionTracker.hpp"
#include "server/core/TeleportManager.hpp"
#include "server/core/TimeManager.hpp"
#include "server/interaction/BlockInteractionManager.hpp"
#include "server/interaction/ContainerManager.hpp"
#include "server/interaction/InventoryManager.hpp"
#include "server/interaction/MiningManager.hpp"
#include "server/sync/ChunkSendManager.hpp"
#include "server/world/ServerChunkManager.hpp"
#include "server/world/ServerWorld.hpp"
#include "server/world/entity/EntityTracker.hpp"
#include "server/world/entity/ItemPickupManager.hpp"
#include "server/world/weather/WeatherManager.hpp"
#include <algorithm>
#include <gtest/gtest.h>

using namespace mc;
using namespace mc::command;

// ========== StringReader Tests ==========

class StringReaderTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(StringReaderTest, BasicRead)
{
    StringReader reader("hello world");

    EXPECT_TRUE(reader.canRead());
    EXPECT_EQ(reader.peek(), 'h');
    EXPECT_EQ(reader.read(), 'h');
    EXPECT_EQ(reader.getCursor(), 1);
}

TEST_F(StringReaderTest, ReadUnquotedString)
{
    StringReader reader("hello world");

    std::string word = reader.readUnquotedString();
    EXPECT_EQ(word, "hello");
    EXPECT_EQ(reader.getRemaining(), " world");

    reader.skipWhitespace();
    word = reader.readUnquotedString();
    EXPECT_EQ(word, "world");
}

TEST_F(StringReaderTest, ReadQuotedString)
{
    StringReader reader("\"hello world\" rest");

    std::string str = reader.readQuotedString();
    EXPECT_EQ(str, "hello world");
    EXPECT_EQ(reader.getRemaining(), " rest");
}

TEST_F(StringReaderTest, ReadQuotedStringWithEscape)
{
    StringReader reader("\"hello \\\"world\\\"\" rest");

    std::string str = reader.readQuotedString();
    EXPECT_EQ(str, "hello \"world\"");
}

TEST_F(StringReaderTest, ReadStringAutoDetect)
{
    StringReader reader1("hello world");
    StringReader reader2("\"quoted string\"");

    EXPECT_EQ(reader1.readString(), "hello");
    EXPECT_EQ(reader2.readString(), "quoted string");
}

TEST_F(StringReaderTest, ReadInt)
{
    StringReader reader("123");

    i32 value = reader.readInt();
    EXPECT_EQ(value, 123);
    EXPECT_FALSE(reader.canRead());
}

TEST_F(StringReaderTest, ReadNegativeInt)
{
    StringReader reader("-456");

    i32 value = reader.readInt();
    EXPECT_EQ(value, -456);
}

TEST_F(StringReaderTest, ReadIntWithRange)
{
    StringReader reader1("50");
    StringReader reader2("150");

    EXPECT_EQ(reader1.readInt(0, 100), 50);

    EXPECT_THROW((void)reader2.readInt(0, 100), CommandException);
}

TEST_F(StringReaderTest, ReadBool)
{
    StringReader reader1("true");
    StringReader reader2("false");

    EXPECT_TRUE(reader1.readBool());
    EXPECT_FALSE(reader2.readBool());
}

TEST_F(StringReaderTest, ReadDouble)
{
    StringReader reader("3.14159");

    f64 value = reader.readDouble();
    EXPECT_NEAR(value, 3.14159, 0.00001);
}

TEST_F(StringReaderTest, SkipWhitespace)
{
    StringReader reader("   hello");

    reader.skipWhitespace();
    EXPECT_EQ(reader.peek(), 'h');
}

TEST_F(StringReaderTest, Expect)
{
    StringReader reader("hello");

    reader.expect('h');
    EXPECT_EQ(reader.getCursor(), 1);

    EXPECT_THROW(reader.expect('x'), CommandException);
}

TEST_F(StringReaderTest, TryRead)
{
    StringReader reader("hello world");

    EXPECT_TRUE(reader.tryRead("hello"));
    EXPECT_EQ(reader.getRemaining(), " world");

    EXPECT_FALSE(reader.tryRead("xyz"));
}

// ========== CommandNode Tests ==========

class CommandNodeTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(CommandNodeTest, LiteralNode)
{
    auto node = std::make_shared<LiteralCommandNode<int>>("gamemode");

    EXPECT_EQ(node->getType(), NodeType::Literal);
    EXPECT_EQ(node->getName(), "gamemode");
    EXPECT_FALSE(node->hasCommand());
}

TEST_F(CommandNodeTest, NodeWithCommand)
{
    auto node = std::make_shared<LiteralCommandNode<int>>("test");

    node->setCommand([](CommandContext<int>&) { return 1; });

    EXPECT_TRUE(node->hasCommand());
}

TEST_F(CommandNodeTest, NodeWithRequirement)
{
    auto node = std::make_shared<LiteralCommandNode<int>>("admin");

    node->setRequirement([](const int& source) { return source >= 2; });

    EXPECT_TRUE(node->canUse(3));
    EXPECT_FALSE(node->canUse(1));
}

TEST_F(CommandNodeTest, NodeChildren)
{
    auto root = std::make_shared<LiteralCommandNode<int>>("root");
    auto child1 = std::make_shared<LiteralCommandNode<int>>("child1");
    auto child2 = std::make_shared<LiteralCommandNode<int>>("child2");

    root->addChild(child1);
    root->addChild(child2);

    EXPECT_EQ(root->getChildren().size(), 2u);
    EXPECT_NE(root->getChild("child1"), nullptr);
    EXPECT_NE(root->getChild("child2"), nullptr);
    EXPECT_EQ(root->getChild("nonexistent"), nullptr);
}

TEST_F(CommandNodeTest, RootNode)
{
    auto root = std::make_shared<RootCommandNode<int>>();

    EXPECT_EQ(root->getType(), NodeType::Root);
    EXPECT_EQ(root->getName(), "");
}

// ========== ArgumentType Tests ==========

class ArgumentTypeTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(ArgumentTypeTest, StringArgument)
{
    auto wordArg = StringArgumentType::word();
    auto phraseArg = StringArgumentType::string();
    auto greedyArg = StringArgumentType::greedyString();

    StringReader reader1("hello");
    EXPECT_EQ(wordArg->parse(reader1), "hello");

    StringReader reader2("\"hello world\"");
    EXPECT_EQ(phraseArg->parse(reader2), "hello world");

    StringReader reader3("hello world rest");
    EXPECT_EQ(greedyArg->parse(reader3), "hello world rest");
}

TEST_F(ArgumentTypeTest, IntegerArgument)
{
    auto intArg = IntegerArgumentType::integer(0, 100);

    StringReader reader1("50");
    EXPECT_EQ(intArg->parse(reader1), 50);

    StringReader reader2("150");
    EXPECT_THROW((void)intArg->parse(reader2), CommandException);

    StringReader reader3("-10");
    EXPECT_THROW((void)intArg->parse(reader3), CommandException);
}

TEST_F(ArgumentTypeTest, FloatArgument)
{
    auto floatArg = FloatArgumentType::floatArg(0.0f, 1.0f);

    StringReader reader1("0.5");
    EXPECT_NEAR(floatArg->parse(reader1), 0.5f, 0.001f);

    StringReader reader2("1.5");
    EXPECT_THROW((void)floatArg->parse(reader2), CommandException);
}

TEST_F(ArgumentTypeTest, BoolArgument)
{
    auto boolArg = BoolArgumentType::boolArg();

    StringReader reader1("true");
    EXPECT_TRUE(boolArg->parse(reader1));

    StringReader reader2("false");
    EXPECT_FALSE(boolArg->parse(reader2));

    StringReader reader3("invalid");
    EXPECT_THROW((void)boolArg->parse(reader3), CommandException);
}

TEST_F(ArgumentTypeTest, EnumArgument)
{
    enum class TestEnum { A, B, C };

    auto enumArg = std::make_shared<EnumArgumentType<TestEnum>>();
    enumArg->add("a", TestEnum::A);
    enumArg->add("b", TestEnum::B);
    enumArg->add("c", TestEnum::C);

    StringReader reader1("a");
    EXPECT_EQ(enumArg->parse(reader1), TestEnum::A);

    StringReader reader2("b");
    EXPECT_EQ(enumArg->parse(reader2), TestEnum::B);

    StringReader reader3("invalid");
    EXPECT_THROW((void)enumArg->parse(reader3), CommandException);
}

// ========== CommandResult Tests ==========

class CommandResultTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(CommandResultTest, SuccessResult)
{
    auto result = CommandResult::success(5);

    EXPECT_TRUE(result.isSuccess());
    EXPECT_FALSE(result.isFailure());
    EXPECT_EQ(result.result(), 5);
    EXPECT_TRUE(result);
}

TEST_F(CommandResultTest, FailureResult)
{
    auto result = CommandResult::failure("Error message");

    EXPECT_FALSE(result.isSuccess());
    EXPECT_TRUE(result.isFailure());
    EXPECT_TRUE(result.error().has_value());
    EXPECT_EQ(result.error().value(), "Error message");
    EXPECT_FALSE(result);
}

// ========== CommandException Tests ==========

class CommandExceptionTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(CommandExceptionTest, CreateException)
{
    CommandException ex(CommandErrorType::IntegerExpected, "Expected integer", 5);

    EXPECT_EQ(ex.type(), CommandErrorType::IntegerExpected);
    EXPECT_EQ(ex.message(), "Expected integer");
    EXPECT_EQ(ex.cursor(), 5);
}

TEST_F(CommandExceptionTest, SimpleException)
{
    SimpleCommandException simpleEx(CommandErrorType::EntityNotFound, "Entity not found");

    CommandException ex = simpleEx.create();
    EXPECT_EQ(ex.type(), CommandErrorType::EntityNotFound);
    EXPECT_EQ(ex.cursor(), -1);
}

TEST_F(CommandExceptionTest, ExceptionWithInput)
{
    CommandException ex(CommandErrorType::Unknown, "Error", 5);
    CommandException withInput = ex.withInput("test input");

    EXPECT_EQ(withInput.input(), "test input");
}

// ========== Suggestions Tests ==========

class SuggestionsTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(SuggestionsTest, BuildSuggestions)
{
    SuggestionsBuilder builder("test", 0);
    builder.suggest("testing");
    builder.suggest("testcase");
    builder.suggest("example");

    Suggestions suggestions = builder.build();
    // 只测试构建，不过滤（过滤逻辑在 getSuggestions 中实现）
    EXPECT_EQ(suggestions.size(), 3u);
}

TEST_F(SuggestionsTest, ApplySuggestion)
{
    Suggestion suggestion(6, "world");

    std::string result = suggestion.apply("hello ");
    EXPECT_EQ(result, "hello world");
}

TEST_F(SuggestionsTest, MergeSuggestions)
{
    Suggestions a;
    Suggestions b;

    Suggestions merged = Suggestions::merge(a, b);
    EXPECT_TRUE(merged.isEmpty());
}

TEST_F(SuggestionsTest, SuggestionComparison)
{
    Suggestion s1(0, "apple");
    Suggestion s2(0, "banana");
    Suggestion s3(0, "apple");

    EXPECT_TRUE(s1 < s2);
    EXPECT_TRUE(s1 == s3);
}

TEST_F(SuggestionsTest, RequiredArgumentBuilderSuggestsCustomProvider)
{
    CommandDispatcher<int> dispatcher;

    auto destination = argument<int, std::string>("destination",
        std::make_shared<ArgumentCommandNode<int, std::string>>("destination", StringArgumentType::word()))
                           .suggests(std::make_shared<CandidateSuggestionProvider<int>>(
                               std::vector<std::string>{"spawn", "home", "mine"}));
    auto root = std::static_pointer_cast<LiteralCommandNode<int>>(literal<int>("warp").then(destination).build());
    dispatcher.registerCommand(root);

    int source = 0;
    const auto suggestions = dispatcher.getSuggestions("warp ", source).get();

    ASSERT_FALSE(suggestions.isEmpty());
    ASSERT_EQ(suggestions.size(), 3u);

    const auto& list = suggestions.getList();
    EXPECT_EQ(list[0].getText(), "home");
    EXPECT_EQ(list[1].getText(), "mine");
    EXPECT_EQ(list[2].getText(), "spawn");
}

// ========== CommandDispatcher Basic Tests ==========

class CommandDispatcherTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(CommandDispatcherTest, DispatcherCreation)
{
    CommandDispatcher<int> dispatcher;
    EXPECT_NE(dispatcher.getRoot(), nullptr);
    EXPECT_EQ(dispatcher.getRoot()->getType(), NodeType::Root);
}

TEST_F(CommandDispatcherTest, RegisterLiteralNode)
{
    CommandDispatcher<int> dispatcher;

    auto node = std::make_shared<LiteralCommandNode<int>>("test");
    node->setCommand([](CommandContext<int>&) { return 1; });

    dispatcher.registerCommand(node);

    // 验证节点已注册
    EXPECT_NE(dispatcher.getRoot()->getChild("test"), nullptr);
}

TEST_F(CommandDispatcherTest, ParseCommand)
{
    CommandDispatcher<int> dispatcher;

    auto node = std::make_shared<LiteralCommandNode<int>>("test");
    node->setCommand([](CommandContext<int>&) { return 1; });

    dispatcher.registerCommand(node);

    int source = 0;
    auto result = dispatcher.parse("test", source);

    EXPECT_TRUE(result.isSuccess());
    ASSERT_NE(result.getContext(), nullptr);
    ASSERT_NE(result.getContext()->getCurrentNode(), nullptr);
    EXPECT_EQ(result.getContext()->getCurrentNode()->getName(), "test");
    EXPECT_TRUE(result.getRemaining().empty());
}

TEST_F(CommandDispatcherTest, ExecuteArgumentCommandStoresParsedValue)
{
    CommandDispatcher<int> dispatcher;

    auto root = std::make_shared<LiteralCommandNode<int>>("add");
    auto valueArg = std::make_shared<ArgumentCommandNode<int, i32>>("value", IntegerArgumentType::integer());
    valueArg->setCommand([](CommandContext<int>& ctx) { return ctx.getArgument<i32>("value") + ctx.getSource(); });
    root->addChild(valueArg);
    dispatcher.registerCommand(root);

    int source = 5;
    auto result = dispatcher.execute("add 7", source);

    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().isSuccess());
    EXPECT_EQ(result.value().result(), 12);
}

TEST_F(CommandDispatcherTest, ExecuteFailsOnUnknownExtraArgument)
{
    CommandDispatcher<int> dispatcher;

    auto node = std::make_shared<LiteralCommandNode<int>>("list");
    node->setCommand([](CommandContext<int>&) { return 1; });
    dispatcher.registerCommand(node);

    int source = 0;
    auto result = dispatcher.execute("list extra", source);

    EXPECT_TRUE(result.failed());
}

// 权限不足但输入精确匹配某字面量节点时，应报 PermissionDenied 友好提示，
// 而非落到其它字面量在当前 token 上抛出的误导性 Expected literal 'X'。
TEST_F(CommandDispatcherTest, PermissionDeniedWhenLiteralMatchesButNoPermission)
{
    CommandDispatcher<int> dispatcher;

    auto tpNode = std::make_shared<LiteralCommandNode<int>>("tp");
    tpNode->setRequirement([](const int& s) { return s >= 2; });
    tpNode->setCommand([](CommandContext<int>&) { return 1; });
    dispatcher.registerCommand(tpNode);

    // 另一个根字面量，模拟真实命令树里 help/gamemode 等共存节点。
    // 旧逻辑下它会在 "tp" token 上抛 Expected literal 'help' 并成为 bestFailure。
    auto helpNode = std::make_shared<LiteralCommandNode<int>>("help");
    helpNode->setCommand([](CommandContext<int>&) { return 1; });
    dispatcher.registerCommand(helpNode);

    int source = 0; // 权限 0 < 2，tp 节点被跳过
    auto result = dispatcher.parse("tp 100 1000 1000", source);

    EXPECT_FALSE(result.isSuccess());
    const auto ex = result.getException();
    ASSERT_TRUE(ex.has_value());
    EXPECT_EQ(ex->type(), CommandErrorType::PermissionDenied);
    EXPECT_EQ(ex->message(), "commands.permission.denied");
}

// 输入不匹配任何命令时不应误报为权限不足，仍走 Unknown command 路径。
TEST_F(CommandDispatcherTest, UnknownCommandWhenNoLiteralMatches)
{
    CommandDispatcher<int> dispatcher;

    auto tpNode = std::make_shared<LiteralCommandNode<int>>("tp");
    tpNode->setRequirement([](const int& s) { return s >= 2; });
    tpNode->setCommand([](CommandContext<int>&) { return 1; });
    dispatcher.registerCommand(tpNode);

    int source = 0;
    auto result = dispatcher.parse("nonexistent", source);

    EXPECT_FALSE(result.isSuccess());
    const auto ex = result.getException();
    ASSERT_TRUE(ex.has_value());
    EXPECT_NE(ex->type(), CommandErrorType::PermissionDenied);
}

// 权限满足时正常解析，确认友好提示改动不影响正常路径。
TEST_F(CommandDispatcherTest, SucceedsWithPermission)
{
    CommandDispatcher<int> dispatcher;

    auto tpNode = std::make_shared<LiteralCommandNode<int>>("tp");
    tpNode->setRequirement([](const int& s) { return s >= 2; });
    tpNode->setCommand([](CommandContext<int>&) { return 1; });
    dispatcher.registerCommand(tpNode);

    auto helpNode = std::make_shared<LiteralCommandNode<int>>("help");
    helpNode->setCommand([](CommandContext<int>&) { return 1; });
    dispatcher.registerCommand(helpNode);

    int source = 2; // 权限满足
    auto result = dispatcher.parse("tp 100 1000 1000", source);

    EXPECT_TRUE(result.isSuccess());
}

TEST_F(CommandDispatcherTest, ExecuteRedirectAlias)
{
    CommandDispatcher<int> dispatcher;

    auto target = std::make_shared<LiteralCommandNode<int>>("target");
    target->setCommand([](CommandContext<int>&) { return 42; });

    auto alias = std::make_shared<LiteralCommandNode<int>>("alias");
    alias->setRedirect(target);

    dispatcher.registerCommand(target);
    dispatcher.registerCommand(alias);

    int source = 0;
    auto result = dispatcher.execute("alias", source);

    ASSERT_TRUE(result.success());
    EXPECT_TRUE(result.value().isSuccess());
    EXPECT_EQ(result.value().result(), 42);
}

TEST_F(CommandDispatcherTest, SuggestionsFollowRedirectedNode)
{
    CommandDispatcher<int> dispatcher;

    auto target = std::make_shared<LiteralCommandNode<int>>("experience");
    auto amountArg = std::make_shared<ArgumentCommandNode<int, i32>>("mode", IntegerArgumentType::integer());
    target->addChild(amountArg);

    auto alias = std::make_shared<LiteralCommandNode<int>>("xp");
    alias->setRedirect(target);

    dispatcher.registerCommand(target);
    dispatcher.registerCommand(alias);

    int source = 0;
    auto suggestions = dispatcher.getSuggestions("xp ", source).get();

    ASSERT_FALSE(suggestions.isEmpty());
    const auto& list = suggestions.getList();
    EXPECT_TRUE(std::any_of(
        list.begin(), list.end(), [](const Suggestion& suggestion) { return suggestion.getText() == "0"; }));
    EXPECT_TRUE(std::any_of(
        list.begin(), list.end(), [](const Suggestion& suggestion) { return suggestion.getText() == "123"; }));
}

// ========== ICommandSource Tests ==========

class CommandSourceTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(CommandSourceTest, SilentCommandSource)
{
    auto& silent = SilentCommandSource::instance();

    EXPECT_FALSE(silent.shouldReceiveFeedback());
    EXPECT_FALSE(silent.shouldReceiveErrors());
    EXPECT_FALSE(silent.allowLogging());
}

// 注意：命令执行测试已移至 server 模块的集成测试中
// 这些测试需要完整的 manager mock 实现
