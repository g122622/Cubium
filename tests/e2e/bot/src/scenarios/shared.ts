/*
 * shared.ts — 场景文件共用的交互助手与槽位常量。
 *
 * inventory / crafting / furnace 三个场景文件此前各自复制了一份 setSlot / slotItemName /
 * containerSyncCount；新增容器、持久化、实体等场景后副本会继续膨胀，故统一收口到本文件。
 *
 * 槽位号的两种坐标系必须分清（跨窗口切换时极易错）：
 *   - 玩家背包窗口（containerId=0，InventoryMenu）：0=合成结果、1-4=2x2 合成格、5-8=护甲、
 *     9-35=主背包、36-44=快捷栏、45=副手；
 *   - 方块容器窗口：先排容器自己的槽位，再排 27 格主背包，最后 9 格快捷栏。
 *     故同一个"快捷栏首槽"在箱子窗口里是 54-9=45（箱子 27 格时），在熔炉窗口里是 39-9=30。
 */

import type { Bot } from "mineflayer";
import { waitForCondition } from "../bot/wait.ts";
import { vec3 } from "../assert/surface.ts";

/** 玩家背包窗口（containerId=0）的合成结果槽。 */
export const INV_RESULT_SLOT = 0;
/** 玩家背包窗口的 2x2 合成格首槽。 */
export const INV_GRID_SLOT_START = 1;
/** 玩家背包窗口的主背包首槽。 */
export const INV_MAIN_SLOT_0 = 9;
/** 玩家背包窗口的快捷栏首槽。 */
export const INV_HOTBAR_SLOT_0 = 36;

/** 从 bot 的 registry 里取物品定义。 */
export function itemDef(bot: Bot, itemName: string): { id: number } | undefined {
    const typed = bot as unknown as { registry: { itemsByName: Record<string, { id: number }> } };
    return typed.registry.itemsByName[itemName];
}

/**
 * 读某个窗口（或 bot.inventory）的槽位物品名；空格返回 null。
 *
 * 注意：不同窗口的 slots 坐标系不同，调用方必须用该窗口自己的槽位号。
 */
export function slotItemName(window: unknown, slot: number): string | null {
    const slots = (window as { slots: ({ name?: string } | null)[] }).slots;
    const item = slots[slot];
    return item === null || item === undefined ? null : (item.name ?? null);
}

/**
 * 数一遍客户端收到的「容器同步」包。
 *
 * **这是判断「服务端到底有没有处理这个上行包」的唯一可靠判据**：mineflayer 在
 * `creative.setInventorySlot` 与 `clickWindow` 上都会立刻写本地镜像、不等服务端回包
 * （creative.js 的本地写入、prismarine-windows `Window#acceptClick` 在发包前就改好了
 * 槽位与光标）。于是「等本地槽位变成某物品」这类断言，在服务端把包整个丢掉时照样通过——
 * 用例是绿的却什么都没验证。服务端回包是它确实受理了的直接证据，本地预测伪造不出来。
 *
 * 两条通路都算：vanilla 只回变化槽位的 `set_slot`，Cubium 回整份物品栏的 `window_items`。
 */
export function containerSyncCount(trace: { count(name: string): number }): number {
    return trace.count("window_items") + trace.count("set_slot") + trace.count("container_set_slot");
}

/**
 * 经创造模式把某个物品放进玩家背包的指定菜单槽位。
 *
 * 走 mineflayer 的 creative 插件（内部发 set_creative_mode_slot），因此这条路径同时
 * 检验了服务端对该上行包的处理与回包。
 *
 * @returns 注册表中找不到该物品时返回 false。
 */
export async function setSlot(bot: Bot, slot: number, itemName: string, count: number): Promise<boolean> {
    const def = itemDef(bot, itemName);
    if (def === undefined) {
        return false;
    }
    const typed = bot as unknown as {
        creative: { setInventorySlot(slot: number, item: unknown): Promise<void> };
        version: string;
    };
    type ItemCtor = new (id: number, count: number) => unknown;
    const ItemModule = (await import("prismarine-item")).default as unknown as (version: string) => ItemCtor;
    const Item = ItemModule(typed.version);
    await typed.creative.setInventorySlot(slot, new Item(def.id, count));
    return true;
}

