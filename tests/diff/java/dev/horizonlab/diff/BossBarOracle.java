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
import java.util.Locale;
import net.minecraft.world.BossEvent;

/**
 * {@code BossEvent.BossBarColor} / {@code BossBarOverlay} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.BossBarOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link BossEvent.BossBarColor#getName()} /
 * {@link BossEvent.BossBarColor#getFormatting()} /
 * {@link BossEvent.BossBarOverlay#getName()}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/boss_bar.cpp 逐项一致。
 */
public final class BossBarOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "colorName|PINK",
                    "colorName|BLUE",
                    "colorName|RED",
                    "colorName|GREEN",
                    "colorName|YELLOW",
                    "colorName|PURPLE",
                    "colorName|WHITE",
                    "colorSerial|PINK",
                    "colorSerial|RED",
                    "colorFormatting|PINK",
                    "colorFormatting|RED",
                    "colorFormatting|PURPLE",
                    "colorFormatting|BLUE",
                    "colorFormatting|GREEN",
                    "colorFormatting|YELLOW",
                    "colorFormatting|WHITE",
                    "overlayName|PROGRESS",
                    "overlayName|NOTCHED_6",
                    "overlayName|NOTCHED_10",
                    "overlayName|NOTCHED_12",
                    "overlayName|NOTCHED_20",
                    "overlaySerial|NOTCHED_6",
                    "overlaySerial|NOTCHED_20",
                    "colorCount",
                    "overlayCount");

    private BossBarOracle() {}

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
                "usage: BossBarOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static BossEvent.BossBarColor colorByName(String name) {
        return BossEvent.BossBarColor.valueOf(name.toUpperCase(Locale.ROOT));
    }

    private static BossEvent.BossBarOverlay overlayByName(String name) {
        return BossEvent.BossBarOverlay.valueOf(name.toUpperCase(Locale.ROOT));
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (parts.length == 1) {
            switch (parts[0]) {
                case "colorCount" -> out.addProperty("value",
                        String.valueOf(BossEvent.BossBarColor.values().length));
                case "overlayCount" -> out.addProperty("value",
                        String.valueOf(BossEvent.BossBarOverlay.values().length));
                default -> throw new IllegalArgumentException("boss_bar: 无法识别的单元: " + unit);
            }
            return out;
        }
        if (parts.length != 2) {
            throw new IllegalArgumentException("boss_bar: 单元段数不符: " + unit);
        }

        switch (parts[0]) {
            case "colorName" -> out.addProperty("value", colorByName(parts[1]).getName());
            case "colorSerial" -> out.addProperty("value",
                    colorByName(parts[1]).getSerializedName());
            case "colorFormatting" -> out.addProperty("value",
                    colorByName(parts[1]).getFormatting().getName());
            case "overlayName" -> out.addProperty("value", overlayByName(parts[1]).getName());
            case "overlaySerial" -> out.addProperty("value",
                    overlayByName(parts[1]).getSerializedName());
            default -> throw new IllegalArgumentException("boss_bar: 无法识别的单元: " + unit);
        }

        return out;
    }
}