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
 * @file JigsawTemplateCacheClearTest.cpp
 * @brief `/reload` 清理结构模板缓存的契约
 *
 * JigsawAssembler 的模板管理器是进程级单例，没有数据包视图的概念：同一个
 * ResourceLocation 一旦加载过就一直返回同一个 Template 对象。数据包里的 .nbt
 * 结构文件被修改后，必须清空这个缓存，否则新生成的结构仍沿用旧模板，且没有任何
 * 提示——表现为"改了数据包却不生效，要重启服务器"。
 *
 * 这里钉住 clearCache() 的这一职责：清空之后缓存为空，且可以重复调用。
 */

#include <gtest/gtest.h>

#include "common/resource/ResourceLocation.hpp"
#include "server/world/gen/feature/template/Template.hpp"
#include "server/world/gen/feature/template/TemplateManager.hpp"
#include "server/world/gen/jigsaw/JigsawAssembler.hpp"

#include <memory>

using namespace mc;
using namespace mc::world::gen::jigsaw;

class JigsawTemplateCacheClearTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        // 每个用例从干净状态开始，避免受其它用例注入的模板影响。
        JigsawAssembler::clearCache();
    }

    void TearDown() override
    {
        JigsawAssembler::clearCache();
    }
};

/**
 * @test clearCache 之后缓存为空
 *
 * 先手工注入一个模板（addTemplate 会把对象放进缓存），确认它确实被计入缓存大小，
 * 再调用 clearCache 并确认缓存归零。
 */
TEST_F(JigsawTemplateCacheClearTest, ClearCacheDropsInjectedTemplates)
{
    auto& manager = JigsawAssembler::getTemplateManager();
    ASSERT_EQ(manager.cacheSize(), 0u) << "cache must start empty in a fresh test";

    const ResourceLocation location("minecraft:test/jigsaw_cache_probe");
    manager.addTemplate(location, std::make_unique<world::gen::feature::template_::Template>());
    ASSERT_EQ(manager.cacheSize(), 1u) << "addTemplate must populate the cache";

    JigsawAssembler::clearCache();

    EXPECT_EQ(manager.cacheSize(), 0u) << "clearCache must drop every cached template";
    EXPECT_FALSE(manager.hasTemplate(location)) << "the injected template must be gone after clearCache";
}

/**
 * @test clearCache 可重复调用
 *
 * `/reload` 可能被连续执行多次，清空操作必须是幂等的；对空缓存再清一次不能崩溃。
 */
TEST_F(JigsawTemplateCacheClearTest, ClearCacheIsIdempotent)
{
    auto& manager = JigsawAssembler::getTemplateManager();

    JigsawAssembler::clearCache();
    EXPECT_EQ(manager.cacheSize(), 0u);

    JigsawAssembler::clearCache();
    EXPECT_EQ(manager.cacheSize(), 0u);
}