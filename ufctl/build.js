/**
 * Build script for ufctl (Linux only)
 * 
 * Uses `pkg` to produce a standalone Linux binary.
 * Run: node build.js
 * 
 * Output: dist/ufctl-linux
 */

'use strict';

const { execSync } = require('child_process');
const path = require('path');
const fs = require('fs');

const distDir = path.join(__dirname, 'dist');
if (!fs.existsSync(distDir)) fs.mkdirSync(distDir);

const out = path.join(distDir, 'ufctl-linux');
const cmd = `npx @yao-pkg/pkg . --no-bytecode --public --public-packages "*" --compress GZip --targets node22-linux-x64 --output "${out}"`;

console.log(`Building ufctl for Linux...`);
console.log(`  ${cmd}`);
execSync(cmd, { stdio: 'inherit', cwd: __dirname });
console.log(`\n  → ${out}`);
console.log(`\nCopy to Build/Service/ for install.sh packaging:`);
console.log(`  cp dist/ufctl-linux ../Build/Service/`);
