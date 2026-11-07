package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.world.entity.EquipmentSlot;
import net.minecraft.world.inventory.InventoryMenu;

/**
 * 玩家背包内部索引与背包菜单（InventoryMenu）槽位号的互转。
 *
 * <pre>
 *   java dev.horizonlab.diff.InventorySlotMappingOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>原版没有把这张表抽成一个函数，而是**分散在 InventoryMenu 的构造里**：
 *
 * <ul>
 *   <li>{@code for (int i = 0; i < 4; i++) addSlot(new ArmorSlot(..., SLOT_IDS[i], 39 - i, ...))}：
 *       菜单槽 {@code 5 + i} 绑到背包槽 {@code 39 - i}，而 {@code SLOT_IDS = {HEAD, CHEST, LEGS, FEET}}，
 *       即菜单槽 5=HEAD→背包 39、6=CHEST→38、7=LEGS→37、8=FEET→36；</li>
 *   <li>{@code addStandardInventorySlots}：主背包 9..35 原样对应，快捷栏 0..8 → 菜单 36..44；</li>
 *   <li>最后一个 {@code new Slot(..., 40, ...)}：副手，菜单 45 ← 背包 40。</li>
 * </ul>
 *
 * <p>这里的推导直接用原版的 {@code EquipmentSlot.getIndex()} 与三个区间常量算出映射，
 * 不把结论写死——常量顺序一旦被改，这里会跟着变。
 *
 * <p>注意数值重叠：背包槽 36..39 是护甲，菜单槽 36..44 是快捷栏。同一个数字在两套
 * 索引里指完全不同的东西，这正是这张表最容易写错的地方。
 */
public final class InventorySlotMappingOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private InventorySlotMappingOracle() {}

    /** 背包内部索引上界：副手槽 40。 */
    private static final int INVENTORY_MAX = EquipmentSlot.OFFHAND.getIndex(39);

    /** 菜单槽上界：副手槽。 */
    private static final int MENU_MAX = InventoryMenu.SHIELD_SLOT;

    /**
     * 背包内部索引 → 菜单槽。
     *
     * <p>按原版 InventoryMenu 的绑槽顺序反推，越界返回 -1。
     */
    private static int playerInvToMenuSlot(int slot) {
        // 快捷栏 0..8 → 菜单 36..44（快捷栏宽度 = USE_ROW_SLOT_END - USE_ROW_SLOT_START）
        if (slot >= 0 && slot < InventoryMenu.USE_ROW_SLOT_END - InventoryMenu.USE_ROW_SLOT_START) {
            return InventoryMenu.USE_ROW_SLOT_START + slot;
        }
        // 主背包 9..35 原样
        if (slot >= InventoryMenu.INV_SLOT_START && slot < InventoryMenu.INV_SLOT_END) {
            return slot;
        }
        // 护甲：背包 36..39 → 菜单 8..5（FEET→8, LEGS→7, CHEST→6, HEAD→5）
        if (slot >= EquipmentSlot.FEET.getIndex(36) && slot <= EquipmentSlot.HEAD.getIndex(36)) {
            return InventoryMenu.ARMOR_SLOT_START + (EquipmentSlot.HEAD.getIndex(36) - slot);
        }
        // 副手 40 → 菜单 45
        if (slot == 40) {
            return InventoryMenu.SHIELD_SLOT;
        }
        return -1;
    }

    /** 菜单槽 → 背包内部索引，越界或合成槽返回 -1。 */
    private static int menuSlotToPlayerInvId(int slot) {
        // 护甲 5..8 ← 背包 39..36：菜单 5+i 对应背包 (HEAD.getIndex(36) - i)
        if (slot >= InventoryMenu.ARMOR_SLOT_START && slot < InventoryMenu.ARMOR_SLOT_END) {
            return EquipmentSlot.HEAD.getIndex(36) - (slot - InventoryMenu.ARMOR_SLOT_START);
        }
        // 合成结果 0 与合成输入 1..4 在玩家背包里没有对应槽
        if (slot >= 0 && slot < InventoryMenu.ARMOR_SLOT_START) {
            return -1;
        }
        // 主背包 9..35 原样
        if (slot >= InventoryMenu.INV_SLOT_START && slot < InventoryMenu.INV_SLOT_END) {
            return slot;
        }
        // 快捷栏 36..44 → 背包 0..8
        if (slot >= InventoryMenu.USE_ROW_SLOT_START && slot < InventoryMenu.USE_ROW_SLOT_END) {
            return slot - InventoryMenu.USE_ROW_SLOT_START;
        }
        // 副手 45 → 背包 40
        if (slot == InventoryMenu.SHIELD_SLOT) {
            return 40;
        }
        return -1;
    }

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (int slot = 0; slot <= INVENTORY_MAX; slot++) {
                text.append("inv2menu|").append(slot).append('\n');
            }
            for (int slot : new int[] {-1, 41, 100}) {
                text.append("inv2menu|").append(slot).append('\n');
            }
            for (int slot = 0; slot <= MENU_MAX; slot++) {
                text.append("menu2inv|").append(slot).append('\n');
            }
            for (int slot : new int[] {-1, 46, 100}) {
                text.append("menu2inv|").append(slot).append('\n');
            }
            Files.writeString(Path.of(args[1]), text.toString(), StandardCharsets.UTF_8);
            return;
        }

        if (args.length == 3 && args[0].equals("run")) {
            List<String> inputs = Files.readAllLines(Path.of(args[1]), StandardCharsets.UTF_8);
            List<String> outputs = new ArrayList<>(inputs.size());
            for (String line : inputs) {
                if (line.isBlank()) {
                    continue;
                }
                outputs.add(runLine(line));
            }
            Files.write(Path.of(args[2]), outputs, StandardCharsets.UTF_8);
            return;
        }

        throw new IllegalArgumentException("usage: InventorySlotMappingOracle list <units.txt> | run <in> <out>");
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();

        int bar = requested.indexOf('|');
        if (bar < 0) {
            return missing(requested);
        }
        String mode = requested.substring(0, bar);
        int slot;
        try {
            slot = Integer.parseInt(requested.substring(bar + 1));
        } catch (NumberFormatException error) {
            return missing(requested);
        }

        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        if (mode.equals("inv2menu")) {
            output.addProperty("mapped", playerInvToMenuSlot(slot));
        } else if (mode.equals("menu2inv")) {
            output.addProperty("mapped", menuSlotToPlayerInvId(slot));
        } else {
            return missing(requested);
        }
        return GSON.toJson(output);
    }

    private static String missing(String requested) {
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("missing", true);
        return GSON.toJson(output);
    }
}