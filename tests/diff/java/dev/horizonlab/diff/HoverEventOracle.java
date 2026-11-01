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
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.mojang.serialization.DataResult;
import com.mojang.serialization.JsonOps;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Locale;
import net.minecraft.network.chat.HoverEvent;

/**
 * {@code HoverEvent.Action} 序列化与 {@code toString} 的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.HoverEventOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link HoverEvent.Action#getSerializedName()} /
 * {@link HoverEvent.Action#isAllowedFromServer()} /
 * {@link HoverEvent.Action#filterForSerialization(HoverEvent.Action)}。
 *
 * <p>{@code filterForSerialization} 是私有方法，本 oracle 走公开的
 * {@link HoverEvent.Action#CODEC} 的 {@code encodeStart} 观察是否成功，语义等价。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/hover_event.cpp 逐项一致。
 */
public final class HoverEventOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "serial|SHOW_TEXT",
                    "serial|SHOW_ITEM",
                    "serial|SHOW_ENTITY",
                    "allowed|SHOW_TEXT",
                    "allowed|SHOW_ITEM",
                    "allowed|SHOW_ENTITY",
                    "filter|SHOW_TEXT",
                    "filter|SHOW_ITEM",
                    "filter|SHOW_ENTITY",
                    "tostring|SHOW_TEXT",
                    "tostring|SHOW_ITEM",
                    "tostring|SHOW_ENTITY",
                    "names",
                    "count");

    private HoverEventOracle() {}

    public static void main(String[] args) throws IOException {
        // HoverEvent.Action 的静态初始化会经 codec 触及注册表，
        // 而注册表在 Bootstrap.bootStrap() 之前一碰就抛 "Not bootstrapped"；
        // bootStrap 本身又要求先设置游戏版本。
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
                "usage: HoverEventOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static HoverEvent.Action actionByName(String name) {
        return HoverEvent.Action.valueOf(name.toUpperCase(Locale.ROOT));
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (parts.length == 1 && parts[0].equals("names")) {
            StringBuilder names = new StringBuilder();
            for (HoverEvent.Action action : HoverEvent.Action.values()) {
                if (names.length() > 0) {
                    names.append(',');
                }
                names.append(action.getSerializedName());
            }
            out.addProperty("value", names.toString());
            return out;
        }
        if (parts.length == 1 && parts[0].equals("count")) {
            out.addProperty("value", HoverEvent.Action.values().length);
            return out;
        }
        if (parts.length != 2) {
            throw new IllegalArgumentException("hover_event: 单元段数不符: " + unit);
        }

        HoverEvent.Action action = actionByName(parts[1]);
        switch (parts[0]) {
            case "serial" -> out.addProperty("value", action.getSerializedName());
            case "allowed" -> out.addProperty("value", action.isAllowedFromServer());
            case "filter" -> {
                // CODEC = UNSAFE_CODEC.validate(filterForSerialization)，故 encodeStart 的成功
                // 等价于 filterForSerialization 成功。
                DataResult<JsonElement> encoded = HoverEvent.Action.CODEC.encodeStart(
                        JsonOps.INSTANCE, action);
                out.addProperty("value", encoded.result().isPresent());
            }
            case "tostring" -> out.addProperty("value", action.toString());
            default -> throw new IllegalArgumentException(
                    "hover_event: 无法识别的单元: " + unit);
        }

        return out;
    }
}