/** 等某个背包槽位变成指定物品。 */
export async function waitForSlot(bot: Bot, slot: number, itemName: string, what: string): Promise<void> {
    await waitForCondition(() => slotItemName(bot.inventory, slot) === itemName, {
        timeoutMs: 10_000,
        pollMs: 50,
        what,
        describe: () => `槽位 ${slot} 当前为 ${String(slotItemName(bot.inventory, slot))}`,
    });
}

/**
 * 当前的实体 id 列表（用于与之后的快照比对，找出"新出现的实体"）。
 *
 * 注意：**不要用 `entity.objectType`**（prismarine-entity 里它是 deprecated getter，
 * 每次访问都会 `console.trace`，runner 的噪声过滤器覆盖不到），也不要用
 * `entity.type === 'object'`（1.21.11 里掉落物的 `type` 是 `'other'`）。
 * 判断掉落物请用 `entity.name === 'item'`。
 */
export function entityIds(bot: Bot): number[] {
    return Object.keys(bot.entities as unknown as Record<string, unknown>).map((key) => Number(key));
}

/** 与给定基线相比新增的实体 id。 */
export function newEntityIds(bot: Bot, baselineIds: readonly number[]): number[] {
    return entityIds(bot).filter((id) => !baselineIds.includes(id));
}

/**
 * 把创造模式给的方块放到 (x, surfaceY+1, z)。
 *
 * 以 (x, surfaceY, z) 的地表方块为参考面向上放。发放物品与放置之间留出一点时间：
 * 服务端入站有「接收线程 enqueue → 主线程 drain」的一次 tick 延迟，手持槽尚未生效时
 * 放置会因 heldItem 为空被直接拒绝（服务端不报错，客户端表现为 Server refused to place）。
 *
 * @returns 放置后的方块对象所在坐标（供调用方继续 blockAt）。
 */
export async function placeFromHotbar(
    bot: Bot,
    itemName: string,
    x: number,
    surfaceY: number,
    z: number,
    settleMs: number,
): Promise<void> {
    const given = await setSlot(bot, INV_HOTBAR_SLOT_0, itemName, 1);
    if (!given) {
        throw new Error(`无法把 ${itemName} 放进快捷栏（registry 中找不到该物品）`);
    }
    await new Promise((resolve) => setTimeout(resolve, settleMs));
    const refBlock = bot.blockAt(vec3(x, surfaceY, z));
    if (refBlock === null) {
        throw new Error(`参考方块 (${x},${surfaceY},${z}) 不可读`);
    }
    await bot.placeBlock(refBlock as never, vec3(0, 1, 0));
    await new Promise((resolve) => setTimeout(resolve, settleMs));
}

/** 服务端命令反馈中出现这些特征即视为失败（权限不足、语法错误、参数非法等）。 */
const COMMAND_ERROR_PATTERN =
    /unknown or incomplete command|incorrect argument|do not have permission|unknown command|expected literal|commands\.[a-z_.]*(error|failed|invalid|unknown|denied)/i;

/**
 * 发送一条服务端命令并等待其反馈。
 *
 * 为什么必须显式等待并检查反馈：命令需要 OP 权限，**权限不足时服务端只会回一条聊天消息、
 * 命令被静默跳过**，用例随后会以「效果断言超时」告终，看不出真正原因。这里对错误特征
 * 立即抛异常，把服务端的原话带到失败信息里。
 *
 * 命令本身的成功与否仍以**效果**为准（例如维度是否真的切换、方块是否真的被放置）——
 * 反馈文本在两个服务端之间不可能逐字一致，不能作为断言依据。
 *
 * @param bot 执行命令的 bot（必须已在 ops.json 中拥有 OP）。
 * @param command 完整命令（含前导斜杠）。
 * @param feedbackTimeoutMs 等待反馈的上限。
 * @returns 收到的反馈文本；未收到任何反馈时返回 null。
 */
