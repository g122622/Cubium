const { execSync } = require('child_process');

// Get all files tracked by git
const allFiles = execSync('git ls-files', { encoding: 'utf8' }).trim().split('\n');

// Build a map: lowercase path -> actual git paths
const lowerMap = {};
for (const f of allFiles) {
  const lower = f.toLowerCase();
  if (!lowerMap[lower]) lowerMap[lower] = [];
  lowerMap[lower].push(f);
}

// For each source file, find all #include references and check case
const sourceFiles = allFiles.filter(f => /\.(hpp|cpp|h|c)$/i.test(f));
const issues = [];

for (const file of sourceFiles) {
  let content;
  try {
    content = execSync('git show HEAD:' + file, { encoding: 'utf8' });
  } catch(e) { continue; }

  const lines = content.split('\n');
  const dir = file.substring(0, file.lastIndexOf('/'));

  for (let i = 0; i < lines.length; i++) {
    const line = lines[i];
    // Match #include "..." (quoted includes, relative paths)
    const match = line.match(/^\s*#\s*include\s+"([^"]+)"/);
    if (!match) continue;

    const includePath = match[1];
    // Resolve relative path
    const parts = (dir + '/' + includePath).split('/');
    const resolved2 = [];
    for (const p of parts) {
      if (p === '..') resolved2.pop();
      else if (p !== '.') resolved2.push(p);
    }
    const finalPath = resolved2.join('/');

    // Check if this exact path exists in git
    if (!allFiles.includes(finalPath)) {
      // Check if a case-insensitive match exists
      const lower = finalPath.toLowerCase();
      if (lowerMap[lower] && lowerMap[lower].length > 0) {
        const actualPath = lowerMap[lower][0];
        if (actualPath !== finalPath) {
          issues.push({
            file: file,
            line: i + 1,
            include: includePath,
            resolved: finalPath,
            actual: actualPath
          });
        }
      }
    }
  }
}

if (issues.length === 0) {
  console.log('No include case mismatches found.');
} else {
  for (const issue of issues) {
    console.log(issue.file + ':' + issue.line);
    console.log('  #include "' + issue.include + '"');
    console.log('  Resolved: ' + issue.resolved);
    console.log('  Actual:   ' + issue.actual);
  }
  console.log('\nTotal: ' + issues.length + ' case mismatch(es)');
}
