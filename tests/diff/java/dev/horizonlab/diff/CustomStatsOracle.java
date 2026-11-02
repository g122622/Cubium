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
import java.lang.reflect.Field;
import java.lang.reflect.Modifier;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import net.minecraft.SharedConstants;
import net.minecraft.resources.Identifier;
import net.minecraft.server.Bootstrap;
import net.minecraft.stats.Stat;
import net.minecraft.stats.Stats;

/**
 * 自定义统计 {@code Stats} 常量表的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.CustomStatsOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>不硬编码统计名，而是**反射扫描 {@link Stats} 里所有 {@code Identifier} 类型的
 * public static final 字段**，再从 {@code BuiltInRegistries.CUSTOM_STAT} 里取回对应的
 * {@link StatFormatter}。这样原版新增统计时 oracle 自动跟上，不会漏。
 *
 * <p>单元表必须与 probes/custom_stats.cpp 的 VANILLA_NAMES 逐项一致。
 */
public final class CustomStatsOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final String[] UNITS = {
        "leave_game", "play_time", "total_world_time", "time_since_death", "time_since_rest", "sneak_time",
        "walk_one_cm", "crouch_one_cm", "sprint_one_cm", "walk_on_water_one_cm", "fall_one_cm", "climb_one_cm",
        "fly_one_cm", "walk_under_water_one_cm", "minecart_one_cm", "boat_one_cm", "pig_one_cm",
        "happy_ghast_one_cm", "horse_one_cm", "aviate_one_cm", "swim_one_cm", "strider_one_cm",
        "nautilus_one_cm", "jump", "drop", "damage_dealt", "damage_dealt_absorbed", "damage_dealt_resisted",
        "damage_taken", "damage_blocked_by_shield", "damage_absorbed", "damage_resisted", "deaths", "mob_kills",
        "animals_bred", "player_kills", "fish_caught", "talked_to_villager", "traded_with_villager",
        "eat_cake_slice", "fill_cauldron", "use_cauldron", "clean_armor", "clean_banner", "clean_shulker_box",
        "interact_with_brewingstand", "interact_with_beacon", "inspect_dropper", "inspect_hopper",
        "inspect_dispenser", "play_noteblock", "tune_noteblock", "pot_flower", "trigger_trapped_chest",
        "open_enderchest", "enchant_item", "play_record", "interact_with_furnace",
        "interact_with_crafting_table", "open_chest", "sleep_in_bed", "open_shulker_box", "open_barrel",
        "interact_with_blast_furnace", "interact_with_smoker", "interact_with_lectern",
        "interact_with_campfire", "interact_with_cartography_table", "interact_with_loom",
        "interact_with_stonecutter", "bell_ring", "raid_trigger", "raid_win", "interact_with_anvil",
        "interact_with_grindstone", "target_hit", "interact_with_smithing_table",
    };

    private CustomStatsOracle() {}

    /** 用于识别 formatter 语义的固定输入。 */
    private static final int SAMPLE_VALUE = 12345;

    /** 统计名 → `format(12345)` 的结果。 */
    private static Map<String, String> collect() throws IllegalAccessException {
        Map<String, String> out = new LinkedHashMap<>();
        for (Field field : Stats.class.getDeclaredFields()) {
            if (!Modifier.isStatic(field.getModifiers()) || !Modifier.isFinal(field.getModifiers())) {
                continue;
            }
            if (field.getType() != Identifier.class) {
                continue;
            }
            Identifier id = (Identifier) field.get(null);
            StatTypeMarker marker = new StatTypeMarker(id);
            out.put(id.getPath(), marker.formatterName());
        }
        return out;
    }

    /** 从注册表取回 formatter；取不到时记为 UNKNOWN。 */
    private record StatTypeMarker(Identifier id) {
        String formatterName() {
            try {
                // `Stat` 不暴露 formatter，只有 format(int)。用固定输入的实际输出作为比对依据，
                // 这样同时验证了「统计存在」与「格式化语义正确」两件事。
                return Stats.CUSTOM.get(id).format(SAMPLE_VALUE);
            } catch (RuntimeException e) {
                return "UNKNOWN";
            }
        }
    }

    public static void main(String[] args) throws Exception {
        SharedConstants.tryDetectVersion();
        Bootstrap.bootStrap();
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
        System.err.println("usage: CustomStatsOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws Exception {
        Map<String, String> known = collect();
        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String unit = JsonParser.parseString(line).getAsJsonObject().get("unit").getAsString();
            JsonObject row = new JsonObject();
            row.addProperty("result", known.getOrDefault(unit, "MISSING"));
            row.addProperty("unit", unit);
            text.append(GSON.toJson(row)).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }
}