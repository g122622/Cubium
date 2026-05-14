import fs from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const rootDir = path.resolve(__dirname, "..");
const scriptPath = path.resolve(__filename);
const licensePath = path.resolve(__dirname, "license_header.txt");
const gitignorePath = path.resolve(rootDir, ".gitignore");

const CODE_FILE_EXTENSIONS = new Set([
    ".c",
    ".cc",
    ".cpp",
    ".cxx",
    ".h",
    ".hh",
    ".hpp",
    ".hxx",
    ".ixx",
    ".cppm",
    ".mjs",
    ".cjs",
    ".js",
    ".ts",
    ".tsx",
    ".jsx",
    ".java",
    ".kt",
    ".kts",
    ".cs",
    ".rs",
    ".go",
]);

function normalizePathForMatch(targetPath) {
    const relativePath = path.relative(rootDir, targetPath);
    if (!relativePath || relativePath.startsWith("..")) {
        return "";
    }
    return relativePath.split(path.sep).join("/");
}

function createGitignoreMatcher(gitignoreContent) {
    const rawRules = gitignoreContent
        .split(/\r?\n/u)
        .map((line) => line.trim())
        .filter((line) => line.length > 0 && !line.startsWith("#"))
        .map((line) => {
            const isNegated = line.startsWith("!");
            const rule = isNegated ? line.slice(1) : line;
            return {
                isNegated,
                rule,
            };
        });

    return (targetPath, isDirectory) => {
        const relativePath = normalizePathForMatch(targetPath);
        if (!relativePath) {
            return false;
        }

        let ignored = false;
        for (const { isNegated, rule } of rawRules) {
            if (matchesGitignoreRule(rule, relativePath, isDirectory)) {
                ignored = !isNegated;
            }
        }
        return ignored;
    };
}

function matchesGitignoreRule(rule, relativePath, isDirectory) {
    if (!rule) {
        return false;
    }

    const normalizedRule = rule.replace(/\\/gu, "/");
    const directoryOnly = normalizedRule.endsWith("/");
    const effectiveRule = directoryOnly ? normalizedRule.slice(0, -1) : normalizedRule;
    const anchored = effectiveRule.startsWith("/");
    const relativeRule = anchored ? effectiveRule.slice(1) : effectiveRule;
    const normalizedPath = relativePath.replace(/\\/gu, "/");

    if (!relativeRule) {
        return false;
    }

    const regex = globToRegExp(relativeRule, anchored);
    if (!regex.test(normalizedPath)) {
        if (!directoryOnly) {
            return false;
        }

        const directoryPrefix = normalizedPath.startsWith(`${relativeRule}/`);
        return !anchored && directoryPrefix;
    }

    if (!directoryOnly) {
        return true;
    }

    return isDirectory || normalizedPath.startsWith(`${relativeRule}/`);
}

function globToRegExp(pattern, anchored) {
    let regex = anchored ? "^" : "(^|.*/)";
    for (let i = 0; i < pattern.length; i += 1) {
        const char = pattern[i];
        if (char === "*") {
            const nextChar = pattern[i + 1];
            if (nextChar === "*") {
                const afterNext = pattern[i + 2];
                if (afterNext === "/") {
                    regex += "(?:.*/)?";
                    i += 2;
                } else {
                    regex += ".*";
                    i += 1;
                }
            } else {
                regex += "[^/]*";
            }
            continue;
        }

        if (char === "?") {
            regex += "[^/]";
            continue;
        }

        regex += escapeRegExpCharacter(char);
    }

    regex += "$";
    return new RegExp(regex, "u");
}

function escapeRegExpCharacter(char) {
    switch (char) {
        case "\\":
        case "^":
        case "$":
        case ".":
        case "|":
        case "+":
        case "(":
        case ")":
        case "[":
        case "]":
        case "{":
        case "}":
            return `\\${char}`;
        default:
            return char;
    }
}

function isCodeFile(filePath) {
    const extension = path.extname(filePath);
    return CODE_FILE_EXTENSIONS.has(extension);
}

function hasUtf8Bom(content) {
    return content.charCodeAt(0) === 0xfeff;
}

function splitShebang(content) {
    if (!content.startsWith("#!")) {
        return {
            shebang: "",
            body: content,
        };
    }

    const lineEndIndex = content.indexOf("\n");
    if (lineEndIndex === -1) {
        return {
            shebang: content,
            body: "",
        };
    }

    return {
        shebang: content.slice(0, lineEndIndex + 1),
        body: content.slice(lineEndIndex + 1),
    };
}

async function collectFiles(currentDir, isIgnored, result) {
    const entries = await fs.readdir(currentDir, { withFileTypes: true });
    for (const entry of entries) {
        const fullPath = path.join(currentDir, entry.name);
        if (fullPath === scriptPath || fullPath === licensePath) {
            continue;
        }

        if (isIgnored(fullPath, entry.isDirectory())) {
            continue;
        }

        if (entry.isDirectory()) {
            await collectFiles(fullPath, isIgnored, result);
            continue;
        }

        if (entry.isFile() && isCodeFile(fullPath)) {
            result.push(fullPath);
        }
    }
}

async function prependLicenseToFile(filePath, licenseText) {
    const originalContent = await fs.readFile(filePath, "utf8");
    const hasBom = hasUtf8Bom(originalContent);
    const content = hasBom ? originalContent.slice(1) : originalContent;
    const { shebang, body } = splitShebang(content);

    if (body.startsWith(licenseText)) {
        return false;
    }

    const nextContent = `${shebang}${licenseText}${body}`;
    const finalContent = hasBom ? `\ufeff${nextContent}` : nextContent;
    await fs.writeFile(filePath, finalContent, "utf8");
    return true;
}

async function main() {
    const [licenseTextRaw, gitignoreContent] = await Promise.all([
        fs.readFile(licensePath, "utf8"),
        fs.readFile(gitignorePath, "utf8"),
    ]);

    const licenseText = licenseTextRaw.endsWith("\n") ? licenseTextRaw : `${licenseTextRaw}\n`;
    const isIgnored = createGitignoreMatcher(gitignoreContent);
    const candidateFiles = [];
    await collectFiles(rootDir, isIgnored, candidateFiles);
    candidateFiles.sort((left, right) => left.localeCompare(right));

    let updatedCount = 0;
    const totalCount = candidateFiles.length;
    for (let index = 0; index < candidateFiles.length; index += 1) {
        const filePath = candidateFiles[index];
        const updated = await prependLicenseToFile(filePath, licenseText);
        const progressText = `[${index + 1}/${totalCount}]`;
        if (updated) {
            updatedCount += 1;
            console.log(`${progressText} Added license: ${normalizePathForMatch(filePath)}`);
            continue;
        }

        console.log(`${progressText} Skipped: ${normalizePathForMatch(filePath)}`);
    }

    console.log(`Scanned ${candidateFiles.length} files, updated ${updatedCount} files.`);
}

main().catch((error) => {
    console.error(error);
    process.exitCode = 1;
});
