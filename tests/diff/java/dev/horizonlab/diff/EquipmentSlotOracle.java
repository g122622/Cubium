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

package dev.horizonlab.diff;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import net.minecraft.world.entity.EquipmentSlot;
import net.minecraft.world.entity.EquipmentSlotGroup;

/**
 * {@code EquipmentSlot} / {@code EquipmentSlotGroup} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.EquipmentSlotOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link EquipmentSlot#values()} 及其实例方法 `getType` / `getIndex` /
 * `isArmor`，以及 {@link EquipmentSlotGroup#test(EquipmentSlot)} /
 * {@link EquipmentSlotGroup#BY_ID}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/equipment_slot.cpp 逐项一致。
 */
public final class EquipmentSlotOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "slotInfo|MAINHAND",
                    "slotInfo|OFFHAND",
                    "slotInfo|FEET",
                    "slotInfo|LEGS",
                    "slotInfo|CHEST",
                    "slotInfo|HEAD",
                    "slotInfo|BODY",
                    "slotInfo|SADDLE",
                    "groupMembers|ANY",
                    "groupMembers|MAINHAND",
                    "groupMembers|OFFHAND",
                    "groupMembers|HAND",
                    "groupMembers|FEET",
                    "groupMembers|LEGS",
                    "groupMembers|CHEST",
                    "groupMembers|HEAD",
                    "groupMembers|ARMOR",
                    "groupMembers|BODY",
                    "groupMembers|SADDLE",
                    "groupTest|ARMOR|BODY",
                    "groupTest|ARMOR|SADDLE",
                    "groupTest|ARMOR|HEAD",
                    "groupTest|HAND|MAINHAND",
                    "groupTest|HAND|OFFHAND",
                    "groupTest|HAND|FEET",
                    "groupTest|ANY|SADDLE",
                    "groupTest|BODY|BODY",
                    "groupTest|BODY|CHEST",
                    "groupById|0",
                    "groupById|3",
                    "groupById|8",
                    "groupById|10",
                    "groupById|11",
                    "groupById|-1",
                    "groupById|999",
                    "filterBit|MAINHAND|0",
                    "filterBit|CHEST|0",
                    "filterBit|BODY|0",
                    "filterBit|SADDLE|0",
                    "filterBit|HEAD|10",
                    "canXp|MAINHAND",
                    "canXp|CHEST",
                    "canXp|BODY",
                    "canXp|SADDLE",
                    "groupInfo|ANY",
                    "groupInfo|ARMOR",
                    "groupInfo|SADDLE");

    private EquipmentSlotOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            StringBuilder text = new StringBuilder();
            for (String unit : UNITS) {
                text.append(unit).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println(
                "usage: EquipmentSlotOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String unit = JsonParser.parseString(line).getAsJsonObject().get("unit").getAsString();
            text.append(GSON.toJson(summarise(unit))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static EquipmentSlot findSlot(String name) {
        for (EquipmentSlot slot : EquipmentSlot.values()) {
            if (slot.name().equals(name)) {
                return slot;
            }
        }
        return null;
    }

    private static EquipmentSlotGroup findGroup(String name) {
        for (EquipmentSlotGroup group : EquipmentSlotGroup.values()) {
            if (group.name().equals(name)) {
                return group;
            }
        }
        return null;
    }

    /**
     * 取 `EquipmentSlot` 的数量上限。
     *
     * <p>原版没有公开的 getter，用 `limit` 对一个空栈取值不可行，因此用反射读私有字段。
     * 若反射失败则记为 `-1`。
     */
    private static int countLimitOf(EquipmentSlot slot) {
        try {
            var field = EquipmentSlot.class.getDeclaredField("countLimit");
            field.setAccessible(true);
            return field.getInt(slot);
        } catch (ReflectiveOperationException ex) {
            return -1;
        }
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);
        if (parts.length < 2) {
            throw new IllegalArgumentException("equipment_slot: 单元至少两段: " + unit);
        }

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        switch (parts[0]) {
            case "slotInfo" -> {
                EquipmentSlot slot = findSlot(parts[1]);
                if (slot == null) {
                    out.addProperty("value", "null");
                } else {
                    out.addProperty("value",
                            slot.getType().name() + "," + slot.getIndex() + ","
                                    + countLimitOf(slot) + "," + slot.getId() + ","
                                    + slot.getName() + ","
                                    + (slot.isArmor() ? "true" : "false"));
                }
            }
            case "groupTest" -> {
                EquipmentSlot slot = findSlot(parts[2]);
                if (slot == null) {
                    out.addProperty("value", "null");
                } else {
                    EquipmentSlotGroup group = findGroup(parts[1]);
                    if (group == null) {
                        throw new IllegalArgumentException(
                                "equipment_slot: 未知槽位组: " + parts[1]);
                    }
                    out.addProperty("value", group.test(slot) ? "true" : "false");
                }
            }
            case "groupMembers" -> {
                EquipmentSlotGroup group = findGroup(parts[1]);
                if (group == null) {
                    throw new IllegalArgumentException("equipment_slot: 未知槽位组: " + parts[1]);
                }
                StringBuilder members = new StringBuilder();
                for (EquipmentSlot slot : EquipmentSlot.values()) {
                    if (group.test(slot)) {
                        if (members.length() > 0) {
                            members.append(',');
                        }
                        members.append(slot.name());
                    }
                }
                out.addProperty("value", members.toString());
            }
            case "groupById" -> {
                // BY_ID 是 IntFunction，直接对越界值调用即可验证 ZERO 策略。
                EquipmentSlotGroup group =
                        EquipmentSlotGroup.BY_ID.apply(Integer.parseInt(parts[1]));
                out.addProperty("value", group.name());
            }
            case "filterBit" -> {
                EquipmentSlot slot = findSlot(parts[1]);
                if (slot == null) {
                    out.addProperty("value", "null");
                } else {
                    out.addProperty("value", slot.getFilterBit(Integer.parseInt(parts[2])));
                }
            }
            case "canXp" -> {
                EquipmentSlot slot = findSlot(parts[1]);
                if (slot == null) {
                    out.addProperty("value", "null");
                } else {
                    out.addProperty("value", slot.canIncreaseExperience() ? "true" : "false");
                }
            }
            case "groupInfo" -> {
                EquipmentSlotGroup group = findGroup(parts[1]);
                if (group == null) {
                    out.addProperty("value", "null");
                } else {
                    out.addProperty("value",
                            group.ordinal() + "," + group.getSerializedName());
                }
            }
            default -> throw new IllegalArgumentException(
                    "equipment_slot: 无法识别的单元: " + unit);
        }

        return out;
    }
}
