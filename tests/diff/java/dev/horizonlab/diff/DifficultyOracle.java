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
import net.minecraft.network.chat.Component;
import net.minecraft.network.chat.contents.TranslatableContents;
import net.minecraft.world.Difficulty;

/**
 * {@code Difficulty} 枚举的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.DifficultyOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt;
 *       &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link Difficulty#getSerializedName()} / {@link Difficulty#getId()} /
 * {@link Difficulty#getDisplayName()} / {@link Difficulty#getInfo()} /
 * {@link Difficulty#byId(int)} / {@link Difficulty#byName(String)}。
 *
 * <p>{@code byId} 标注了 {@code @Deprecated}，但行为仍有效（内部走 {@code BY_ID} 的
 * {@code OutOfBoundsStrategy.WRAP}）。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/difficulty.cpp 逐项一致。
 */
public final class DifficultyOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "serial|PEACEFUL",
                    "serial|EASY",
                    "serial|NORMAL",
                    "serial|HARD",
                    "key|PEACEFUL",
                    "key|HARD",
                    "id|PEACEFUL",
                    "id|EASY",
                    "id|NORMAL",
                    "id|HARD",
                    "displayName|PEACEFUL",
                    "displayName|HARD",
                    "info|PEACEFUL",
                    "info|EASY",
                    "info|HARD",
                    "byId|0",
                    "byId|1",
                    "byId|2",
                    "byId|3",
                    "byId|4",
                    "byId|5",
                    "byId|-1",
                    "byId|-4",
                    "byId|1000",
                    "byName|peaceful",
                    "byName|hard",
                    "byName|PEACEFUL",
                    "byName|",
                    "count");

    private DifficultyOracle() {}

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
                "usage: DifficultyOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    /** 从 {@code Component} 取出翻译键。 */
    private static String translationKey(Component component) {
        if (component.getContents() instanceof TranslatableContents translatable) {
            return translatable.getKey();
        }
        throw new IllegalStateException("difficulty: 期望 TranslatableContents，实际是 "
                + component.getContents().getClass().getName());
    }

    private static Difficulty byEnumName(String name) {
        return Difficulty.valueOf(name.toUpperCase(Locale.ROOT));
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (parts.length == 1 && parts[0].equals("count")) {
            out.addProperty("value", String.valueOf(Difficulty.values().length));
            return out;
        }
        if (parts.length != 2) {
            throw new IllegalArgumentException("difficulty: 单元段数不符: " + unit);
        }

        switch (parts[0]) {
            case "serial" -> out.addProperty("value",
                    byEnumName(parts[1]).getSerializedName());
            case "key" -> out.addProperty("value", byEnumName(parts[1]).getKey());
            case "id" -> out.addProperty("value",
                    String.valueOf(byEnumName(parts[1]).getId()));
            case "displayName" -> out.addProperty("value",
                    translationKey(byEnumName(parts[1]).getDisplayName()));
            case "info" -> out.addProperty("value",
                    translationKey(byEnumName(parts[1]).getInfo()));
            case "byId" -> out.addProperty("value",
                    Difficulty.byId(Integer.parseInt(parts[1])).name());
            case "byName" -> {
                Difficulty difficulty = Difficulty.byName(parts[1]);
                out.addProperty("value", difficulty == null ? "null" : difficulty.name());
            }
            default -> throw new IllegalArgumentException(
                    "difficulty: 无法识别的单元: " + unit);
        }

        return out;
    }
}