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
import net.minecraft.world.level.GameType;

/**
 * {@code GameType} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.GameTypeOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt;
 *       &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link GameType#getId()} / {@link GameType#getName()} /
 * {@link GameType#getShortDisplayName()} / {@link GameType#getLongDisplayName()} /
 * {@link GameType#byId(int)} / {@link GameType#isCreative()} / {@link GameType#isSurvival()} /
 * {@link GameType#isBlockPlacingRestricted()}。
 *
 * <p><b>本能力的探针对接 Cubium 的真实实现</b>（{@code GameMode} 枚举与名字转换）。
 *
 * <p>{@code DEFAULT_MODE} 是 {@code SURVIVAL} 的别名，不在 {@code values()} 中，
 * 因此本能力只覆盖 4 个真实枚举项。
 *
 * <p>本能力需要 bootstrap——{@code Component.translatable} 的静态初始化涉及
 * {@code TranslatableContents}。
 *
 * <p>单元表必须与 probes/game_type.cpp 逐项一致。
 */
public final class GameTypeOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "count",
                    "names",
                    "id|SURVIVAL",
                    "id|CREATIVE",
                    "id|ADVENTURE",
                    "id|SPECTATOR",
                    "name|SURVIVAL",
                    "name|CREATIVE",
                    "name|ADVENTURE",
                    "name|SPECTATOR",
                    "shortName|SURVIVAL",
                    "shortName|CREATIVE",
                    "shortName|SPECTATOR",
                    "longName|SURVIVAL",
                    "longName|CREATIVE",
                    "longName|SPECTATOR",
                    "byId|0",
                    "byId|1",
                    "byId|2",
                    "byId|3",
                    "byId|4",
                    "byId|-1",
                    "byId|100",
                    "flags|SURVIVAL",
                    "flags|CREATIVE",
                    "flags|ADVENTURE",
                    "flags|SPECTATOR");

    private GameTypeOracle() {}

    public static void main(String[] args) throws IOException {
        net.minecraft.SharedConstants.tryDetectVersion();
        net.minecraft.server.Bootstrap.bootStrap();
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
                "usage: GameTypeOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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
        throw new IllegalStateException("game_type: 期望 TranslatableContents，实际是 "
                + component.getContents().getClass().getName());
    }

    private static GameType gameTypeByName(String name) {
        return GameType.valueOf(name.toUpperCase(Locale.ROOT));
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (parts.length == 1) {
            switch (parts[0]) {
                case "count" -> out.addProperty("value",
                        String.valueOf(GameType.values().length));
                case "names" -> {
                    StringBuilder names = new StringBuilder();
                    for (GameType type : GameType.values()) {
                        if (names.length() > 0) {
                            names.append(',');
                        }
                        names.append(type.getName());
                    }
                    out.addProperty("value", names.toString());
                }
                default -> throw new IllegalArgumentException(
                        "game_type: 无法识别的单元: " + unit);
            }
            return out;
        }
        if (parts.length != 2) {
            throw new IllegalArgumentException("game_type: 单元段数不符: " + unit);
        }

        switch (parts[0]) {
            case "id" -> out.addProperty("value",
                    String.valueOf(gameTypeByName(parts[1]).getId()));
            case "name" -> out.addProperty("value", gameTypeByName(parts[1]).getName());
            case "shortName" -> out.addProperty("value",
                    translationKey(gameTypeByName(parts[1]).getShortDisplayName()));
            case "longName" -> out.addProperty("value",
                    translationKey(gameTypeByName(parts[1]).getLongDisplayName()));
            case "byId" -> out.addProperty("value",
                    GameType.byId(Integer.parseInt(parts[1])).name());
            case "flags" -> {
                GameType type = gameTypeByName(parts[1]);
                String flags = (type.isCreative() ? "1" : "0")
                        + (type.isSurvival() ? "1" : "0")
                        + (type.isBlockPlacingRestricted() ? "1" : "0");
                out.addProperty("value", flags);
            }
            default -> throw new IllegalArgumentException(
                    "game_type: 无法识别的单元: " + unit);
        }

        return out;
    }
}