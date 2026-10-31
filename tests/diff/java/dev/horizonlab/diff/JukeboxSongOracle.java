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
import java.util.Locale;
import net.minecraft.core.registries.Registries;
import net.minecraft.resources.Identifier;
import net.minecraft.world.item.JukeboxSong;

/**
 * 唱片机歌曲的原版 oracle。
 *
 * <pre>
 *   java dev.horizonlab.diff.JukeboxSongOracle list &lt;units.txt&gt; | run &lt;input.jsonl&gt; &lt;output.jsonl&gt;
 * </pre>
 *
 * <p>与 probes/jukebox_song.cpp 同构：比较声音事件、描述翻译键、时长、比较器输出。
 * 歌曲来自数据包 {@code data/minecraft/jukebox_song/*.json}，必须走
 * {@link DataPackRegistries} 加载之后才非空。
 */
public final class JukeboxSongOracle {
    private static final Gson GSON = new GsonBuilder().disableHtmlEscaping().create();

    private JukeboxSongOracle() {}

    public static void main(String[] args) throws IOException {
        if (args.length == 2 && args[0].equals("list")) {
            var registries = DataPackRegistries.load();
            var songs = registries.lookup.lookupOrThrow(Registries.JUKEBOX_SONG);
            List<String> names = new ArrayList<>();
            songs.listElements().forEach(holder -> {
                names.add(holder.key().identifier().toString());
            });
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
        System.err.println("usage: JukeboxSongOracle list <units.txt> | run <input.jsonl> <output.jsonl>");
        System.exit(2);
    }

    private static void run(Path input, Path output) throws IOException {
        var registries = DataPackRegistries.load();
        var songs = registries.lookup.lookupOrThrow(Registries.JUKEBOX_SONG);

        StringBuilder text = new StringBuilder();
        for (String line : Files.readAllLines(input, StandardCharsets.UTF_8)) {
            if (line.isBlank()) {
                continue;
            }
            String name = JsonParser.parseString(line).getAsJsonObject().get("song").getAsString();
            text.append(GSON.toJson(summarise(songs, name))).append('\n');
        }
        Files.writeString(output, text, StandardCharsets.UTF_8);
    }

/**
     * 6 位有效数字后去掉尾随零。
     *
     * <p>C++ 的 `%.6g` 把 178.0 打成 "178"，Java 的 `%.6g` 却打成 "178.000"。
     * 两个语言里 `%g` 的尾零处理不同，必须统一成「6 位有效数字再去掉尾随零」，
     * 即 Java `Float.toString` 的口径。
     */
    private static String formatSeconds(float value) {
        String text = String.format(Locale.ROOT, "%.6g", value);
        if (text.indexOf('e') >= 0 || text.indexOf('E') >= 0 || text.indexOf('.') < 0) {
            return text;
        }
        int end = text.length();
        while (end > 0 && text.charAt(end - 1) == '0') {
            end--;
        }
        if (end > 0 && text.charAt(end - 1) == '.') {
            end--;
        }
        return text.substring(0, end);
    }

    private static JsonObject summarise(
            net.minecraft.core.HolderLookup.RegistryLookup<JukeboxSong> registry, String name) {
        JsonObject result = new JsonObject();
        result.addProperty("song", name);

        var key = net.minecraft.resources.ResourceKey.create(Registries.JUKEBOX_SONG, Identifier.parse(name));
        var holder = registry.get(key).orElse(null);
        if (holder == null) {
            result.addProperty("missing", true);
            return result;
        }

        JukeboxSong song = holder.value();
        result.addProperty("soundEvent", song.soundEvent().value().location().toString());
        // 取**原始翻译键**而不是 `getString()`：后者会把 Component 解析成当前语言下的
// 显示文本（"C418 - 11"），而 Cubium 侧存的是 `jukebox_song.minecraft.11` 这个键。
// 两者不是同一个量——本能力比的是数据本身，不是渲染结果。
        var description = song.description();
        String descriptionKey = description.getContents() instanceof net.minecraft.network.chat.contents.TranslatableContents translatable
                ? translatable.getKey()
                : description.getString();
        result.addProperty("descriptionKey", descriptionKey);
        // 按 6 位有效数字输出成字符串，与 C++ 侧 `%.6g` 的口径对齐。
        result.addProperty("lengthInSeconds", formatSeconds(song.lengthInSeconds()));
        result.addProperty("comparatorOutput", song.comparatorOutput());
        return result;
    }
}