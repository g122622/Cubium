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

/*
 * entity_attributes_reference.ts — 生物默认属性基础值的原版参考值。
 *
 * 本表**不是** oracle：原版部分生物（史莱姆、恶魂、幻翼、末影龙等）的属性基础值
 * 会按尺寸/随机数/状态在运行时调整，`EntityAttributesOracle` 读到的是「创建实体后的
 * 那一刻」的值，拿它当默认值比会把正常行为误判成差异（例如史莱姆尺寸 1 时 max_health
 * 是 1，而它的默认供应器是 20）。
 *
 * 因此 entity_attributes harness 的通过条件是：
 *   Cubium 的值必须等于**表里登记的原版默认值**；
 *   表里没有的生物回退到 `EntityAttributesOracle`（默认供应器与实例值一致）。
 *
 * 表中数值取自原版 1.21.11 的 `DefaultAttributes` / 各类 `createAttributes`
 * （`javap -c` 直接读字节码常量），与 `Minecraft wiki` 一致。新增生物时请照此补齐，
 * 不要用 oracle 输出的实例值反推。
 */
export const VANILLA_ENTITY_ATTRIBUTES: ReadonlyMap<string, ReadonlyMap<string, number>> = new Map<
    string,
    ReadonlyMap<string, number>
>([
    ["minecraft:cow", new Map([["max_health", 10.0], ["movement_speed", 0.2], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:pig", new Map([["max_health", 10.0], ["movement_speed", 0.25], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:chicken", new Map([["max_health", 4.0], ["movement_speed", 0.25], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:sheep", new Map([["max_health", 8.0], ["movement_speed", 0.23], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:zombie", new Map([["max_health", 20.0], ["movement_speed", 0.23], ["follow_range", 35.0], ["attack_damage", 3.0], ["armor", 2.0], ["armor_toughness", 0.0]])],
    ["minecraft:skeleton", new Map([["max_health", 20.0], ["movement_speed", 0.25], ["follow_range", 16.0], ["attack_damage", 2.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:creeper", new Map([["max_health", 20.0], ["movement_speed", 0.25], ["follow_range", 16.0], ["attack_damage", 2.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:spider", new Map([["max_health", 16.0], ["movement_speed", 0.3], ["follow_range", 16.0], ["attack_damage", 2.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:cave_spider", new Map([["max_health", 12.0], ["movement_speed", 0.3], ["follow_range", 16.0], ["attack_damage", 2.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:husk", new Map([["max_health", 20.0], ["movement_speed", 0.23], ["follow_range", 35.0], ["attack_damage", 3.0], ["armor", 2.0], ["armor_toughness", 0.0]])],
    ["minecraft:drowned", new Map([["max_health", 20.0], ["movement_speed", 0.23], ["follow_range", 35.0], ["attack_damage", 3.0], ["armor", 2.0], ["armor_toughness", 0.0]])],
    ["minecraft:stray", new Map([["max_health", 20.0], ["movement_speed", 0.25], ["follow_range", 16.0], ["attack_damage", 2.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:bogged", new Map([["max_health", 16.0], ["movement_speed", 0.25], ["follow_range", 16.0], ["attack_damage", 2.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:wither_skeleton", new Map([["max_health", 20.0], ["movement_speed", 0.25], ["follow_range", 16.0], ["attack_damage", 2.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:blaze", new Map([["max_health", 20.0], ["movement_speed", 0.23], ["follow_range", 48.0], ["attack_damage", 6.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:enderman", new Map([["max_health", 40.0], ["movement_speed", 0.3], ["follow_range", 64.0], ["attack_damage", 7.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:endermite", new Map([["max_health", 8.0], ["movement_speed", 0.25], ["follow_range", 16.0], ["attack_damage", 2.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:silverfish", new Map([["max_health", 8.0], ["movement_speed", 0.25], ["follow_range", 16.0], ["attack_damage", 1.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:witch", new Map([["max_health", 26.0], ["movement_speed", 0.25], ["follow_range", 16.0], ["attack_damage", 2.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:pillager", new Map([["max_health", 24.0], ["movement_speed", 0.35], ["follow_range", 32.0], ["attack_damage", 5.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:vindicator", new Map([["max_health", 24.0], ["movement_speed", 0.35], ["follow_range", 12.0], ["attack_damage", 5.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:evoker", new Map([["max_health", 24.0], ["movement_speed", 0.5], ["follow_range", 12.0], ["attack_damage", 2.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:ravager", new Map([["max_health", 100.0], ["movement_speed", 0.3], ["follow_range", 32.0], ["attack_damage", 12.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:vex", new Map([["max_health", 14.0], ["movement_speed", 0.7], ["follow_range", 16.0], ["attack_damage", 4.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:illusioner", new Map([["max_health", 32.0], ["movement_speed", 0.5], ["follow_range", 18.0], ["attack_damage", 2.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:guardian", new Map([["max_health", 30.0], ["movement_speed", 0.5], ["follow_range", 16.0], ["attack_damage", 6.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:elder_guardian", new Map([["max_health", 80.0], ["movement_speed", 0.3], ["follow_range", 16.0], ["attack_damage", 8.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:zombified_piglin", new Map([["max_health", 20.0], ["movement_speed", 0.23], ["follow_range", 35.0], ["attack_damage", 5.0], ["armor", 2.0], ["armor_toughness", 0.0]])],
    ["minecraft:piglin", new Map([["max_health", 16.0], ["movement_speed", 0.35], ["follow_range", 16.0], ["attack_damage", 5.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:piglin_brute", new Map([["max_health", 50.0], ["movement_speed", 0.35], ["follow_range", 12.0], ["attack_damage", 7.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:hoglin", new Map([["max_health", 40.0], ["movement_speed", 0.3], ["follow_range", 16.0], ["attack_damage", 6.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:zoglin", new Map([["max_health", 40.0], ["movement_speed", 0.3], ["follow_range", 16.0], ["attack_damage", 6.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:strider", new Map([["max_health", 20.0], ["movement_speed", 0.175], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:warden", new Map([["max_health", 500.0], ["movement_speed", 0.3], ["follow_range", 24.0], ["attack_damage", 30.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:breeze", new Map([["max_health", 30.0], ["movement_speed", 0.63], ["follow_range", 24.0], ["attack_damage", 3.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:iron_golem", new Map([["max_health", 100.0], ["movement_speed", 0.25], ["follow_range", 16.0], ["attack_damage", 15.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:snow_golem", new Map([["max_health", 4.0], ["movement_speed", 0.2], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:villager", new Map([["max_health", 20.0], ["movement_speed", 0.5], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:wandering_trader", new Map([["max_health", 20.0], ["movement_speed", 0.7], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:wolf", new Map([["max_health", 8.0], ["movement_speed", 0.3], ["follow_range", 16.0], ["attack_damage", 4.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:cat", new Map([["max_health", 10.0], ["movement_speed", 0.3], ["follow_range", 16.0], ["attack_damage", 3.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:ocelot", new Map([["max_health", 10.0], ["movement_speed", 0.3], ["follow_range", 16.0], ["attack_damage", 3.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:fox", new Map([["max_health", 10.0], ["movement_speed", 0.3], ["follow_range", 32.0], ["attack_damage", 2.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:rabbit", new Map([["max_health", 3.0], ["movement_speed", 0.3], ["follow_range", 16.0], ["attack_damage", 3.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:bee", new Map([["max_health", 10.0], ["movement_speed", 0.3], ["follow_range", 16.0], ["attack_damage", 2.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:goat", new Map([["max_health", 10.0], ["movement_speed", 0.2], ["follow_range", 16.0], ["attack_damage", 2.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:panda", new Map([["max_health", 20.0], ["movement_speed", 0.15], ["follow_range", 16.0], ["attack_damage", 6.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:polar_bear", new Map([["max_health", 30.0], ["movement_speed", 0.25], ["follow_range", 20.0], ["attack_damage", 6.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:frog", new Map([["max_health", 10.0], ["movement_speed", 1.0], ["follow_range", 16.0], ["attack_damage", 10.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:tadpole", new Map([["max_health", 6.0], ["movement_speed", 1.0], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:axolotl", new Map([["max_health", 14.0], ["movement_speed", 1.0], ["follow_range", 16.0], ["attack_damage", 2.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:dolphin", new Map([["max_health", 10.0], ["movement_speed", 1.2], ["follow_range", 16.0], ["attack_damage", 3.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:turtle", new Map([["max_health", 30.0], ["movement_speed", 0.25], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:armadillo", new Map([["max_health", 12.0], ["movement_speed", 0.14], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:camel", new Map([["max_health", 32.0], ["movement_speed", 0.09], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:camel_husk", new Map([["max_health", 32.0], ["movement_speed", 0.09], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:llama", new Map([["max_health", 53.0], ["movement_speed", 0.175], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:trader_llama", new Map([["max_health", 53.0], ["movement_speed", 0.175], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:horse", new Map([["max_health", 53.0], ["movement_speed", 0.225], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:donkey", new Map([["max_health", 53.0], ["movement_speed", 0.175], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:mule", new Map([["max_health", 53.0], ["movement_speed", 0.175], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:skeleton_horse", new Map([["max_health", 15.0], ["movement_speed", 0.2], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:zombie_horse", new Map([["max_health", 25.0], ["movement_speed", 0.225], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:bat", new Map([["max_health", 6.0], ["movement_speed", 0.7], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:glow_squid", new Map([["max_health", 10.0], ["movement_speed", 0.7], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:cod", new Map([["max_health", 3.0], ["movement_speed", 0.7], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:salmon", new Map([["max_health", 3.0], ["movement_speed", 0.7], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:tropical_fish", new Map([["max_health", 3.0], ["movement_speed", 0.7], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:pufferfish", new Map([["max_health", 3.0], ["movement_speed", 0.7], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:nautilus", new Map([["max_health", 15.0], ["movement_speed", 1.0], ["follow_range", 16.0], ["attack_damage", 3.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:copper_golem", new Map([["max_health", 12.0], ["movement_speed", 0.2], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:sniffer", new Map([["max_health", 14.0], ["movement_speed", 0.1], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:happy_ghast", new Map([["max_health", 20.0], ["movement_speed", 0.05], ["follow_range", 16.0], ["armor", 0.0], ["armor_toughness", 0.0]])],
    ["minecraft:armor_stand", new Map([["max_health", 20.0], ["movement_speed", 0.7], ["armor", 0.0], ["armor_toughness", 0.0]])],
]);

/**
 * 运行时按尺寸/随机数改写属性基础值的生物。
 *
 * 这些生物在 `EntityAttributesOracle` 里读到的是「刚创建出来那一刻」的值（例如尺寸 1
 * 的史莱姆只有 1 点血），不能当作默认值。参考值取自原版 `DefaultAttributes` 表里
 * 登记的属性工厂常量：
 *
 * 马族（horse/donkey/mule/llama/trader_llama/skeleton_horse/zombie_horse）不在此列：
 * harness 按「刚创建」比对，原版只在 `finalizeSpawn` 用随机数覆盖生命值/速度/跳跃力，
 * 所以刚创建的实体拿到的是属性工厂里的默认值，参考表登记的就是这些默认值
 * （source: AbstractHorse / AbstractChestedHorse.createBase*Attributes）。
 *
 *   - `minecraft:slime`：max_health = 20.0、movement_speed = 0.7、follow_range = 16.0、armor = 0.0、knockback_resistance = 0.0
 *   - `minecraft:magma_cube`：max_health = 20.0、movement_speed = 0.2、follow_range = 16.0、armor = 0.0、knockback_resistance = 0.0
 *   - `minecraft:phantom`：max_health = 20.0、movement_speed = 0.7、follow_range = 16.0、armor = 0.0、knockback_resistance = 0.0
 */
export const RUNTIME_DIVERGENT: readonly string[] = [
    "minecraft:slime",
    "minecraft:magma_cube",
    "minecraft:phantom",
];
