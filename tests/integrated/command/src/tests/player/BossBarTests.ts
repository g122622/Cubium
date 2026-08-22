// /bossbar 命令 GameTest：自定义 Boss 栏管理。
//
// 覆盖 wiki 命令章节核心行为（Ref: wiki commands/bossbar.txt）：
//   - /bossbar add <id> <name>：创建 BossBar
//   - /bossbar remove <id>：删除 BossBar
//   - /bossbar set <id> value <n>：设置当前值
//   - /bossbar set <id> max <n>：设置最大值
//   - /bossbar set <id> color <c>：设置颜色
//   - /bossbar set <id> style <s>：设置样式（overlay）
//   - /bossbar set <id> visible <bool>：设置可见性
//   - /bossbar set <id> name <name>：设置显示名
//   - /bossbar get <id> <property>：命令侧反馈（JS 侧用 world.bossbar.get(id) 读属性断言）
//
// 设计要点：
//   1. BossBarCommand 已实现 5 个子命令（add/remove/list/set/get），set 7 属性、get 4 属性，核心
//      CustomServerBossInfoManager/CustomServerBossInfo 真实存储（非 stub）。脚本侧此前无 BossBar 绑定
//      （BossBar 类型全在 server 层，common 层无法 include），GameTest JS 无法读 BossBar 断言。本批测试
//      伴随新增的 world.bossbar 脚本绑定（BossBarManager.get/getAll + BossBar 类，经 ScriptWorldAccessor
//      BossBarView 值快照桥接 server 层，见 [[bossbar-script-binding-and-command-tests]]）。
//   2. 断言用 world.bossbar.get(id) 读 BossBar 对象（不存在返 undefined），bar.id/name/value/max/color/
//      overlay/visible 读属性。属性每次访问重新取快照保证 set 后实时可见。getAll() 返 BossBar[]。
//   3. BossBar 是世界级单例（CustomServerBossInfoManager 全局唯一），跨测试持久化不自动重置。故每个
//      测试用唯一 id（含测试函数名后缀）避免互斥，独占 batch 串行 + runOnFinish remove 清理，防污染
//      后续依赖空 BossBar 集的测试。
//   4. BossBarCommand 需 permLevel≥2（hasPermission(2)）。SimulatedPlayer permLevel 固定=4 满足。
//   5. add 的 name 是 greedyString（可含空格），id 是 string。set value/max 用整数，color/style 是
//      字面量（color: pink/blue/red/green/yellow/purple/white；style: progress/notched_6/10/12/20）。
//   6. 默认属性对齐 vanilla：value=0、max=100、color=white、overlay=progress、visible=true
//      （CustomServerBossInfo 构造默认值）。
//   7. cmd_arena 9×7×9：内部空气腔 x,z∈[1,7]，y∈[1,5]。玩家初始 (5,1,5)。
//
// 注：chat 执行命令仅 Cubium 端有效，命令类测试 Cubium one-sided。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\命令_Boss栏.txt（BossBar 命令）

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { world } from "@minecraft/server";

const PLAYER_POS = { x: 5, y: 1, z: 5 };

// Cubium 扩展的 BossBar/Manager 接口（world.bossbar 经 as 断言，官方类型无此属性）。
interface CubiumBossBar {
    id: string;
    name: string;
    value: number;
    max: number;
    color: string;
    overlay: string;
    visible: boolean;
    players: string[];
}

interface CubiumBossBarManager {
    get(id: string): CubiumBossBar | undefined;
    getAll(): CubiumBossBar[];
}

// 将官方 world 断言为含 bossbar 属性的类型。
function getBossBarManager(): CubiumBossBarManager {
    return (world as unknown as { bossbar: CubiumBossBarManager }).bossbar;
}

// /bossbar add <id> <name> 创建 BossBar，断言 get(id) 非 undefined 且 id/name 匹配，默认属性对齐 vanilla
// （value=0/max=100/color=white/overlay=progress/visible=true）。
// Ref: wiki commands/bossbar.txt（add）
function bossbarAddCreatesBar(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const id = "bar_add";

    player.chat(`/bossbar add ${id} MyBar`);

    const bar = getBossBarManager().get(id);
    test.assert(bar !== undefined, `bossbar ${id} should exist after add`);
    // bar.id 是规范 namespace:path（ResourceLocation 构造补全 "minecraft:"），命令用短名 "bar_add"
    // 创建，存储与读取均经 ResourceLocation 归一化为 "minecraft:bar_add"。
    test.assert(bar!.id === `minecraft:${id}`, `bossbar id should be minecraft:${id}, got ${bar!.id}`);
    test.assert(bar!.name === "MyBar", `bossbar name should be MyBar, got ${bar!.name}`);
    // 默认属性对齐 vanilla（CustomServerBossInfo 构造默认值）。
    test.assert(bar!.value === 0, `default value should be 0, got ${bar!.value}`);
    test.assert(bar!.max === 100, `default max should be 100, got ${bar!.max}`);
    test.assert(bar!.color === "white", `default color should be white, got ${bar!.color}`);
    test.assert(bar!.overlay === "progress", `default overlay should be progress, got ${bar!.overlay}`);
    test.assert(bar!.visible === true, `default visible should be true`);

    test.runOnFinish(() => {
        player.chat(`/bossbar remove ${id}`);
    });
    test.succeed();
}

