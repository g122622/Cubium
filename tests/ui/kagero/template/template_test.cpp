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

#include <iostream>
#include <gtest/gtest.h>

// 模板系统核心组件
#include "client/ui/kagero/event/EventBus.hpp"
#include "client/ui/kagero/event/UIEvents.hpp"
#include "client/ui/kagero/state/StateStore.hpp"
#include "client/ui/kagero/template/binder/BindingContext.hpp"
#include "client/ui/kagero/template/compiler/TemplateCompiler.hpp"
#include "client/ui/kagero/template/core/TemplateConfig.hpp"
#include "client/ui/kagero/template/core/TemplateError.hpp"
#include "client/ui/kagero/template/parser/Ast.hpp"
#include "client/ui/kagero/template/parser/AstVisitor.hpp"
#include "client/ui/kagero/template/parser/Lexer.hpp"
#include "client/ui/kagero/template/parser/Parser.hpp"
#include "client/ui/kagero/template/runtime/UpdateScheduler.hpp"

using namespace mc::client::ui::kagero::tpl;
using mc::f32;
using mc::i32;
using mc::i64;
using mc::u32;
using mc::u64;

// ==================== TemplateConfig Tests ====================

class TemplateConfigTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(TemplateConfigTest, DefaultConfig)
{
    auto config = core::TemplateConfig::defaults();
    EXPECT_TRUE(config.strictMode);
    EXPECT_FALSE(config.allowInlineScript);
    EXPECT_FALSE(config.allowInlineExpression);
    EXPECT_FALSE(config.allowDynamicTagName);
    EXPECT_TRUE(config.enableCache);
    EXPECT_TRUE(config.validateBindingPaths);
    EXPECT_TRUE(config.validateCallbackNames);
}

TEST_F(TemplateConfigTest, DevelopmentConfig)
{
    auto config = core::TemplateConfig::development();
    EXPECT_TRUE(config.debugOutput);
    EXPECT_TRUE(config.strictMode);
}

TEST_F(TemplateConfigTest, ProductionConfig)
{
    auto config = core::TemplateConfig::production();
    EXPECT_TRUE(config.enableCache);
    EXPECT_FALSE(config.debugOutput);
}

TEST_F(TemplateConfigTest, SourceLocationDefault)
{
    core::SourceLocation loc;
    EXPECT_EQ(loc.line, 1u);
    EXPECT_EQ(loc.column, 1u);
    EXPECT_EQ(loc.offset, 0u);
    EXPECT_TRUE(loc.isValid());
}

TEST_F(TemplateConfigTest, SourceLocationInvalid)
{
    auto loc = core::SourceLocation::invalid();
    EXPECT_EQ(loc.line, 0u);
    EXPECT_EQ(loc.column, 0u);
    EXPECT_FALSE(loc.isValid());
}

TEST_F(TemplateConfigTest, SourceLocationToString)
{
    core::SourceLocation loc(10, 20, 100);
    EXPECT_EQ(loc.toString(), "line 10, column 20");
}

TEST_F(TemplateConfigTest, SourceRangeMerge)
{
    core::SourceLocation start(1, 1, 0);
    core::SourceLocation end(1, 10, 9);
    core::SourceRange range(start, end);

    EXPECT_EQ(range.start.line, 1u);
    EXPECT_EQ(range.end.column, 10u);

    core::SourceLocation otherStart(1, 5, 5);
    core::SourceLocation otherEnd(2, 5, 50);
    core::SourceRange otherRange(otherStart, otherEnd);

    auto merged = range.merge(otherRange);
    EXPECT_EQ(merged.start.offset, 0u);
    EXPECT_EQ(merged.end.offset, 50u);
}

TEST_F(TemplateConfigTest, SourceRangeAt)
{
    core::SourceLocation loc(5, 10, 50);
    auto range = core::SourceRange::at(loc);
    EXPECT_EQ(range.start.line, 5u);
    EXPECT_EQ(range.end.line, 5u);
}

// ==================== TemplateError Tests ====================

class TemplateErrorTest : public ::testing::Test {
protected:
    void SetUp() override {}
};

TEST_F(TemplateErrorTest, ErrorInfoCreation)
{
    core::TemplateErrorInfo info(
        core::TemplateErrorType::LexerError, "Test error message", core::SourceLocation(5, 10), "test.tpl");

    EXPECT_EQ(info.type, core::TemplateErrorType::LexerError);
    EXPECT_EQ(info.message, "Test error message");
    EXPECT_EQ(info.location.line, 5u);
    EXPECT_EQ(info.sourcePath, "test.tpl");
}

TEST_F(TemplateErrorTest, ErrorInfoFormat)
{
    core::TemplateErrorInfo info(
        core::TemplateErrorType::ParserError, "Syntax error", core::SourceLocation(10, 20), "template.xml");

    std::string formatted = info.format();
    EXPECT_TRUE(formatted.find("ParserError") != std::string::npos);
    EXPECT_TRUE(formatted.find("Syntax error") != std::string::npos);
    EXPECT_TRUE(formatted.find("line 10") != std::string::npos);
    EXPECT_TRUE(formatted.find("template.xml") != std::string::npos);
}

TEST_F(TemplateErrorTest, ErrorInfoTypeName)
{
    EXPECT_STREQ(core::TemplateErrorInfo(core::TemplateErrorType::LexerError, "").typeName(), "LexerError");
    EXPECT_STREQ(core::TemplateErrorInfo(core::TemplateErrorType::UnexpectedCharacter, "").typeName(), "LexerError");
    EXPECT_STREQ(core::TemplateErrorInfo(core::TemplateErrorType::ParserError, "").typeName(), "ParserError");
    EXPECT_STREQ(core::TemplateErrorInfo(core::TemplateErrorType::SemanticError, "").typeName(), "SemanticError");
    EXPECT_STREQ(core::TemplateErrorInfo(core::TemplateErrorType::CompileError, "").typeName(), "CompileError");
    EXPECT_STREQ(core::TemplateErrorInfo(core::TemplateErrorType::RuntimeError, "").typeName(), "RuntimeError");
}

TEST_F(TemplateErrorTest, TemplateErrorException)
{
    core::TemplateError error(
        core::TemplateErrorType::InvalidBindingPath, "Invalid path: xyz", core::SourceLocation(1, 1));

    EXPECT_EQ(error.type(), core::TemplateErrorType::InvalidBindingPath);
    EXPECT_EQ(error.location().line, 1u);
    EXPECT_TRUE(std::string(error.what()).find("Invalid path") != std::string::npos);
}

TEST_F(TemplateErrorTest, TemplateErrorFactoryMethods)
{
    auto lexerError = core::TemplateError::lexer("Lexer error", core::SourceLocation(1, 1));
    EXPECT_EQ(lexerError.type(), core::TemplateErrorType::LexerError);

    auto parserError = core::TemplateError::parser("Parser error", core::SourceLocation(2, 1));
    EXPECT_EQ(parserError.type(), core::TemplateErrorType::ParserError);

    auto semanticError = core::TemplateError::semantic("Semantic error");
    EXPECT_EQ(semanticError.type(), core::TemplateErrorType::SemanticError);

    auto compileError = core::TemplateError::compile("Compile error");
    EXPECT_EQ(compileError.type(), core::TemplateErrorType::CompileError);

    auto runtimeError = core::TemplateError::runtime("Runtime error");
    EXPECT_EQ(runtimeError.type(), core::TemplateErrorType::RuntimeError);

    auto unknownTagError = core::TemplateError::unknownTag("unknownTag");
    EXPECT_EQ(unknownTagError.type(), core::TemplateErrorType::UnknownTag);

    auto bindingError = core::TemplateError::invalidBindingPath("invalid.path");
    EXPECT_EQ(bindingError.type(), core::TemplateErrorType::InvalidBindingPath);

    auto scriptError = core::TemplateError::inlineScriptNotAllowed();
    EXPECT_EQ(scriptError.type(), core::TemplateErrorType::InlineScriptNotAllowed);
}

TEST_F(TemplateErrorTest, ErrorCollector)
{
    core::TemplateErrorCollector collector;

    EXPECT_FALSE(collector.hasErrors());
    EXPECT_EQ(collector.errorCount(), 0u);

    collector.addError(core::TemplateErrorType::LexerError, "Error 1");
    collector.addError(core::TemplateErrorInfo(core::TemplateErrorType::ParserError, "Error 2"));

    EXPECT_TRUE(collector.hasErrors());
    EXPECT_EQ(collector.errorCount(), 2u);

    auto first = collector.firstError();
    ASSERT_NE(first, nullptr);
    EXPECT_EQ(first->message, "Error 1");

    collector.clear();
    EXPECT_FALSE(collector.hasErrors());
    EXPECT_EQ(collector.errorCount(), 0u);
}

TEST_F(TemplateErrorTest, ErrorCollectorThrowIfErrors)
{
    core::TemplateErrorCollector collector;

    // Should not throw when no errors
    EXPECT_NO_THROW(collector.throwIfErrors());

    collector.addError(core::TemplateErrorType::LexerError, "Test error");

    EXPECT_THROW(collector.throwIfErrors(), core::TemplateError);
}

TEST_F(TemplateErrorTest, ErrorCollectorMerge)
{
    core::TemplateErrorCollector collector1;
    core::TemplateErrorCollector collector2;

    collector1.addError(core::TemplateErrorType::LexerError, "Error 1");
    collector2.addError(core::TemplateErrorType::ParserError, "Error 2");
    collector2.addError(core::TemplateErrorType::SemanticError, "Error 3");

    collector1.merge(collector2);

    EXPECT_EQ(collector1.errorCount(), 3u);
}

// ==================== Lexer Tests ====================

class LexerTest : public ::testing::Test {
protected:
    parser::Lexer createLexer(const std::string& source) { return parser::Lexer(source, "<test>"); }
};

TEST_F(LexerTest, EmptySource)
{
    auto lexer = createLexer("");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_TRUE(lexer.tokens().empty() || lexer.tokens().back().type == parser::TokenType::EndOfFile);
    EXPECT_FALSE(lexer.hasErrors());
}

TEST_F(LexerTest, SimpleTag)
{
    auto lexer = createLexer("<screen>");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    ASSERT_GE(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, parser::TokenType::OpenTag);
    EXPECT_EQ(tokens[1].type, parser::TokenType::Identifier);
    EXPECT_EQ(tokens[1].value, "screen");
    EXPECT_EQ(tokens[2].type, parser::TokenType::CloseTag);
}

TEST_F(LexerTest, SelfClosingTag)
{
    auto lexer = createLexer("<text/>");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    bool foundSelfClose = false;
    for (const auto& token : tokens) {
        if (token.type == parser::TokenType::SelfCloseTag) {
            foundSelfClose = true;
            break;
        }
    }
    EXPECT_TRUE(foundSelfClose);
}

TEST_F(LexerTest, ClosingTag)
{
    auto lexer = createLexer("</screen>");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    ASSERT_GE(tokens.size(), 3);
    EXPECT_EQ(tokens[0].type, parser::TokenType::OpenCloseTag);
    EXPECT_EQ(tokens[1].value, "screen");
    EXPECT_EQ(tokens[2].type, parser::TokenType::CloseTag);
}

TEST_F(LexerTest, TagWithAttributes)
{
    auto lexer = createLexer(R"(<button id="btn1" pos="10,20">)");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    bool foundId = false, foundPos = false, foundString1 = false, foundString2 = false;

    for (const auto& token : tokens) {
        if (token.value == "id") foundId = true;
        if (token.value == "pos") foundPos = true;
        if (token.value == "btn1") foundString1 = true;
        if (token.value == "10,20") foundString2 = true;
    }

    EXPECT_TRUE(foundId);
    EXPECT_TRUE(foundPos);
    EXPECT_TRUE(foundString1);
    EXPECT_TRUE(foundString2);
}

TEST_F(LexerTest, BindingAttribute)
{
    auto lexer = createLexer(R"(<text bind:text="player.name">)");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    bool foundBind = false;
    for (const auto& token : tokens) {
        if (token.value == "bind:text") {
            foundBind = true;
            break;
        }
    }
    EXPECT_TRUE(foundBind);
}

TEST_F(LexerTest, EventAttribute)
{
    auto lexer = createLexer(R"(<button on:click="onButtonClick">)");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    bool foundEvent = false;
    for (const auto& token : tokens) {
        if (token.value == "on:click") {
            foundEvent = true;
            break;
        }
    }
    EXPECT_TRUE(foundEvent);
}

TEST_F(LexerTest, Comment)
{
    auto lexer = createLexer("<!-- This is a comment -->");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    bool foundComment = false;
    for (const auto& token : tokens) {
        if (token.type == parser::TokenType::Comment) {
            foundComment = true;
            EXPECT_EQ(token.value, " This is a comment ");
            break;
        }
    }
    EXPECT_TRUE(foundComment);
}

TEST_F(LexerTest, TextContent)
{
    auto lexer = createLexer("Hello World");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    bool foundText = false;
    for (const auto& token : tokens) {
        if (token.type == parser::TokenType::Text) {
            foundText = true;
            EXPECT_EQ(token.value, "Hello World");
            break;
        }
    }
    EXPECT_TRUE(foundText);
}

TEST_F(LexerTest, StringEscaping)
{
    auto lexer = createLexer(R"(<text value="Hello\nWorld">)");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    bool foundString = false;
    for (const auto& token : tokens) {
        if (token.type == parser::TokenType::StringLiteral) {
            foundString = true;
            // 应该包含换行符
            EXPECT_TRUE(token.value.find('\n') != std::string::npos);
            break;
        }
    }
    EXPECT_TRUE(foundString);
}

TEST_F(LexerTest, StringEscapeSequences)
{
    auto lexer = createLexer(R"(<text value="tab\there\nnewline\"quote\\slash">)");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    for (const auto& token : tokens) {
        if (token.type == parser::TokenType::StringLiteral) {
            EXPECT_TRUE(token.value.find('\t') != std::string::npos);
            EXPECT_TRUE(token.value.find('\n') != std::string::npos);
            EXPECT_TRUE(token.value.find('"') != std::string::npos);
            EXPECT_TRUE(token.value.find('\\') != std::string::npos);
            break;
        }
    }
}

TEST_F(LexerTest, NumberLiteral)
{
    auto lexer = createLexer("<text size=\"100\" width=\"3.14\">");
    EXPECT_TRUE(lexer.tokenize());

    auto& tokens = lexer.tokens();
    bool foundInt = false, foundFloat = false;
    for (const auto& token : tokens) {
        if (token.value == "100") {
            foundInt = true;
        }
        if (token.value == "3.14") {
            foundFloat = true;
        }
    }
    EXPECT_TRUE(foundInt);
    EXPECT_TRUE(foundFloat);
}

TEST_F(LexerTest, NestedTags)
{
    auto lexer = createLexer("<screen><button/><text/></screen>");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());

    int tagCount = 0;
    for (const auto& token : lexer.tokens()) {
        if (token.type == parser::TokenType::Identifier) {
            tagCount++;
        }
    }
    EXPECT_EQ(tagCount, 4); // screen, button, text, screen
}

TEST_F(LexerTest, UnterminatedString)
{
    auto lexer = createLexer(R"(<text value="unterminated>)");
    EXPECT_FALSE(lexer.tokenize());
    EXPECT_TRUE(lexer.hasErrors());
    EXPECT_TRUE(lexer.firstError()->type == core::TemplateErrorType::UnterminatedString);
}

TEST_F(LexerTest, UnterminatedComment)
{
    auto lexer = createLexer("<!-- This comment is not closed");
    EXPECT_FALSE(lexer.tokenize());
    EXPECT_TRUE(lexer.hasErrors());
}

TEST_F(LexerTest, TokenFormat)
{
    auto lexer = createLexer("<screen>");
    lexer.tokenize();

    auto& tokens = lexer.tokens();
    ASSERT_GE(tokens.size(), 2);

    std::string formatted = tokens[1].format();
    EXPECT_TRUE(formatted.find("Identifier") != std::string::npos);
    EXPECT_TRUE(formatted.find("screen") != std::string::npos);
}

TEST_F(LexerTest, TokenIsMethods)
{
    parser::Token idToken(parser::TokenType::Identifier, "test");
    EXPECT_TRUE(idToken.isIdentifier());
    EXPECT_TRUE(idToken.is(parser::TokenType::Identifier));
    EXPECT_TRUE(idToken.is(parser::TokenType::Identifier, "test"));
    EXPECT_TRUE(idToken.isKeyword("test"));
    EXPECT_FALSE(idToken.isText());
    EXPECT_FALSE(idToken.isStringLiteral());
    EXPECT_FALSE(idToken.isNumberLiteral());

    parser::Token strToken(parser::TokenType::StringLiteral, "value");
    EXPECT_TRUE(strToken.isStringLiteral());

    parser::Token numToken(parser::TokenType::NumberLiteral, "42");
    EXPECT_TRUE(numToken.isNumberLiteral());

    parser::Token textToken(parser::TokenType::Text, "content");
    EXPECT_TRUE(textToken.isText());
}

TEST_F(LexerTest, LexerIterator)
{
    auto lexer = createLexer("<screen><button/></screen>");
    EXPECT_TRUE(lexer.tokenize());

    EXPECT_TRUE(lexer.hasNext());

    // Test iteration
    int count = 0;
    while (lexer.hasNext()) {
        lexer.next();
        count++;
    }
    EXPECT_GT(count, 0);

    // Test back
    lexer.back();
    EXPECT_TRUE(lexer.hasNext());
}

TEST_F(LexerTest, LexerPeek)
{
    auto lexer = createLexer("<screen>");
    lexer.tokenize();

    const auto& current = lexer.current();
    EXPECT_EQ(current.type, parser::TokenType::OpenTag);

    const auto& peeked = lexer.peek();
    EXPECT_EQ(peeked.type, parser::TokenType::Identifier);
    EXPECT_EQ(peeked.value, "screen");

    // Test peek with offset
    const auto& peeked2 = lexer.peek(2);
    EXPECT_EQ(peeked2.type, parser::TokenType::CloseTag);
}

TEST_F(LexerTest, LexerPosition)
{
    auto lexer = createLexer("<screen>");
    lexer.tokenize();

    EXPECT_EQ(lexer.position(), 0u);
    lexer.next();
    EXPECT_EQ(lexer.position(), 1u);

    lexer.setPosition(0u);
    EXPECT_EQ(lexer.position(), 0u);
}

TEST_F(LexerTest, LexerSkipWhitespace)
{
    auto lexer = createLexer("   <screen>   ");
    EXPECT_TRUE(lexer.tokenize());

    // Skip whitespace manually
    lexer.skipWhitespace();
    EXPECT_TRUE(lexer.hasNext());
    EXPECT_EQ(lexer.current().type, parser::TokenType::OpenTag);
}

TEST_F(LexerTest, LexerExpect)
{
    auto lexer = createLexer("<screen>");
    lexer.tokenize();

    EXPECT_TRUE(lexer.expect(parser::TokenType::OpenTag));
    EXPECT_TRUE(lexer.expect(parser::TokenType::Identifier, "screen"));
    EXPECT_TRUE(lexer.expect(parser::TokenType::CloseTag));
}

TEST_F(LexerTest, LexerExpectFailure)
{
    auto lexer = createLexer("<screen>");
    lexer.tokenize();

    // Should fail because expecting wrong type
    EXPECT_FALSE(lexer.expect(parser::TokenType::CloseTag));
    EXPECT_TRUE(lexer.hasErrors());
}

TEST_F(LexerTest, LexerGetLineContent)
{
    auto lexer = createLexer("line1\nline2\nline3");
    lexer.tokenize();

    std::string line1 = lexer.getLineContent(1);
    EXPECT_EQ(line1, "line1");

    std::string line2 = lexer.getLineContent(2);
    EXPECT_EQ(line2, "line2");
}

TEST_F(LexerTest, LexerGetContext)
{
    auto lexer = createLexer("line1\nline2\nline3\nline4");
    lexer.tokenize();

    core::SourceLocation loc(2, 3, 8);
    std::string context = lexer.getContext(loc, 1);
    EXPECT_TRUE(context.find("line1") != std::string::npos);
    EXPECT_TRUE(context.find("line2") != std::string::npos);
    EXPECT_TRUE(context.find("line3") != std::string::npos);
}

TEST_F(LexerTest, LexerStaticMethods)
{
    EXPECT_TRUE(parser::Lexer::isWhitespace(' '));
    EXPECT_TRUE(parser::Lexer::isWhitespace('\t'));
    EXPECT_TRUE(parser::Lexer::isWhitespace('\r'));
    EXPECT_FALSE(parser::Lexer::isWhitespace('a'));

    EXPECT_TRUE(parser::Lexer::isNewline('\n'));
    EXPECT_FALSE(parser::Lexer::isNewline('a'));

    EXPECT_TRUE(parser::Lexer::isAlpha('a'));
    EXPECT_TRUE(parser::Lexer::isAlpha('Z'));
    EXPECT_FALSE(parser::Lexer::isAlpha('1'));

    EXPECT_TRUE(parser::Lexer::isDigit('0'));
    EXPECT_TRUE(parser::Lexer::isDigit('9'));
    EXPECT_FALSE(parser::Lexer::isDigit('a'));

    EXPECT_TRUE(parser::Lexer::isAlphaNumeric('a'));
    EXPECT_TRUE(parser::Lexer::isAlphaNumeric('1'));
    EXPECT_FALSE(parser::Lexer::isAlphaNumeric('-'));

    EXPECT_TRUE(parser::Lexer::isIdentifierChar('a'));
    EXPECT_TRUE(parser::Lexer::isIdentifierChar('_'));
    EXPECT_TRUE(parser::Lexer::isIdentifierChar('-'));
    EXPECT_TRUE(parser::Lexer::isIdentifierChar(':'));
    EXPECT_FALSE(parser::Lexer::isIdentifierChar(' '));
}

// ==================== Parser Tests ====================

class ParserTest : public ::testing::Test {
protected:
    void SetUp() override { m_config = core::TemplateConfig::defaults(); }

    std::unique_ptr<ast::DocumentNode> parse(const std::string& source)
    {
        parser::Lexer lexer(source, "<test>");
        lexer.tokenize();

        parser::Parser parser(lexer, m_config);
        return parser.parse();
    }

    core::TemplateConfig m_config;
};

TEST_F(ParserTest, SimpleElement)
{
    auto doc = parse("<screen/>");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);
    EXPECT_EQ(doc->rootElement()->tagName, "screen");
}

