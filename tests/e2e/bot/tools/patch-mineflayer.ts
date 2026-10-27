/*
 * patch-mineflayer.ts — 给 mineflayer 打上游缺陷补丁。
 *
 * 为什么需要补丁
 * --------------
 * mineflayer 4.39.0 的 `lib/plugins/creative.js` 在实现「创造模式改槽位」时这样等待服务端确认：
 *
 *     bot.inventory.once(`updateSlot:${slot}`, (oldItem, newItem) => {
 *       if (newItem.itemId !== item.itemId) { ... }
 *     })
 *
 * 但槽位内容在协议里是**可空**的：`ContainerSetSlot` / `ContainerSetContent` 对空槽
 * 传的就是 null。于是只要该槽被同步成空，`newItem.itemId` 就会抛
 * `TypeError: Cannot read properties of null (reading 'itemId')`。
 *
 * 更麻烦的是这个异常从 packet 分发链里抛出，会中断**当前正在处理的那个包**。
 * 表现因此极具迷惑性：打开容器类用例全部停在「服务端未下发窗口内容」——被中断的
 * 恰恰是 OpenScreen 之后紧跟的 ContainerSetContent，而不是哪条断言失败。
 *
 * 服务端发 null 完全合规（空槽就该是 null），所以只能在客户端侧绕过这个缺陷。
 *
 * 为什么不写在测试代码里
 * ----------------------
 * 确认回调里用到的期望物品 `item` 位于 `setInventorySlot` 的闭包内，测试侧拿不到，
 * 无法在回调外部还原正确的比较语义。唯一干净的修法是在源头加判空。
 *
 * 幂等性
 * ------
 * 重复执行不会重复插入（已修补过则直接跳过），可以安全地在每次安装后运行。
 */

import fs from "node:fs";
import path from "node:path";

/**
 * 本文件所在目录。
 *
 * 本仓库用 Node 原生 type stripping 直接跑 .ts（无构建步骤），模块是 ESM，
 * 没有 `__dirname`；Node 20.11+ 提供 `import.meta.dirname`。
 */
const HERE = import.meta.dirname;

/** 待修补的文件（相对本目录）。 */
const TARGET = path.join(HERE, "..", "node_modules", "mineflayer", "lib", "plugins", "creative.js");

/**
 * 原始代码块（上游 4.39.0 的实际写法）。
 *
 * 这是「无 ack 版本客户端」的等待分支：注册一次性监听，服务端回显的槽位与期望不符就
 * `reject('Server rejected')`，超时未回显则 `resolve()`。
 */
const ORIGINAL = [
    "        function updateSlot (oldItem, newItem) {",
    "          if (newItem.itemId !== item.itemId) {",
    "            creativeSlotsUpdates[slot] = false",
    "            reject(Error('Server rejected'))",
    "          }",
    "        }",
].join("\n");

/**
 * 修补后的代码块。
 *
 * 三处改动，都是为了处理「槽位条目可以为空」这一协议事实：
 *
 *  1. `newItem` 为空 → 该槽被同步成空槽。此时**不该**当成「服务端拒绝」：调用方
 *     （clearSlot、或刚把物品移走后服务端回显空槽）要的正是这个状态，reject 会让
 *     调用方把一个完全正常的响应当成失败。置标志复位后 resolve 即可。
 *  2. `item` 为空 → 调用方自己就是 clearSlot。同理，任何回显都不构成「拒绝」，
 *     直接 resolve。
 *  3. 两者都非空时才做原有的 itemId 比较。
 *
 * 原实现对前两种情形都会直接读空值的 `itemId` 抛 TypeError；由于这个异常是在
 * packet 分发链里抛出的，它会中断**当前正在处理的那个包**（表现为打开容器一律
 * 报「服务端未下发窗口内容」），而不只是一条可读的失败信息。
 */
const PATCHED = [
    "        function updateSlot (oldItem, newItem) {",
    "          // 空槽是合法的同步状态（见本文件顶部由 tools/patch-mineflayer.ts 写入的说明）：",
    "          // 期望物品为空、或回显物品为空时都不构成「服务端拒绝」，直接确认完成。",
    "          if (newItem === null || newItem === undefined || item === null || item === undefined) {",
    "            creativeSlotsUpdates[slot] = false",
    "            resolve()",
    "            return",
    "          }",
    "          if (newItem.itemId !== item.itemId) {",
    "            creativeSlotsUpdates[slot] = false",
    "            reject(Error('Server rejected'))",
    "          }",
    "        }",
].join("\n");

function main(): void {
    if (!fs.existsSync(TARGET)) {
        // 依赖尚未安装：静默跳过，等 postinstall 跑完之后再来。
        return;
    }

    const source = fs.readFileSync(TARGET, "utf8");
    if (source.includes(PATCHED)) {
        console.log("patch-mineflayer: creative.js 已修补，跳过");
        return;
    }
    if (!source.includes(ORIGINAL)) {
        console.warn(
            "patch-mineflayer: creative.js 中未找到预期的代码片段，可能上游已修复或结构变更。" +
                "请确认 mineflayer 版本是否仍为 4.39.0，并据实调整本脚本。",
        );
        return;
    }

    fs.writeFileSync(TARGET, source.replace(ORIGINAL, PATCHED), "utf8");
    console.log("patch-mineflayer: 已修补 creative.js 的槽位确认判空缺陷");
}

main();