// /bossbar remove <id> 删除 BossBar：add 后 remove，断言 get(id) 返 undefined。
// Ref: wiki commands/bossbar.txt（remove）
function bossbarRemoveDeletesBar(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const id = "bar_rm";

    player.chat(`/bossbar add ${id} MyBar`);
    test.assert(getBossBarManager().get(id) !== undefined, "precondition: bossbar exists");

    player.chat(`/bossbar remove ${id}`);
    test.assert(
        getBossBarManager().get(id) === undefined,
        `bossbar ${id} should not exist after remove`,
    );

    // 已 remove，无需 runOnFinish。
    test.succeed();
}

// /bossbar set <id> value <n> 修改当前值，断言读回。
// Ref: wiki commands/bossbar.txt（set value）
function bossbarSetValue(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const id = "bar_val";

    player.chat(`/bossbar add ${id} MyBar`);
    player.chat(`/bossbar set ${id} value 42`);

    test.assert(
        getBossBarManager().get(id)!.value === 42,
        `value should be 42, got ${getBossBarManager().get(id)!.value}`,
    );

    test.runOnFinish(() => {
        player.chat(`/bossbar remove ${id}`);
    });
    test.succeed();
}

// /bossbar set <id> max <n> 修改最大值，断言读回。
// Ref: wiki commands/bossbar.txt（set max）
function bossbarSetMax(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const id = "bar_max";

    player.chat(`/bossbar add ${id} MyBar`);
    player.chat(`/bossbar set ${id} max 200`);

    test.assert(
        getBossBarManager().get(id)!.max === 200,
        `max should be 200, got ${getBossBarManager().get(id)!.max}`,
    );

    test.runOnFinish(() => {
        player.chat(`/bossbar remove ${id}`);
    });
    test.succeed();
}

// /bossbar set <id> value 上限受 max 钳制：set max 50 后 set value 100，value 应钳为 50。
// 对齐 vanilla setValue clamp(value, 0, max)（CustomServerBossInfo::setValue）。
// Ref: wiki commands/bossbar.txt（set value/max 钳制语义）
function bossbarValueClampedToMax(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const id = "bar_clamp";

    player.chat(`/bossbar add ${id} MyBar`);
    player.chat(`/bossbar set ${id} max 50`);
    player.chat(`/bossbar set ${id} value 100`);

    const bar = getBossBarManager().get(id)!;
    test.assert(bar.max === 50, `max should be 50, got ${bar.max}`);
    test.assert(bar.value === 50, `value should be clamped to 50, got ${bar.value}`);

    test.runOnFinish(() => {
        player.chat(`/bossbar remove ${id}`);
    });
    test.succeed();
}

// /bossbar set <id> color <c> 修改颜色，断言读回。
// Ref: wiki commands/bossbar.txt（set color）
function bossbarSetColor(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const id = "bar_color";

    player.chat(`/bossbar add ${id} MyBar`);
    player.chat(`/bossbar set ${id} color red`);

    test.assert(
        getBossBarManager().get(id)!.color === "red",
        `color should be red, got ${getBossBarManager().get(id)!.color}`,
    );

    test.runOnFinish(() => {
        player.chat(`/bossbar remove ${id}`);
    });
    test.succeed();
}

// /bossbar set <id> style <s> 修改样式（overlay），断言读回。
// Ref: wiki commands/bossbar.txt（set style）
function bossbarSetStyle(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const id = "bar_style";

    player.chat(`/bossbar add ${id} MyBar`);
    player.chat(`/bossbar set ${id} style notched_6`);

    test.assert(
        getBossBarManager().get(id)!.overlay === "notched_6",
        `overlay should be notched_6, got ${getBossBarManager().get(id)!.overlay}`,
    );

    test.runOnFinish(() => {
        player.chat(`/bossbar remove ${id}`);
    });
    test.succeed();
}