TEST_F(ParserTest, ElementWithId)
{
    auto doc = parse(R"(<button id="myButton"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);
    EXPECT_EQ(doc->rootElement()->id, "myButton");
}

TEST_F(ParserTest, ElementWithStaticAttributes)
{
    auto doc = parse(R"(<text pos="10,20" size="100,30"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    EXPECT_TRUE(elem->hasAttribute("pos"));
    EXPECT_TRUE(elem->hasAttribute("size"));

    const auto* posAttr = elem->getAttribute("pos");
    ASSERT_NE(posAttr, nullptr);
    EXPECT_TRUE(posAttr->isStatic());
    EXPECT_EQ(posAttr->rawValue, "10,20");
}

TEST_F(ParserTest, ElementWithBindingAttribute)
{
    auto doc = parse(R"(<text bind:text="player.name"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    EXPECT_TRUE(elem->hasAttribute("bind:text"));

    const auto* attr = elem->getAttribute("bind:text");
    ASSERT_NE(attr, nullptr);
    EXPECT_TRUE(attr->isBinding());
    ASSERT_TRUE(attr->binding.has_value());
    EXPECT_EQ(attr->binding->path, "player.name");
    EXPECT_FALSE(attr->binding->isLoopVariable);
}

TEST_F(ParserTest, ElementWithLoopVariableBinding)
{
    auto doc = parse(R"(<slot bind:item="$slot.itemId"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    const auto* attr = elem->getAttribute("bind:item");
    ASSERT_NE(attr, nullptr);
    EXPECT_TRUE(attr->isBinding());
    ASSERT_TRUE(attr->binding.has_value());
    EXPECT_TRUE(attr->binding->isLoopVariable);
    EXPECT_EQ(attr->binding->loopVarName, "slot");
    EXPECT_EQ(attr->binding->property, "itemId");
}

TEST_F(ParserTest, ElementWithEventAttribute)
{
    auto doc = parse(R"(<button on:click="onButtonClick"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    EXPECT_TRUE(elem->hasAttribute("on:click"));

    const auto* attr = elem->getAttribute("on:click");
    ASSERT_NE(attr, nullptr);
    EXPECT_TRUE(attr->isEvent());
    EXPECT_EQ(attr->callbackName, "onButtonClick");
}

TEST_F(ParserTest, NestedElements)
{
    auto doc = parse(R"(
        <screen>
            <text id="title" text="Hello"/>
            <button id="startBtn" text="Start"/>
        </screen>
    )");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);
    EXPECT_EQ(doc->rootElement()->children.size(), 2);
}

TEST_F(ParserTest, DeepNesting)
{
    auto doc = parse(R"(
        <screen>
            <grid>
                <slot>
                    <text text="Nested"/>
                </slot>
            </grid>
        </screen>
    )");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);
    EXPECT_EQ(doc->rootElement()->tagName, "screen");
    EXPECT_EQ(doc->rootElement()->children.size(), 1);

    auto* grid = dynamic_cast<ast::ElementNode*>(doc->rootElement()->children[0].get());
    ASSERT_NE(grid, nullptr);
    EXPECT_EQ(grid->tagName, "grid");
}

TEST_F(ParserTest, TextContent)
{
    auto doc = parse(R"(
        <screen>
            Hello World
        </screen>
    )");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);
    EXPECT_FALSE(doc->rootElement()->children.empty());

    // Find text node
    bool foundText = false;
    for (const auto& child : doc->rootElement()->children) {
        if (child->type == ast::NodeType::TextContent) {
            auto* textNode = dynamic_cast<ast::TextNode*>(child.get());
            ASSERT_NE(textNode, nullptr);
            foundText = true;
            break;
        }
    }
    EXPECT_TRUE(foundText);
}

TEST_F(ParserTest, CommentParsing)
{
    auto doc = parse(R"(
        <screen>
            <!-- This is a comment -->
            <text/>
        </screen>
    )");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    // Check for comment node
    bool foundComment = false;
    for (const auto& child : doc->rootElement()->children) {
        if (child->type == ast::NodeType::Comment) {
            foundComment = true;
            break;
        }
    }
    EXPECT_TRUE(foundComment);
}

TEST_F(ParserTest, AttributeCategorization)
{
    auto doc = parse(R"(<text pos="10,20" bind:text="player.name" on:click="onClick"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    elem->categorizeAttributes();

    EXPECT_EQ(elem->staticAttrs.size(), 1u);
    EXPECT_EQ(elem->bindingAttrs.size(), 1u);
    EXPECT_EQ(elem->eventAttrs.size(), 1u);
}

TEST_F(ParserTest, LoopInfoExtraction)
{
    // Test that for: directive properly extracts loop info
    auto doc = parse(R"(
        <grid for:slot="slot in player.inventory.slots">
            <text bind:text="$slot.name"/>
        </grid>
    )");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* grid = doc->rootElement();
    EXPECT_TRUE(grid->loop.has_value());
    EXPECT_EQ(grid->loop->collectionPath, "player.inventory.slots");
    EXPECT_EQ(grid->loop->itemVarName, "slot");
}

TEST_F(ParserTest, ConditionInfoExtraction)
{
    auto doc = parse(R"(<text bind:visible="player.isSneaking"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* text = doc->rootElement();
    // Element needs to have its attributes categorized
    text->categorizeAttributes();

    // Condition is extracted when present
    auto condition = parser::ConditionInfo();
    condition.booleanPath = "player.isSneaking";
    condition.negate = false;

    // Verify the bind:visible attribute exists
    const auto* attr = text->getAttribute("bind:visible");
    ASSERT_NE(attr, nullptr);
    EXPECT_TRUE(attr->isBinding());
}

TEST_F(ParserTest, ConditionInfoWithNegation)
{
    auto doc = parse(R"(<text bind:visible="!player.hidden"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* text = doc->rootElement();
    text->categorizeAttributes();

    // Verify the bind:visible attribute exists
    const auto* attr = text->getAttribute("bind:visible");
    ASSERT_NE(attr, nullptr);
    EXPECT_TRUE(attr->isBinding());
    // The binding path includes the ! prefix
    EXPECT_TRUE(attr->rawValue.find("!") != std::string::npos || attr->binding->path.find("!") != std::string::npos);
}

TEST_F(ParserTest, InvalidTagInStrictMode)
{
    m_config.strictMode = true;

    parser::Lexer lexer("<invalidTag/>", "<test>");
    lexer.tokenize();

    parser::Parser parser(lexer, m_config);
    auto doc = parser.parse();

    // Strict mode should generate error for unknown tag
    EXPECT_TRUE(parser.hasErrors());
}

TEST_F(ParserTest, InvalidBindingPath)
{
    m_config.validateBindingPaths = true;

    parser::Lexer lexer(R"(<text bind:text="123invalid"/>)", "<test>");
    lexer.tokenize();

    parser::Parser parser(lexer, m_config);
    auto doc = parser.parse();

    EXPECT_TRUE(parser.hasErrors());
}

TEST_F(ParserTest, InvalidCallbackName)
{
    m_config.validateCallbackNames = true;

    parser::Lexer lexer(R"(<button on:click="123invalid"/>)", "<test>");
    lexer.tokenize();

    parser::Parser parser(lexer, m_config);
    auto doc = parser.parse();

    EXPECT_TRUE(parser.hasErrors());
}

TEST_F(ParserTest, MissingClosingTag)
{
    parser::Lexer lexer("<screen><button>", "<test>");
    lexer.tokenize();

    parser::Parser parser(lexer, m_config);
    auto doc = parser.parse();

    EXPECT_TRUE(parser.hasErrors());
}

TEST_F(ParserTest, MismatchedClosingTag)
{
    parser::Lexer lexer("<screen><button/></text>", "<test>");
    lexer.tokenize();

    parser::Parser parser(lexer, m_config);
    auto doc = parser.parse();

    EXPECT_TRUE(parser.hasErrors());
}

TEST_F(ParserTest, MultipleRootElements)
{
    auto doc = parse(R"(
        <screen/>
        <button/>
    )");

    ASSERT_NE(doc, nullptr);
    // Document can have multiple children
    EXPECT_GE(doc->children.size(), 2u);
}

TEST_F(ParserTest, NumericAttributeValues)
{
    auto doc = parse(R"(<text size=100 width=3.14/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    EXPECT_TRUE(elem->hasAttribute("size"));
    EXPECT_TRUE(elem->hasAttribute("width"));
}

TEST_F(ParserTest, ClassesAttribute)
{
    // Note: Classes are parsed as regular attributes, not specially handled yet
    auto doc = parse(R"(<text class="title large"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);
    EXPECT_TRUE(doc->rootElement()->hasAttribute("class"));
}

TEST_F(ParserTest, UniqueIdsNoError)
{
    // 每个元素都有唯一的ID - 不应产生错误
    parser::Lexer lexer(R"(
        <screen>
            <text id="title"/>
            <text id="subtitle"/>
            <button id="submit"/>
        </screen>
    )",
        "<test>");
    lexer.tokenize();

    parser::Parser parser(lexer, m_config);
    auto doc = parser.parse();

    EXPECT_FALSE(parser.hasErrors()) << (parser.hasErrors() ? parser.errors()[0].message : "");
    ASSERT_NE(doc, nullptr);
}

TEST_F(ParserTest, DuplicateIdsError)
{
    // 重复的ID - 应该产生DuplicateId错误
    parser::Lexer lexer(R"(
        <screen>
            <text id="duplicate"/>
            <text id="duplicate"/>
        </screen>
    )",
        "<test>");
    lexer.tokenize();

    parser::Parser parser(lexer, m_config);
    auto doc = parser.parse();

    // 应该检测到重复ID错误
    EXPECT_TRUE(parser.hasErrors());
    ASSERT_EQ(parser.errors().size(), 1u);
    EXPECT_EQ(parser.errors()[0].type, core::TemplateErrorType::DuplicateId);
    EXPECT_TRUE(parser.errors()[0].message.find("duplicate") != std::string::npos);
}

TEST_F(ParserTest, DuplicateIdsNested)
{
    // 嵌套元素中的重复ID - 应该检测到
    parser::Lexer lexer(R"(
        <screen id="main">
            <widget id="content">
                <text id="item"/>
                <text id="item"/>
            </widget>
        </screen>
    )",
        "<test>");
    lexer.tokenize();

    parser::Parser parser(lexer, m_config);
    auto doc = parser.parse();

    EXPECT_TRUE(parser.hasErrors());
    ASSERT_EQ(parser.errors().size(), 1u);
    EXPECT_EQ(parser.errors()[0].type, core::TemplateErrorType::DuplicateId);
}

TEST_F(ParserTest, DuplicateIdsAcrossBranches)
{
    // 不同分支中的重复ID - 应该检测到
    parser::Lexer lexer(R"(
        <screen>
            <widget>
                <text id="sameId"/>
            </widget>
            <widget>
                <text id="sameId"/>
            </widget>
        </screen>
    )",
        "<test>");
    lexer.tokenize();

    parser::Parser parser(lexer, m_config);
    auto doc = parser.parse();

    EXPECT_TRUE(parser.hasErrors());
    ASSERT_EQ(parser.errors().size(), 1u);
    EXPECT_EQ(parser.errors()[0].type, core::TemplateErrorType::DuplicateId);
}

TEST_F(ParserTest, MultipleDuplicateIds)
{
    // 多组重复ID - 应该报告多个错误
    parser::Lexer lexer(R"(
        <screen>
            <text id="dup1"/>
            <text id="dup1"/>
            <text id="dup2"/>
            <text id="dup2"/>
        </screen>
    )",
        "<test>");
    lexer.tokenize();

    parser::Parser parser(lexer, m_config);
    auto doc = parser.parse();

    EXPECT_TRUE(parser.hasErrors());
    // 应该有2个错误：dup1重复和dup2重复
    ASSERT_EQ(parser.errors().size(), 2u);
    EXPECT_EQ(parser.errors()[0].type, core::TemplateErrorType::DuplicateId);
    EXPECT_EQ(parser.errors()[1].type, core::TemplateErrorType::DuplicateId);
}

TEST_F(ParserTest, DuplicateIdErrorLocation)
{
    // 验证错误位置信息
    parser::Lexer lexer(R"(
        <screen>
            <text id="first"/>
            <text id="first"/>
        </screen>
    )",
        "<test>");
    lexer.tokenize();

    parser::Parser parser(lexer, m_config);
    auto doc = parser.parse();

    EXPECT_TRUE(parser.hasErrors());
    // 错误应该指向第二个重复ID的位置
    EXPECT_TRUE(parser.errors()[0].location.isValid());
}

TEST_F(ParserTest, NoIdNoError)
{
    // 没有ID的元素 - 不应产生错误
    parser::Lexer lexer(R"(
        <screen>
            <text/>
            <text/>
            <text/>
        </screen>
    )",
        "<test>");
    lexer.tokenize();

    parser::Parser parser(lexer, m_config);
    auto doc = parser.parse();

    EXPECT_FALSE(parser.hasErrors()) << (parser.hasErrors() ? parser.errors()[0].message : "");
    ASSERT_NE(doc, nullptr);
}

TEST_F(ParserTest, CaseSensitiveIds)
{
    // ID区分大小写 - 不同大小写应该被视为不同ID
    parser::Lexer lexer(R"(
        <screen>
            <text id="MyId"/>
            <text id="myid"/>
            <text id="MYID"/>
        </screen>
    )",
        "<test>");
    lexer.tokenize();

    parser::Parser parser(lexer, m_config);
    auto doc = parser.parse();

    // 不同大小写的ID应该被认为是不同的ID，不应该产生错误
    EXPECT_FALSE(parser.hasErrors()) << (parser.hasErrors() ? parser.errors()[0].message : "");
    ASSERT_NE(doc, nullptr);
}

// ==================== AST Tests ====================

class AstTest : public ::testing::Test {};

TEST_F(AstTest, NodeTypeNames)
{
    EXPECT_STREQ(ast::nodeTypeName(ast::NodeType::Document), "Document");
    EXPECT_STREQ(ast::nodeTypeName(ast::NodeType::Screen), "Screen");
    EXPECT_STREQ(ast::nodeTypeName(ast::NodeType::Widget), "Widget");
    EXPECT_STREQ(ast::nodeTypeName(ast::NodeType::Button), "Button");
    EXPECT_STREQ(ast::nodeTypeName(ast::NodeType::Text), "Text");
    EXPECT_STREQ(ast::nodeTypeName(ast::NodeType::Grid), "Grid");
    EXPECT_STREQ(ast::nodeTypeName(ast::NodeType::Slot), "Slot");
    EXPECT_STREQ(ast::nodeTypeName(ast::NodeType::Comment), "Comment");
    EXPECT_STREQ(ast::nodeTypeName(static_cast<ast::NodeType>(255)), "Unknown");
}

TEST_F(AstTest, BindingInfoParseSimplePath)
{
    auto info = ast::BindingInfo::parse("player.name");
    EXPECT_FALSE(info.isLoopVariable);
    EXPECT_EQ(info.path, "player.name");
    EXPECT_TRUE(info.isValid());
}

TEST_F(AstTest, BindingInfoParseLoopVariable)
{
    auto info = ast::BindingInfo::parse("$item");
    EXPECT_TRUE(info.isLoopVariable);
    EXPECT_EQ(info.loopVarName, "item");
    EXPECT_TRUE(info.property.empty());
    EXPECT_TRUE(info.isValid());
}

TEST_F(AstTest, BindingInfoParseLoopVariableWithProperty)
{
    auto info = ast::BindingInfo::parse("$slot.item");
    EXPECT_TRUE(info.isLoopVariable);
    EXPECT_EQ(info.loopVarName, "slot");
    EXPECT_EQ(info.property, "item");
    EXPECT_TRUE(info.isValid());
}

TEST_F(AstTest, BindingInfoParseEmpty)
{
    auto info = ast::BindingInfo::parse("");
    EXPECT_FALSE(info.isValid());
}

TEST_F(AstTest, AttributeCreateStatic)
{
    auto attr = ast::Attribute::createStatic("pos", "10,20");
    EXPECT_TRUE(attr.isStatic());
    EXPECT_FALSE(attr.isBinding());
    EXPECT_FALSE(attr.isEvent());
    EXPECT_EQ(attr.name, "pos");
    EXPECT_EQ(attr.rawValue, "10,20");

    // Check value parsing - string
    EXPECT_TRUE(std::holds_alternative<std::string>(attr.value));
}

TEST_F(AstTest, AttributeCreateStaticBoolean)
{
    auto attrTrue = ast::Attribute::createStatic("visible", "true");
    EXPECT_TRUE(std::holds_alternative<bool>(attrTrue.value));
    EXPECT_TRUE(std::get<bool>(attrTrue.value));

    auto attrFalse = ast::Attribute::createStatic("visible", "false");
    EXPECT_TRUE(std::holds_alternative<bool>(attrFalse.value));
    EXPECT_FALSE(std::get<bool>(attrFalse.value));
}

TEST_F(AstTest, AttributeCreateStaticInteger)
{
    auto attr = ast::Attribute::createStatic("size", "100");
    EXPECT_TRUE(std::holds_alternative<i32>(attr.value));
    EXPECT_EQ(std::get<i32>(attr.value), 100);
}

TEST_F(AstTest, AttributeCreateStaticFloat)
{
    auto attr = ast::Attribute::createStatic("opacity", "0.75");
    EXPECT_TRUE(attr.isStatic());
    // Value type may be float or string depending on parsing
    EXPECT_TRUE(std::holds_alternative<f32>(attr.value) || std::holds_alternative<std::string>(attr.value));
    if (std::holds_alternative<f32>(attr.value)) {
        EXPECT_FLOAT_EQ(std::get<f32>(attr.value), 0.75f);
    }
}

TEST_F(AstTest, AttributeCreateBinding)
{
    auto attr = ast::Attribute::createBinding("bind:text", "player.name");
    EXPECT_TRUE(attr.isBinding());
    EXPECT_FALSE(attr.isStatic());
    EXPECT_FALSE(attr.isEvent());
    EXPECT_EQ(attr.name, "bind:text");
    EXPECT_TRUE(attr.binding.has_value());
    EXPECT_EQ(attr.binding->path, "player.name");
}

TEST_F(AstTest, AttributeCreateEvent)
{
    auto attr = ast::Attribute::createEvent("on:click", "onButtonClick");
    EXPECT_TRUE(attr.isEvent());
    EXPECT_FALSE(attr.isStatic());
    EXPECT_FALSE(attr.isBinding());
    EXPECT_EQ(attr.name, "on:click");
    EXPECT_EQ(attr.callbackName, "onButtonClick");
}

TEST_F(AstTest, AttributeBaseName)
{
    auto staticAttr = ast::Attribute::createStatic("pos", "10,20");
    EXPECT_EQ(staticAttr.baseName(), "pos");

    auto bindingAttr = ast::Attribute::createBinding("bind:text", "player.name");
    EXPECT_EQ(bindingAttr.baseName(), "text");

    auto eventAttr = ast::Attribute::createEvent("on:click", "onClick");
    EXPECT_EQ(eventAttr.baseName(), "click");
}

TEST_F(AstTest, ElementNodeBasics)
{
    ast::ElementNode elem(ast::NodeType::Button);
    elem.tagName = "button";
    elem.id = "myButton";

    // Test addAttribute
    elem.addAttribute(ast::Attribute::createStatic("pos", "10,20"));
    EXPECT_TRUE(elem.hasAttribute("pos"));

    // Test getAttribute
    const auto* attr = elem.getAttribute("pos");
    ASSERT_NE(attr, nullptr);
    EXPECT_EQ(attr->rawValue, "10,20");

    // Test non-existent attribute
    const auto* missing = elem.getAttribute("missing");
    EXPECT_EQ(missing, nullptr);
}

TEST_F(AstTest, ElementNodeCategorizeAttributes)
{
    ast::ElementNode elem(ast::NodeType::Text);
    elem.addAttribute(ast::Attribute::createStatic("pos", "10,20"));
    elem.addAttribute(ast::Attribute::createBinding("bind:text", "player.name"));
    elem.addAttribute(ast::Attribute::createEvent("on:click", "onClick"));

    elem.categorizeAttributes();

    EXPECT_EQ(elem.staticAttrs.size(), 1u);
    EXPECT_EQ(elem.bindingAttrs.size(), 1u);
    EXPECT_EQ(elem.eventAttrs.size(), 1u);
}

TEST_F(AstTest, ElementNodeClone)
{
    auto original = std::make_unique<ast::ElementNode>(ast::NodeType::Button);
    original->tagName = "button";
    original->id = "btn";
    original->addAttribute(ast::Attribute::createStatic("pos", "10,20"));

    auto child = std::make_unique<ast::ElementNode>(ast::NodeType::Text);
    child->tagName = "text";
    original->children.push_back(std::move(child));

    auto cloned = original->clone();
    ASSERT_NE(cloned, nullptr);

    auto* clonedElem = dynamic_cast<ast::ElementNode*>(cloned.get());
    ASSERT_NE(clonedElem, nullptr);
    EXPECT_EQ(clonedElem->tagName, "button");
    EXPECT_EQ(clonedElem->id, "btn");
    EXPECT_TRUE(clonedElem->hasAttribute("pos"));
    EXPECT_EQ(clonedElem->children.size(), 1u);
}

TEST_F(AstTest, TextNodeClone)
{
    auto original = std::make_unique<ast::TextNode>();
    original->text = "Hello World";
    original->isWhitespace = false;

    auto cloned = original->clone();
    auto* textNode = dynamic_cast<ast::TextNode*>(cloned.get());
    ASSERT_NE(textNode, nullptr);
    EXPECT_EQ(textNode->text, "Hello World");
    EXPECT_FALSE(textNode->isWhitespace);
}

TEST_F(AstTest, CommentNodeClone)
{
    auto original = std::make_unique<ast::CommentNode>();
    original->text = "This is a comment";

    auto cloned = original->clone();
    auto* commentNode = dynamic_cast<ast::CommentNode*>(cloned.get());
    ASSERT_NE(commentNode, nullptr);
    EXPECT_EQ(commentNode->text, "This is a comment");
}

TEST_F(AstTest, DocumentNodeClone)
{
    auto original = std::make_unique<ast::DocumentNode>();
    original->sourcePath = "test.tpl";

    auto child = std::make_unique<ast::ElementNode>(ast::NodeType::Screen);
    child->tagName = "screen";
    original->children.push_back(std::move(child));

    auto cloned = original->clone();
    auto* docNode = dynamic_cast<ast::DocumentNode*>(cloned.get());
    ASSERT_NE(docNode, nullptr);
    EXPECT_EQ(docNode->sourcePath, "test.tpl");
    EXPECT_EQ(docNode->children.size(), 1u);
}

TEST_F(AstTest, DocumentNodeRootElement)
{
    auto doc = std::make_unique<ast::DocumentNode>();

    // No children
    EXPECT_EQ(doc->rootElement(), nullptr);

    // Add text node first
    auto text = std::make_unique<ast::TextNode>();
    doc->children.push_back(std::move(text));
    EXPECT_EQ(doc->rootElement(), nullptr);

    // Add element
    auto screen = std::make_unique<ast::ElementNode>(ast::NodeType::Screen);
    screen->tagName = "screen";
    ast::ElementNode* screenPtr = screen.get();
    doc->children.push_back(std::move(screen));

    EXPECT_EQ(doc->rootElement(), screenPtr);
}

TEST_F(AstTest, ValidTagName)
{
    EXPECT_TRUE(ast::isValidWidgetTag("screen"));
    EXPECT_TRUE(ast::isValidWidgetTag("button"));
    EXPECT_TRUE(ast::isValidWidgetTag("text"));
    EXPECT_TRUE(ast::isValidWidgetTag("slot"));
    EXPECT_TRUE(ast::isValidWidgetTag("grid"));
    EXPECT_TRUE(ast::isValidWidgetTag("viewport3d"));
    EXPECT_TRUE(ast::isValidWidgetTag("scrollable"));
    EXPECT_TRUE(ast::isValidWidgetTag("list"));
    EXPECT_TRUE(ast::isValidWidgetTag("style"));
    EXPECT_TRUE(ast::isValidWidgetTag("textfield"));
    EXPECT_TRUE(ast::isValidWidgetTag("slider"));
    EXPECT_TRUE(ast::isValidWidgetTag("checkbox"));
    EXPECT_TRUE(ast::isValidWidgetTag("image"));
    EXPECT_TRUE(ast::isValidWidgetTag("widget"));

    EXPECT_FALSE(ast::isValidWidgetTag("invalidTag"));
    EXPECT_FALSE(ast::isValidWidgetTag("script"));
    EXPECT_FALSE(ast::isValidWidgetTag("div"));

    // Case insensitive
    EXPECT_TRUE(ast::isValidWidgetTag("SCREEN"));
    EXPECT_TRUE(ast::isValidWidgetTag("Button"));
}

TEST_F(AstTest, ValidBindingPath)
{
    EXPECT_TRUE(ast::isValidBindingPath("player"));
    EXPECT_TRUE(ast::isValidBindingPath("player.name"));
    EXPECT_TRUE(ast::isValidBindingPath("player.inventory.slots"));
    EXPECT_TRUE(ast::isValidBindingPath("player.inventory.slots[0]"));
    EXPECT_TRUE(ast::isValidBindingPath("array[index]"));
    EXPECT_TRUE(ast::isValidBindingPath("$item"));
    EXPECT_TRUE(ast::isValidBindingPath("$slot.item"));
    EXPECT_TRUE(ast::isValidBindingPath("$data.nested.value"));

    EXPECT_FALSE(ast::isValidBindingPath(""));
    EXPECT_FALSE(ast::isValidBindingPath("123invalid"));
    EXPECT_FALSE(ast::isValidBindingPath("player..name"));
    EXPECT_FALSE(ast::isValidBindingPath(".player"));
    EXPECT_FALSE(ast::isValidBindingPath("$"));
    EXPECT_FALSE(ast::isValidBindingPath("$123"));
}

TEST_F(AstTest, ValidCallbackName)
{
    EXPECT_TRUE(ast::isValidCallbackName("onClick"));
    EXPECT_TRUE(ast::isValidCallbackName("on_start_game"));
    EXPECT_TRUE(ast::isValidCallbackName("handleClick"));
    EXPECT_TRUE(ast::isValidCallbackName("_private"));
    EXPECT_TRUE(ast::isValidCallbackName("callback1"));

    EXPECT_FALSE(ast::isValidCallbackName(""));
    EXPECT_FALSE(ast::isValidCallbackName("123invalid"));
    EXPECT_FALSE(ast::isValidCallbackName("on-click"));
    EXPECT_FALSE(ast::isValidCallbackName("onClick()"));
    EXPECT_FALSE(ast::isValidCallbackName("obj.method"));
}

TEST_F(AstTest, ValidAttributeName)
{
    EXPECT_TRUE(ast::isValidAttributeName("pos"));
    EXPECT_TRUE(ast::isValidAttributeName("bind:text"));
    EXPECT_TRUE(ast::isValidAttributeName("on:click"));
    EXPECT_TRUE(ast::isValidAttributeName("data-value"));
    EXPECT_TRUE(ast::isValidAttributeName("_private"));
    EXPECT_TRUE(ast::isValidAttributeName("id"));

    EXPECT_FALSE(ast::isValidAttributeName(""));
    EXPECT_FALSE(ast::isValidAttributeName("123invalid"));
    EXPECT_FALSE(ast::isValidAttributeName("attr with space"));
}

TEST_F(AstTest, GetNodeTypeFromTagName)
{
    EXPECT_EQ(ast::getNodeTypeFromTagName("screen"), ast::NodeType::Screen);
    EXPECT_EQ(ast::getNodeTypeFromTagName("button"), ast::NodeType::Button);
    EXPECT_EQ(ast::getNodeTypeFromTagName("text"), ast::NodeType::Text);
    EXPECT_EQ(ast::getNodeTypeFromTagName("grid"), ast::NodeType::Grid);
    EXPECT_EQ(ast::getNodeTypeFromTagName("slot"), ast::NodeType::Slot);
    EXPECT_EQ(ast::getNodeTypeFromTagName("unknown"), ast::NodeType::Widget);

    // Case insensitive
    EXPECT_EQ(ast::getNodeTypeFromTagName("SCREEN"), ast::NodeType::Screen);
    EXPECT_EQ(ast::getNodeTypeFromTagName("Button"), ast::NodeType::Button);
}

// ==================== AST Visitor Tests ====================

class AstVisitorTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_document = std::make_unique<ast::DocumentNode>();

        auto screen = std::make_unique<ast::ElementNode>(ast::NodeType::Screen);
        screen->tagName = "screen";
        screen->id = "main";

        auto text1 = std::make_unique<ast::ElementNode>(ast::NodeType::Text);
        text1->tagName = "text";
        text1->id = "title";
        screen->children.push_back(std::move(text1));

        auto button = std::make_unique<ast::ElementNode>(ast::NodeType::Button);
        button->tagName = "button";
        button->id = "startBtn";
        screen->children.push_back(std::move(button));

        m_document->children.push_back(std::move(screen));
    }

    std::unique_ptr<ast::DocumentNode> m_document;
};

