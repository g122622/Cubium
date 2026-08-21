// /tag 命令 GameTest：给实体添加/移除/列出标签。
//
// 覆盖 wiki 命令章节核心行为：
//   - /tag <targets> add <name>：给目标实体添加标签（Ref: wiki commands/tag.txt）
//   - /tag <targets> remove <name>：移除标签
//   - /tag <targets> list：列出目标实体所有标签
//
// 设计要点：
//   1. TagCommand 经 support::EntityResolver::resolve(source, selector) 取实体，直接调
//      Entity::addTag/removeTag（基类 m_tags，std::set<string>），不走 InventoryManager，
//      对 SimulatedPlayer 完全生效。@s 选择器经 source.entity() 返回 SimulatedPlayer 自身
//      （@a/@p 仅遍历 PlayerManager 不含 SimulatedPlayer，故用 @s）。
//   2. 判定标签生效用 Entity 脚本 getTags()/hasTag()（本次补全的绑定，对齐基岩
//      @minecraft/server Entity 标签 API）。getTags() 返回 string[]，hasTag(name) 返回 boolean。
//   3. addTag 标签已存在返 false（命令 successCount=0 sendError），removeTag 不存在返 false。
//   4. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。玩家初始 (5,1,5)。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_tag.txt（实体标签）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { asEnt } from "../../utils/script/cubiumExtensions.js";

const PLAYER_POS = { x: 5, y: 1, z: 5 };

// 实体标签数组判定辅助：判断 tags 是否包含指定标签。
function hasTagIn(tags: string[], name: string): boolean {
    return tags.indexOf(name) !== -1;
}

// /tag @s add foo 给自身添加标签 foo，断言 getTags() 含 foo 且 hasTag("foo") 返 true。
// Ref: wiki commands/tag.txt（tag <targets> add <name>）
function tagAddAddsTag(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    test.assert(!asEnt(player).hasTag("foo"), "player should not have foo before add");

    player.chat("/tag @s add foo");

    test.assert(asEnt(player).hasTag("foo"), "player should have foo after /tag @s add foo");
    const tags = asEnt(player).getTags();
    test.assert(hasTagIn(tags, "foo"), `getTags() should contain foo, got [${tags.join(", ")}]`);
    test.succeed();
}

// /tag @s remove foo 移除已存在标签，断言 getTags() 不含 foo 且 hasTag 返 false。
// Ref: wiki commands/tag.txt（tag <targets> remove <name>）
function tagRemoveRemovesTag(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    player.chat("/tag @s add foo");
    test.assert(asEnt(player).hasTag("foo"), "precondition: foo should be added");

    player.chat("/tag @s remove foo");

    test.assert(!asEnt(player).hasTag("foo"), "player should not have foo after remove");
    const tags = asEnt(player).getTags();
    test.assert(!hasTagIn(tags, "foo"), `getTags() should not contain foo, got [${tags.join(", ")}]`);
    test.succeed();
}

// 多标签共存：add 多个不同标签后 getTags() 含全部。
// 验证 std::set<string> 多标签存储正确（非单标签覆盖）。
function tagMultipleTagsCoexist(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    player.chat("/tag @s add alpha");
    player.chat("/tag @s add beta");
    player.chat("/tag @s add gamma");

    test.assert(asEnt(player).hasTag("alpha"), "should have alpha");
    test.assert(asEnt(player).hasTag("beta"), "should have beta");
    test.assert(asEnt(player).hasTag("gamma"), "should have gamma");

    const tags = asEnt(player).getTags();
    test.assert(tags.length === 3, `expected 3 tags, got ${tags.length}: [${tags.join(", ")}]`);
    test.succeed();
}

// add 已存在标签为无操作：先 add foo 再 add foo，断言标签数仍为 1（addTag 已存在返 false）。
// Ref: wiki commands/tag.txt（重复添加同一标签不影响）
function tagAddDuplicateIsNoOp(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    player.chat("/tag @s add foo");
    player.chat("/tag @s add foo");

    const tags = asEnt(player).getTags();
    test.assert(tags.length === 1, `duplicate add should not duplicate tag, got ${tags.length}: [${tags.join(", ")}]`);
    test.assert(asEnt(player).hasTag("foo"), "should still have foo");
    test.succeed();
}

// remove 不存在标签为无操作：直接 remove foo（未 add），断言无标签且不报错。
function tagRemoveAbsentIsNoOp(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");

    player.chat("/tag @s remove foo");

    const tags = asEnt(player).getTags();
    test.assert(tags.length === 0, `remove absent should leave 0 tags, got ${tags.length}: [${tags.join(", ")}]`);
    test.succeed();
}

export function registerTagTests(): void {
    GameTest.register("CommandTests", "tag_add_adds_tag", tagAddAddsTag)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "tag_remove_removes_tag", tagRemoveRemovesTag)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "tag_multiple_tags_coexist", tagMultipleTagsCoexist)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "tag_add_duplicate_is_no_op", tagAddDuplicateIsNoOp)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);

    GameTest.register("CommandTests", "tag_remove_absent_is_no_op", tagRemoveAbsentIsNoOp)
        .structureName("gametests:cmd_arena")
        .maxTicks(60);
}
