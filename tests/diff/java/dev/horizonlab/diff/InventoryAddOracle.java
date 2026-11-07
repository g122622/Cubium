package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import net.minecraft.core.registries.BuiltInRegistries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.entity.EntityEquipment;
import net.minecraft.world.entity.player.Inventory;
import net.minecraft.world.item.Item;
import net.minecraft.world.item.ItemStack;

/**
 * 物品进入玩家背包时的装填顺序（原版 {@code Inventory.add} / {@code addResource}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.InventoryAddOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接用原版 {@code Inventory} 对象驱动：{@code add(ItemStack)} 是公开方法，
 * 与探针侧的 {@code PlayerInventory::add} 一一对应。槽位读写走 {@code setItem} / {@code getItem}，
 * 选中槽走 {@code selected} 字段（原版 public）。
 */
public final class InventoryAddOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private InventoryAddOracle() {}

    private record Preset(String item, int slot, int count) {}

    private record Scene(String name, int selected, List<Preset> presets, String addItem, int addCount) {}

    private static Preset preset(String item, int slot, int count) {
        return new Preset(item, slot, count);
    }

    private static List<Preset> hotbarFull() {
        List<Preset> list = new ArrayList<>();
        for (int slot = 0; slot < 9; slot++) {
            list.add(preset("minecraft:dirt", slot, 64));
        }
        return list;
    }

    private static final List<Scene> SCENES = List.of(
        new Scene("empty_inventory", 0, List.of(), "minecraft:stone", 1),
        new Scene("empty_full_stack", 0, List.of(), "minecraft:stone", 64),
        new Scene("empty_over_stack", 0, List.of(), "minecraft:stone", 100),
        new Scene("merge_into_selected", 5, List.of(preset("minecraft:stone", 5, 10)), "minecraft:stone", 20),
        new Scene("merge_into_offhand", 0, List.of(preset("minecraft:stone", 40, 10)), "minecraft:stone", 20),
        new Scene("offhand_before_scan", 0,
            List.of(preset("minecraft:stone", 1, 10), preset("minecraft:stone", 40, 10)), "minecraft:stone", 20),
        new Scene("scan_ascending", 0,
            List.of(preset("minecraft:stone", 1, 50), preset("minecraft:stone", 3, 50)), "minecraft:stone", 20),
        new Scene("hotbar_full", 0, hotbarFull(), "minecraft:stone", 1),
        new Scene("different_item_no_merge", 0, List.of(preset("minecraft:dirt", 0, 10)), "minecraft:stone", 10),
        new Scene("add_zero", 0, List.of(), "minecraft:stone", 0),
        new Scene("add_air", 0, List.of(), "minecraft:air", 5),
        new Scene("selected_full", 0, List.of(preset("minecraft:stone", 0, 64)), "minecraft:stone", 10),
        new Scene("all_matching_full", 0,
            List.of(preset("minecraft:stone", 0, 64), preset("minecraft:stone", 40, 64)), "minecraft:stone", 10),
        new Scene("spans_multiple", 0, List.of(preset("minecraft:stone", 0, 60)), "minecraft:stone", 200)
    );

    /** 按 ID 取物品；未注册返回 null。 */
    private static Item findItem(String id) {
        Identifier key = Identifier.tryParse(id);
        return key == null ? null : BuiltInRegistries.ITEM.getValue(key);
    }

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (Scene scene : SCENES) {
                text.append(scene.name()).append('\n');
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

        throw new IllegalArgumentException("usage: InventoryAddOracle list <units.txt> | run <in> <out>");
    }

    private static String runLine(String line) {
        JsonObject input = JsonParser.parseString(line).getAsJsonObject();
        String requested = input.get("unit").getAsString();

        Scene scene = SCENES.stream().filter(candidate -> candidate.name().equals(requested)).findFirst().orElse(null);
        if (scene == null) {
            return missing(requested);
        }

        // 原版 Inventory 的 items 只有 36 个槽（0..35），护甲与副手放在 EntityEquipment 里；
        // getContainerSize() = 36 + 5，getItem(40) 会转发到装备，所以 add 确实能装填副手槽。
        Inventory inventory = new Inventory(null, new EntityEquipment());
        inventory.setSelectedSlot(scene.selected());

        for (Preset entry : scene.presets()) {
            Item item = findItem(entry.item());
            if (item == null) {
                return missing(requested);
            }
            inventory.setItem(entry.slot(), new ItemStack(item, entry.count()));
        }

        Item addItem = findItem(scene.addItem());
        if (addItem == null) {
            return missing(requested);
        }
        ItemStack toAdd = new ItemStack(addItem, scene.addCount());
        int beforeCount = toAdd.getCount();
        inventory.add(toAdd);
        // 输出「实际进了背包多少个」。不用 add 的布尔返回值（只表示有没有装进去一点，
        // 空堆叠恒为 false），也不用堆叠的残留数量（两侧归一化时机不同，会造出
        // 与游戏表现无关的假差异）。
        int placed = beforeCount - toAdd.getCount();

        JsonArray slots = new JsonArray();
        for (int slot = 0; slot < inventory.getContainerSize(); slot++) {
            ItemStack stack = inventory.getItem(slot);
            if (stack.isEmpty()) {
                continue;
            }
            JsonObject entry = new JsonObject();
            entry.addProperty("slot", slot);
            entry.addProperty("item", BuiltInRegistries.ITEM.getKey(stack.getItem()).toString());
            entry.addProperty("count", stack.getCount());
            slots.add(entry);
        }

        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("placed", placed);
        output.add("slots", slots);
        return GSON.toJson(output);
    }

    private static String missing(String requested) {
        JsonObject output = new JsonObject();
        output.addProperty("unit", requested);
        output.addProperty("missing", true);
        return GSON.toJson(output);
    }
}