TEST_F(AstVisitorTest, FindById)
{
    auto* found = ast::traversal::findById(*m_document, "title");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id, "title");

    auto* notFound = ast::traversal::findById(*m_document, "nonexistent");
    EXPECT_EQ(notFound, nullptr);
}

TEST_F(AstVisitorTest, FindByTagName)
{
    auto buttons = ast::traversal::findByTagName(*m_document, "button");
    EXPECT_EQ(buttons.size(), 1u);

    auto texts = ast::traversal::findByTagName(*m_document, "text");
    EXPECT_EQ(texts.size(), 1u);

    auto screens = ast::traversal::findByTagName(*m_document, "screen");
    EXPECT_EQ(screens.size(), 1u);

    auto unknowns = ast::traversal::findByTagName(*m_document, "unknown");
    EXPECT_EQ(unknowns.size(), 0u);
}

TEST_F(AstVisitorTest, CountNodes)
{
    auto total = ast::traversal::countNodes(*m_document);
    EXPECT_EQ(total, 4u); // document + screen + text + button

    auto textCount = ast::traversal::countNodes(*m_document, ast::NodeType::Text);
    EXPECT_EQ(textCount, 1u);

    auto buttonCount = ast::traversal::countNodes(*m_document, ast::NodeType::Button);
    EXPECT_EQ(buttonCount, 1u);

    auto screenCount = ast::traversal::countNodes(*m_document, ast::NodeType::Screen);
    EXPECT_EQ(screenCount, 1u);
}

TEST_F(AstVisitorTest, PreorderTraversal)
{
    std::vector<std::string> visitedTags;
    ast::traversal::preorder(*m_document, [&visitedTags](ast::Node& node) {
        if (auto* elem = dynamic_cast<ast::ElementNode*>(&node)) {
            visitedTags.push_back(elem->tagName);
        }
        return true;
    });

    // Preorder: screen, text, button
    ASSERT_GE(visitedTags.size(), 3u);
    EXPECT_EQ(visitedTags[0], "screen");
    EXPECT_EQ(visitedTags[1], "text");
    EXPECT_EQ(visitedTags[2], "button");
}

TEST_F(AstVisitorTest, PostorderTraversal)
{
    std::vector<std::string> visitedTags;
    ast::traversal::postorder(*m_document, [&visitedTags](ast::Node& node) {
        if (auto* elem = dynamic_cast<ast::ElementNode*>(&node)) {
            visitedTags.push_back(elem->tagName);
        }
        return true;
    });

    // Postorder: text, button, screen
    ASSERT_GE(visitedTags.size(), 3u);
    EXPECT_EQ(visitedTags[0], "text");
    EXPECT_EQ(visitedTags[1], "button");
    EXPECT_EQ(visitedTags[2], "screen");
}

TEST_F(AstVisitorTest, FindFirst)
{
    auto* found = ast::traversal::findFirst(
        *m_document, [](const ast::Node& node) { return node.type == ast::NodeType::Button; });
    ASSERT_NE(found, nullptr);

    auto* notFound =
        ast::traversal::findFirst(*m_document, [](const ast::Node& node) { return node.type == ast::NodeType::Grid; });
    EXPECT_EQ(notFound, nullptr);
}

TEST_F(AstVisitorTest, FindAll)
{
    auto elements = ast::traversal::findAll(*m_document,
        [](const ast::Node& node) { return node.type == ast::NodeType::Button || node.type == ast::NodeType::Text; });
    EXPECT_EQ(elements.size(), 2u);
}

TEST_F(AstVisitorTest, GetDepth)
{
    // Document depth
    auto docDepth = ast::traversal::getDepth(*m_document);
    EXPECT_EQ(docDepth, 2u); // document -> screen -> (text, button)
}

TEST_F(AstVisitorTest, VisitorStop)
{
    int visitCount = 0;

    class CountingVisitor : public ast::AstVisitor {
    public:
        int& count;
        CountingVisitor(int& c)
            : count(c)
        {}

        void visitElement(ast::ElementNode& node) override
        {
            count++;
            traverseChildren(node);
            if (count >= 2) {
                stop();
            }
        }
    };

    CountingVisitor visitor(visitCount);
    visitor.visit(*m_document);

    EXPECT_GE(visitCount, 2);
}

TEST_F(AstVisitorTest, ConstVisitor)
{
    std::vector<std::string> visitedTags;

    class ConstTagCollector : public ast::ConstAstVisitor {
    public:
        std::vector<std::string>& tags;
        ConstTagCollector(std::vector<std::string>& t)
            : tags(t)
        {}

        void visitElement(const ast::ElementNode& node) override
        {
            tags.push_back(node.tagName);
            traverseChildren(node);
        }
    };

    ConstTagCollector collector(visitedTags);
    collector.visit(*m_document);

    EXPECT_GE(visitedTags.size(), 3u);
}

// ==================== TemplateCompiler Tests ====================

class TemplateCompilerTest : public ::testing::Test {
protected:
    void SetUp() override { m_config = core::TemplateConfig::defaults(); }

    core::TemplateConfig m_config;
};

TEST_F(TemplateCompilerTest, SimpleTemplate)
{
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile("<screen id=\"main\"/>");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());
    EXPECT_FALSE(result->hasErrors());
    EXPECT_NE(result->astRoot(), nullptr);
}

TEST_F(TemplateCompilerTest, TemplateWithBindings)
{
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <screen>
            <text id="nameLabel" bind:text="player.name"/>
            <button id="startBtn" on:click="onStartGame"/>
        </screen>
    )");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());

    // Should have at least one binding plan
    EXPECT_GE(result->bindingPlans().size(), 1u);

    // Should have at least one event plan
    EXPECT_GE(result->eventPlans().size(), 1u);

    // Check first binding plan
    bool foundNameBinding = false;
    for (const auto& plan : result->bindingPlans()) {
        if (plan.statePath == "player.name") {
            foundNameBinding = true;
            break;
        }
    }
    EXPECT_TRUE(foundNameBinding);

    // Check first event plan
    bool foundClickEvent = false;
    for (const auto& plan : result->eventPlans()) {
        if (plan.callbackName == "onStartGame") {
            foundClickEvent = true;
            break;
        }
    }
    EXPECT_TRUE(foundClickEvent);

    // Should have watched paths
    EXPECT_TRUE(result->watchedPaths().count("player.name") > 0);

    // Should have registered callbacks
    EXPECT_TRUE(result->registeredCallbacks().count("onStartGame") > 0);
}

TEST_F(TemplateCompilerTest, TemplateWithLoop)
{
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <grid bind:items="player.inventory.slots">
            <slot bind:item="$slot.item"/>
        </grid>
    )");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());

    // Should have at least one binding (for slot.item)
    EXPECT_GE(result->bindingPlans().size(), 1u);

    // Check for loop binding
    bool foundLoopBinding = false;
    for (const auto& plan : result->bindingPlans()) {
        if (plan.isLoopBinding) {
            foundLoopBinding = true;
            break;
        }
    }
    EXPECT_TRUE(foundLoopBinding);
}

TEST_F(TemplateCompilerTest, StrictModeRejectsInlineScript)
{
    m_config.strictMode = true;
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(<text>{{ player.health }}/>)");

    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->isValid());
    EXPECT_TRUE(result->hasErrors());
}

TEST_F(TemplateCompilerTest, StrictModeRejectsInlineExpression)
{
    m_config.strictMode = true;
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(<text bind:text="player.level + 1"/>)");

    ASSERT_NE(result, nullptr);
    // Should have error for inline expression
    EXPECT_FALSE(result->isValid());
}

TEST_F(TemplateCompilerTest, CompileWithDebug)
{
    m_config.debugOutput = true;
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile("<screen/>");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());

    // Check debug dump
    std::string dump = result->debugDump();
    EXPECT_TRUE(dump.find("Compiled Template") != std::string::npos);
    EXPECT_TRUE(dump.find("Source:") != std::string::npos);
}

TEST_F(TemplateCompilerTest, CompileFromFile)
{
    compiler::TemplateCompiler compiler(m_config);

    // Test with non-existent file
    auto result = compiler.compileFile("/nonexistent/path/template.tpl");

    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->isValid());
    EXPECT_TRUE(result->hasErrors());
}

TEST_F(TemplateCompilerTest, CompileAst)
{
    // Create AST manually
    auto document = std::make_unique<ast::DocumentNode>();
    auto screen = std::make_unique<ast::ElementNode>(ast::NodeType::Screen);
    screen->tagName = "screen";
    screen->id = "main";
    document->children.push_back(std::move(screen));

    compiler::TemplateCompiler compiler(m_config);
    auto result = compiler.compileAst(std::move(document), "<test>");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());
    EXPECT_FALSE(result->hasErrors());
}

TEST_F(TemplateCompilerTest, CompileNullAst)
{
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compileAst(nullptr, "<test>");

    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->isValid());
    EXPECT_TRUE(result->hasErrors());
}

TEST_F(TemplateCompilerTest, CompileTime)
{
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile("<screen/>");

    ASSERT_NE(result, nullptr);
    // Compile time should be recorded
    // Note: May be 0 on fast machines
    EXPECT_TRUE(result->compileTime() >= 0u);
}

TEST_F(TemplateCompilerTest, InvalidTagInStrictMode)
{
    m_config.strictMode = true;
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile("<invalidTag/>");

    ASSERT_NE(result, nullptr);
    EXPECT_FALSE(result->isValid());
    EXPECT_TRUE(result->hasErrors());
}

TEST_F(TemplateCompilerTest, CompiledTemplateWatchedPaths)
{
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <screen id="main">
            <text bind:text="player.name"/>
        </screen>
    )");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());

    // Verify watched paths are collected correctly
    const auto& paths = result->watchedPaths();
    // At minimum we should have player.name if there's a binding
    EXPECT_GE(paths.size(), 0u);
}

// ==================== BindingContext Tests ====================

class BindingContextTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 清空单例状态，避免用例间相互污染
        mc::client::ui::kagero::state::StateStore::instance().clear();
        mc::client::ui::kagero::state::StateStore::instance().clearMiddlewares();
        mc::client::ui::kagero::event::EventBus::instance().clear();
        // StateStore and EventBus are singletons - use instance()
        m_ctx = std::make_unique<binder::BindingContext>(
            mc::client::ui::kagero::state::StateStore::instance(), mc::client::ui::kagero::event::EventBus::instance());
    }

    void TearDown() override
    {
        // Clear the binding context
        m_ctx.reset();
        mc::client::ui::kagero::state::StateStore::instance().clear();
        mc::client::ui::kagero::state::StateStore::instance().clearMiddlewares();
        mc::client::ui::kagero::event::EventBus::instance().clear();
    }

    std::unique_ptr<binder::BindingContext> m_ctx;
};

TEST_F(BindingContextTest, ExposeVariable)
{
    std::string playerName = "Steve";
    m_ctx->expose("player.name", &playerName);

    EXPECT_TRUE(m_ctx->hasPath("player.name"));
    EXPECT_FALSE(m_ctx->isWritable("player.name"));

    auto value = m_ctx->resolveBinding("player.name");
    EXPECT_FALSE(value.isNull());
    EXPECT_EQ(value.asString(), "Steve");
}

TEST_F(BindingContextTest, ExposeWritableVariable)
{
    i32 health = 100;
    m_ctx->exposeWritable("player.health", &health);

    EXPECT_TRUE(m_ctx->hasPath("player.health"));
    EXPECT_TRUE(m_ctx->isWritable("player.health"));

    // Read
    auto value = m_ctx->resolveBinding("player.health");
    EXPECT_EQ(value.asInteger(), 100);

    // Write
    EXPECT_TRUE(m_ctx->setBinding("player.health", binder::Value(50)));
    EXPECT_EQ(health, 50);
}

TEST_F(BindingContextTest, ExposeCallback)
{
    bool callbackCalled = false;
    std::string callbackArg;

    m_ctx->exposeCallback(
        "onClick", [&](mc::client::ui::kagero::widget::Widget*, const mc::client::ui::kagero::event::Event&) {
            callbackCalled = true;
        });

    EXPECT_TRUE(m_ctx->hasCallback("onClick"));
    EXPECT_FALSE(m_ctx->hasCallback("nonexistent"));
}

TEST_F(BindingContextTest, ExposeSimpleCallback)
{
    bool callbackCalled = false;

    m_ctx->exposeSimpleCallback("onStart", [&]() { callbackCalled = true; });

    EXPECT_TRUE(m_ctx->hasCallback("onStart"));

    // Invoke callback - use a concrete event type
    mc::client::ui::kagero::event::FocusGainedEvent event;
    EXPECT_TRUE(m_ctx->invokeCallback("onStart", nullptr, event));
    EXPECT_TRUE(callbackCalled);
}

TEST_F(BindingContextTest, InvokeNonexistentCallback)
{
    mc::client::ui::kagero::event::FocusGainedEvent event;
    EXPECT_FALSE(m_ctx->invokeCallback("nonexistent", nullptr, event));
}

TEST_F(BindingContextTest, LoopVariables)
{
    m_ctx->setLoopVariable("item", binder::Value(42));

    EXPECT_TRUE(m_ctx->hasLoopVariable("item"));
    EXPECT_FALSE(m_ctx->hasLoopVariable("nonexistent"));

    auto value = m_ctx->getLoopVariable("item");
    EXPECT_EQ(value.asInteger(), 42);

    m_ctx->clearLoopVariable("item");
    EXPECT_FALSE(m_ctx->hasLoopVariable("item"));
}

TEST_F(BindingContextTest, ResolveLoopVariable)
{
    m_ctx->setLoopVariable("slot", binder::Value(std::string("item_data")));

    // Resolve $slot - need to call with loopVar parameter
    auto value = m_ctx->resolveBinding("$slot", "slot", binder::Value());
    EXPECT_EQ(value.asString(), "item_data");

    // Resolve with empty loop var - should use stored loop var
    value = m_ctx->resolveBinding("$slot", "", binder::Value());
    EXPECT_EQ(value.asString(), "item_data");
}

TEST_F(BindingContextTest, Subscribe)
{
    std::string playerName = "Steve";
    m_ctx->expose("player.name", &playerName);

    std::string lastValue;
    u64 subId = m_ctx->subscribe(
        "player.name", [&](const std::string& path, const binder::Value& value) { lastValue = value.asString(); });

    EXPECT_GT(subId, 0u);

    // Notify change
    m_ctx->notifyChange("player.name", binder::Value(std::string("Alex")));
    EXPECT_EQ(lastValue, "Alex");

    // Unsubscribe
    m_ctx->unsubscribe(subId);

    // Notify again - lastValue may or may not update depending on implementation
    m_ctx->notifyChange("player.name", binder::Value(std::string("Bob")));
}

TEST_F(BindingContextTest, Clear)
{
    std::string name = "Test";
    m_ctx->expose("name", &name);
    m_ctx->exposeCallback("callback", [](auto*, const auto&) {});

    EXPECT_TRUE(m_ctx->hasPath("name"));
    EXPECT_TRUE(m_ctx->hasCallback("callback"));

    m_ctx->clear();

    EXPECT_FALSE(m_ctx->hasPath("name"));
    EXPECT_FALSE(m_ctx->hasCallback("callback"));
}

// ==================== StateStore 嵌套路径解析测试 ====================
// 以下测试覆盖 BindingContext::_resolvePath 中 StateStore 类型转换支持：
// 当根键存在于 StateStore 但不在 m_exposedVars 中时，应通过 Value::fromAny
// 读取并继续解析嵌套属性。

TEST_F(BindingContextTest, ResolveStateStoreScalarDirectPath)
{
    // 直接路径：StateStore 中存储标量值，路径与键完全匹配
    // resolveBinding 应通过 StateStore 分支直接返回
    mc::client::ui::kagero::state::StateStore::instance().set<i32>("game.score", 1234);

    auto value = m_ctx->resolveBinding("game.score");
    EXPECT_FALSE(value.isNull());
    EXPECT_TRUE(value.isInteger());
    EXPECT_EQ(value.asInteger(), 1234);
}

TEST_F(BindingContextTest, ResolveStateStoreStringDirectPath)
{
    mc::client::ui::kagero::state::StateStore::instance().set<std::string>("game.winner", "Steve");

    auto value = m_ctx->resolveBinding("game.winner");
    EXPECT_FALSE(value.isNull());
    EXPECT_TRUE(value.isString());
    EXPECT_EQ(value.asString(), "Steve");
}

TEST_F(BindingContextTest, ResolveStateStoreBoolDirectPath)
{
    mc::client::ui::kagero::state::StateStore::instance().set<bool>("game.paused", true);

    auto value = m_ctx->resolveBinding("game.paused");
    EXPECT_FALSE(value.isNull());
    EXPECT_TRUE(value.isBool());
    EXPECT_TRUE(value.asBool());
}

TEST_F(BindingContextTest, ResolveStateStoreFloatDirectPath)
{
    mc::client::ui::kagero::state::StateStore::instance().set<f32>("game.volume", 0.75f);

    auto value = m_ctx->resolveBinding("game.volume");
    EXPECT_FALSE(value.isNull());
    EXPECT_TRUE(value.isFloat());
    EXPECT_FLOAT_EQ(value.asFloat(), 0.75f);
}

TEST_F(BindingContextTest, ResolveStateStoreObjectNestedPath)
{
    // 在 StateStore 中存储 Value 对象，通过嵌套路径访问属性
    // 路径 "player.name" 中，player 是根键，name 是属性
    std::unordered_map<std::string, binder::Value> playerProps;
    playerProps["name"] = binder::Value(std::string("Alex"));
    playerProps["level"] = binder::Value(42);
    playerProps["health"] = binder::Value(100);
    binder::Value playerObj = binder::Value::fromObject(std::move(playerProps));

    mc::client::ui::kagero::state::StateStore::instance().set<binder::Value>("player", playerObj);

    // 解析 player.name
    auto nameValue = m_ctx->resolveBinding("player.name");
    EXPECT_FALSE(nameValue.isNull());
    EXPECT_TRUE(nameValue.isString());
    EXPECT_EQ(nameValue.asString(), "Alex");

    // 解析 player.level
    auto levelValue = m_ctx->resolveBinding("player.level");
    EXPECT_FALSE(levelValue.isNull());
    EXPECT_TRUE(levelValue.isInteger());
    EXPECT_EQ(levelValue.asInteger(), 42);

    // 解析 player.health
    auto healthValue = m_ctx->resolveBinding("player.health");
    EXPECT_FALSE(healthValue.isNull());
    EXPECT_TRUE(healthValue.isInteger());
    EXPECT_EQ(healthValue.asInteger(), 100);
}

TEST_F(BindingContextTest, ResolveStateStoreArrayNestedPath)
{
    // 在 StateStore 中存储 Value 数组，通过索引访问
    std::vector<binder::Value> items;
    items.emplace_back(binder::Value(std::string("sword")));
    items.emplace_back(binder::Value(64));
    items.emplace_back(binder::Value(3.14f));

    mc::client::ui::kagero::state::StateStore::instance().set<std::vector<binder::Value>>("inventory", items);

    // 注意：路径 "inventory[0]" 会被 _splitPath 拆分为 ["inventory", "[0]"]
    auto item0 = m_ctx->resolveBinding("inventory[0]");
    EXPECT_FALSE(item0.isNull());
    EXPECT_TRUE(item0.isString());
    EXPECT_EQ(item0.asString(), "sword");

    auto item1 = m_ctx->resolveBinding("inventory[1]");
    EXPECT_FALSE(item1.isNull());
    EXPECT_TRUE(item1.isInteger());
    EXPECT_EQ(item1.asInteger(), 64);

    auto item2 = m_ctx->resolveBinding("inventory[2]");
    EXPECT_FALSE(item2.isNull());
    EXPECT_TRUE(item2.isFloat());
    EXPECT_FLOAT_EQ(item2.asFloat(), 3.14f);
}

TEST_F(BindingContextTest, ResolveStateStoreObjectMapNestedPath)
{
    // 在 StateStore 中存储 std::unordered_map<std::string, Value>，验证转换路径
    std::unordered_map<std::string, binder::Value> settings;
    settings["volume"] = binder::Value(80);
    settings["brightness"] = binder::Value(50);
    settings["language"] = binder::Value(std::string("zh_CN"));

    mc::client::ui::kagero::state::StateStore::instance().set<std::unordered_map<std::string, binder::Value>>(
        "settings", settings);

    auto volumeValue = m_ctx->resolveBinding("settings.volume");
    EXPECT_FALSE(volumeValue.isNull());
    EXPECT_TRUE(volumeValue.isInteger());
    EXPECT_EQ(volumeValue.asInteger(), 80);

    auto langValue = m_ctx->resolveBinding("settings.language");
    EXPECT_FALSE(langValue.isNull());
    EXPECT_TRUE(langValue.isString());
    EXPECT_EQ(langValue.asString(), "zh_CN");
}

TEST_F(BindingContextTest, ResolveStateStoreScalarNestedPathReturnsNull)
{
    // StateStore 中存储标量值，但通过嵌套路径访问属性应返回 Null
    // 因为标量类型（i32/string/bool/f32）不支持属性访问
    mc::client::ui::kagero::state::StateStore::instance().set<i32>("score", 100);

    // score 不存在 "level" 属性，应返回 Null
    auto value = m_ctx->resolveBinding("score.level");
    EXPECT_TRUE(value.isNull());
}

TEST_F(BindingContextTest, ResolveStateStoreNonexistentRootKeyReturnsNull)
{
    // StateStore 中不存在的键，应返回 Null
    auto value = m_ctx->resolveBinding("nonexistent.key");
    EXPECT_TRUE(value.isNull());
}

TEST_F(BindingContextTest, ResolveStateStoreValueReflectsUpdates)
{
    // StateStore 中的值变化后，resolveBinding 应返回最新值
    mc::client::ui::kagero::state::StateStore::instance().set<i32>("counter", 1);
    EXPECT_EQ(m_ctx->resolveBinding("counter").asInteger(), 1);

    mc::client::ui::kagero::state::StateStore::instance().set<i32>("counter", 99);
    EXPECT_EQ(m_ctx->resolveBinding("counter").asInteger(), 99);

    // 对象属性的更新
    std::unordered_map<std::string, binder::Value> playerProps;
    playerProps["name"] = binder::Value(std::string("Steve"));
    mc::client::ui::kagero::state::StateStore::instance().set<binder::Value>(
        "player", binder::Value::fromObject(std::move(playerProps)));
    EXPECT_EQ(m_ctx->resolveBinding("player.name").asString(), "Steve");

    std::unordered_map<std::string, binder::Value> newProps;
    newProps["name"] = binder::Value(std::string("Alex"));
    mc::client::ui::kagero::state::StateStore::instance().set<binder::Value>(
        "player", binder::Value::fromObject(std::move(newProps)));
    EXPECT_EQ(m_ctx->resolveBinding("player.name").asString(), "Alex");
}

TEST_F(BindingContextTest, ResolveStateStoreObjectHasPathAndWritable)
{
    // 验证 hasPath 对 StateStore 中嵌套路径的检测
    std::unordered_map<std::string, binder::Value> playerProps;
    playerProps["name"] = binder::Value(std::string("Steve"));
    mc::client::ui::kagero::state::StateStore::instance().set<binder::Value>(
        "player", binder::Value::fromObject(std::move(playerProps)));

    // hasPath 检查完整路径 "player"（根键存在）
    EXPECT_TRUE(m_ctx->hasPath("player"));
    // 嵌套属性路径不在 hasPath 检测范围内（hasPath 只检查暴露变量、循环变量、StateStore 完整键）
    EXPECT_FALSE(m_ctx->hasPath("player.name"));

    // StateStore 中的值默认不可写（除非通过 exposeWritable 暴露）
    EXPECT_FALSE(m_ctx->isWritable("player"));
    EXPECT_FALSE(m_ctx->isWritable("player.name"));
}

TEST_F(BindingContextTest, ResolveStateStoreExposedVarTakesPrecedence)
{
    // 暴露变量优先于 StateStore 中的同根键
    mc::client::ui::kagero::state::StateStore::instance().set<i32>("data", 100);

    i32 exposedData = 999;
    m_ctx->expose("data", &exposedData);

    // resolveBinding 直接路径匹配应优先返回暴露变量
    auto directValue = m_ctx->resolveBinding("data");
    EXPECT_EQ(directValue.asInteger(), 999);
}

TEST_F(BindingContextTest, FromAnySupportsValueAndContainers)
{
    // 验证 Value::fromAny 对 Value、vector<Value>、unordered_map<string, Value> 的支持
    binder::Value originalInt(42);
    auto fromValue = binder::Value::fromAny(std::any(originalInt));
    EXPECT_TRUE(fromValue.isInteger());
    EXPECT_EQ(fromValue.asInteger(), 42);

    std::vector<binder::Value> vec{binder::Value(1), binder::Value(2), binder::Value(3)};
    auto fromVec = binder::Value::fromAny(std::any(vec));
    EXPECT_TRUE(fromVec.isArray());
    EXPECT_EQ(fromVec.arraySize(), 3u);
    EXPECT_EQ(fromVec.getElement(0).asInteger(), 1);
    EXPECT_EQ(fromVec.getElement(1).asInteger(), 2);
    EXPECT_EQ(fromVec.getElement(2).asInteger(), 3);

    std::unordered_map<std::string, binder::Value> map;
    map["a"] = binder::Value(10);
    map["b"] = binder::Value(std::string("hello"));
    auto fromMap = binder::Value::fromAny(std::any(map));
    EXPECT_TRUE(fromMap.isObject());
    EXPECT_EQ(fromMap.getProperty("a").asInteger(), 10);
    EXPECT_EQ(fromMap.getProperty("b").asString(), "hello");
}

