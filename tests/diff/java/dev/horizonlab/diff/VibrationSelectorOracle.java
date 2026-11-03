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
 * 幽匿感测体振动选优的原版 oracle（{@code VibrationSelector#shouldReplaceVibration}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.VibrationSelectorOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元名形如 {@code explode:4:100}（事件 : 距离 : 时刻）。
 *
 * <p>已有候选固定在时刻 100、距离 8、事件 step（频率 1）。
 *
 * <p>原版三级选优：
 *
 * <pre>
 *   时刻不同                          -&gt; 不替换
 *   时刻相同，新距离 &lt; 旧距离         -&gt; 替换
 *   时刻相同，新距离 &gt; 旧距离         -&gt; 不替换
 *   时刻、距离都相同，新频率 &gt; 旧频率 -&gt; 替换
 * </pre>
 *
 * <p>顺序不可颠倒：先比频率再比距离，远处一次「爆炸」（频率 15）会盖掉近处的脚步声。
 * 允许跨 tick 替换则高频事件不断插队，低频事件永远得不到处理。
 */
public final class VibrationSelectorOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final long BASE_TICK = 100L;
    private static final float BASE_DISTANCE = 8.0F;
    private static final String BASE_EVENT = "minecraft:step";

    /** 事件 -> 原版振动频率（只列出本探针用到的四个） */
    private static int frequency(String id) {
        switch (id) {
            case "minecraft:step": return 1;
            case "minecraft:shear": return 6;
            case "minecraft:block_change": return 11;
            case "minecraft:explode": return 15;
            default: return 0;
        }
    }

    private static final String[] EVENT_LABELS = {"step", "shear", "block_change", "explode"};
    private static final float[] DISTANCE_OPTIONS = {4.0F, 8.0F, 12.0F};
    private static final long[] TICK_OPTIONS = {100L, 101L};

    private VibrationSelectorOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (String label : EVENT_LABELS) {
                for (float distance : DISTANCE_OPTIONS) {
                    for (long tick : TICK_OPTIONS) {
                        units.add(label + ":" + String.format("%.0f", distance) + ":" + tick);
                    }
                }
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: VibrationSelectorOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        String[] parts = unit.split(":");
        if (parts.length != 3) {
            out.addProperty("missing", true);
            return out;
        }
        String label = parts[0];
        boolean known = false;
        for (String candidate : EVENT_LABELS) {
            if (candidate.equals(label)) {
                known = true;
                break;
            }
        }
        float distance;
        long tick;
        try {
            distance = Float.parseFloat(parts[1]);
            tick = Long.parseLong(parts[2]);
        } catch (NumberFormatException e) {
            out.addProperty("missing", true);
            return out;
        }
        if (!known) {
            out.addProperty("missing", true);
            return out;
        }

        String newEvent = "minecraft:" + label;

        // 逐字对应原版 shouldReplaceVibration
        boolean replace;
        if (tick != BASE_TICK) {
            replace = false;
        } else if (distance < BASE_DISTANCE) {
            replace = true;
        } else if (distance > BASE_DISTANCE) {
            replace = false;
        } else {
            replace = frequency(newEvent) > frequency(BASE_EVENT);
        }

        out.addProperty("chosen", replace ? newEvent : BASE_EVENT);
        out.addProperty("chosenDistance", replace ? (double) distance : (double) BASE_DISTANCE);
        return out;
    }
}
