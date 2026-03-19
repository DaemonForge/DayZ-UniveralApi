/**
 * Postinstall script to patch packages that use "exports" without "main".
 * pkg (@yao-pkg/pkg) only reads the "main" field, so we need to add it.
 */
const fs = require('fs');
const path = require('path');

const patches = [
  {
    package: 'gamedig',
    main: './dist/index.cjs'
  }
];

for (const patch of patches) {
  const pkgPath = path.join(__dirname, 'node_modules', patch.package, 'package.json');
  if (!fs.existsSync(pkgPath)) continue;

  const pkg = JSON.parse(fs.readFileSync(pkgPath, 'utf-8'));
  if (!pkg.main) {
    pkg.main = patch.main;
    fs.writeFileSync(pkgPath, JSON.stringify(pkg, null, 4) + '\n');
    console.log(`postinstall: patched ${patch.package}/package.json with main: "${patch.main}"`);
  }
}