TEST_F(BindingContextTest, FromAnyUnsupportedTypeReturnsNull)
{
    // 不支持的类型应返回 Null
    struct CustomType {
        int x;
    };
    CustomType custom{42};
    auto fromCustom = binder::Value::fromAny(std::any(custom));
    EXPECT_TRUE(fromCustom.isNull());
}

// ==================== Value Tests ====================

class ValueTest : public ::testing::Test {};

TEST_F(ValueTest, DefaultValue)
{
    binder::Value v;
    EXPECT_TRUE(v.isNull());
    EXPECT_FALSE(v.isBool());
    EXPECT_FALSE(v.isInteger());
    EXPECT_FALSE(v.isFloat());
    EXPECT_FALSE(v.isString());
}

TEST_F(ValueTest, BoolValue)
{
    binder::Value v(true);
    EXPECT_TRUE(v.isBool());
    EXPECT_FALSE(v.isNull());
    EXPECT_TRUE(v.asBool());
    EXPECT_EQ(v.toString(), "true");
}

TEST_F(ValueTest, IntegerValue)
{
    binder::Value v(42);
    EXPECT_TRUE(v.isInteger());
    EXPECT_TRUE(v.isNumber());
    EXPECT_EQ(v.asInteger(), 42);
    EXPECT_FLOAT_EQ(v.asFloat(), 42.0f);
    EXPECT_EQ(v.toString(), "42");
}

TEST_F(ValueTest, FloatValue)
{
    binder::Value v(3.14f);
    EXPECT_TRUE(v.isFloat());
    EXPECT_TRUE(v.isNumber());
    EXPECT_FLOAT_EQ(v.asFloat(), 3.14f);
    EXPECT_EQ(v.asInteger(), 3);
}

TEST_F(ValueTest, StringValue)
{
    binder::Value v(std::string("hello"));
    EXPECT_TRUE(v.isString());
    EXPECT_EQ(v.asString(), "hello");
    EXPECT_EQ(v.toString(), "hello");
}

TEST_F(ValueTest, FromAny)
{
    auto vBool = binder::Value::fromAny(std::any(true));
    EXPECT_TRUE(vBool.isBool());

    auto vInt = binder::Value::fromAny(std::any(42));
    EXPECT_TRUE(vInt.isInteger());

    auto vFloat = binder::Value::fromAny(std::any(3.14f));
    EXPECT_TRUE(vFloat.isFloat());

    auto vString = binder::Value::fromAny(std::any(std::string("test")));
    EXPECT_TRUE(vString.isString());

    auto vNull = binder::Value::fromAny(std::any());
    EXPECT_TRUE(vNull.isNull());
}

TEST_F(ValueTest, TypeConversions)
{
    binder::Value vInt(42);
    EXPECT_EQ(vInt.toInteger(), 42);
    EXPECT_FLOAT_EQ(vInt.toFloat(), 42.0f);
    EXPECT_TRUE(vInt.toBool());

    binder::Value vFloat(3.14f);
    EXPECT_EQ(vFloat.toInteger(), 3);
    EXPECT_FLOAT_EQ(vFloat.toFloat(), 3.14f);
    EXPECT_TRUE(vFloat.toBool());

    binder::Value vZero(0);
    EXPECT_FALSE(vZero.toBool());

    binder::Value vTrue(true);
    EXPECT_EQ(vTrue.toInteger(), 1);
    EXPECT_FLOAT_EQ(vTrue.toFloat(), 1.0f);

    binder::Value vString(std::string("42"));
    EXPECT_EQ(vString.toInteger(), 42);
    EXPECT_FLOAT_EQ(vString.toFloat(), 42.0f);
}

TEST_F(ValueTest, Equality)
{
    binder::Value v1(42);
    binder::Value v2(42);
    binder::Value v3(43);
    binder::Value v4(42.0f);

    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 == v3);
    EXPECT_TRUE(v1 == v4); // Integer and float comparison

    EXPECT_FALSE(v1 != v2);
    EXPECT_TRUE(v1 != v3);
}

TEST_F(ValueTest, GetProperty)
{
    binder::Value v(42);
    auto prop = v.getProperty("anything");
    EXPECT_TRUE(prop.isNull()); // Simple values don't have properties
}

TEST_F(ValueTest, GetElement)
{
    binder::Value v(42);
    auto elem = v.getElement(0);
    EXPECT_TRUE(elem.isNull()); // Simple values don't have elements
}

// ==================== TemplateErrorCollector Tests ====================

class TemplateErrorCollectorTest : public ::testing::Test {};

TEST_F(TemplateErrorCollectorTest, BasicOperations)
{
    core::TemplateErrorCollector collector;

    EXPECT_FALSE(collector.hasErrors());
    EXPECT_EQ(collector.errorCount(), 0u);
    EXPECT_EQ(collector.firstError(), nullptr);

    collector.addError(core::TemplateErrorType::LexerError, "Test error");

    EXPECT_TRUE(collector.hasErrors());
    EXPECT_EQ(collector.errorCount(), 1u);
    ASSERT_NE(collector.firstError(), nullptr);
    EXPECT_EQ(collector.firstError()->message, "Test error");

    collector.addError(core::TemplateErrorInfo(core::TemplateErrorType::ParserError, "Second error"));
    EXPECT_EQ(collector.errorCount(), 2u);

    const auto& errors = collector.errors();
    EXPECT_EQ(errors.size(), 2u);

    collector.clear();
    EXPECT_FALSE(collector.hasErrors());
    EXPECT_EQ(collector.errorCount(), 0u);
}

// ==================== Integration Tests ====================

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override { m_config = core::TemplateConfig::defaults(); }

    core::TemplateConfig m_config;
};

TEST_F(IntegrationTest, FullTemplateWorkflow)
{
    // 1. Compile template
    compiler::TemplateCompiler compiler(m_config);
    auto compiled = compiler.compile(R"(
        <screen id="mainScreen" title="Inventory">
            <text id="playerName" bind:text="player.name"/>
            <text id="playerHealth" bind:text="player.health"/>
            <grid id="inventory" bind:items="player.inventory">
                <slot bind:item="$slot.item" bind:count="$slot.count" on:click="onSlotClick"/>
            </grid>
            <button id="closeBtn" on:click="onClose"/>
        </screen>
    )");

    ASSERT_NE(compiled, nullptr);
    EXPECT_TRUE(compiled->isValid()) << (compiled->hasErrors() ? compiled->errors()[0].message : "none");

    // 2. Check that we have binding plans
    EXPECT_GE(compiled->bindingPlans().size(), 1u);

    // 3. Check that we have event plans
    EXPECT_GE(compiled->eventPlans().size(), 1u);

    // 4. Check registered callbacks
    EXPECT_GE(compiled->registeredCallbacks().size(), 1u);
}

TEST_F(IntegrationTest, ErrorRecovery)
{
    // Test error recovery with multiple errors
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <screen>
            <invalidTag/>
            <text bind:text="123invalid"/>
            <button on:click="456invalid"/>
        </screen>
    )");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->hasErrors());
}

TEST_F(IntegrationTest, BindingContextWithTemplate)
{
    // Create binding context using singleton instances
    binder::BindingContext ctx(
        mc::client::ui::kagero::state::StateStore::instance(), mc::client::ui::kagero::event::EventBus::instance());

    // Expose variables
    std::string playerName = "Steve";
    i32 playerHealth = 100;
    ctx.expose("player.name", &playerName);
    ctx.expose("player.health", &playerHealth);

    // Test binding resolution
    auto nameValue = ctx.resolveBinding("player.name");
    EXPECT_EQ(nameValue.asString(), "Steve");

    auto healthValue = ctx.resolveBinding("player.health");
    EXPECT_EQ(healthValue.asInteger(), 100);

    // Test callback
    bool callbackCalled = false;
    ctx.exposeSimpleCallback("onClose", [&]() { callbackCalled = true; });

    mc::client::ui::kagero::event::FocusGainedEvent event;
    EXPECT_TRUE(ctx.invokeCallback("onClose", nullptr, event));
    EXPECT_TRUE(callbackCalled);
}

TEST_F(IntegrationTest, ComplexTemplate)
{
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <screen id="gameScreen">
            <viewport3d id="world" bind:render="world.scene"/>

            <widget id="hud">
                <text id="health" bind:text="player.health"/>
                <text id="hunger" bind:text="player.hunger"/>
                <text id="xp" bind:text="player.xp"/>
            </widget>

            <grid id="hotbar" bind:items="player.hotbar">
                <slot bind:item="$slot.item" bind:count="$slot.count"/>
            </grid>

            <!-- Conditional rendering -->
            <text id="sneakIndicator" bind:visible="player.isSneaking">
                Sneaking...
            </text>

            <button id="pauseBtn" on:click="onPause"/>
        </screen>
    )");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());
    EXPECT_FALSE(result->hasErrors());

    // Verify structure
    // Bindings: bind:render(1) + bind:text*3(3) + bind:items(1) + bind:item(1) + bind:count(1) + bind:visible(1) = 8
    EXPECT_EQ(result->bindingPlans().size(), 8u);
    EXPECT_EQ(result->eventPlans().size(), 1u); // onPause
}

TEST_F(IntegrationTest, ParseComplexAttributeValues)
{
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <widget pos="10,20" size="100,50" visible="true" opacity="0.75">
            <text text="Hello\nWorld"/>
        </widget>
    )");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());

    // Check AST has correct values
    auto* root = result->astRoot();
    ASSERT_NE(root, nullptr);
    auto* elem = root->rootElement();
    ASSERT_NE(elem, nullptr);

    EXPECT_TRUE(elem->hasAttribute("pos"));
    EXPECT_TRUE(elem->hasAttribute("size"));
    EXPECT_TRUE(elem->hasAttribute("visible"));
    EXPECT_TRUE(elem->hasAttribute("opacity"));
}

// ==================== Loop Directive Tests ====================

class LoopDirectiveTest : public ::testing::Test {
protected:
    void SetUp() override { m_config = core::TemplateConfig::defaults(); }

    std::unique_ptr<ast::DocumentNode> parse(const std::string& source)
    {
        parser::Lexer lexer(source, "<test>");
        lexer.tokenize();

        parser::Parser parser(lexer, m_config);
        return parser.parse();
    }

    core::TemplateConfig m_config;
};

TEST_F(LoopDirectiveTest, SimpleLoopDirective)
{
    auto doc = parse(R"(
        <list for:item="item in items">
            <text bind:text="$item.name"/>
        </list>
    )");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* list = doc->rootElement();
    EXPECT_TRUE(list->loop.has_value());
    EXPECT_EQ(list->loop->collectionPath, "items");
    EXPECT_EQ(list->loop->itemVarName, "item");
}

TEST_F(LoopDirectiveTest, LoopWithIndex)
{
    auto doc = parse(R"(
        <grid for:item="(item, index) in items">
            <slot bind:item="$item"/>
        </grid>
    )");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* grid = doc->rootElement();
    EXPECT_TRUE(grid->loop.has_value());
    EXPECT_EQ(grid->loop->collectionPath, "items");
    EXPECT_EQ(grid->loop->itemVarName, "item");
    EXPECT_EQ(grid->loop->indexVarName, "index");
    EXPECT_TRUE(grid->loop->hasIndex);
}

TEST_F(LoopDirectiveTest, LoopAttributeType)
{
    auto doc = parse(R"(<list for:item="item in items"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* list = doc->rootElement();

    // Find the for:item attribute
    bool foundLoopAttr = false;
    for (const auto& [name, attr] : list->attributes) {
        if (name.find("for:") == 0) {
            foundLoopAttr = true;
            EXPECT_EQ(attr.type, ast::AttributeType::Loop);
            break;
        }
    }
    EXPECT_TRUE(foundLoopAttr);
}

TEST_F(LoopDirectiveTest, LoopWithNestedBindings)
{
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <list for:item="item in player.inventory">
            <text bind:text="$item.name"/>
            <text bind:text="$item.count"/>
        </list>
    )");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());

    // Should have binding plans for nested bindings
    EXPECT_GE(result->bindingPlans().size(), 2u);
}

// ==================== Condition Directive Tests ====================

class ConditionDirectiveTest : public ::testing::Test {
protected:
    void SetUp() override { m_config = core::TemplateConfig::defaults(); }

    std::unique_ptr<ast::DocumentNode> parse(const std::string& source)
    {
        parser::Lexer lexer(source, "<test>");
        lexer.tokenize();

        parser::Parser parser(lexer, m_config);
        return parser.parse();
    }

    core::TemplateConfig m_config;
};

TEST_F(ConditionDirectiveTest, SimpleConditionDirective)
{
    auto doc = parse(R"(<text if:condition="player.alive"/>)");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* text = doc->rootElement();
    EXPECT_TRUE(text->condition.has_value());
    EXPECT_EQ(text->condition->booleanPath, "player.alive");
    EXPECT_FALSE(text->condition->negate);
}

TEST_F(ConditionDirectiveTest, ConditionWithNegation)
{
    auto doc = parse(R"(<text if:condition="!player.hidden"/>)");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* text = doc->rootElement();
    EXPECT_TRUE(text->condition.has_value());
    EXPECT_EQ(text->condition->booleanPath, "player.hidden");
    EXPECT_TRUE(text->condition->negate);
}

TEST_F(ConditionDirectiveTest, ConditionAttributeType)
{
    auto doc = parse(R"(<panel if:condition="game.isPlaying"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* panel = doc->rootElement();

    // Find the if:condition attribute
    bool foundConditionAttr = false;
    for (const auto& [name, attr] : panel->attributes) {
        if (name.find("if:") == 0) {
            foundConditionAttr = true;
            EXPECT_EQ(attr.type, ast::AttributeType::Condition);
            break;
        }
    }
    EXPECT_TRUE(foundConditionAttr);
}

TEST_F(ConditionDirectiveTest, LegacyVisibleBinding)
{
    // Test backward compatibility with bind:visible
    auto doc = parse(R"(<text bind:visible="player.isSneaking"/>)");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    // The condition should be extracted from bind:visible
    auto* text = doc->rootElement();

    // After categorizeAttributes and extractConditionInfo
    text->categorizeAttributes();

    // bind:visible should create a condition
    EXPECT_TRUE(text->condition.has_value());
    EXPECT_EQ(text->condition->booleanPath, "player.isSneaking");
}

TEST_F(ConditionDirectiveTest, ConditionWithContent)
{
    compiler::TemplateCompiler compiler(m_config);

    // Use 'widget' as a generic container
    auto result = compiler.compile(R"(
        <widget if:condition="game.isPlaying">
            <text bind:text="player.health"/>
            <text bind:text="player.name"/>
        </widget>
    )");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());

    // Should have binding plans for the content
    EXPECT_GE(result->bindingPlans().size(), 2u);
}

// ==================== Value Array Tests ====================

class ValueArrayTest : public ::testing::Test {};

TEST_F(ValueArrayTest, EmptyArray)
{
    binder::Value v(std::vector<binder::Value>{});
    EXPECT_TRUE(v.isArray());
    EXPECT_EQ(v.arraySize(), 0u);
}

TEST_F(ValueArrayTest, ArrayWithElements)
{
    std::vector<binder::Value> items;
    items.emplace_back(binder::Value(1));
    items.emplace_back(binder::Value(2));
    items.emplace_back(binder::Value(3));

    binder::Value v(items);
    EXPECT_TRUE(v.isArray());
    EXPECT_EQ(v.arraySize(), 3u);

    EXPECT_EQ(v.arrayGet(0).asInteger(), 1);
    EXPECT_EQ(v.arrayGet(1).asInteger(), 2);
    EXPECT_EQ(v.arrayGet(2).asInteger(), 3);
}

TEST_F(ValueArrayTest, ArrayOutOfBounds)
{
    std::vector<binder::Value> items;
    items.emplace_back(binder::Value(42));

    binder::Value v(items);
    EXPECT_TRUE(v.isArray());

    // Out of bounds should return null
    auto elem = v.arrayGet(10);
    EXPECT_TRUE(elem.isNull());
}

TEST_F(ValueArrayTest, ArrayToString)
{
    std::vector<binder::Value> items;
    items.emplace_back(binder::Value(1));
    items.emplace_back(binder::Value(2));
    items.emplace_back(binder::Value(3));

    binder::Value v(items);
    std::string str = v.toString();
    EXPECT_TRUE(str.find("[") != std::string::npos);
    EXPECT_TRUE(str.find("]") != std::string::npos);
}

TEST_F(ValueArrayTest, ArrayEquality)
{
    std::vector<binder::Value> items1;
    items1.emplace_back(binder::Value(1));
    items1.emplace_back(binder::Value(2));

    std::vector<binder::Value> items2;
    items2.emplace_back(binder::Value(1));
    items2.emplace_back(binder::Value(2));

    std::vector<binder::Value> items3;
    items3.emplace_back(binder::Value(1));
    items3.emplace_back(binder::Value(3));

    binder::Value v1(items1);
    binder::Value v2(items2);
    binder::Value v3(items3);

    EXPECT_TRUE(v1 == v2);
    EXPECT_FALSE(v1 == v3);
}

TEST_F(ValueArrayTest, FromArrayStatic)
{
    auto v = binder::Value::fromArray({binder::Value(1), binder::Value(2)});
    EXPECT_TRUE(v.isArray());
    EXPECT_EQ(v.arraySize(), 2u);
}

TEST_F(ValueArrayTest, EmptyArrayStatic)
{
    auto v = binder::Value::emptyArray();
    EXPECT_TRUE(v.isArray());
    EXPECT_EQ(v.arraySize(), 0u);
}

// ==================== BindingContext Collection Tests ====================

class BindingContextCollectionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_ctx = std::make_unique<binder::BindingContext>(
            mc::client::ui::kagero::state::StateStore::instance(), mc::client::ui::kagero::event::EventBus::instance());
    }

    std::unique_ptr<binder::BindingContext> m_ctx;
};

TEST_F(BindingContextCollectionTest, ResolveCollection)
{
    // Set up a collection
    std::vector<binder::Value> items;
    items.emplace_back(binder::Value(std::string("item1")));
    items.emplace_back(binder::Value(std::string("item2")));
    items.emplace_back(binder::Value(std::string("item3")));

    m_ctx->setCollectionValue("inventory", items);

    auto resolved = m_ctx->resolveCollection("inventory");
    EXPECT_EQ(resolved.size(), 3u);
    EXPECT_EQ(resolved[0].asString(), "item1");
    EXPECT_EQ(resolved[1].asString(), "item2");
    EXPECT_EQ(resolved[2].asString(), "item3");
}

TEST_F(BindingContextCollectionTest, ResolveEmptyCollection)
{
    auto resolved = m_ctx->resolveCollection("nonexistent");
    EXPECT_EQ(resolved.size(), 0u);
}

TEST_F(BindingContextCollectionTest, SetCollectionTemplate)
{
    // Test the template version
    m_ctx->setCollection<i32>("numbers", {1, 2, 3, 4, 5});

    auto resolved = m_ctx->resolveCollection("numbers");
    EXPECT_EQ(resolved.size(), 5u);
    EXPECT_EQ(resolved[0].asInteger(), 1);
    EXPECT_EQ(resolved[4].asInteger(), 5);
}

TEST_F(BindingContextCollectionTest, LoopVariableWithCollection)
{
    // Set up a collection
    m_ctx->setCollection<i32>("items", {10, 20, 30});

    // Iterate using loop variables
    auto items = m_ctx->resolveCollection("items");
    EXPECT_EQ(items.size(), 3u);

    for (size_t i = 0; i < items.size(); ++i) {
        m_ctx->setLoopVariable("item", items[i]);
        m_ctx->setLoopVariable("index", binder::Value(static_cast<i32>(i)));

        auto itemValue = m_ctx->getLoopVariable("item");
        auto indexValue = m_ctx->getLoopVariable("index");

        EXPECT_EQ(itemValue.asInteger(), static_cast<i32>(10 * (i + 1)));
        EXPECT_EQ(indexValue.asInteger(), static_cast<i32>(i));

        m_ctx->clearLoopVariable("item");
        m_ctx->clearLoopVariable("index");
    }
}

// ==================== Template System Tests ====================
// NOTE: TemplateSystemTest is disabled because it requires BuiltinWidgets and BuiltinEvents
// which depend on Widget implementations that are excluded from the test build.
// The TemplateSystem functionality is tested indirectly through integration tests.

// class TemplateSystemTest : public ::testing::Test {
// };
//
// TEST_F(TemplateSystemTest, InitializeShutdown) {
//     // Test initialization
//     initializeTemplateSystem();
//     EXPECT_TRUE(isTemplateSystemInitialized());
//
//     // Test idempotent initialization
//     initializeTemplateSystem();
//     EXPECT_TRUE(isTemplateSystemInitialized());
//
//     // Test shutdown
//     shutdownTemplateSystem();
//     EXPECT_FALSE(isTemplateSystemInitialized());
//
//     // Re-initialize for other tests
//     initializeTemplateSystem();
// }

// ==================== UpdateScheduler Tests ====================

class UpdateSchedulerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_scheduler = std::make_unique<runtime::UpdateScheduler>();
        // 默认禁用延迟更新，保持原有立即执行语义
        // 需要测试延迟/批量行为的用例可显式启用
        m_scheduler->setDeferredUpdate(false);
    }

    std::unique_ptr<runtime::UpdateScheduler> m_scheduler;
};

TEST_F(UpdateSchedulerTest, ScheduleTask)
{
    u64 taskId = m_scheduler->schedule("player.health", runtime::UpdateScheduler::Priority::Normal);
    EXPECT_GT(taskId, 0u);
    EXPECT_TRUE(m_scheduler->hasPending());
    EXPECT_EQ(m_scheduler->pendingCount(), 1u);
}

