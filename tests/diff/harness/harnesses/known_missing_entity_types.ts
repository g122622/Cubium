/**
 * `entity_type_size` 能力的已知缺口：原版已注册、Cubium 尚未注册的实体类型。
 *
 * 这些类型在 Cubium 的 `EntityRegistry` 里查不到，探针返回 `{ missing: true }`，
 * 因此不参与尺寸比对。清单必须随实现推进逐条删除——否则会出现「豁免了原版也有的项」
 * 的假一致，比不测更危险。
 *
 * 分类（括号内为原版 `minecraft:` 后的名字）：
 *
 * - **船 / 筏**（21）：oak/spruce/birch/jungle/acacia/cherry/dark_oak/pale_oak/mangrove
 *   各自的 boat 与 chest_boat，外加 bamboo_raft / bamboo_chest_raft。
 * - **展示实体**（3）：block_display / item_display / text_display。
 * - **无尺寸实体**（2）：marker / interaction —— 原版三项尺寸均为 0。
 * - **投掷药水**（2）：splash_potion / lingering_potion。
 * - **其它**（3）：breeze_wind_charge / command_block_minecart / creaking / mannequin。
 *
 * TODO: Cubium 补齐上述实体类型后，逐条删除本清单；删除后 `entity_type_size`
 * 会自动开始比对它们的尺寸，无需改探针或 oracle。
 */
export const KNOWN_MISSING: readonly string[] = [
    "minecraft:acacia_boat",
    "minecraft:acacia_chest_boat",
    "minecraft:bamboo_chest_raft",
    "minecraft:bamboo_raft",
    "minecraft:birch_boat",
    "minecraft:birch_chest_boat",
    "minecraft:block_display",
    "minecraft:breeze_wind_charge",
    "minecraft:cherry_boat",
    "minecraft:cherry_chest_boat",
    "minecraft:command_block_minecart",
    "minecraft:creaking",
    "minecraft:dark_oak_boat",
    "minecraft:dark_oak_chest_boat",
    "minecraft:interaction",
    "minecraft:item_display",
    "minecraft:jungle_boat",
    "minecraft:jungle_chest_boat",
    "minecraft:lingering_potion",
    "minecraft:mangrove_boat",
    "minecraft:mangrove_chest_boat",
    "minecraft:mannequin",
    "minecraft:marker",
    "minecraft:oak_boat",
    "minecraft:oak_chest_boat",
    "minecraft:pale_oak_boat",
    "minecraft:pale_oak_chest_boat",
    "minecraft:splash_potion",
    "minecraft:spruce_boat",
    "minecraft:spruce_chest_boat",
    "minecraft:text_display",
];
