import fs from 'fs/promises';
import path from 'path';

// DFS
export async function forEachFile(rootDir: string, callback: (filePath: string) => void | Promise<void>): Promise<void> {
    // 每遍历到一个文件，就call一次callback
    const entries = await fs.readdir(rootDir, { withFileTypes: true });
    for (const entry of entries) {
        const fullPath = path.join(rootDir, entry.name);
        if (entry.isDirectory()) {
            await forEachFile(fullPath, callback);
        } else if (entry.isFile()) {
            await callback(fullPath);
        }
    }
}