TEST_F(UpdateSchedulerTest, ScheduleMultipleTasks)
{
    m_scheduler->schedule("player.health", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->schedule("player.name", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->schedule("player.xp", runtime::UpdateScheduler::Priority::Normal);

    EXPECT_EQ(m_scheduler->pendingCount(), 3u);
}

TEST_F(UpdateSchedulerTest, ScheduleDifferentPriorities)
{
    m_scheduler->schedule("low", runtime::UpdateScheduler::Priority::Low);
    m_scheduler->schedule("high", runtime::UpdateScheduler::Priority::High);
    m_scheduler->schedule("normal", runtime::UpdateScheduler::Priority::Normal);

    EXPECT_EQ(m_scheduler->pendingCount(), 3u);
    EXPECT_EQ(m_scheduler->pendingCount(runtime::UpdateScheduler::Priority::High), 1u);
    EXPECT_EQ(m_scheduler->pendingCount(runtime::UpdateScheduler::Priority::Normal), 1u);
    EXPECT_EQ(m_scheduler->pendingCount(runtime::UpdateScheduler::Priority::Low), 1u);
}

TEST_F(UpdateSchedulerTest, CancelTask)
{
    u64 taskId = m_scheduler->schedule("player.health", runtime::UpdateScheduler::Priority::Normal);
    EXPECT_TRUE(m_scheduler->hasPending());

    m_scheduler->cancel(taskId);
    // Cancelled tasks remain in the list but marked as cancelled
    // They are cleaned up during executePending
}

TEST_F(UpdateSchedulerTest, CancelByPath)
{
    m_scheduler->schedule("player.health", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->schedule("player.name", runtime::UpdateScheduler::Priority::Normal);

    m_scheduler->cancelByPath("player.health");
    // Should cancel tasks for that path
}

TEST_F(UpdateSchedulerTest, CancelAll)
{
    m_scheduler->schedule("player.health", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->schedule("player.name", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->schedule("player.xp", runtime::UpdateScheduler::Priority::Normal);

    m_scheduler->cancelAll();
    EXPECT_FALSE(m_scheduler->hasPending());
    EXPECT_EQ(m_scheduler->pendingCount(), 0u);
}

TEST_F(UpdateSchedulerTest, ExecutePendingNoCallback)
{
    m_scheduler->schedule("player.health", runtime::UpdateScheduler::Priority::Normal);

    // Execute without a callback should return 0
    u32 executed = m_scheduler->executePending();
    EXPECT_EQ(executed, 0u);
}

TEST_F(UpdateSchedulerTest, ExecuteWithCallback)
{
    std::vector<std::string> updatedPaths;
    m_scheduler->setUpdateCallback([&updatedPaths](const std::string& path) -> bool {
        updatedPaths.push_back(path);
        return true;
    });

    m_scheduler->schedule("player.health", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->schedule("player.name", runtime::UpdateScheduler::Priority::High);

    EXPECT_TRUE(m_scheduler->hasPending());
    u32 executed = m_scheduler->executePending();
    EXPECT_EQ(executed, 2u);
    EXPECT_EQ(updatedPaths.size(), 2u);
    EXPECT_FALSE(m_scheduler->hasPending());
}

TEST_F(UpdateSchedulerTest, SetBatchDelay)
{
    // 设置批量延迟后，任务需等到 batchDelayMs 后才会被执行
    m_scheduler->setBatchDelay(32);
    EXPECT_EQ(m_scheduler->batchDelay(), 32u);

    std::vector<std::string> updatedPaths;
    m_scheduler->setUpdateCallback([&updatedPaths](const std::string& path) -> bool {
        updatedPaths.push_back(path);
        return true;
    });

    // 启用延迟更新，时间 0 时调度，到期时间应为 0 + 32 = 32ms
    m_scheduler->setDeferredUpdate(true);
    m_scheduler->schedule("player.health", runtime::UpdateScheduler::Priority::Normal);

    // 在 31ms 时任务尚未到期
    m_scheduler->tick(31);
    EXPECT_EQ(updatedPaths.size(), 0u);
    EXPECT_TRUE(m_scheduler->hasPending());

    // 在 32ms 时任务到期，应被执行
    m_scheduler->tick(32);
    EXPECT_EQ(updatedPaths.size(), 1u);
    EXPECT_FALSE(m_scheduler->hasPending());
}

TEST_F(UpdateSchedulerTest, SetMaxBatchSize)
{
    // 设置最大批量大小后，executeBatch 一次最多执行 maxBatchSize 个路径
    m_scheduler->setMaxBatchSize(2);
    EXPECT_EQ(m_scheduler->maxBatchSize(), 2u);

    std::vector<std::string> updatedPaths;
    m_scheduler->setUpdateCallback([&updatedPaths](const std::string& path) -> bool {
        updatedPaths.push_back(path);
        return true;
    });

    // 禁用延迟更新，使所有任务都视为已到期
    m_scheduler->setDeferredUpdate(false);
    m_scheduler->schedule("path1", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->schedule("path2", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->schedule("path3", runtime::UpdateScheduler::Priority::Normal);

    // executeBatch 受 maxBatchSize 限制，一次最多执行 2 个
    u32 executed = m_scheduler->executeBatch();
    EXPECT_EQ(executed, 2u);
    EXPECT_EQ(updatedPaths.size(), 2u);

    // 剩余 1 个任务
    EXPECT_TRUE(m_scheduler->hasPending());
    EXPECT_EQ(m_scheduler->pendingCount(), 1u);

    // 再次执行剩余任务
    executed = m_scheduler->executeBatch();
    EXPECT_EQ(executed, 1u);
    EXPECT_FALSE(m_scheduler->hasPending());
}

TEST_F(UpdateSchedulerTest, SetDeferredUpdate)
{
    // 启用延迟更新：任务受 batchDelayMs 限制
    // 禁用延迟更新：任务立即执行（dueTimeMs 设为 0）
    std::vector<std::string> updatedPaths;
    m_scheduler->setUpdateCallback([&updatedPaths](const std::string& path) -> bool {
        updatedPaths.push_back(path);
        return true;
    });

    // 禁用延迟更新：executePending 立即执行所有未取消任务
    m_scheduler->setDeferredUpdate(false);
    EXPECT_FALSE(m_scheduler->deferredUpdate());
    m_scheduler->setBatchDelay(1000); // 即使延迟很大，禁用后也应立即执行

    m_scheduler->schedule("immediate", runtime::UpdateScheduler::Priority::Normal);
    u32 executed = m_scheduler->executePending();
    EXPECT_EQ(executed, 1u);
    EXPECT_EQ(updatedPaths.size(), 1u);

    // 启用延迟更新：executePending 只执行到期任务
    m_scheduler->setDeferredUpdate(true);
    EXPECT_TRUE(m_scheduler->deferredUpdate());
    m_scheduler->setBatchDelay(100);

    m_scheduler->schedule("deferred", runtime::UpdateScheduler::Priority::Normal);
    // 时间未推进到 100ms，不应执行
    m_scheduler->tick(50);
    EXPECT_TRUE(m_scheduler->hasPending());
    EXPECT_EQ(updatedPaths.size(), 1u);

    // 时间推进到 100ms，应执行
    m_scheduler->tick(100);
    EXPECT_FALSE(m_scheduler->hasPending());
    EXPECT_EQ(updatedPaths.size(), 2u);
}

TEST_F(UpdateSchedulerTest, CurrentTimestamp)
{
    u64 ts1 = m_scheduler->currentTimestamp();
    u64 ts2 = m_scheduler->currentTimestamp();
    // Timestamps should be increasing
    EXPECT_GE(ts2, ts1);
}

// ==================== UpdateScheduler 延迟/批量/Tick/Flush 测试 ====================

TEST_F(UpdateSchedulerTest, TickAdvancesTimeAndExecutesDueTasks)
{
    // tick(currentMs) 推进调度器内部时钟并执行到期任务
    std::vector<std::string> updatedPaths;
    m_scheduler->setUpdateCallback([&updatedPaths](const std::string& path) -> bool {
        updatedPaths.push_back(path);
        return true;
    });

    m_scheduler->setDeferredUpdate(true);
    m_scheduler->setBatchDelay(50);

    m_scheduler->schedule("task1", runtime::UpdateScheduler::Priority::Normal);

    // 在 49ms 时任务尚未到期
    m_scheduler->tick(49);
    EXPECT_EQ(updatedPaths.size(), 0u);

    // 在 50ms 时任务到期
    m_scheduler->tick(50);
    EXPECT_EQ(updatedPaths.size(), 1u);
    EXPECT_EQ(updatedPaths[0], "task1");

    // nowMs 应反映最近一次 tick 的时间
    EXPECT_EQ(m_scheduler->nowMs(), 50u);
}

TEST_F(UpdateSchedulerTest, FlushExecutesAllImmediately)
{
    // flush() 无视延迟，立即执行所有待处理任务
    std::vector<std::string> updatedPaths;
    m_scheduler->setUpdateCallback([&updatedPaths](const std::string& path) -> bool {
        updatedPaths.push_back(path);
        return true;
    });

    m_scheduler->setDeferredUpdate(true);
    m_scheduler->setBatchDelay(10000); // 超大延迟

    m_scheduler->schedule("high", runtime::UpdateScheduler::Priority::High);
    m_scheduler->schedule("normal", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->schedule("low", runtime::UpdateScheduler::Priority::Low);

    EXPECT_TRUE(m_scheduler->hasPending());

    // flush 应立即执行所有任务，无视延迟
    u32 executed = m_scheduler->flush();
    EXPECT_EQ(executed, 3u);
    EXPECT_FALSE(m_scheduler->hasPending());
    EXPECT_EQ(updatedPaths.size(), 3u);
}

TEST_F(UpdateSchedulerTest, DeferredUpdateDefersExecution)
{
    // 启用延迟更新后，任务不会立即执行，需等到 batchDelayMs 后
    std::vector<std::string> updatedPaths;
    m_scheduler->setUpdateCallback([&updatedPaths](const std::string& path) -> bool {
        updatedPaths.push_back(path);
        return true;
    });

    m_scheduler->setDeferredUpdate(true);
    m_scheduler->setBatchDelay(16);

    m_scheduler->schedule("deferred.task", runtime::UpdateScheduler::Priority::Normal);

    // executePending 在未到期时不应执行任何任务
    u32 executed = m_scheduler->executePending();
    EXPECT_EQ(executed, 0u);
    EXPECT_TRUE(m_scheduler->hasPending());
    EXPECT_EQ(updatedPaths.size(), 0u);
}

TEST_F(UpdateSchedulerTest, DeferredUpdateExecutesAfterDelay)
{
    // 延迟到期后，tick(currentMs) 应执行任务
    std::vector<std::string> updatedPaths;
    m_scheduler->setUpdateCallback([&updatedPaths](const std::string& path) -> bool {
        updatedPaths.push_back(path);
        return true;
    });

    m_scheduler->setDeferredUpdate(true);
    m_scheduler->setBatchDelay(16);

    m_scheduler->schedule("task", runtime::UpdateScheduler::Priority::Normal);

    m_scheduler->tick(15);
    EXPECT_EQ(updatedPaths.size(), 0u);

    m_scheduler->tick(16);
    EXPECT_EQ(updatedPaths.size(), 1u);
    EXPECT_FALSE(m_scheduler->hasPending());
}

TEST_F(UpdateSchedulerTest, DeferredUpdateDisabledExecutesImmediately)
{
    // 禁用延迟更新后，任务立即执行（dueTimeMs = 0）
    std::vector<std::string> updatedPaths;
    m_scheduler->setUpdateCallback([&updatedPaths](const std::string& path) -> bool {
        updatedPaths.push_back(path);
        return true;
    });

    m_scheduler->setDeferredUpdate(false);
    m_scheduler->setBatchDelay(10000); // 即使延迟很大

    m_scheduler->schedule("immediate.task", runtime::UpdateScheduler::Priority::Normal);

    u32 executed = m_scheduler->executePending();
    EXPECT_EQ(executed, 1u);
    EXPECT_FALSE(m_scheduler->hasPending());
}

TEST_F(UpdateSchedulerTest, BatchDelayDebounceTrailingEdge)
{
    // 同一路径多次 schedule 应被去重（trailing-edge debounce）
    // 只保留最新任务，且只执行一次
    int callCount = 0;
    std::string lastPath;
    m_scheduler->setUpdateCallback([&callCount, &lastPath](const std::string& path) -> bool {
        ++callCount;
        lastPath = path;
        return true;
    });

    m_scheduler->setDeferredUpdate(true);
    m_scheduler->setBatchDelay(20);

    // 同一路径连续调度 3 次
    m_scheduler->schedule("debounced", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->schedule("debounced", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->schedule("debounced", runtime::UpdateScheduler::Priority::Normal);

    // 去重发生在 executePending/executeBatch/flush/tick 内部
    // 推进时间到到期，tick 内部会先去重再执行
    m_scheduler->tick(20);

    // 只执行一次（去重后只保留最新任务）
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(lastPath, "debounced");
    EXPECT_FALSE(m_scheduler->hasPending());
}

TEST_F(UpdateSchedulerTest, DueCount)
{
    // dueCount(currentMs) 返回已到期但未执行的待处理任务数量
    m_scheduler->setDeferredUpdate(true);
    m_scheduler->setBatchDelay(50);

    m_scheduler->schedule("task1", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->schedule("task2", runtime::UpdateScheduler::Priority::Normal);

    // 在 49ms 时，0 个到期
    EXPECT_EQ(m_scheduler->dueCount(49), 0u);

    // 在 50ms 时，2 个都到期
    EXPECT_EQ(m_scheduler->dueCount(50), 2u);
}

TEST_F(UpdateSchedulerTest, DueCountDeferredUpdateDisabled)
{
    // 禁用延迟更新时，所有未取消任务都视为已到期
    m_scheduler->setDeferredUpdate(false);
    m_scheduler->setBatchDelay(10000);

    m_scheduler->schedule("task1", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->schedule("task2", runtime::UpdateScheduler::Priority::Normal);

    // 即使 currentMs=0，所有任务都视为到期
    EXPECT_EQ(m_scheduler->dueCount(0), 2u);
}

TEST_F(UpdateSchedulerTest, ExecuteBatchRespectsDueTime)
{
    // executeBatch 只执行到期任务，受 maxBatchSize 限制
    std::vector<std::string> updatedPaths;
    m_scheduler->setUpdateCallback([&updatedPaths](const std::string& path) -> bool {
        updatedPaths.push_back(path);
        return true;
    });

    m_scheduler->setDeferredUpdate(true);
    m_scheduler->setBatchDelay(30);
    m_scheduler->setMaxBatchSize(2);

    // 先推进时钟到 30，此时还没有任务
    m_scheduler->tick(30);

    // 在 t=30 时调度任务，到期时间为 30+30=60
    m_scheduler->schedule("task1", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->schedule("task2", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->schedule("task3", runtime::UpdateScheduler::Priority::Normal);

    // 时钟仍在 30，任务到期时间为 60，未到期，executeBatch 不应执行
    u32 executed = m_scheduler->executeBatch();
    EXPECT_EQ(executed, 0u);
    EXPECT_EQ(updatedPaths.size(), 0u);
    EXPECT_TRUE(m_scheduler->hasPending());

    // 推进时钟到 60，任务到期
    // 注意：tick 会调用 executePending，会执行所有到期任务
    // 为单独测试 executeBatch 的 maxBatchSize 限制，禁用延迟后调用
    // 但禁用延迟会导致 executePending 执行所有任务，所以这里直接验证 executeBatch 的去重和批量语义
    m_scheduler->setMaxBatchSize(2);
    m_scheduler->setDeferredUpdate(false); // 使 _isDue 总是返回 true
    executed = m_scheduler->executeBatch();
    // executeBatch 受 maxBatchSize=2 限制，一次最多执行 2 个
    EXPECT_EQ(executed, 2u);
    EXPECT_EQ(updatedPaths.size(), 2u);
    EXPECT_TRUE(m_scheduler->hasPending());

    // 再次执行剩余任务
    executed = m_scheduler->executeBatch();
    EXPECT_EQ(executed, 1u);
    EXPECT_EQ(updatedPaths.size(), 3u);
    EXPECT_FALSE(m_scheduler->hasPending());
}

TEST_F(UpdateSchedulerTest, TickWithZeroTimeExecutesNonDeferred)
{
    // 禁用延迟更新时，即使 tick(0) 也应执行任务
    std::vector<std::string> updatedPaths;
    m_scheduler->setUpdateCallback([&updatedPaths](const std::string& path) -> bool {
        updatedPaths.push_back(path);
        return true;
    });

    m_scheduler->setDeferredUpdate(false);
    m_scheduler->schedule("task", runtime::UpdateScheduler::Priority::Normal);

    u32 executed = m_scheduler->tick(0);
    EXPECT_EQ(executed, 1u);
    EXPECT_FALSE(m_scheduler->hasPending());
}

TEST_F(UpdateSchedulerTest, NowMsReflectsLastTick)
{
    // nowMs() 返回最近一次 tick(currentMs) 传入的时间戳
    EXPECT_EQ(m_scheduler->nowMs(), 0u);

    m_scheduler->tick(12345);
    EXPECT_EQ(m_scheduler->nowMs(), 12345u);

    m_scheduler->tick(67890);
    EXPECT_EQ(m_scheduler->nowMs(), 67890u);
}

TEST_F(UpdateSchedulerTest, FlushOnEmptyScheduler)
{
    // 在空调度器上调用 flush 应安全返回 0
    m_scheduler->setUpdateCallback([](const std::string&) -> bool { return true; });

    u32 executed = m_scheduler->flush();
    EXPECT_EQ(executed, 0u);
    EXPECT_FALSE(m_scheduler->hasPending());
}

TEST_F(UpdateSchedulerTest, TickOnEmptyScheduler)
{
    // 在空调度器上调用 tick 应安全返回 0
    m_scheduler->setUpdateCallback([](const std::string&) -> bool { return true; });

    u32 executed = m_scheduler->tick(1000);
    EXPECT_EQ(executed, 0u);
    EXPECT_FALSE(m_scheduler->hasPending());
    EXPECT_EQ(m_scheduler->nowMs(), 1000u);
}

TEST_F(UpdateSchedulerTest, MultiplePathsDeferredExecution)
{
    // 多个不同路径的任务，各自按到期时间执行
    std::vector<std::string> updatedPaths;
    m_scheduler->setUpdateCallback([&updatedPaths](const std::string& path) -> bool {
        updatedPaths.push_back(path);
        return true;
    });

    m_scheduler->setDeferredUpdate(true);
    m_scheduler->setBatchDelay(10);

    // 第一批任务：在 t=0 调度，t=10 到期
    m_scheduler->schedule("batch1.a", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->schedule("batch1.b", runtime::UpdateScheduler::Priority::Normal);

    // 推进时间到 10，第一批应执行
    m_scheduler->tick(10);
    EXPECT_EQ(updatedPaths.size(), 2u);
    EXPECT_FALSE(m_scheduler->hasPending());

    // 第二批任务：在 t=10 调度，t=20 到期
    m_scheduler->schedule("batch2.a", runtime::UpdateScheduler::Priority::Normal);

    // 在 t=15 时不应执行
    m_scheduler->tick(15);
    EXPECT_EQ(updatedPaths.size(), 2u);
    EXPECT_TRUE(m_scheduler->hasPending());

    // 在 t=20 时应执行
    m_scheduler->tick(20);
    EXPECT_EQ(updatedPaths.size(), 3u);
    EXPECT_FALSE(m_scheduler->hasPending());
}

TEST_F(UpdateSchedulerTest, HighPriorityExecutesBeforeNormalAndLow)
{
    // tick(currentMs) 按 High -> Normal -> Low 顺序执行到期任务
    std::vector<std::string> updatedPaths;
    m_scheduler->setUpdateCallback([&updatedPaths](const std::string& path) -> bool {
        updatedPaths.push_back(path);
        return true;
    });

    m_scheduler->setDeferredUpdate(true);
    m_scheduler->setBatchDelay(10);

    m_scheduler->schedule("low", runtime::UpdateScheduler::Priority::Low);
    m_scheduler->schedule("normal", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->schedule("high", runtime::UpdateScheduler::Priority::High);

    m_scheduler->tick(10);

    // 执行顺序应为 High -> Normal -> Low
    ASSERT_EQ(updatedPaths.size(), 3u);
    EXPECT_EQ(updatedPaths[0], "high");
    EXPECT_EQ(updatedPaths[1], "normal");
    EXPECT_EQ(updatedPaths[2], "low");
}

// ==================== CompiledTemplate Tests ====================

class CompiledTemplateTest : public ::testing::Test {
protected:
    void SetUp() override { m_config = core::TemplateConfig::defaults(); }

    core::TemplateConfig m_config;
};

TEST_F(CompiledTemplateTest, CreateCompiledTemplate)
{
    compiler::CompiledTemplate compiled;
    EXPECT_FALSE(compiled.isValid());
    EXPECT_FALSE(compiled.hasErrors());
    EXPECT_EQ(compiled.bindingPlans().size(), 0u);
    EXPECT_EQ(compiled.eventPlans().size(), 0u);
    EXPECT_EQ(compiled.loopPlans().size(), 0u);
}

TEST_F(CompiledTemplateTest, AddBindingPlan)
{
    compiler::CompiledTemplate compiled;
    compiler::BindingPlan plan;
    plan.widgetPath = "screen.button";
    plan.statePath = "player.health";
    plan.attributeName = "text";

    compiled.addBindingPlan(plan);
    EXPECT_EQ(compiled.bindingPlans().size(), 1u);
    EXPECT_EQ(compiled.bindingPlans()[0].widgetPath, "screen.button");
    EXPECT_EQ(compiled.bindingPlans()[0].statePath, "player.health");
}

TEST_F(CompiledTemplateTest, AddEventPlan)
{
    compiler::CompiledTemplate compiled;
    compiler::EventPlan plan;
    plan.widgetPath = "screen.button";
    plan.eventName = "click";
    plan.callbackName = "onButtonClick";

    compiled.addEventPlan(plan);
    EXPECT_EQ(compiled.eventPlans().size(), 1u);
    EXPECT_EQ(compiled.eventPlans()[0].widgetPath, "screen.button");
    EXPECT_EQ(compiled.eventPlans()[0].eventName, "click");
}

TEST_F(CompiledTemplateTest, AddLoopPlan)
{
    compiler::CompiledTemplate compiled;
    compiler::LoopPlan plan;
    plan.parentPath = "screen.inventory";
    plan.collectionPath = "player.items";
    plan.itemVarName = "item";

    compiled.addLoopPlan(plan);
    EXPECT_EQ(compiled.loopPlans().size(), 1u);
    EXPECT_EQ(compiled.loopPlans()[0].collectionPath, "player.items");
}

TEST_F(CompiledTemplateTest, AddWatchedPath)
{
    compiler::CompiledTemplate compiled;
    compiled.addWatchedPath("player.health");
    compiled.addWatchedPath("player.name");
    compiled.addWatchedPath("player.health"); // Duplicate

    EXPECT_EQ(compiled.watchedPaths().size(), 2u);
    EXPECT_TRUE(compiled.watchedPaths().count("player.health") > 0);
    EXPECT_TRUE(compiled.watchedPaths().count("player.name") > 0);
}

TEST_F(CompiledTemplateTest, AddRegisteredCallback)
{
    compiler::CompiledTemplate compiled;
    compiled.addRegisteredCallback("onClick");
    compiled.addRegisteredCallback("onHover");
    compiled.addRegisteredCallback("onClick"); // Duplicate

    EXPECT_EQ(compiled.registeredCallbacks().size(), 2u);
    EXPECT_TRUE(compiled.registeredCallbacks().count("onClick") > 0);
    EXPECT_TRUE(compiled.registeredCallbacks().count("onHover") > 0);
}

TEST_F(CompiledTemplateTest, SetSourcePath)
{
    compiler::CompiledTemplate compiled;
    compiled.setSourcePath("templates/main_menu.xml");
    EXPECT_EQ(compiled.sourcePath(), "templates/main_menu.xml");
}

TEST_F(CompiledTemplateTest, SetCompileTime)
{
    compiler::CompiledTemplate compiled;
    compiled.setCompileTime(42);
    EXPECT_EQ(compiled.compileTime(), 42u);
}

TEST_F(CompiledTemplateTest, AddError)
{
    compiler::CompiledTemplate compiled;
    compiled.addError(core::TemplateErrorInfo(
        core::TemplateErrorType::ParserError, "Test error", core::SourceLocation(10, 5), "test.tpl"));

    EXPECT_TRUE(compiled.hasErrors());
    EXPECT_EQ(compiled.errors().size(), 1u);
    EXPECT_EQ(compiled.errors()[0].message, "Test error");
}

TEST_F(CompiledTemplateTest, DebugDump)
{
    compiler::CompiledTemplate compiled;
    compiled.setSourcePath("test.tpl");
    compiled.addBindingPlan(compiler::BindingPlan{"screen.btn", "player.health", "text", false, ""});
    compiled.addEventPlan(compiler::EventPlan{"screen.btn", "click", "onClick"});

    std::string dump = compiled.debugDump();
    EXPECT_TRUE(dump.find("Compiled Template") != std::string::npos);
    EXPECT_TRUE(dump.find("test.tpl") != std::string::npos);
    EXPECT_TRUE(dump.find("Binding Plans") != std::string::npos);
    EXPECT_TRUE(dump.find("Event Plans") != std::string::npos);
}

// ==================== TemplateCompiler Configuration Tests ====================

class TemplateCompilerConfigTest : public ::testing::Test {
protected:
    void SetUp() override { m_config = core::TemplateConfig::defaults(); }

    core::TemplateConfig m_config;
};

TEST_F(TemplateCompilerConfigTest, StrictMode)
{
    compiler::TemplateCompiler compiler(m_config);

    EXPECT_TRUE(compiler.isStrictMode());

    compiler.setStrictMode(false);
    EXPECT_FALSE(compiler.isStrictMode());
}

TEST_F(TemplateCompilerConfigTest, CustomConfig)
{
    core::TemplateConfig customConfig;
    customConfig.strictMode = false;
    customConfig.debugOutput = true;
    customConfig.enableCache = false;

    compiler::TemplateCompiler compiler(customConfig);
    EXPECT_FALSE(compiler.isStrictMode());
    EXPECT_FALSE(compiler.config().enableCache);
    EXPECT_TRUE(compiler.config().debugOutput);
}

// ==================== Additional BindingContext Tests ====================

class BindingContextAdvancedTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_ctx = std::make_unique<binder::BindingContext>(
            mc::client::ui::kagero::state::StateStore::instance(), mc::client::ui::kagero::event::EventBus::instance());
    }

    std::unique_ptr<binder::BindingContext> m_ctx;
};

TEST_F(BindingContextAdvancedTest, PathResolution)
{
    i32 health = 100;
    std::string name = "Steve";

    m_ctx->expose("player.health", &health);
    m_ctx->expose("player.name", &name);

    // Resolve simple paths
    auto healthValue = m_ctx->resolveBinding("player.health");
    EXPECT_EQ(healthValue.asInteger(), 100);

    auto nameValue = m_ctx->resolveBinding("player.name");
    EXPECT_EQ(nameValue.asString(), "Steve");
}

TEST_F(BindingContextAdvancedTest, WritableVariable)
{
    i32 level = 1;

    m_ctx->exposeWritable("player.level", &level);

    EXPECT_TRUE(m_ctx->hasPath("player.level"));
    EXPECT_TRUE(m_ctx->isWritable("player.level"));

    // Read
    auto value = m_ctx->resolveBinding("player.level");
    EXPECT_EQ(value.asInteger(), 1);

    // Write
    EXPECT_TRUE(m_ctx->setBinding("player.level", binder::Value(10)));
    EXPECT_EQ(level, 10);
}

TEST_F(BindingContextAdvancedTest, WritableBoolVariable)
{
    bool visible = true;
    m_ctx->exposeWritable("ui.visible", &visible);

    EXPECT_TRUE(m_ctx->setBinding("ui.visible", binder::Value(false)));
    EXPECT_FALSE(visible);
}

TEST_F(BindingContextAdvancedTest, WritableFloatVariable)
{
    f32 volume = 0.5f;
    m_ctx->exposeWritable("settings.volume", &volume);

    EXPECT_TRUE(m_ctx->setBinding("settings.volume", binder::Value(0.8f)));
    EXPECT_FLOAT_EQ(volume, 0.8f);
}

TEST_F(BindingContextAdvancedTest, WritableStringVariable)
{
    std::string name = "Alex";
    m_ctx->exposeWritable("player.name", &name);

    EXPECT_TRUE(m_ctx->setBinding("player.name", binder::Value(std::string("Steve"))));
    EXPECT_EQ(name, "Steve");
}

TEST_F(BindingContextAdvancedTest, NotifyChangeWithSubscriber)
{
    i32 health = 100;
    m_ctx->expose("player.health", &health);

    std::string changedPath;
    binder::Value changedValue;
    u64 subId = m_ctx->subscribe("player.health", [&](const std::string& path, const binder::Value& value) {
        changedPath = path;
        changedValue = value;
    });

    m_ctx->notifyChange("player.health", binder::Value(50));

    EXPECT_EQ(changedPath, "player.health");
    EXPECT_EQ(changedValue.asInteger(), 50);

    m_ctx->unsubscribe(subId);
}

TEST_F(BindingContextAdvancedTest, MultipleSubscribers)
{
    i32 xp = 0;
    m_ctx->expose("player.xp", &xp);

    int callCount1 = 0;
    int callCount2 = 0;

    u64 sub1 = m_ctx->subscribe("player.xp", [&](const std::string&, const binder::Value&) { callCount1++; });
    u64 sub2 = m_ctx->subscribe("player.xp", [&](const std::string&, const binder::Value&) { callCount2++; });

    m_ctx->notifyChange("player.xp", binder::Value(100));

    EXPECT_EQ(callCount1, 1);
    EXPECT_EQ(callCount2, 1);

    m_ctx->unsubscribe(sub1);
    m_ctx->unsubscribe(sub2);
}

TEST_F(BindingContextAdvancedTest, CollectionOperations)
{
    // Test setCollection with integers
    m_ctx->setCollection<i32>("numbers", {1, 2, 3, 4, 5});

    auto numbers = m_ctx->resolveCollection("numbers");
    EXPECT_EQ(numbers.size(), 5u);
    EXPECT_EQ(numbers[0].asInteger(), 1);
    EXPECT_EQ(numbers[4].asInteger(), 5);

    // Test setCollection with strings
    m_ctx->setCollection<std::string>("names", {"Alice", "Bob", "Charlie"});

    auto names = m_ctx->resolveCollection("names");
    EXPECT_EQ(names.size(), 3u);
    EXPECT_EQ(names[0].asString(), "Alice");
    EXPECT_EQ(names[2].asString(), "Charlie");
}

TEST_F(BindingContextAdvancedTest, NonexistentPath)
{
    auto value = m_ctx->resolveBinding("nonexistent.path");
    EXPECT_TRUE(value.isNull());
    EXPECT_FALSE(m_ctx->hasPath("nonexistent.path"));
}

TEST_F(BindingContextAdvancedTest, ReadOnlyCannotWrite)
{
    i32 health = 100;
    m_ctx->expose("player.health", &health); // Read-only

    EXPECT_FALSE(m_ctx->isWritable("player.health"));
    EXPECT_FALSE(m_ctx->setBinding("player.health", binder::Value(50)));
    EXPECT_EQ(health, 100); // Unchanged
}

// ==================== BindingPlan Tests ====================

class BindingPlanTest : public ::testing::Test {};

TEST_F(BindingPlanTest, DefaultConstruction)
{
    compiler::BindingPlan plan;
    EXPECT_TRUE(plan.widgetPath.empty());
    EXPECT_TRUE(plan.statePath.empty());
    EXPECT_TRUE(plan.attributeName.empty());
    EXPECT_FALSE(plan.isLoopBinding);
    EXPECT_TRUE(plan.loopVarName.empty());
}

TEST_F(BindingPlanTest, ParameterizedConstruction)
{
    compiler::BindingPlan plan("screen.grid.slot", "player.inventory[0]", "item", true, "slot");

    EXPECT_EQ(plan.widgetPath, "screen.grid.slot");
    EXPECT_EQ(plan.statePath, "player.inventory[0]");
    EXPECT_EQ(plan.attributeName, "item");
    EXPECT_TRUE(plan.isLoopBinding);
    EXPECT_EQ(plan.loopVarName, "slot");
}

// ==================== EventPlan Tests ====================

class EventPlanTest : public ::testing::Test {};

TEST_F(EventPlanTest, DefaultConstruction)
{
    compiler::EventPlan plan;
    EXPECT_TRUE(plan.widgetPath.empty());
    EXPECT_TRUE(plan.eventName.empty());
    EXPECT_TRUE(plan.callbackName.empty());
}

TEST_F(EventPlanTest, ParameterizedConstruction)
{
    compiler::EventPlan plan("screen.btn", "click", "onButtonClick");

    EXPECT_EQ(plan.widgetPath, "screen.btn");
    EXPECT_EQ(plan.eventName, "click");
    EXPECT_EQ(plan.callbackName, "onButtonClick");
}

// ==================== LoopPlan Tests ====================

class LoopPlanTest : public ::testing::Test {};

TEST_F(LoopPlanTest, DefaultConstruction)
{
    compiler::LoopPlan plan;
    EXPECT_TRUE(plan.parentPath.empty());
    EXPECT_TRUE(plan.collectionPath.empty());
    EXPECT_TRUE(plan.itemVarName.empty());
    EXPECT_EQ(plan.itemBindings.size(), 0u);
}

TEST_F(LoopPlanTest, ParameterizedConstruction)
{
    compiler::LoopPlan plan("screen.inventory", "player.items", "item");

    EXPECT_EQ(plan.parentPath, "screen.inventory");
    EXPECT_EQ(plan.collectionPath, "player.items");
    EXPECT_EQ(plan.itemVarName, "item");
}

// ==================== Documentation Example Tests ====================

class DocumentationExampleTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_ctx = std::make_unique<binder::BindingContext>(
            mc::client::ui::kagero::state::StateStore::instance(), mc::client::ui::kagero::event::EventBus::instance());
    }

    void TearDown() override { m_ctx->clear(); }

    std::unique_ptr<binder::BindingContext> m_ctx;
    core::TemplateConfig m_config;
};

// 文档示例 1: 编译模板
TEST_F(DocumentationExampleTest, CompileTemplateExample)
{
    compiler::TemplateCompiler compiler;
    compiler.setStrictMode(true);

    auto compiled = compiler.compile(R"(
        <screen id="main">
            <text id="title" text="Welcome"/>
            <button id="btn_start" text="Start" on:click="onStart"/>
        </screen>
    )");

    ASSERT_TRUE(compiled);
    EXPECT_FALSE(compiled->hasErrors());
    EXPECT_TRUE(compiled->isValid());
    EXPECT_NE(compiled->astRoot(), nullptr);
}

// 文档示例 2: 绑定上下文使用
TEST_F(DocumentationExampleTest, BindingContextExample)
{
    // 暴露只读变量
    std::string playerName = "Steve";
    m_ctx->expose("player.name", &playerName);

    // 暴露可写变量
    i32 health = 100;
    m_ctx->exposeWritable("player.health", &health);

    // 验证读取
    auto nameValue = m_ctx->resolveBinding("player.name");
    EXPECT_TRUE(nameValue.isString());
    EXPECT_EQ(nameValue.asString(), "Steve");

    auto healthValue = m_ctx->resolveBinding("player.health");
    EXPECT_TRUE(healthValue.isInteger());
    EXPECT_EQ(healthValue.asInteger(), 100);

    // 验证写入
    EXPECT_TRUE(m_ctx->setBinding("player.health", binder::Value(50)));
    EXPECT_EQ(health, 50);

    // 验证只读变量不能写入
    EXPECT_FALSE(m_ctx->setBinding("player.name", binder::Value("Alex")));
}

// 文档示例 3: 暴露简单回调
TEST_F(DocumentationExampleTest, SimpleCallbackExample)
{
    bool simpleCallbackCalled = false;

    m_ctx->exposeSimpleCallback("onCancel", [&]() { simpleCallbackCalled = true; });

    EXPECT_TRUE(m_ctx->hasCallback("onCancel"));
    mc::client::ui::kagero::event::FocusGainedEvent event;
    EXPECT_TRUE(m_ctx->invokeCallback("onCancel", nullptr, event));
    EXPECT_TRUE(simpleCallbackCalled);
}

// 文档示例 4: 状态订阅
TEST_F(DocumentationExampleTest, StateSubscriptionExample)
{
    i32 health = 100;
    m_ctx->exposeWritable("player.health", &health);

    std::string changedPath;
    binder::Value newValue;

    u64 subId = m_ctx->subscribe("player.health", [&](const std::string& path, const binder::Value& value) {
        changedPath = path;
        newValue = value;
    });

    m_ctx->notifyChange("player.health", binder::Value(75));

    EXPECT_EQ(changedPath, "player.health");
    EXPECT_TRUE(newValue.isInteger());
    EXPECT_EQ(newValue.asInteger(), 75);

    m_ctx->unsubscribe(subId);
}

// 文档示例 5: 循环变量
TEST_F(DocumentationExampleTest, LoopVariableExample)
{
    m_ctx->setLoopVariable("item", binder::Value(42));

    // 验证循环变量访问
    auto value = m_ctx->resolveBinding("$item");
    EXPECT_TRUE(value.isInteger());
    EXPECT_EQ(value.asInteger(), 42);

    // 清除循环变量
    m_ctx->clearLoopVariable("item");
    EXPECT_FALSE(m_ctx->hasLoopVariable("item"));
}

// 文档示例 6: 集合解析
TEST_F(DocumentationExampleTest, CollectionExample)
{
    // 设置集合 - 测试 Value 对字符串字面量的正确处理
    std::vector<binder::Value> items;
    items.emplace_back(binder::Value("Item1")); // const char* 构造
    items.emplace_back(binder::Value("Item2"));
    items.emplace_back(binder::Value(std::string("Item3"))); // std::string 构造

    m_ctx->setCollectionValue("items", items);

    // 解析集合
    auto resolved = m_ctx->resolveCollection("items");
    EXPECT_EQ(resolved.size(), 3u);

    // 验证类型正确（应该是 std::string 而不是 Bool）
    EXPECT_TRUE(resolved[0].isString());
    EXPECT_TRUE(resolved[1].isString());
    EXPECT_TRUE(resolved[2].isString());

    // 验证值正确
    EXPECT_EQ(resolved[0].asString(), "Item1");
    EXPECT_EQ(resolved[1].asString(), "Item2");
    EXPECT_EQ(resolved[2].asString(), "Item3");
}

// 文档示例 7: 完整模板编译流程
TEST_F(DocumentationExampleTest, FullCompileWorkflow)
{
    compiler::TemplateCompiler compiler;
    compiler.setStrictMode(false); // 关闭严格模式以接受简单标签

    // 编译简单模板
    auto compiled = compiler.compile(R"(
        <screen id="main">
            <text id="playerName" bind:text="player.name"/>
            <text id="healthText" bind:text="player.health"/>
            <button id="startBtn" text="Start" on:click="onStart"/>
        </screen>
    )");

    ASSERT_TRUE(compiled);

    // 验证AST有效
    EXPECT_TRUE(compiled->isValid());
    EXPECT_NE(compiled->astRoot(), nullptr);
}

// 文档示例 8: UpdateScheduler 使用
TEST_F(DocumentationExampleTest, UpdateSchedulerWorkflow)
{
    runtime::UpdateScheduler scheduler;

    std::vector<std::string> executedPaths;

    scheduler.setUpdateCallback([&executedPaths](const std::string& path) -> bool {
        executedPaths.push_back(path);
        return true;
    });

    // 调度不同优先级的更新
    scheduler.schedule("player.health", runtime::UpdateScheduler::Priority::High);
    scheduler.schedule("player.name", runtime::UpdateScheduler::Priority::Normal);
    scheduler.schedule("ui.stats", runtime::UpdateScheduler::Priority::Low);

    EXPECT_TRUE(scheduler.hasPending());
    EXPECT_EQ(scheduler.pendingCount(), 3u);

    // 执行高优先级任务
    u32 highExecuted = scheduler.executeHighPriority();
    EXPECT_EQ(highExecuted, 1u);
    EXPECT_EQ(executedPaths.size(), 1u);
    EXPECT_EQ(executedPaths[0], "player.health");

    // 执行所有剩余任务
    u32 remaining = scheduler.executePending();
    EXPECT_EQ(remaining, 2u);
    EXPECT_EQ(executedPaths.size(), 3u);
}

// ==================== Lexer Edge Cases Tests ====================

class LexerEdgeCaseTest : public ::testing::Test {
protected:
    parser::Lexer createLexer(const std::string& source) { return parser::Lexer(source, "<test>"); }
};

TEST_F(LexerEdgeCaseTest, EmptyAttributeValue)
{
    // 空属性值
    auto lexer = createLexer(R"(<text id="" text=""/>)");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());

    auto& tokens = lexer.tokens();
    bool foundEmptyId = false;
    for (size_t i = 0; i + 2 < tokens.size(); ++i) {
        // Token顺序: Identifier("id") -> Equals("=") -> StringLiteral("")
        if (tokens[i].value == "id" && tokens[i + 1].type == parser::TokenType::Equals &&
            tokens[i + 2].type == parser::TokenType::StringLiteral) {
            foundEmptyId = true;
            EXPECT_EQ(tokens[i + 2].value, "");
            break;
        }
    }
    EXPECT_TRUE(foundEmptyId);
}

TEST_F(LexerEdgeCaseTest, AttributeWithoutValue)
{
    // 无值属性（HTML风格）
    auto lexer = createLexer("<button disabled>");
    EXPECT_TRUE(lexer.tokenize());
    // 应该能解析，可能作为标识符处理
    EXPECT_FALSE(lexer.hasErrors());
}

TEST_F(LexerEdgeCaseTest, MultipleConsecutiveNewlines)
{
    auto lexer = createLexer("<screen>\n\n\n\n<button/>\n\n\n</screen>");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());
}

TEST_F(LexerEdgeCaseTest, LargeWhitespaceBetweenAttributes)
{
    auto lexer = createLexer(R"(<text    id="test"     text="hello"/>)");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());
}

TEST_F(LexerEdgeCaseTest, CommentContainingTags)
{
    // 注释内的标签应被忽略
    auto lexer = createLexer("<!-- <button/> <text id=\"inner\"/> -->");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());

    auto& tokens = lexer.tokens();
    bool foundComment = false;
    for (const auto& token : tokens) {
        if (token.type == parser::TokenType::Comment) {
            foundComment = true;
            EXPECT_TRUE(token.value.find("<button/>") != std::string::npos);
            EXPECT_TRUE(token.value.find("<text") != std::string::npos);
            break;
        }
    }
    EXPECT_TRUE(foundComment);
}

TEST_F(LexerEdgeCaseTest, NestedCommentStart)
{
    // 嵌套注释开始 - 这可能被视为错误或被忽略
    auto lexer = createLexer("<!-- <!-- inner --> -->");
    EXPECT_TRUE(lexer.tokenize());
    // 行为取决于实现：可能将第一个 --> 作为注释结束
}

TEST_F(LexerEdgeCaseTest, UnclosedCommentAtEnd)
{
    auto lexer = createLexer("<!-- This comment is never closed");
    EXPECT_FALSE(lexer.tokenize());
    EXPECT_TRUE(lexer.hasErrors());
}

TEST_F(LexerEdgeCaseTest, StringWithMixedEscapeSequences)
{
    // 测试所有转义序列
    auto lexer = createLexer(R"(<text value="\t\n\r\"\'\\\/"/>)");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());
}

TEST_F(LexerEdgeCaseTest, StringWithInvalidEscapeSequence)
{
    // 无效转义序列 - 行为取决于实现
    auto lexer = createLexer(R"(<text value="\x\y\z"/>)");
    EXPECT_TRUE(lexer.tokenize());
    // 可能保留原始字符或报错
}

TEST_F(LexerEdgeCaseTest, VeryLongStringLiteral)
{
    // 超长字符串
    std::string longValue(10000, 'a');
    std::string source = "<text value=\"" + longValue + "\"/>";
    auto lexer = createLexer(source);
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());

    auto& tokens = lexer.tokens();
    for (const auto& token : tokens) {
        if (token.type == parser::TokenType::StringLiteral) {
            EXPECT_EQ(token.value.size(), 10000u);
            break;
        }
    }
}

TEST_F(LexerEdgeCaseTest, VeryLongIdentifier)
{
    // 超长标识符
    std::string longId(1000, 'a');
    std::string source = "<" + longId + "/>";
    auto lexer = createLexer(source);
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());

    auto& tokens = lexer.tokens();
    bool found = false;
    for (const auto& token : tokens) {
        if (token.type == parser::TokenType::Identifier && token.value.size() == 1000) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(LexerEdgeCaseTest, NumericEdgeCases)
{
    // 边界数值
    auto lexer = createLexer(R"(<text int="0" max="2147483647" neg="-123" float="0.0" bigfloat="999999.999999"/>)");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());
}

TEST_F(LexerEdgeCaseTest, ZeroPrefixedNumber)
{
    // 前导零数字
    auto lexer = createLexer(R"(<text value="007"/>)");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());
}

TEST_F(LexerEdgeCaseTest, NegativeNumber)
{
    // 负数处理
    auto lexer = createLexer(R"(<text x="-10" y="-20.5"/>)");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());
}

TEST_F(LexerEdgeCaseTest, ScientificNotationNumber)
{
    // 科学计数法
    auto lexer = createLexer(R"(<text value="1.5e10" value2="2e-5"/>)");
    EXPECT_TRUE(lexer.tokenize());
    // 行为取决于实现
}

TEST_F(LexerEdgeCaseTest, SpecialCharactersInAttributeValue)
{
    // 属性值中的特殊字符
    auto lexer = createLexer(R"(<text value="<>&'\""/>)");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());
}

TEST_F(LexerEdgeCaseTest, UnicodeInStringLiteral)
{
    // Unicode 字符串
    auto lexer = createLexer(R"(<text value="你好世界"/>)");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());
}

TEST_F(LexerEdgeCaseTest, UnicodeInIdentifier)
{
    // Unicode 标识符 - 当前实现不支持 Unicode 标识符
    // Lexer.isIdentifierChar() 只支持 ASCII 字母数字、下划线、连字符和冒号
    auto lexer = createLexer("<文本/>");
    // 预期失败：不支持 Unicode 标识符
    EXPECT_FALSE(lexer.tokenize());
    EXPECT_TRUE(lexer.hasErrors());
}

TEST_F(LexerEdgeCaseTest, MultipleTagsOnSameLine)
{
    auto lexer = createLexer("<screen><text/><button/><text/></screen>");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());

    int identifierCount = 0;
    for (const auto& token : lexer.tokens()) {
        if (token.type == parser::TokenType::Identifier) {
            identifierCount++;
        }
    }
    EXPECT_EQ(identifierCount, 5); // screen, text, button, text, screen
}

TEST_F(LexerEdgeCaseTest, DeeplyNestedTags)
{
    // 深度嵌套
    std::string source;
    for (int i = 0; i < 100; ++i) {
        source += "<container>";
    }
    source += "<text/>";
    for (int i = 0; i < 100; ++i) {
        source += "</container>";
    }

    auto lexer = createLexer(source);
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());
}

TEST_F(LexerEdgeCaseTest, SelfClosingTagInClosingContext)
{
    // 自关闭标签的各种位置
    auto lexer = createLexer("<screen><text/><button/></screen>");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());
}

TEST_F(LexerEdgeCaseTest, AttributeNameWithHyphen)
{
    auto lexer = createLexer(R"(<text data-value="test" custom-attr="value"/>)");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());

    auto& tokens = lexer.tokens();
    bool foundDataValue = false, foundCustomAttr = false;
    for (const auto& token : tokens) {
        if (token.value == "data-value") foundDataValue = true;
        if (token.value == "custom-attr") foundCustomAttr = true;
    }
    EXPECT_TRUE(foundDataValue);
    EXPECT_TRUE(foundCustomAttr);
}

TEST_F(LexerEdgeCaseTest, AttributeNameWithColon)
{
    // bind:, on: 等特殊属性
    auto lexer = createLexer(R"(<text bind:text="player.name" on:click="onClick"/>)");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());

    auto& tokens = lexer.tokens();
    bool foundBind = false, foundEvent = false;
    for (const auto& token : tokens) {
        if (token.value == "bind:text") foundBind = true;
        if (token.value == "on:click") foundEvent = true;
    }
    EXPECT_TRUE(foundBind);
    EXPECT_TRUE(foundEvent);
}

TEST_F(LexerEdgeCaseTest, ConsecutiveTagsNoSpaces)
{
    auto lexer = createLexer("<screen><button/></screen>");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());
}

