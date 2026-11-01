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
import com.mojang.serialization.DataResult;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Locale;
import net.minecraft.network.chat.ClickEvent;

/**
 * {@code ClickEvent.Action} 序列化过滤与 {@code ChangePage} 页码校验的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.ClickEventOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>直接调用原版 {@link ClickEvent.Action#getSerializedName()} /
 * {@link ClickEvent.Action#isAllowedFromServer()} /
 * {@link ClickEvent.Action#filterForSerialization(ClickEvent.Action)}。
 *
 * <p>本能力不需要注册表。
 *
 * <p>单元表必须与 probes/click_event.cpp 逐项一致。
 */
public final class ClickEventOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    /** `ExtraCodecs.POSITIVE_INT` 的下界。 */
    private static final int POSITIVE_INT_MIN = 1;

    /** 单元表。 */
    private static final List<String> UNITS =
            List.of(
                    "serial|OPEN_URL",
                    "serial|OPEN_FILE",
                    "serial|RUN_COMMAND",
                    "serial|SUGGEST_COMMAND",
                    "serial|SHOW_DIALOG",
                    "serial|CHANGE_PAGE",
                    "serial|COPY_TO_CLIPBOARD",
                    "serial|CUSTOM",
                    "allowed|OPEN_URL",
                    "allowed|OPEN_FILE",
                    "allowed|RUN_COMMAND",
                    "allowed|CUSTOM",
                    "filter|OPEN_URL",
                    "filter|OPEN_FILE",
                    "filter|RUN_COMMAND",
                    "filter|CHANGE_PAGE",
                    "names",
                    "page|0",
                    "page|1",
                    "page|2",
                    "page|-1",
                    "page|2147483647",
                    "page|-2147483648");

    private ClickEventOracle() {}

    public static void main(String[] args) throws IOException {
        // ClickEvent.Action 的静态初始化会经 codec 触及注册表，
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
                "usage: ClickEventOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
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

    private static ClickEvent.Action actionByName(String name) {
        return ClickEvent.Action.valueOf(name.toUpperCase(Locale.ROOT));
    }

    private static JsonObject summarise(String unit) {
        String[] parts = unit.split("\\|", -1);

        JsonObject out = new JsonObject();
        out.addProperty("unit", unit);

        if (parts.length == 1 && parts[0].equals("names")) {
            StringBuilder names = new StringBuilder();
            for (ClickEvent.Action action : ClickEvent.Action.values()) {
                if (names.length() > 0) {
                    names.append(',');
                }
                names.append(action.getSerializedName());
            }
            out.addProperty("value", names.toString());
            return out;
        }
        if (parts.length != 2) {
            throw new IllegalArgumentException("click_event: 单元段数不符: " + unit);
        }

        switch (parts[0]) {
            case "serial" -> out.addProperty("value",
                    actionByName(parts[1]).getSerializedName());
            case "allowed" -> out.addProperty("value",
                    actionByName(parts[1]).isAllowedFromServer());
            case "filter" -> {
                DataResult<ClickEvent.Action> result =
                        ClickEvent.Action.filterForSerialization(actionByName(parts[1]));
                out.addProperty("value", result.result().isPresent());
            }
            case "page" -> {
                // 原版用 ExtraCodecs.POSITIVE_INT：范围 [1, Integer.MAX_VALUE]。
                // 超出 int 范围的输入在 parseInt 阶段就失败。
                try {
                    int page = Integer.parseInt(parts[1]);
                    out.addProperty("value", page >= POSITIVE_INT_MIN);
                } catch (NumberFormatException exception) {
                    out.addProperty("value", false);
                }
            }
            default -> throw new IllegalArgumentException(
                    "click_event: 无法识别的单元: " + unit);
        }

        return out;
    }
}