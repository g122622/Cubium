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

/**
 * 游戏事件到振动频率映射的原版 oracle
 * （{@code VibrationSystem.VIBRATION_FREQUENCY_FOR_EVENT}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.VibrationFrequencyOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元是游戏事件 id 的裸路径（如 {@code block_change}）。
 *
 * <p>这张表把事件归入 1..15 的频段，频段决定红石比较器输出强度与紫水晶共振方块的
 * RESONATE_N 事件；表里没有的事件频率为 0，不产生任何振动。
 *
 * <p>分组整体偏移不会崩、不会报错，只是所有振动装置的读数同时「差不多」地错掉 ——
 * 这是最难靠手动测试发现的一类缺陷，必须逐条固定。
 */
public final class VibrationFrequencyOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 原版 VIBRATION_FREQUENCY_FOR_EVENT 全表（事件 id -> 频率） */
    private static int vanillaFrequency(String id) {
        switch (id) {
            case "STEP": return 1;
            case "SWIM": return 1;
            case "FLAP": return 1;
            case "PROJECTILE_LAND": return 2;
            case "HIT_GROUND": return 2;
            case "SPLASH": return 2;
            case "ITEM_INTERACT_FINISH": return 3;
            case "PROJECTILE_SHOOT": return 3;
            case "INSTRUMENT_PLAY": return 3;
            case "ENTITY_ACTION": return 4;
            case "ELYTRA_GLIDE": return 4;
            case "UNEQUIP": return 4;
            case "ENTITY_DISMOUNT": return 5;
            case "EQUIP": return 5;
            case "ENTITY_INTERACT": return 6;
            case "SHEAR": return 6;
            case "ENTITY_MOUNT": return 6;
            case "ENTITY_DAMAGE": return 7;
            case "DRINK": return 8;
            case "EAT": return 8;
            case "CONTAINER_CLOSE": return 9;
            case "BLOCK_CLOSE": return 9;
            case "BLOCK_DEACTIVATE": return 9;
            case "BLOCK_DETACH": return 9;
            case "CONTAINER_OPEN": return 10;
            case "BLOCK_OPEN": return 10;
            case "BLOCK_ACTIVATE": return 10;
            case "BLOCK_ATTACH": return 10;
            case "PRIME_FUSE": return 10;
            case "NOTE_BLOCK_PLAY": return 10;
            case "BLOCK_CHANGE": return 11;
            case "BLOCK_DESTROY": return 12;
            case "FLUID_PICKUP": return 12;
            case "BLOCK_PLACE": return 13;
            case "FLUID_PLACE": return 13;
            case "ENTITY_PLACE": return 14;
            case "LIGHTNING_STRIKE": return 14;
            case "TELEPORT": return 14;
            case "ENTITY_DIE": return 15;
            case "EXPLODE": return 15;
            default: return 0;
        }
    }

    private static final String[] EVENTS = {
            "BLOCK_ACTIVATE",
            "BLOCK_ATTACH",
            "BLOCK_CHANGE",
            "BLOCK_CLOSE",
            "BLOCK_DEACTIVATE",
            "BLOCK_DESTROY",
            "BLOCK_DETACH",
            "BLOCK_OPEN",
            "BLOCK_PLACE",
            "CONTAINER_CLOSE",
            "CONTAINER_OPEN",
            "DRINK",
            "EAT",
            "ELYTRA_GLIDE",
            "ENTITY_ACTION",
            "ENTITY_DAMAGE",
            "ENTITY_DIE",
            "ENTITY_DISMOUNT",
            "ENTITY_INTERACT",
            "ENTITY_MOUNT",
            "ENTITY_PLACE",
            "EQUIP",
            "EXPLODE",
            "FLAP",
            "FLUID_PICKUP",
            "FLUID_PLACE",
            "HIT_GROUND",
            "INSTRUMENT_PLAY",
            "ITEM_INTERACT_FINISH",
            "LIGHTNING_STRIKE",
            "NOTE_BLOCK_PLAY",
            "PRIME_FUSE",
            "PROJECTILE_LAND",
            "PROJECTILE_SHOOT",
            "SHEAR",
            "SPLASH",
            "STEP",
            "SWIM",
            "TELEPORT",
            "UNEQUIP",
    };

    private VibrationFrequencyOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (String id : EVENTS) {
                units.add(id);
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: VibrationFrequencyOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        List<String> lines = Files.readAllLines(input, StandardCharsets.UTF_8);
        List<String> results = new ArrayList<>(lines.size());
        for (String line : lines) {
            if (line.isBlank()) {
                continue;
            }
            JsonObject obj = JsonParser.parseString(line).getAsJsonObject();
            results.add(GSON.toJson(solve(obj.get("unit").getAsString())));
        }
        Files.write(output, results, StandardCharsets.UTF_8);
    }

    private static JsonObject solve(String unit) {
        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        boolean known = false;
        for (String id : EVENTS) {
            if (id.equals(unit)) {
                known = true;
                break;
            }
        }
        if (!known) {
            out.addProperty("missing", true);
            return out;
        }

        out.addProperty("frequency", vanillaFrequency(unit));
        return out;
    }
}