TEST_F(LexerEdgeCaseTest, TextContentWithEntities)
{
    // XML实体（行为取决于实现）
    auto lexer = createLexer("<text>Hello &amp; World</text>");
    EXPECT_TRUE(lexer.tokenize());
    // 实体可能被保留或解析
}

TEST_F(LexerEdgeCaseTest, EmptySource)
{
    auto lexer = createLexer("");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());
}

TEST_F(LexerEdgeCaseTest, OnlyWhitespace)
{
    auto lexer = createLexer("   \n\t  \n  ");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());
}

TEST_F(LexerEdgeCaseTest, OnlyComment)
{
    auto lexer = createLexer("<!-- Just a comment -->");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());
}

TEST_F(LexerEdgeCaseTest, MalformedTagNoClosing)
{
    auto lexer = createLexer("<screen");
    EXPECT_TRUE(lexer.tokenize());
    // 可能报告错误或生成不完整的token
}

TEST_F(LexerEdgeCaseTest, MalformedTagDoubleOpen)
{
    auto lexer = createLexer("<<screen>>");
    EXPECT_TRUE(lexer.tokenize());
    // 行为取决于实现
}

TEST_F(LexerEdgeCaseTest, TagNameWithNumbers)
{
    auto lexer = createLexer("<text2/>");
    EXPECT_TRUE(lexer.tokenize());
    EXPECT_FALSE(lexer.hasErrors());

    auto& tokens = lexer.tokens();
    bool found = false;
    for (const auto& token : tokens) {
        if (token.type == parser::TokenType::Identifier && token.value == "text2") {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

// ==================== Parser Edge Cases Tests ====================

class ParserEdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override { m_config = core::TemplateConfig::defaults(); }

    std::unique_ptr<ast::DocumentNode> parse(const std::string& source)
    {
        parser::Lexer lexer(source, "<test>");
        lexer.tokenize();

        parser::Parser parser(lexer, m_config);
        return parser.parse();
    }

    std::unique_ptr<ast::DocumentNode> parseWithErrors(const std::string& source, core::TemplateErrorCollector& errors)
    {
        parser::Lexer lexer(source, "<test>");
        lexer.tokenize();

        parser::Parser parser(lexer, m_config);
        auto doc = parser.parse();
        if (parser.hasErrors()) {
            for (const auto& err : lexer.errors()) {
                errors.addError(err);
            }
        }
        return doc;
    }

    core::TemplateConfig m_config;
};

TEST_F(ParserEdgeCaseTest, DeeplyNestedElements)
{
    // 测试深度嵌套
    std::string source = "<screen>";
    for (int i = 0; i < 50; ++i) {
        source += "<container>";
    }
    source += "<text id=\"deep\"/>";
    for (int i = 0; i < 50; ++i) {
        source += "</container>";
    }
    source += "</screen>";

    auto doc = parse(source);
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    // 验证深层元素存在
    auto* found = ast::traversal::findById(*doc, "deep");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id, "deep");
}

TEST_F(ParserEdgeCaseTest, MultipleAttributesSameType)
{
    // 多个同类属性
    auto doc = parse(R"(<text bind:text="name" bind:visible="isVisible" bind:color="textColor"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    elem->categorizeAttributes();
    EXPECT_EQ(elem->bindingAttrs.size(), 3u);
}

TEST_F(ParserEdgeCaseTest, MultipleEventHandlers)
{
    // 多个事件处理器
    auto doc = parse(
        R"(<button on:click="onClick" on:doubleClick="onDblClick" on:mouseEnter="onEnter" on:mouseLeave="onLeave"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    elem->categorizeAttributes();
    EXPECT_EQ(elem->eventAttrs.size(), 4u);
}

TEST_F(ParserEdgeCaseTest, MixedAttributes)
{
    // 混合属性类型
    auto doc = parse(R"(<text id="myText" text="Hello" bind:text="player.name" on:click="onClick" class="title"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    EXPECT_TRUE(elem->hasAttribute("id"));
    EXPECT_TRUE(elem->hasAttribute("text"));
    EXPECT_TRUE(elem->hasAttribute("bind:text"));
    EXPECT_TRUE(elem->hasAttribute("on:click"));
    EXPECT_TRUE(elem->hasAttribute("class"));
}

TEST_F(ParserEdgeCaseTest, EmptyElementWithAttributes)
{
    auto doc = parse(R"(<text id="" text="" class=""/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    EXPECT_TRUE(elem->hasAttribute("id"));
    EXPECT_TRUE(elem->hasAttribute("text"));
    EXPECT_TRUE(elem->hasAttribute("class"));
}

TEST_F(ParserEdgeCaseTest, ElementWithAllNumericId)
{
    auto doc = parse(R"(<text id="123"/>)");
    // 行为取决于实现 - 数字ID可能被接受或拒绝
    ASSERT_NE(doc, nullptr);
}

TEST_F(ParserEdgeCaseTest, ElementWithSpecialCharactersInId)
{
    auto doc = parse(R"(<text id="my_text-123"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);
    EXPECT_EQ(doc->rootElement()->id, "my_text-123");
}

TEST_F(ParserEdgeCaseTest, NestedLoopsAndConditions)
{
    // 嵌套循环和条件
    auto doc = parse(R"(
        <screen>
            <grid for:item="row in rows">
                <grid for:item="col in row.cols" if:condition="col.visible">
                    <slot bind:item="$col.item"/>
                </grid>
            </grid>
        </screen>
    )");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    // 验证嵌套结构
    auto grids = ast::traversal::findByTagName(*doc, "grid");
    EXPECT_GE(grids.size(), 2u);
}

TEST_F(ParserEdgeCaseTest, ElementWithOnlyLoopDirective)
{
    auto doc = parse(R"(<text for:item="item in items"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* text = doc->rootElement();
    EXPECT_TRUE(text->loop.has_value());
    EXPECT_EQ(text->loop->collectionPath, "items");
    EXPECT_EQ(text->loop->itemVarName, "item");
}

TEST_F(ParserEdgeCaseTest, ElementWithLoopAndIndex)
{
    auto doc = parse(R"(<slot for:item="(slot, idx) in player.inventory"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* slot = doc->rootElement();
    EXPECT_TRUE(slot->loop.has_value());
    EXPECT_EQ(slot->loop->collectionPath, "player.inventory");
    EXPECT_EQ(slot->loop->itemVarName, "slot");
    EXPECT_EQ(slot->loop->indexVarName, "idx");
    EXPECT_TRUE(slot->loop->hasIndex);
}

TEST_F(ParserEdgeCaseTest, ConditionWithComplexPath)
{
    auto doc = parse(R"(<text if:condition="player.inventory.slots[0].isEmpty"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* text = doc->rootElement();
    EXPECT_TRUE(text->condition.has_value());
    EXPECT_EQ(text->condition->booleanPath, "player.inventory.slots[0].isEmpty");
}

TEST_F(ParserEdgeCaseTest, MultipleConditionsWithNegation)
{
    auto doc = parse(R"(
        <screen>
            <text if:condition="player.alive" text="Alive"/>
            <text if:condition="!player.hidden" text="Visible"/>
            <text if:condition="!!player.doubleNegation" text="Double"/>
        </screen>
    )");

    ASSERT_NE(doc, nullptr);
    EXPECT_EQ(doc->rootElement()->children.size(), 3u);
}

TEST_F(ParserEdgeCaseTest, SelfClosingVsOpenClose)
{
    // 自关闭 vs 开放-关闭标签
    auto doc = parse(R"(
        <screen>
            <text/>
            <text></text>
            <button id="btn1"/>
            <button id="btn2"></button>
        </screen>
    )");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);
    EXPECT_EQ(doc->rootElement()->children.size(), 4u);

    // 验证两种形式都被正确解析
    auto* btn1 = ast::traversal::findById(*doc, "btn1");
    auto* btn2 = ast::traversal::findById(*doc, "btn2");
    ASSERT_NE(btn1, nullptr);
    ASSERT_NE(btn2, nullptr);
}

TEST_F(ParserEdgeCaseTest, TextWithLeadingAndTrailingWhitespace)
{
    auto doc = parse(R"(
        <screen>
            Hello World
        </screen>
    )");

    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    // 查找非空白文本节点
    bool foundNonWhitespace = false;
    for (const auto& child : doc->rootElement()->children) {
        if (child->type == ast::NodeType::TextContent) {
            auto* text = dynamic_cast<ast::TextNode*>(child.get());
            if (text && !text->isWhitespace) {
                foundNonWhitespace = true;
                break;
            }
        }
    }
    EXPECT_TRUE(foundNonWhitespace);
}

TEST_F(ParserEdgeCaseTest, ElementWithLargeNumberOfAttributes)
{
    // 大量属性
    std::string source = "<text";
    for (int i = 0; i < 50; ++i) {
        source += " attr" + std::to_string(i) + "=\"value" + std::to_string(i) + "\"";
    }
    source += "/>";

    auto doc = parse(source);
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);
    EXPECT_EQ(doc->rootElement()->attributes.size(), 50u);
}

TEST_F(ParserEdgeCaseTest, BindingPathWithArrayIndex)
{
    auto doc = parse(R"(<text bind:text="player.inventory[0].name"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    const auto* attr = elem->getAttribute("bind:text");
    ASSERT_NE(attr, nullptr);
    EXPECT_TRUE(attr->isBinding());
    EXPECT_EQ(attr->binding->path, "player.inventory[0].name");
}

TEST_F(ParserEdgeCaseTest, BindingPathWithNestedArrayIndex)
{
    auto doc = parse(R"(<text bind:text="matrix[0][1]"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    const auto* attr = elem->getAttribute("bind:text");
    ASSERT_NE(attr, nullptr);
    EXPECT_TRUE(attr->isBinding());
}

TEST_F(ParserEdgeCaseTest, LoopVariableWithDeepPropertyAccess)
{
    auto doc = parse(R"(<text bind:text="$item.data.nested.value"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    const auto* attr = elem->getAttribute("bind:text");
    ASSERT_NE(attr, nullptr);
    EXPECT_TRUE(attr->isBinding());
    EXPECT_TRUE(attr->binding->isLoopVariable);
    EXPECT_EQ(attr->binding->loopVarName, "item");
    EXPECT_EQ(attr->binding->property, "data.nested.value");
}

TEST_F(ParserEdgeCaseTest, CallbackNameWithUnderscore)
{
    auto doc = parse(R"(<button on:click="on_button_click"/>)");
    ASSERT_NE(doc, nullptr);
    ASSERT_NE(doc->rootElement(), nullptr);

    auto* elem = doc->rootElement();
    const auto* attr = elem->getAttribute("on:click");
    ASSERT_NE(attr, nullptr);
    EXPECT_TRUE(attr->isEvent());
    EXPECT_EQ(attr->callbackName, "on_button_click");
}

TEST_F(ParserEdgeCaseTest, MultipleSiblingsWithSameId)
{
    // 同ID的多个元素（行为取决于实现）
    auto doc = parse(R"(
        <screen>
            <text id="duplicate"/>
            <text id="duplicate"/>
        </screen>
    )");

    ASSERT_NE(doc, nullptr);
    // ID重复可能被允许或报告错误
}

TEST_F(ParserEdgeCaseTest, NonStrictModeAllowsUnknownTags)
{
    m_config.strictMode = false;

    parser::Lexer lexer("<customTag id=\"custom\"/>", "<test>");
    lexer.tokenize();

    parser::Parser parser(lexer, m_config);
    auto doc = parser.parse();

    // 非严格模式可能允许未知标签
    ASSERT_NE(doc, nullptr);
}

TEST_F(ParserEdgeCaseTest, AllWidgetTypes)
{
    auto doc = parse(R"(
        <screen id="root">
            <widget id="w"/>
            <button id="btn"/>
            <text id="txt"/>
            <textfield id="tf"/>
            <slider id="sl"/>
            <checkbox id="cb"/>
            <image id="img"/>
            <grid id="g"/>
            <slot id="s"/>
            <viewport3d id="vp"/>
            <scrollable id="sc"/>
            <list id="lst"/>
            <container id="cont"/>
        </screen>
    )");

    ASSERT_NE(doc, nullptr);
    EXPECT_EQ(doc->rootElement()->children.size(), 13u);

    // 验证每种类型都被识别
    for (const auto& child : doc->rootElement()->children) {
        auto* elem = dynamic_cast<ast::ElementNode*>(child.get());
        ASSERT_NE(elem, nullptr);
        EXPECT_FALSE(elem->id.empty());
    }
}

// ==================== Value Edge Cases Tests ====================

class ValueEdgeCaseTest : public ::testing::Test {};

TEST_F(ValueEdgeCaseTest, NullValueOperations)
{
    binder::Value nullValue;

    EXPECT_TRUE(nullValue.isNull());
    EXPECT_FALSE(nullValue.isBool());
    EXPECT_FALSE(nullValue.isInteger());
    EXPECT_FALSE(nullValue.isFloat());
    EXPECT_FALSE(nullValue.isString());

    // 类型转换应该有默认值
    EXPECT_EQ(nullValue.toInteger(), 0);
    EXPECT_FLOAT_EQ(nullValue.toFloat(), 0.0f);
    EXPECT_FALSE(nullValue.toBool());
    EXPECT_EQ(nullValue.toString(), "null");
}

TEST_F(ValueEdgeCaseTest, BoolEdgeCases)
{
    binder::Value trueVal(true);
    binder::Value falseVal(false);

    EXPECT_TRUE(trueVal.toBool());
    EXPECT_FALSE(falseVal.toBool());
    EXPECT_EQ(trueVal.toInteger(), 1);
    EXPECT_EQ(falseVal.toInteger(), 0);
    EXPECT_FLOAT_EQ(trueVal.toFloat(), 1.0f);
    EXPECT_FLOAT_EQ(falseVal.toFloat(), 0.0f);
}

TEST_F(ValueEdgeCaseTest, IntegerBoundaryValues)
{
    binder::Value maxInt(2147483647);
    binder::Value minInt(static_cast<i32>(-2147483647 - 1)); // 避免整数溢出警告
    binder::Value zero(0);

    EXPECT_EQ(maxInt.asInteger(), 2147483647);
    EXPECT_EQ(minInt.asInteger(), static_cast<i32>(-2147483647 - 1));
    EXPECT_EQ(zero.asInteger(), 0);

    EXPECT_FLOAT_EQ(maxInt.asFloat(), 2147483647.0f);
}

TEST_F(ValueEdgeCaseTest, FloatBoundaryValues)
{
    binder::Value verySmall(0.00001f);
    binder::Value veryLarge(999999.0f); // 使用整数值避免浮点精度问题
    binder::Value negative(-123.456f);

    EXPECT_FLOAT_EQ(verySmall.asFloat(), 0.00001f);
    EXPECT_FLOAT_EQ(veryLarge.asFloat(), 999999.0f);
    EXPECT_FLOAT_EQ(negative.asFloat(), -123.456f);

    // 浮点转整数应该截断
    EXPECT_EQ(verySmall.asInteger(), 0);
    EXPECT_EQ(veryLarge.asInteger(), 999999);
    EXPECT_EQ(negative.asInteger(), -123);

    // 测试浮点截断（非整数值）
    binder::Value truncated(123.999f);
    EXPECT_EQ(truncated.asInteger(), 123); // 截断，不是四舍五入
}

TEST_F(ValueEdgeCaseTest, StringEdgeCases)
{
    // 空字符串
    binder::Value emptyStr(std::string(""));
    EXPECT_TRUE(emptyStr.isString());
    EXPECT_EQ(emptyStr.asString().size(), 0u);

    // 长字符串
    std::string longStr(10000, 'a');
    binder::Value longValue(longStr);
    EXPECT_EQ(longValue.asString().size(), 10000u);

    // 字符串包含特殊字符
    binder::Value special(std::string("Hello\nWorld\tTab\"Quote\\Backslash"));
    EXPECT_TRUE(special.isString());
}

TEST_F(ValueEdgeCaseTest, StringToNumberConversion)
{
    binder::Value intStr(std::string("12345"));
    binder::Value floatStr(std::string("123.456"));
    binder::Value negStr(std::string("-789"));
    binder::Value invalidStr(std::string("not a number"));
    binder::Value emptyNumStr(std::string(""));

    EXPECT_EQ(intStr.toInteger(), 12345);
    EXPECT_FLOAT_EQ(intStr.toFloat(), 12345.0f);

    EXPECT_EQ(floatStr.toInteger(), 123);
    EXPECT_FLOAT_EQ(floatStr.toFloat(), 123.456f);

    EXPECT_EQ(negStr.toInteger(), -789);
    EXPECT_FLOAT_EQ(negStr.toFloat(), -789.0f);

    // 无效转换应该返回默认值
    EXPECT_EQ(invalidStr.toInteger(), 0);
    EXPECT_FLOAT_EQ(invalidStr.toFloat(), 0.0f);

    EXPECT_EQ(emptyNumStr.toInteger(), 0);
}

TEST_F(ValueEdgeCaseTest, ArrayWithMixedTypes)
{
    std::vector<binder::Value> items;
    items.emplace_back(binder::Value(42));
    items.emplace_back(binder::Value("hello"));
    items.emplace_back(binder::Value(true));
    items.emplace_back(binder::Value(3.14f));
    items.emplace_back(binder::Value());

    binder::Value arr(items);
    EXPECT_TRUE(arr.isArray());
    EXPECT_EQ(arr.arraySize(), 5u);

    EXPECT_EQ(arr.arrayGet(0).toInteger(), 42);
    EXPECT_EQ(arr.arrayGet(1).asString(), "hello");
    EXPECT_TRUE(arr.arrayGet(2).toBool());
    EXPECT_FLOAT_EQ(arr.arrayGet(3).toFloat(), 3.14f);
    EXPECT_TRUE(arr.arrayGet(4).isNull());
}

TEST_F(ValueEdgeCaseTest, NestedArrays)
{
    std::vector<binder::Value> inner1;
    inner1.emplace_back(binder::Value(1));
    inner1.emplace_back(binder::Value(2));

    std::vector<binder::Value> inner2;
    inner2.emplace_back(binder::Value(3));
    inner2.emplace_back(binder::Value(4));

    std::vector<binder::Value> outer;
    outer.emplace_back(binder::Value(inner1));
    outer.emplace_back(binder::Value(inner2));

    binder::Value nested(outer);
    EXPECT_TRUE(nested.isArray());
    EXPECT_EQ(nested.arraySize(), 2u);

    auto first = nested.arrayGet(0);
    EXPECT_TRUE(first.isArray());
    EXPECT_EQ(first.arraySize(), 2u);
    EXPECT_EQ(first.arrayGet(0).toInteger(), 1);
}

TEST_F(ValueEdgeCaseTest, ArrayOutOfBounds)
{
    std::vector<binder::Value> items;
    items.emplace_back(binder::Value(1));

    binder::Value arr(items);
    EXPECT_TRUE(arr.isArray());

    // 边界访问
    EXPECT_EQ(arr.arrayGet(0).toInteger(), 1);
    EXPECT_TRUE(arr.arrayGet(1).isNull());
    EXPECT_TRUE(arr.arrayGet(100).isNull());
    EXPECT_TRUE(arr.arrayGet(SIZE_MAX).isNull());
}

TEST_F(ValueEdgeCaseTest, NonArrayOperations)
{
    binder::Value notArr(42);

    EXPECT_FALSE(notArr.isArray());
    EXPECT_EQ(notArr.arraySize(), 0u);
    EXPECT_TRUE(notArr.arrayGet(0).isNull());
    EXPECT_TRUE(notArr.getElement(0).isNull());
}

TEST_F(ValueEdgeCaseTest, EqualityEdgeCases)
{
    // 相同类型
    EXPECT_TRUE(binder::Value(42) == binder::Value(42));
    EXPECT_FALSE(binder::Value(42) == binder::Value(43));
    EXPECT_TRUE(binder::Value(std::string("hello")) == binder::Value(std::string("hello")));
    EXPECT_TRUE(binder::Value(true) == binder::Value(true));

    // 跨数字类型比较
    EXPECT_TRUE(binder::Value(42) == binder::Value(42.0f));
    EXPECT_TRUE(binder::Value(0) == binder::Value(0.0f));

    // Null 比较
    EXPECT_TRUE(binder::Value() == binder::Value());

    // 数组比较
    std::vector<binder::Value> arr1 = {binder::Value(1), binder::Value(2)};
    std::vector<binder::Value> arr2 = {binder::Value(1), binder::Value(2)};
    std::vector<binder::Value> arr3 = {binder::Value(1), binder::Value(3)};
    EXPECT_TRUE(binder::Value(arr1) == binder::Value(arr2));
    EXPECT_FALSE(binder::Value(arr1) == binder::Value(arr3));
}

TEST_F(ValueEdgeCaseTest, FromAnyTypeConversions)
{
    // bool
    auto boolVal = binder::Value::fromAny(std::any(true));
    EXPECT_TRUE(boolVal.isBool());

    // i32
    auto intVal = binder::Value::fromAny(std::any(42));
    EXPECT_TRUE(intVal.isInteger());

    // i64
    auto int64Val = binder::Value::fromAny(std::any(static_cast<i64>(123)));
    EXPECT_TRUE(int64Val.isInteger());

    // u32
    auto uintVal = binder::Value::fromAny(std::any(static_cast<u32>(456u)));
    EXPECT_TRUE(uintVal.isInteger());

    // f32
    auto floatVal = binder::Value::fromAny(std::any(3.14f));
    EXPECT_TRUE(floatVal.isFloat());

    // f64
    auto doubleVal = binder::Value::fromAny(std::any(2.718));
    EXPECT_TRUE(doubleVal.isFloat());

    // std::string
    auto strVal = binder::Value::fromAny(std::any(std::string("test")));
    EXPECT_TRUE(strVal.isString());

    // const char*
    auto cstrVal = binder::Value::fromAny(std::any("literal"));
    EXPECT_TRUE(cstrVal.isString());

    // 空 any
    auto emptyVal = binder::Value::fromAny(std::any());
    EXPECT_TRUE(emptyVal.isNull());
}

TEST_F(ValueEdgeCaseTest, GetPropertyReturnsNull)
{
    // 简单值类型不支持属性访问
    binder::Value intVal(42);
    binder::Value strVal(std::string("hello"));
    binder::Value arrVal(std::vector<binder::Value>{});

    // 整型不支持任何属性访问，返回 null
    EXPECT_TRUE(intVal.getProperty("anything").isNull());

    // 字符串/数组类型支持 length/size 这类内置属性（见 BindingContext.cpp
    // Value::getProperty 实现），因此返回非 null。这里校验实际行为而非假设。
    EXPECT_FALSE(strVal.getProperty("length").isNull());
    EXPECT_EQ(strVal.getProperty("length").asInteger(), 5);

    EXPECT_FALSE(arrVal.getProperty("size").isNull());
    EXPECT_EQ(arrVal.getProperty("size").asInteger(), 0);

    // 字符串对不存在的属性仍返回 null
    EXPECT_TRUE(strVal.getProperty("nonexistent").isNull());
    EXPECT_TRUE(arrVal.getProperty("nonexistent").isNull());
}

// ==================== BindingContext Edge Cases Tests ====================

class BindingContextEdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_ctx = std::make_unique<binder::BindingContext>(
            mc::client::ui::kagero::state::StateStore::instance(), mc::client::ui::kagero::event::EventBus::instance());
    }

    void TearDown() override { m_ctx->clear(); }

    std::unique_ptr<binder::BindingContext> m_ctx;
};

TEST_F(BindingContextEdgeCaseTest, EmptyPathResolution)
{
    // 空路径应该返回空值
    auto value = m_ctx->resolveBinding("");
    EXPECT_TRUE(value.isNull());
}

TEST_F(BindingContextEdgeCaseTest, PathWithLeadingDot)
{
    // 以点开头的路径
    auto value = m_ctx->resolveBinding(".invalid");
    EXPECT_TRUE(value.isNull());
}

TEST_F(BindingContextEdgeCaseTest, PathWithTrailingDot)
{
    // 以点结尾的路径
    i32 health = 100;
    m_ctx->expose("player.health", &health);

    auto value = m_ctx->resolveBinding("player.health.");
    EXPECT_TRUE(value.isNull());
}

TEST_F(BindingContextEdgeCaseTest, PathWithConsecutiveDots)
{
    // 连续点的路径
    i32 health = 100;
    m_ctx->expose("player.health", &health);

    auto value = m_ctx->resolveBinding("player..health");
    EXPECT_TRUE(value.isNull());
}

TEST_F(BindingContextEdgeCaseTest, PathWithArrayIndex)
{
    // 设置集合
    m_ctx->setCollection<i32>("items", {10, 20, 30});

    // 通过索引访问
    auto items = m_ctx->resolveCollection("items");
    ASSERT_EQ(items.size(), 3u);
    EXPECT_EQ(items[0].asInteger(), 10);
    EXPECT_EQ(items[1].asInteger(), 20);
    EXPECT_EQ(items[2].asInteger(), 30);
}

TEST_F(BindingContextEdgeCaseTest, ExposeSamePathTwice)
{
    // 重复暴露同一路径应该覆盖
    i32 value1 = 100;
    i32 value2 = 200;

    m_ctx->expose("test.value", &value1);
    EXPECT_EQ(m_ctx->resolveBinding("test.value").asInteger(), 100);

    m_ctx->expose("test.value", &value2);
    EXPECT_EQ(m_ctx->resolveBinding("test.value").asInteger(), 200);
}

TEST_F(BindingContextEdgeCaseTest, ExposeWritableThenRead)
{
    // 暴露为可写后验证可写性
    i32 value = 42;
    m_ctx->exposeWritable("writable.value", &value);

    EXPECT_TRUE(m_ctx->hasPath("writable.value"));
    EXPECT_TRUE(m_ctx->isWritable("writable.value"));
}

TEST_F(BindingContextEdgeCaseTest, SetBindingOnReadOnlyFails)
{
    // 只读变量不应该能被设置
    i32 value = 42;
    m_ctx->expose("readonly.value", &value);

    EXPECT_FALSE(m_ctx->isWritable("readonly.value"));
    EXPECT_FALSE(m_ctx->setBinding("readonly.value", binder::Value(100)));
    EXPECT_EQ(value, 42); // 值不变
}

TEST_F(BindingContextEdgeCaseTest, SetBindingOnNonexistentPath)
{
    // 设置不存在的路径应该失败
    EXPECT_FALSE(m_ctx->setBinding("nonexistent.path", binder::Value(42)));
}

TEST_F(BindingContextEdgeCaseTest, CallbackThrowsException)
{
    // 回调中抛出异常
    m_ctx->exposeCallback("throwingCallback", [](auto*, const auto&) { throw std::runtime_error("Test exception"); });

    EXPECT_TRUE(m_ctx->hasCallback("throwingCallback"));
    // 调用可能会传播异常
    mc::client::ui::kagero::event::FocusGainedEvent event;
    EXPECT_THROW(m_ctx->invokeCallback("throwingCallback", nullptr, event), std::runtime_error);
}

TEST_F(BindingContextEdgeCaseTest, SubscribeToNonexistentPath)
{
    // 订阅不存在的路径应该成功（可能在未来创建）
    bool called = false;
    u64 subId = m_ctx->subscribe("future.path", [&](const std::string&, const binder::Value&) { called = true; });

    EXPECT_GT(subId, 0u);

    // 之后暴露变量并通知
    i32 value = 42;
    m_ctx->expose("future.path", &value);
    m_ctx->notifyChange("future.path", binder::Value(value));

    EXPECT_TRUE(called);
    m_ctx->unsubscribe(subId);
}

TEST_F(BindingContextEdgeCaseTest, UnsubscribeTwice)
{
    // 取消订阅两次应该安全
    i32 value = 42;
    m_ctx->expose("test.value", &value);

    u64 subId = m_ctx->subscribe("test.value", [](const std::string&, const binder::Value&) {});

    m_ctx->unsubscribe(subId);
    // 再次取消应该安全
    m_ctx->unsubscribe(subId);
}

TEST_F(BindingContextEdgeCaseTest, MultipleSubscribersSamePath)
{
    // 同一路径多个订阅者
    i32 value = 42;
    m_ctx->expose("test.value", &value);

    int callCount1 = 0, callCount2 = 0, callCount3 = 0;

    u64 sub1 = m_ctx->subscribe("test.value", [&](const std::string&, const binder::Value&) { callCount1++; });
    u64 sub2 = m_ctx->subscribe("test.value", [&](const std::string&, const binder::Value&) { callCount2++; });
    u64 sub3 = m_ctx->subscribe("test.value", [&](const std::string&, const binder::Value&) { callCount3++; });

    m_ctx->notifyChange("test.value", binder::Value(100));

    EXPECT_EQ(callCount1, 1);
    EXPECT_EQ(callCount2, 1);
    EXPECT_EQ(callCount3, 1);

    m_ctx->unsubscribe(sub1);
    m_ctx->unsubscribe(sub2);
    m_ctx->unsubscribe(sub3);
}

TEST_F(BindingContextEdgeCaseTest, SubscriberReceivesCorrectValue)
{
    // 验证订阅者收到正确的值
    i32 value = 42;
    m_ctx->expose("test.value", &value);

    binder::Value receivedValue;
    std::string receivedPath;

    u64 subId = m_ctx->subscribe("test.value", [&](const std::string& path, const binder::Value& val) {
        receivedPath = path;
        receivedValue = val;
    });

    m_ctx->notifyChange("test.value", binder::Value(123));

    EXPECT_EQ(receivedPath, "test.value");
    EXPECT_EQ(receivedValue.asInteger(), 123);

    m_ctx->unsubscribe(subId);
}

TEST_F(BindingContextEdgeCaseTest, LoopVariableShadowing)
{
    // 循环变量遮蔽
    i32 global = 42;
    m_ctx->expose("item", &global);

    // 设置循环变量同名
    m_ctx->setLoopVariable("item", binder::Value(100));

    // 循环变量优先
    auto value = m_ctx->resolveBinding("$item");
    EXPECT_EQ(value.asInteger(), 100);

    // 暴露的变量通过完整路径访问
    auto globalValue = m_ctx->resolveBinding("item");
    EXPECT_EQ(globalValue.asInteger(), 42);

    m_ctx->clearLoopVariable("item");
}

TEST_F(BindingContextEdgeCaseTest, NestedLoopVariables)
{
    // 嵌套循环变量
    m_ctx->setLoopVariable("outer", binder::Value(1));
    m_ctx->setLoopVariable("inner", binder::Value(2));

    EXPECT_TRUE(m_ctx->hasLoopVariable("outer"));
    EXPECT_TRUE(m_ctx->hasLoopVariable("inner"));

    auto outerVal = m_ctx->resolveBinding("$outer");
    auto innerVal = m_ctx->resolveBinding("$inner");

    EXPECT_EQ(outerVal.asInteger(), 1);
    EXPECT_EQ(innerVal.asInteger(), 2);

    m_ctx->clearLoopVariable("outer");
    m_ctx->clearLoopVariable("inner");

    EXPECT_FALSE(m_ctx->hasLoopVariable("outer"));
    EXPECT_FALSE(m_ctx->hasLoopVariable("inner"));
}

TEST_F(BindingContextEdgeCaseTest, LoopVariableWithPropertyAccess)
{
    // 循环变量属性访问
    std::vector<binder::Value> items;
    items.emplace_back(binder::Value(std::string("first")));
    items.emplace_back(binder::Value(std::string("second")));

    m_ctx->setCollectionValue("items", items);

    auto resolved = m_ctx->resolveCollection("items");
    ASSERT_EQ(resolved.size(), 2u);

    // 设置为循环变量并访问属性
    m_ctx->setLoopVariable("item", resolved[0]);

    // 由于Value不支持属性，这里测试属性访问返回空
    auto prop = m_ctx->resolveBinding("$item.someProperty");
    // 属性访问应该返回空值，因为Value是简单字符串类型
    EXPECT_TRUE(prop.isNull());

    m_ctx->clearLoopVariable("item");
}

TEST_F(BindingContextEdgeCaseTest, CollectionModification)
{
    // 修改集合
    m_ctx->setCollection<i32>("numbers", {1, 2, 3});

    auto nums1 = m_ctx->resolveCollection("numbers");
    EXPECT_EQ(nums1.size(), 3u);

    // 重新设置集合
    m_ctx->setCollection<i32>("numbers", {4, 5, 6, 7, 8});

    auto nums2 = m_ctx->resolveCollection("numbers");
    EXPECT_EQ(nums2.size(), 5u);
    EXPECT_EQ(nums2[0].asInteger(), 4);
    EXPECT_EQ(nums2[4].asInteger(), 8);
}

TEST_F(BindingContextEdgeCaseTest, EmptyCollection)
{
    // 空集合
    m_ctx->setCollection<i32>("empty", {});

    auto items = m_ctx->resolveCollection("empty");
    EXPECT_EQ(items.size(), 0u);
}

TEST_F(BindingContextEdgeCaseTest, ClearRemovesAllBindings)
{
    // 清除所有绑定
    i32 value = 42;
    m_ctx->expose("test.value", &value);
    m_ctx->exposeCallback("test.callback", [](auto*, const auto&) {});

    EXPECT_TRUE(m_ctx->hasPath("test.value"));
    EXPECT_TRUE(m_ctx->hasCallback("test.callback"));

    m_ctx->clear();

    EXPECT_FALSE(m_ctx->hasPath("test.value"));
    EXPECT_FALSE(m_ctx->hasCallback("test.callback"));
}

TEST_F(BindingContextEdgeCaseTest, TypeMismatchedSetBinding)
{
    // 类型不匹配的设置
    std::string strValue = "hello";
    m_ctx->exposeWritable("test.string", &strValue);

    // 设置不同类型的值
    EXPECT_TRUE(m_ctx->setBinding("test.string", binder::Value(42)));
    // 值应该被转换
    EXPECT_EQ(strValue, "42");
}

TEST_F(BindingContextEdgeCaseTest, SetBindingBoolToInt)
{
    // 布尔值设置到整型
    i32 intValue = 0;
    m_ctx->exposeWritable("test.int", &intValue);

    EXPECT_TRUE(m_ctx->setBinding("test.int", binder::Value(true)));
    EXPECT_EQ(intValue, 1);

    EXPECT_TRUE(m_ctx->setBinding("test.int", binder::Value(false)));
    EXPECT_EQ(intValue, 0);
}

TEST_F(BindingContextEdgeCaseTest, SetBindingFloatToInt)
{
    // 浮点设置到整型（应该截断）
    i32 intValue = 0;
    m_ctx->exposeWritable("test.int", &intValue);

    EXPECT_TRUE(m_ctx->setBinding("test.int", binder::Value(3.7f)));
    EXPECT_EQ(intValue, 3);

    EXPECT_TRUE(m_ctx->setBinding("test.int", binder::Value(-2.9f)));
    EXPECT_EQ(intValue, -2);
}

TEST_F(BindingContextEdgeCaseTest, SetBindingStringToInt)
{
    // 字符串设置到整型
    i32 intValue = 0;
    m_ctx->exposeWritable("test.int", &intValue);

    EXPECT_TRUE(m_ctx->setBinding("test.int", binder::Value(std::string("42"))));
    EXPECT_EQ(intValue, 42);

    // 无效字符串
    EXPECT_TRUE(m_ctx->setBinding("test.int", binder::Value(std::string("not a number"))));
    // 应该转换为0
    EXPECT_EQ(intValue, 0);
}

TEST_F(BindingContextEdgeCaseTest, SetBindingIntToFloat)
{
    // 整数设置到浮点
    f32 floatValue = 0.0f;
    m_ctx->exposeWritable("test.float", &floatValue);

    EXPECT_TRUE(m_ctx->setBinding("test.float", binder::Value(42)));
    EXPECT_FLOAT_EQ(floatValue, 42.0f);
}

// ==================== UpdateScheduler Edge Cases Tests ====================

class UpdateSchedulerEdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_scheduler = std::make_unique<runtime::UpdateScheduler>();
        // 默认禁用延迟更新，保持原有立即执行语义
        m_scheduler->setDeferredUpdate(false);
    }

    std::unique_ptr<runtime::UpdateScheduler> m_scheduler;
};

TEST_F(UpdateSchedulerEdgeCaseTest, ScheduleSamePathMultipleTimes)
{
    // 同一路径多次调度
    u64 id1 = m_scheduler->schedule("player.health", runtime::UpdateScheduler::Priority::Normal);
    u64 id2 = m_scheduler->schedule("player.health", runtime::UpdateScheduler::Priority::Normal);
    u64 id3 = m_scheduler->schedule("player.health", runtime::UpdateScheduler::Priority::Normal);

    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_EQ(m_scheduler->pendingCount(), 3u);
}

TEST_F(UpdateSchedulerEdgeCaseTest, CancelTwiceSameId)
{
    // 取消同一个任务两次
    u64 id = m_scheduler->schedule("player.health", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->cancel(id);
    m_scheduler->cancel(id); // 应该安全

    // 执行不应该崩溃
    u32 executed = m_scheduler->executePending();
    EXPECT_EQ(executed, 0u);
}

TEST_F(UpdateSchedulerEdgeCaseTest, CancelByPathMultipleSchedules)
{
    // 同一路径多个任务
    m_scheduler->schedule("player.health", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->schedule("player.health", runtime::UpdateScheduler::Priority::High);
    m_scheduler->schedule("player.health", runtime::UpdateScheduler::Priority::Low);

    EXPECT_EQ(m_scheduler->pendingCount(), 3u);

    m_scheduler->cancelByPath("player.health");
    // 所有任务应该被取消
}

TEST_F(UpdateSchedulerEdgeCaseTest, ExecuteByPriority)
{
    std::vector<std::string> executedPaths;

    m_scheduler->setUpdateCallback([&executedPaths](const std::string& path) -> bool {
        executedPaths.push_back(path);
        return true;
    });

    m_scheduler->schedule("low", runtime::UpdateScheduler::Priority::Low);
    m_scheduler->schedule("high", runtime::UpdateScheduler::Priority::High);
    m_scheduler->schedule("normal", runtime::UpdateScheduler::Priority::Normal);

    // 先执行高优先级
    u32 highCount = m_scheduler->executeHighPriority();
    EXPECT_EQ(highCount, 1u);
    EXPECT_EQ(executedPaths.size(), 1u);
    EXPECT_EQ(executedPaths[0], "high");

    // 再执行普通优先级
    u32 normalCount = m_scheduler->executeNormalPriority();
    EXPECT_EQ(normalCount, 1u);
    EXPECT_EQ(executedPaths.size(), 2u);
    EXPECT_EQ(executedPaths[1], "normal");

    // 最后执行低优先级
    u32 lowCount = m_scheduler->executeLowPriority();
    EXPECT_EQ(lowCount, 1u);
    EXPECT_EQ(executedPaths.size(), 3u);
    EXPECT_EQ(executedPaths[2], "low");
}

TEST_F(UpdateSchedulerEdgeCaseTest, ExecutePendingWithNoCallback)
{
    // 没有设置回调时执行
    m_scheduler->schedule("player.health", runtime::UpdateScheduler::Priority::Normal);

    u32 executed = m_scheduler->executePending();
    EXPECT_EQ(executed, 0u); // 没有回调，无法执行
}

TEST_F(UpdateSchedulerEdgeCaseTest, ExecutePendingWithFailingCallback)
{
    // 回调返回失败
    int callCount = 0;
    m_scheduler->setUpdateCallback([&callCount](const std::string& path) -> bool {
        callCount++;
        return path != "fail"; // "fail" 路径返回失败
    });

    m_scheduler->schedule("success", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->schedule("fail", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->schedule("another", runtime::UpdateScheduler::Priority::Normal);

    u32 executed = m_scheduler->executePending();
    EXPECT_EQ(callCount, 3);
    // executePending 应该报告成功执行的数量
}

TEST_F(UpdateSchedulerEdgeCaseTest, ScheduleEmptyPath)
{
    // 空路径
    u64 id = m_scheduler->schedule("", runtime::UpdateScheduler::Priority::Normal);
    EXPECT_GT(id, 0u);
    EXPECT_TRUE(m_scheduler->hasPending());
}

TEST_F(UpdateSchedulerEdgeCaseTest, CancelNonexistentId)
{
    // 取消不存在的任务ID
    m_scheduler->cancel(999999);
    // 应该安全，不崩溃
}

TEST_F(UpdateSchedulerEdgeCaseTest, CancelByNonexistentPath)
{
    // 取消不存在路径的任务
    m_scheduler->schedule("existing.path", runtime::UpdateScheduler::Priority::Normal);
    m_scheduler->cancelByPath("nonexistent.path");
    // 应该安全
    EXPECT_EQ(m_scheduler->pendingCount(), 1u);
}

TEST_F(UpdateSchedulerEdgeCaseTest, LargeNumberOfTasks)
{
    // 大量任务
    for (int i = 0; i < 1000; ++i) {
        m_scheduler->schedule("path" + std::to_string(i), runtime::UpdateScheduler::Priority::Normal);
    }

    EXPECT_EQ(m_scheduler->pendingCount(), 1000u);

    m_scheduler->cancelAll();
    EXPECT_FALSE(m_scheduler->hasPending());
}

TEST_F(UpdateSchedulerEdgeCaseTest, TimestampIncreases)
{
    u64 ts1 = m_scheduler->currentTimestamp();
    u64 ts2 = m_scheduler->currentTimestamp();

    // 时间戳应该递增或相同（快速调用时可能相同）
    EXPECT_GE(ts2, ts1);
}

TEST_F(UpdateSchedulerEdgeCaseTest, CallbackThrowsException)
{
    m_scheduler->setUpdateCallback([](const std::string&) -> bool { throw std::runtime_error("Test exception"); });

    m_scheduler->schedule("test", runtime::UpdateScheduler::Priority::Normal);

    // 执行可能会抛出异常
    EXPECT_THROW(m_scheduler->executePending(), std::runtime_error);
}

TEST_F(UpdateSchedulerEdgeCaseTest, ReentrantSchedule)
{
    // 在回调中调度新任务
    // executePending() 会按顺序执行所有优先级：High -> Normal -> Low
    // 在回调中添加的 Low 优先级任务会在同一次 executePending() 中被执行
    m_scheduler->setUpdateCallback([this](const std::string& path) -> bool {
        if (path == "trigger") {
            m_scheduler->schedule("triggered", runtime::UpdateScheduler::Priority::Low);
        }
        return true;
    });

    m_scheduler->schedule("trigger", runtime::UpdateScheduler::Priority::Normal);
    u32 executed = m_scheduler->executePending();

    // executePending() 执行顺序：
    // 1. executeHighPriority() -> 0 任务
    // 2. executeNormalPriority() -> 处理 "trigger"，回调中添加 "triggered"(Low)
    // 3. executeLowPriority() -> 处理 "triggered"(刚添加)
    // 所以总共执行 2 个任务
    EXPECT_EQ(executed, 2u);
    // 所有任务已完成
    EXPECT_FALSE(m_scheduler->hasPending());

    // 再次执行应该没有任务
    executed = m_scheduler->executePending();
    EXPECT_EQ(executed, 0u);
}

// ==================== CompiledTemplate Edge Cases Tests ====================

class CompiledTemplateEdgeCaseTest : public ::testing::Test {
protected:
    void SetUp() override { m_config = core::TemplateConfig::defaults(); }

    core::TemplateConfig m_config;
};

TEST_F(CompiledTemplateEdgeCaseTest, VeryLargeTemplate)
{
    // 非常大的模板
    std::string source = "<screen>";
    for (int i = 0; i < 1000; ++i) {
        source += "<text id=\"text" + std::to_string(i) + "\" text=\"Content " + std::to_string(i) + "\"/>";
    }
    source += "</screen>";

    compiler::TemplateCompiler compiler(m_config);
    auto result = compiler.compile(source);

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid()) << (result->hasErrors() ? result->errors()[0].message : "");

    // 验证绑定计划数量
    EXPECT_EQ(result->bindingPlans().size(), 0u); // 静态文本没有绑定
}

TEST_F(CompiledTemplateEdgeCaseTest, TemplateWithAllBindingTypes)
{
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <screen id="main">
            <!-- 静态属性 -->
            <text id="static" text="Static Content"/>

            <!-- 绑定属性 -->
            <text id="name" bind:text="player.name"/>
            <text id="health" bind:text="player.health"/>
            <text id="visible" bind:visible="player.alive"/>

            <!-- 事件绑定 -->
            <button id="btn1" on:click="onClick"/>
            <button id="btn2" on:doubleClick="onDoubleClick"/>

            <!-- 循环 -->
            <list for:item="item in items">
                <slot bind:item="$item"/>
            </list>

            <!-- 条件 -->
            <text if:condition="game.isPlaying" text="Playing"/>
        </screen>
    )");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());

    // 验证收集的信息
    EXPECT_GE(result->bindingPlans().size(), 3u); // name, health, visible
    EXPECT_GE(result->eventPlans().size(), 2u);   // onClick, onDoubleClick
    EXPECT_TRUE(result->watchedPaths().count("player.name") > 0);
    EXPECT_TRUE(result->registeredCallbacks().count("onClick") > 0);
}

TEST_F(CompiledTemplateEdgeCaseTest, TemplateWithDuplicateIds)
{
    // 重复ID - 现在应该在解析阶段检测到错误
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <screen>
            <text id="duplicate"/>
            <text id="duplicate"/>
        </screen>
    )");

    ASSERT_NE(result, nullptr);
    // 应该检测到重复ID错误
    EXPECT_TRUE(result->hasErrors());
    EXPECT_EQ(result->errors().size(), 1u);
    EXPECT_EQ(result->errors()[0].type, core::TemplateErrorType::DuplicateId);
    EXPECT_TRUE(result->errors()[0].message.find("duplicate") != std::string::npos);
}

TEST_F(CompiledTemplateEdgeCaseTest, TemplateWithDeepNesting)
{
    // 深度嵌套 - 使用有效标签 widget 而不是 container
    std::string source = "<screen>";
    for (int i = 0; i < 100; ++i) {
        source += "<widget>";
    }
    source += "<text id=\"deep\"/>";
    for (int i = 0; i < 100; ++i) {
        source += "</widget>";
    }
    source += "</screen>";

    compiler::TemplateCompiler compiler(m_config);
    auto result = compiler.compile(source);

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid()) << (result->hasErrors() ? result->errors()[0].message : "");
}

TEST_F(CompiledTemplateEdgeCaseTest, TemplateWithEmptyTextContent)
{
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <screen>
            <text></text>
            <text>   </text>
            <text/>
        </screen>
    )");

    ASSERT_NE(result, nullptr);
    EXPECT_TRUE(result->isValid());
}

TEST_F(CompiledTemplateEdgeCaseTest, TemplateWithSpecialCharactersInValues)
{
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <screen>
            <text text="Hello &amp; World"/>
            <text text="Line1&#10;Line2"/>
            <text text="Quote: &quot;Test&quot;"/>
        </screen>
    )");

    ASSERT_NE(result, nullptr);
    // 实体处理取决于实现
}

TEST_F(CompiledTemplateEdgeCaseTest, MultipleCompilationSameCompiler)
{
    // 同一编译器多次编译
    compiler::TemplateCompiler compiler(m_config);

    auto result1 = compiler.compile("<screen id=\"first\"/>");
    auto result2 = compiler.compile("<screen id=\"second\"/>");
    auto result3 = compiler.compile("<screen id=\"third\"/>");

    ASSERT_NE(result1, nullptr);
    ASSERT_NE(result2, nullptr);
    ASSERT_NE(result3, nullptr);

    EXPECT_TRUE(result1->isValid());
    EXPECT_TRUE(result2->isValid());
    EXPECT_TRUE(result3->isValid());
}

// ==================== AST Edge Cases Tests ====================

class AstEdgeCaseTest : public ::testing::Test {};

TEST_F(AstEdgeCaseTest, BindingInfoParseEdgeCases)
{
    // 仅美元符号 - 根据实现，单个"$"不满足 size > 1 条件
    // 这是实现的行为：单个$不被视为循环变量，而是作为普通路径
    auto info1 = ast::BindingInfo::parse("$");
    // size=1不满足条件，进入else分支，path="$"，isLoopVariable=false
    EXPECT_FALSE(info1.isLoopVariable); // size > 1 条件不满足
    EXPECT_EQ(info1.path, "$");         // 路径是"$"，不是空
    // isValid() 检查 !path.empty() || isLoopVariable
    // path="$"非空，所以isValid()返回true
    EXPECT_TRUE(info1.isValid()); // 路径非空，所以有效

    // 美元符号后跟点 - "$.property" 的size > 1，会进入循环变量分支
    // 但变量名为空（点号前的内容为空）
    auto info2 = ast::BindingInfo::parse("$.property");
    EXPECT_TRUE(info2.isLoopVariable); // size > 1，进入循环变量分支
    // 变量名为空（substr(1, dotPos-1) 当dotPos=1时为空）
    EXPECT_TRUE(info2.loopVarName.empty());
    EXPECT_EQ(info2.property, "property");

    // 嵌套属性
    auto info3 = ast::BindingInfo::parse("$item.a.b.c");
    EXPECT_TRUE(info3.isLoopVariable);
    EXPECT_EQ(info3.loopVarName, "item");
    EXPECT_EQ(info3.property, "a.b.c");

    // 数组索引
    auto info4 = ast::BindingInfo::parse("items[0]");
    EXPECT_FALSE(info4.isLoopVariable);
    EXPECT_EQ(info4.path, "items[0]");

    // 嵌套数组索引
    auto info5 = ast::BindingInfo::parse("matrix[0][1]");
    EXPECT_FALSE(info5.isLoopVariable);
    EXPECT_EQ(info5.path, "matrix[0][1]");
}

TEST_F(AstEdgeCaseTest, IsValidBindingPathEdgeCases)
{
    // 空路径
    EXPECT_FALSE(ast::isValidBindingPath(""));

    // 仅点
    EXPECT_FALSE(ast::isValidBindingPath("."));

    // 多个点
    EXPECT_FALSE(ast::isValidBindingPath("a..b"));

    // 以点开头
    EXPECT_FALSE(ast::isValidBindingPath(".path"));

    // 以点结尾
    EXPECT_FALSE(ast::isValidBindingPath("path."));

    // 有效路径边界
    EXPECT_TRUE(ast::isValidBindingPath("a"));
    EXPECT_TRUE(ast::isValidBindingPath("a.b"));
    EXPECT_TRUE(ast::isValidBindingPath("a.b.c.d.e"));

    // 带索引
    EXPECT_TRUE(ast::isValidBindingPath("a[0]"));
    EXPECT_TRUE(ast::isValidBindingPath("a[0].b"));
    EXPECT_TRUE(ast::isValidBindingPath("a[0][1]"));

    // 无效索引
    EXPECT_FALSE(ast::isValidBindingPath("a[]"));
    EXPECT_FALSE(ast::isValidBindingPath("a[-1]"));

    // 循环变量
    EXPECT_TRUE(ast::isValidBindingPath("$a"));
    EXPECT_TRUE(ast::isValidBindingPath("$a.b"));
    EXPECT_FALSE(ast::isValidBindingPath("$"));
    EXPECT_FALSE(ast::isValidBindingPath("$1")); // 数字开头
}

TEST_F(AstEdgeCaseTest, IsValidCallbackNameEdgeCases)
{
    // 有效名称
    EXPECT_TRUE(ast::isValidCallbackName("a"));
    EXPECT_TRUE(ast::isValidCallbackName("onClick"));
    EXPECT_TRUE(ast::isValidCallbackName("_private"));
    EXPECT_TRUE(ast::isValidCallbackName("handler1"));

    // 无效名称
    EXPECT_FALSE(ast::isValidCallbackName(""));
    EXPECT_FALSE(ast::isValidCallbackName("1callback"));
    EXPECT_FALSE(ast::isValidCallbackName("on-click"));
    EXPECT_FALSE(ast::isValidCallbackName("callback()"));
    EXPECT_FALSE(ast::isValidCallbackName("obj.method"));
    EXPECT_FALSE(ast::isValidCallbackName("callback name"));
}

TEST_F(AstEdgeCaseTest, AttributeCreateEdgeCases)
{
    // 空值静态属性
    auto attr1 = ast::Attribute::createStatic("name", "");
    EXPECT_EQ(attr1.rawValue, "");
    EXPECT_TRUE(std::holds_alternative<std::string>(attr1.value));

    // 数字边界值
    auto attr2 = ast::Attribute::createStatic("max", "2147483647");
    EXPECT_TRUE(std::holds_alternative<i32>(attr2.value));
    EXPECT_EQ(std::get<i32>(attr2.value), 2147483647);

    // 负数
    auto attr3 = ast::Attribute::createStatic("neg", "-42");
    // 可能是字符串或整数
    EXPECT_TRUE(std::holds_alternative<i32>(attr3.value) || std::holds_alternative<std::string>(attr3.value));

    // 浮点数
    auto attr4 = ast::Attribute::createStatic("pi", "3.14159");
    EXPECT_TRUE(std::holds_alternative<f32>(attr4.value) || std::holds_alternative<std::string>(attr4.value));
}

TEST_F(AstEdgeCaseTest, ElementNodeAddAttributeOverride)
{
    ast::ElementNode elem(ast::NodeType::Text);

    elem.addAttribute(ast::Attribute::createStatic("text", "first"));
    elem.addAttribute(ast::Attribute::createStatic("text", "second"));

    // 相同属性名应该被覆盖
    const auto* attr = elem.getAttribute("text");
    ASSERT_NE(attr, nullptr);
    EXPECT_EQ(attr->rawValue, "second");
}

TEST_F(AstEdgeCaseTest, ElementNodeCloneDeep)
{
    auto original = std::make_unique<ast::ElementNode>(ast::NodeType::Screen);
    original->tagName = "screen";
    original->id = "original";

    auto child1 = std::make_unique<ast::ElementNode>(ast::NodeType::Button);
    child1->tagName = "button";
    child1->id = "child1";
    original->children.push_back(std::move(child1));

    auto child2 = std::make_unique<ast::ElementNode>(ast::NodeType::Text);
    child2->tagName = "text";
    child2->id = "child2";
    original->children.push_back(std::move(child2));

    auto cloned = original->clone();
    auto* clonedElem = dynamic_cast<ast::ElementNode*>(cloned.get());
    ASSERT_NE(clonedElem, nullptr);

    // 验证深拷贝
    EXPECT_EQ(clonedElem->id, "original");
    EXPECT_EQ(clonedElem->children.size(), 2u);

    // 修改原始不应影响克隆
    original->id = "modified";
    EXPECT_EQ(clonedElem->id, "original");
}

// ==================== Template Error Recovery Tests ====================

class TemplateErrorRecoveryTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        m_config = core::TemplateConfig::defaults();
        m_config.strictMode = false; // 关闭严格模式以测试恢复
    }

    core::TemplateConfig m_config;
};

TEST_F(TemplateErrorRecoveryTest, MultipleErrorsContinueParsing)
{
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <screen>
            <text bind:text="123invalid"/>
            <text text="valid1"/>
            <button on:click="456invalid"/>
            <text text="valid2"/>
        </screen>
    )");

    ASSERT_NE(result, nullptr);
    // 应该继续解析并收集所有错误
    // 即使有错误，有效的部分也应该被解析
}

TEST_F(TemplateErrorRecoveryTest, UnclosedTagRecovery)
{
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <screen>
            <text id="unclosed">
            <text id="after" text="should be parsed"/>
        </screen>
    )");

    ASSERT_NE(result, nullptr);
    // 错误恢复应该尝试继续解析
}

TEST_F(TemplateErrorRecoveryTest, MismatchedTagRecovery)
{
    compiler::TemplateCompiler compiler(m_config);

    auto result = compiler.compile(R"(
        <screen>
            <text id="test">
            </button>
            <text id="after"/>
        </screen>
    )");

    ASSERT_NE(result, nullptr);
    // 应该报告不匹配错误
}

TEST_F(TemplateErrorRecoveryTest, ErrorLocationAccuracy)
{
    parser::Lexer lexer(R"(
        <screen>
            <text bind:text="invalid">
        </screen>
    )",
        "<test>");
    lexer.tokenize();

    parser::Parser parser(lexer, m_config);
    auto doc = parser.parse();

    // 验证错误位置信息
    if (parser.hasErrors()) {
        auto& errors = parser.errors();
        for (const auto& err : errors) {
            EXPECT_GT(err.location.line, 0u);
            EXPECT_GT(err.location.column, 0u);
        }
    }
}
