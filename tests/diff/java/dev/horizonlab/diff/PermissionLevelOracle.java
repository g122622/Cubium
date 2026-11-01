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
import net.minecraft.server.permissions.PermissionLevel;

/**
 * {@code PermissionLevel} 枚举的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.PermissionLevelOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt;
 *       &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link PermissionLevel#getSerializedName()} / {@link PermissionLevel#id()} /
 * {@link PermissionLevel#byId(int)} / {@link PermissionLevel#isEqualOrHigherThan(PermissionLevel)}。
 *
 * <p>{@code byId} 走 {@code BY_ID} 的 {@code OutOfBoundsStrategy.CLAMP}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/permission_level.cpp 逐项一致。
 */
public final class PermissionLevelOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "serial|ALL",
                    "serial|MODERATORS",
                    "serial|GAMEMASTERS",
                    "serial|ADMINS",
                    "serial|OWNERS",
                    "id|ALL",
                    "id|MODERATORS",
                    "id|GAMEMASTERS",
                    "id|ADMINS",
                    "id|OWNERS",
                    "byId|0",
                    "byId|1",
                    "byId|2",
                    "byId|3",
                    "byId|4",
                    "byId|-1",
                    "byId|-100",
                    "byId|5",
                    "byId|100",
                    "higher|OWNERS|ALL",
                    "higher|ALL|OWNERS",
                    "higher|ALL|ALL",
                    "higher|ADMINS|GAMEMASTERS",
                    "higher|GAMEMASTERS|ADMINS",
                    "count",
                    "names");

    private PermissionLevelOracle() {}

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
        System.err.println("usage: PermissionLevelOracle list <units.txt> "
                + "| run <input.jsonl> <output.jsonl>");
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

    private static PermissionLevel levelByName(String name) {
        return PermissionLevel.valueOf(name.toUpperCase(Locale.ROOT));
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (parts.length == 1) {
            switch (parts[0]) {
                case "count" -> out.addProperty("value",
                        String.valueOf(PermissionLevel.values().length));
                case "names" -> {
                    StringBuilder names = new StringBuilder();
                    for (PermissionLevel level : PermissionLevel.values()) {
                        if (names.length() > 0) {
                            names.append(',');
                        }
                        names.append(level.getSerializedName());
                    }
                    out.addProperty("value", names.toString());
                }
                default -> throw new IllegalArgumentException(
                        "permission_level: 无法识别的单元: " + unit);
            }
            return out;
        }

        switch (parts[0]) {
            case "serial" -> out.addProperty("value",
                    levelByName(parts[1]).getSerializedName());
            case "id" -> out.addProperty("value",
                    String.valueOf(levelByName(parts[1]).id()));
            case "byId" -> out.addProperty("value",
                    PermissionLevel.byId(Integer.parseInt(parts[1])).name());
            case "higher" -> {
                if (parts.length != 3) {
                    throw new IllegalArgumentException(
                            "permission_level: higher 需要 3 段: " + unit);
                }
                out.addProperty("value", String.valueOf(
                        levelByName(parts[1]).isEqualOrHigherThan(levelByName(parts[2]))));
            }
            default -> throw new IllegalArgumentException(
                    "permission_level: 无法识别的单元: " + unit);
        }

        return out;
    }
}