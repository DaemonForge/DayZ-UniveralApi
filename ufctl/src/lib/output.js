/**
 * ufctl output helpers
 * Shared formatting / colour utilities for the CLI.
 * These are intentionally decoupled from the backend so a future web UI
 * can swap in a different presentation layer while reusing the same backends.
 */

'use strict';

// Colour helpers (respects NO_COLOR / dumb terminals)
const supportsColour = process.env.NO_COLOR === undefined
    && process.env.TERM !== 'dumb'
    && process.stdout.isTTY;

const c = {
    red:    s => supportsColour ? `\x1b[31m${s}\x1b[0m` : s,
    green:  s => supportsColour ? `\x1b[32m${s}\x1b[0m` : s,
    yellow: s => supportsColour ? `\x1b[33m${s}\x1b[0m` : s,
    blue:   s => supportsColour ? `\x1b[34m${s}\x1b[0m` : s,
    cyan:   s => supportsColour ? `\x1b[36m${s}\x1b[0m` : s,
    dim:    s => supportsColour ? `\x1b[2m${s}\x1b[0m`  : s,
    bold:   s => supportsColour ? `\x1b[1m${s}\x1b[0m`  : s,
};

function ok(msg)   { console.log(c.green('✓ ') + msg); }
function info(msg) { console.log(c.cyan('ℹ ') + msg); }
function warn(msg) { console.error(c.yellow('⚠ ') + msg); }
function die(msg)  { console.error(c.red('✗ ') + msg); process.exit(1); }

/**
 * Print a simple table.  rows is an array of arrays (columns).
 * First row is treated as the header.
 */
function table(rows) {
    if (!rows || rows.length === 0) return;

    const widths = [];
    for (const row of rows) {
        for (let i = 0; i < row.length; i++) {
            const len = String(row[i] ?? '').length;
            widths[i] = Math.max(widths[i] || 0, len);
        }
    }

    const sep = widths.map(w => '─'.repeat(w + 2)).join('┼');

    for (let r = 0; r < rows.length; r++) {
        const line = rows[r].map((cell, i) => {
            return (' ' + String(cell ?? '') + ' ').padEnd(widths[i] + 2);
        }).join('│');

        console.log(line);
        if (r === 0) console.log(sep);
    }
}

module.exports = { c, ok, info, warn, die, table };