// /bossbar set <id> visible <bool> 修改可见性，断言读回。
// Ref: wiki commands/bossbar.txt（set visible）
function bossbarSetVisible(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const id = "bar_vis";

    player.chat(`/bossbar add ${id} MyBar`);
    test.assert(getBossBarManager().get(id)!.visible === true, "default visible should be true");

    player.chat(`/bossbar set ${id} visible false`);
    test.assert(
        getBossBarManager().get(id)!.visible === false,
        `visible should be false after set, got ${getBossBarManager().get(id)!.visible}`,
    );

    test.runOnFinish(() => {
        player.chat(`/bossbar remove ${id}`);
    });
    test.succeed();
}

// /bossbar set <id> name <name> 修改显示名，断言读回。
// Ref: wiki commands/bossbar.txt（set name）
function bossbarSetName(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const id = "bar_name";

    player.chat(`/bossbar add ${id} OriginalName`);
    player.chat(`/bossbar set ${id} name NewName`);

    test.assert(
        getBossBarManager().get(id)!.name === "NewName",
        `name should be NewName, got ${getBossBarManager().get(id)!.name}`,
    );

    test.runOnFinish(() => {
        player.chat(`/bossbar remove ${id}`);
    });
    test.succeed();
}

// /bossbar set <id> players @s 添加玩家，断言 players 数组长度变化。
// players 存 PlayerId，JS 侧读 playerUuids（UUID 字符串），断言 length 即可。
// Ref: wiki commands/bossbar.txt（set players）
function bossbarSetPlayers(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const id = "bar_players";

    player.chat(`/bossbar add ${id} MyBar`);
    test.assert(
        getBossBarManager().get(id)!.players.length === 0,
        "default players should be empty",
    );

    player.chat(`/bossbar set ${id} players @s`);
    test.assert(
        getBossBarManager().get(id)!.players.length === 1,
        `players should have 1 member after set, got ${getBossBarManager().get(id)!.players.length}`,
    );

    test.runOnFinish(() => {
        player.chat(`/bossbar remove ${id}`);
    });
    test.succeed();
}

// world.bossbar.getAll() 列出所有 BossBar：add 两个后 getAll 应含两者 id。
// 对齐 wiki /bossbar list 语义（命令侧反馈 + JS 侧 getAll 断言集合）。
// Ref: wiki commands/bossbar.txt（list）
function bossbarGetAllListsBars(test: Test): void {
    const player = test.spawnSimulatedPlayer(PLAYER_POS, "op");
    const id1 = "bar_list1";
    const id2 = "bar_list2";

    player.chat(`/bossbar add ${id1} Bar1`);
    player.chat(`/bossbar add ${id2} Bar2`);

    const ids = getBossBarManager().getAll().map((b) => b.id);
    test.assert(
        ids.includes(`minecraft:${id1}`) && ids.includes(`minecraft:${id2}`),
        `getAll should include both, got ${JSON.stringify(ids)}`,
    );

    test.runOnFinish(() => {
        player.chat(`/bossbar remove ${id1}`);
        player.chat(`/bossbar remove ${id2}`);
    });
    test.succeed();
}

export function registerBossBarTests(): void {
    // BossBar 是世界级单例，独占 batch 串行 + 唯一 id + runOnFinish 清理防跨测试污染。
    GameTest.register("CommandTests", "bossbar_add_creates_bar", bossbarAddCreatesBar)
        .structureName("gametests:cmd_arena")
        .batch("bossbar_add_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "bossbar_remove_deletes_bar", bossbarRemoveDeletesBar)
        .structureName("gametests:cmd_arena")
        .batch("bossbar_remove_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "bossbar_set_value", bossbarSetValue)
        .structureName("gametests:cmd_arena")
        .batch("bossbar_val_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "bossbar_set_max", bossbarSetMax)
        .structureName("gametests:cmd_arena")
        .batch("bossbar_max_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "bossbar_value_clamped_to_max", bossbarValueClampedToMax)
        .structureName("gametests:cmd_arena")
        .batch("bossbar_clamp_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "bossbar_set_color", bossbarSetColor)
        .structureName("gametests:cmd_arena")
        .batch("bossbar_color_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "bossbar_set_style", bossbarSetStyle)
        .structureName("gametests:cmd_arena")
        .batch("bossbar_style_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "bossbar_set_visible", bossbarSetVisible)
        .structureName("gametests:cmd_arena")
        .batch("bossbar_vis_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "bossbar_set_name", bossbarSetName)
        .structureName("gametests:cmd_arena")
        .batch("bossbar_name_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "bossbar_set_players", bossbarSetPlayers)
        .structureName("gametests:cmd_arena")
        .batch("bossbar_players_solo")
        .maxTicks(60);

    GameTest.register("CommandTests", "bossbar_get_all_lists_bars", bossbarGetAllListsBars)
        .structureName("gametests:cmd_arena")
        .batch("bossbar_list_solo")
        .maxTicks(60);
}
