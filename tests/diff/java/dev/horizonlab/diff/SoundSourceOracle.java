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
import net.minecraft.sounds.SoundSource;

/**
 * {@code SoundSource} 枚举的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.SoundSourceOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt;
 *       &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link SoundSource#getName()}。
 *
 * <p>{@code byName} 是**本能力定义的语义**（原版无此入口）：遍历 {@code values()}，
 * 首个 {@code getName().equals(name)} 的枚举项胜出，无匹配返回 {@code null}。
 * 探针按同一规则实现，故这是「约定」而非「对齐」，但仍能钉住 {@code getName()} 的值。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/sound_source.cpp 逐项一致。
 */
public final class SoundSourceOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "name|MASTER",
                    "name|MUSIC",
                    "name|RECORDS",
                    "name|WEATHER",
                    "name|BLOCKS",
                    "name|HOSTILE",
                    "name|NEUTRAL",
                    "name|PLAYERS",
                    "name|AMBIENT",
                    "name|VOICE",
                    "name|UI",
                    "byName|record",
                    "byName|block",
                    "byName|player",
                    "byName|master",
                    "byName|ui",
                    "byName|records",
                    "byName|blocks",
                    "byName|players",
                    "byName|MASTER",
                    "byName|",
                    "count",
                    "names");

    private SoundSourceOracle() {}

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
                "usage: SoundSourceOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (parts.length == 1) {
            switch (parts[0]) {
                case "count" -> out.addProperty("value",
                        String.valueOf(SoundSource.values().length));
                case "names" -> {
                    StringBuilder names = new StringBuilder();
                    for (SoundSource source : SoundSource.values()) {
                        if (names.length() > 0) {
                            names.append(',');
                        }
                        names.append(source.getName());
                    }
                    out.addProperty("value", names.toString());
                }
                default -> throw new IllegalArgumentException(
                        "sound_source: 无法识别的单元: " + unit);
            }
            return out;
        }
        if (parts.length != 2) {
            throw new IllegalArgumentException("sound_source: 单元段数不符: " + unit);
        }

        switch (parts[0]) {
            case "name" -> out.addProperty("value",
                    SoundSource.valueOf(parts[1].toUpperCase(Locale.ROOT)).getName());
            case "byName" -> {
                String found = null;
                for (SoundSource source : SoundSource.values()) {
                    if (source.getName().equals(parts[1])) {
                        found = source.name();
                        break;
                    }
                }
                out.addProperty("value", found == null ? "null" : found);
            }
            default -> throw new IllegalArgumentException(
                    "sound_source: 无法识别的单元: " + unit);
        }

        return out;
    }
}