export async function runCommand(bot: Bot, command: string, feedbackTimeoutMs: number): Promise<string | null> {
    const typed = bot as unknown as { on(event: string, listener: (...args: unknown[]) => void): void; removeListener(event: string, listener: (...args: unknown[]) => void): void };
    let feedback: string | null = null;
    const onMessage = (...args: unknown[]): void => {
        const text = typeof args[0] === "string" ? args[0] : JSON.stringify(args[0]);
        if (COMMAND_ERROR_PATTERN.test(text)) {
            feedback = text;
            return;
        }
        if (feedback === null) {
            feedback = text;
        }
    };
    typed.on("messagestr", onMessage);
    try {
        bot.chat(command);
        const deadline = Date.now() + feedbackTimeoutMs;
        while (feedback === null && Date.now() < deadline) {
            await new Promise((resolve) => setTimeout(resolve, 50));
        }
    } finally {
        typed.removeListener("messagestr", onMessage);
    }
    if (feedback !== null && COMMAND_ERROR_PATTERN.test(feedback)) {
        throw new Error(`命令 "${command}" 被服务端拒绝：${feedback}`);
    }
    return feedback;
}

/** 等某个坐标的方块可读（区块已送达）。 */
export async function waitForBlockReadable(
    bot: Bot,
    x: number,
    y: number,
    z: number,
    what: string,
    timeoutMs: number,
): Promise<void> {
    await waitForCondition(() => bot.blockAt(vec3(x, y, z)) !== null, {
        timeoutMs,
        pollMs: 100,
        what,
        describe: () => `(${x},${y},${z}) 的区块尚未送达`,
    });
}

/**
 * 等物理 tick 恢复。
 *
 * **跨维度传送/重生之后必须显式等这一步**，否则任何内部依赖物理 tick 的调用都会永久悬着。
 * 原因在 mineflayer 侧（physics.js）：
 *   - 收到 `respawn` 会置 `shouldUsePhysics = false`，而 `physicsTick` 只在
 *     `bot.physicsEnabled && shouldUsePhysics` 时才发射；mineflayer 在重生后还会**延迟
 *     1500ms** 才恢复它；
 *   - 另外 `physicsTick` 的前置条件还包含 `bot.blockAt(bot.entity.position) != null`
 *     ——玩家自身所在列必须有区块数据。
 * 而 `bot.openContainer` → `activateBlock` → `bot.lookAt(..., false)` 会等一个物理 tick，
 * 于是「传送后立刻开容器」会卡在发包之前：客户端连 `use_item_on` 都发不出去，
 * 服务端侧完全无痕（实测下界用例就是这样超时的）。
 *
 * @param bot 目标 bot。
 * @param timeoutMs 等待上限。
 */
export async function waitForPhysicsTick(bot: Bot, timeoutMs: number): Promise<void> {
    const typed = bot as unknown as {
        entity?: { position: { x: number; y: number; z: number } };
        physicsEnabled?: boolean;
        once(event: string, listener: () => void): void;
        removeListener(event: string, listener: () => void): void;
    };
    const position = typed.entity?.position;
    const describe = (): string => {
        if (position === undefined) {
            return "  bot.entity 尚未就绪";
        }
        const own = bot.blockAt(vec3(Math.floor(position.x), Math.floor(position.y), Math.floor(position.z)));
        return [
            `  位置: (${position.x}, ${position.y}, ${position.z})`,
            `  自身所在方块: ${own === null ? "<区块未送达>" : String((own as { name?: string }).name)}`,
            `  physicsEnabled: ${String(typed.physicsEnabled)}`,
        ].join("\n");
    };
    let timer: NodeJS.Timeout | null = null;
    try {
        await Promise.race([
            new Promise<void>((resolve) => {
                const onTick = (): void => {
                    typed.removeListener("physicsTick", onTick);
                    resolve();
                };
                typed.once("physicsTick", onTick);
            }),
            new Promise<never>((_, reject) => {
                timer = setTimeout(
                    () => reject(new Error(`等待物理 tick 恢复超时（${timeoutMs}ms）\n${describe()}`)),
                    timeoutMs,
                );
            }),
        ]);
    } finally {
        if (timer !== null) {
            clearTimeout(timer);
        }
    }
}
