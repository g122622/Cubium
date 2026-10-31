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
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import net.minecraft.core.registries.Registries;
import net.minecraft.network.chat.Component;
import net.minecraft.network.chat.contents.TranslatableContents;
import net.minecraft.resources.Identifier;
import net.minecraft.resources.ResourceKey;
import net.minecraft.util.Mth;
import net.minecraft.world.item.Instrument;

/**
 * 乐器的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.InstrumentOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/instrument.cpp 同构：比较音效 ID、吹奏时长、可听距离、描述翻译键，
 * 以及两个派生值——`useDurationTicks`（`Mth.floor(useDuration * 20.0F)`）与
 * `volume`（`range / 16.0F`）。
 *
 * <p>浮点数按「6 位有效数字再去掉尾随零」格式化成字符串输出，避免 C++ 与 Java 的
 * 浮点打印差异。
 */
public final class InstrumentOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private InstrumentOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            var registries = DataPackRegistries.load();
            List<String> names = new ArrayList<>();
            registries.lookup
                    .lookupOrThrow(Registries.INSTRUMENT)
                    .listElements()
                    .forEach(holder -> names.add(holder.key().identifier().toString()));
            Collections.sort(names);
            StringBuilder text = new StringBuilder();
            for (String name : names) {
                text.append(name).append('\n');
            }
            Files.writeString(Path.of(args[1]), text, StandardCharsets.UTF_8);
            return;
        }
        if (args.length == 3 && args[0].equals("run")) {
            run(Path.of(args[1]), Path.of(args[2]));
            return;
        }
        System.err.println("usage: InstrumentOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        var registries = DataPackRegistries.load();
        var instruments = registries.lookup.lookupOrThrow(Registries.INSTRUMENT);

        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("instrument").getAsString();
            text.append(GSON.toJson(summarise(instruments, name))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

    private static JsonObject summarise(
            net.minecraft.core.HolderLookup.RegistryLookup<Instrument> registry, String name) {
        JsonObject result = new JsonObject();
        result.addProperty("instrument", name);

        var holder =
                registry.get(ResourceKey.create(Registries.INSTRUMENT, Identifier.parse(name))).orElse(null);
        if (holder == null) {
            result.addProperty("missing", true);
            return result;
        }

        Instrument instrument = holder.value();
        result.addProperty("soundEvent", instrument.soundEvent().value().location().toString());
        result.addProperty("useDuration", formatFloat(instrument.useDuration()));
        result.addProperty("range", formatFloat(instrument.range()));
        result.addProperty("descriptionKey", translateKey(instrument.description()));

        // 两个派生值各自复刻了 InstrumentItem 里的一行换算：
        //   getUseDuration -> Mth.floor(useDuration * 20.0F)
        //   play           -> range / 16.0F
        // 换算口径错了这两个字段会立刻暴露。
        result.addProperty("useDurationTicks", Mth.floor(instrument.useDuration() * 20.0F));
        result.addProperty("volume", formatFloat(instrument.range() / 16.0F));
        return result;
    }

    /** 提取文本组件的翻译键；非可翻译组件返回渲染后的文本。 */
    private static String translateKey(Component component) {
        if (component.getContents() instanceof TranslatableContents translatable) {
            return translatable.getKey();
        }
        return component.getString();
    }

    /**
     * 把浮点数格式化成「6 位有效数字，去掉尾随零」的字符串。
     *
     * <p>必须与 C++ 侧用完全相同的规则：C++ 的 {@code %.6g} 与 Java 的 {@code %g}
     * 行为不同（{@code 178.0f} 一边给 {@code 178}、另一边给 {@code 178.000}）。
     */
    private static String formatFloat(float value) {
        String text = String.format("%.6g", (double) value);
        if (text.indexOf('.') >= 0 && text.indexOf('e') < 0 && text.indexOf('E') < 0) {
            int end = text.length();
            while (end > 0 && text.charAt(end - 1) == '0') {
                end--;
            }
            if (end > 0 && text.charAt(end - 1) == '.') {
                end--;
            }
            text = text.substring(0, end);
        }
        return text;
    }
}