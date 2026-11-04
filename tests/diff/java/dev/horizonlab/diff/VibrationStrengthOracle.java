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
import net.minecraft.util.Mth;

/**
 * 振动距离到红石强度换算的原版 oracle
 * （{@code VibrationSystem#getRedstoneStrengthForDistance}）。
 *
 * <pre>
 *   java dev.horizonlab.diff.VibrationStrengthOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>单元名形如 {@code 8:4.0}（监听半径 : 距离）。
 *
 * <p>原版公式：
 *
 * <pre>
 *   double d0 = 15.0 / radius;
 *   return Math.max(1, 15 - Mth.floor(d0 * distance));
 * </pre>
 *
 * <p>这是幽匿感测体给红石比较器的唯一输出值，玩家整个振动装置都建立在这个数上。
 *
 * <p>三个容易写错的地方：除号内外（整数截断两次会差 1）、漏掉 max(1,...) 下限、
 * 距离为 0 时必须给 15。三个错都不崩不报错，只是读数差 1。
 */
public final class VibrationStrengthOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private static final int[] RADII = {1, 2, 5, 8, 9, 15, 16, 32};

    private static final float[] DISTANCES = {
        0.0F, 0.5F, 1.0F, 2.0F, 3.5F, 4.0F, 5.0F, 7.5F, 8.0F, 9.0F, 12.0F, 15.0F, 16.0F, 20.0F, 32.0F,
    };

    private VibrationStrengthOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();

        if (args.length == 2 && args[0].equals("list")) {
            List<String> units = new ArrayList<>();
            for (int radius : RADII) {
                for (float distance : DISTANCES) {
                    units.add(radius + ":" + String.format("%.1f", distance));
                }
            }
            Files.write(Path.of(args[1]), units, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: VibrationStrengthOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

        int sep = unit.indexOf(':');
        if (sep < 0) {
            out.addProperty("missing", true);
            return out;
        }
        int radius;
        float distance;
        try {
            radius = Integer.parseInt(unit.substring(0, sep));
            distance = Float.parseFloat(unit.substring(sep + 1));
        } catch (NumberFormatException e) {
            out.addProperty("missing", true);
            return out;
        }
        if (radius <= 0 || distance < 0.0F) {
            out.addProperty("missing", true);
            return out;
        }

        // 原版 getRedstoneStrengthForDistance 逐字对应
        double d0 = 15.0 / radius;
        out.addProperty("strength", Math.max(1, 15 - Mth.floor(d0 * distance)));
        return out;
    }
}
