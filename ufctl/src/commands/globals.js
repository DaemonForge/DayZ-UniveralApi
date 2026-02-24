/**
 * ufctl globals commands (Commander-based)
 *
 * Supports:
 *   globals list                                  — list all global modules
 *   globals get <mod>                             — show a module's data
 *   globals set <mod> --file data.json            — replace data from JSON file
 *   globals set <mod> --content '{...}'           — replace data from inline JSON
 *   globals set-param <mod> <path> <value>        — set a single parameter
 *   globals transaction <mod> <path> <amount>     — atomic increment
 *   globals delete <mod>                          — remove a module's global
 */

'use strict';

const fs   = require('fs');
const path = require('path');
const { resolveBackend }           = require('../lib/backend');
const { ok, info, warn, die, table, c } = require('../lib/output');

// ──────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────

function getBackend(cmd) {
    const root = cmd.optsWithGlobals();
    return resolveBackend(root);
}

/**
 * Parse a JSON string.  Returns the parsed value or calls die() with a
 * user-friendly error.
 */
function parseJSON(text, source) {
    try {
        return JSON.parse(text);
    } catch (err) {
        die(`Invalid JSON${source ? ' from ' + source : ''}: ${err.message}`);
    }
}

/**
 * Read a file, parse it as JSON, and return the parsed value.
 */
function readJSONFile(filePath) {
    const resolved = path.resolve(filePath);
    if (!fs.existsSync(resolved)) die(`File not found: ${resolved}`);
    const raw = fs.readFileSync(resolved, 'utf-8');
    return parseJSON(raw, resolved);
}

/**
 * Coerce a string value using --int, --float, --string, --bool, --json flags.
 * If none given, attempt auto-detection.
 */
function coerceValue(raw, opts) {
    if (opts.asJson) return parseJSON(raw, '--as-json value');
    if (opts.bool) {
        const lower = String(raw).toLowerCase();
        if (lower === 'true' || lower === '1' || lower === 'yes') return true;
        if (lower === 'false' || lower === '0' || lower === 'no') return false;
        die(`Cannot parse "${raw}" as boolean. Use true/false, 1/0, or yes/no.`);
    }
    if (opts.int) {
        const n = parseInt(raw, 10);
        if (isNaN(n)) die(`Cannot parse "${raw}" as integer.`);
        return n;
    }
    if (opts.float) {
        const n = parseFloat(raw);
        if (isNaN(n)) die(`Cannot parse "${raw}" as float.`);
        return n;
    }
    if (opts.string) return String(raw);

    // Auto-detect
    if (raw === 'true')  return true;
    if (raw === 'false') return false;
    if (raw === 'null')  return null;
    if (/^-?\d+$/.test(raw))        return parseInt(raw, 10);
    if (/^-?\d+\.\d+$/.test(raw))   return parseFloat(raw);
    // Try JSON (arrays/objects)
    try { return JSON.parse(raw); } catch { /* not JSON */ }
    return raw; // keep as string
}

// ──────────────────────────────────────────────────────
// Command registration
// ──────────────────────────────────────────────────────

module.exports = function registerGlobalsCommands(program) {
    const globals = program
        .command('globals')
        .description('manage Global state documents');

    // ── list ─────────────────────────────────────────

    globals
        .command('list')
        .description('list all global modules')
        .action(async function () {
            const be = getBackend(this);
            try {
                const list = await be.globals.list();
                const root = this.optsWithGlobals();

                if (root.json) {
                    console.log(JSON.stringify(list, null, 2));
                    return;
                }

                if (!list || list.length === 0) {
                    info('No global documents found.');
                    return;
                }

                const rows = [['Mod', 'ID']];
                for (const g of list) {
                    rows.push([g.mod, g.id]);
                }
                table(rows);
                info(`${list.length} global(s)`);
            } catch (err) {
                die(err.message);
            } finally {
                await be.close();
            }
        });

    // ── get ──────────────────────────────────────────

    globals
        .command('get <mod>')
        .description('show global data for a module')
        .action(async function (mod) {
            const be = getBackend(this);
            try {
                const doc = await be.globals.get(mod);
                const root = this.optsWithGlobals();

                if (!doc) {
                    if (root.json) { console.log('null'); return; }
                    die(`Global for module "${mod}" not found.`);
                }

                if (root.json) {
                    console.log(JSON.stringify(doc, null, 2));
                } else {
                    ok(`Module: ${c.bold(doc.mod)}`);
                    console.log(JSON.stringify(doc.data, null, 2));
                }
            } catch (err) {
                die(err.message);
            } finally {
                await be.close();
            }
        });

    // ── set ──────────────────────────────────────────

    globals
        .command('set <mod>')
        .description('set / replace global data for a module')
        .option('--file <path>', 'path to a JSON file')
        .option('--content <json>', 'inline JSON string')
        .action(async function (mod) {
            const opts = this.opts();
            if (!opts.file && !opts.content) {
                die('Provide --file <path> or --content <json>.');
            }
            if (opts.file && opts.content) {
                die('Use --file or --content, not both.');
            }

            const data = opts.file
                ? readJSONFile(opts.file)
                : parseJSON(opts.content, '--content');

            if (data === null || (typeof data !== 'object')) {
                die('Global data must be a JSON object or array.');
            }

            const be = getBackend(this);
            try {
                const success = await be.globals.set(mod, data);
                const root = this.optsWithGlobals();

                if (root.json) {
                    console.log(JSON.stringify({ mod, success }, null, 2));
                } else if (success) {
                    ok(`Global for "${mod}" saved.`);
                } else {
                    warn(`Global for "${mod}" may not have been saved.`);
                }
            } catch (err) {
                die(err.message);
            } finally {
                await be.close();
            }
        });

    // ── set-param ────────────────────────────────────

    globals
        .command('set-param <mod> <path> <value>')
        .description('set a single parameter inside a module\'s global data')
        .option('--int',    'coerce value to integer')
        .option('--float',  'coerce value to float')
        .option('--string', 'keep value as string')
        .option('--bool',   'coerce value to boolean')
        .option('--as-json','parse value as JSON')
        .action(async function (mod, paramPath, rawValue) {
            const opts  = this.opts();
            const value = coerceValue(rawValue, opts);

            const be = getBackend(this);
            try {
                const result = await be.globals.setParam(mod, paramPath, value);
                const root   = this.optsWithGlobals();

                if (result === null) {
                    if (root.json) { console.log(JSON.stringify({ mod, path: paramPath, result: null })); return; }
                    die(`Module "${mod}" not found or field "${paramPath}" could not be set.`);
                }

                if (root.json) {
                    console.log(JSON.stringify({ mod, path: paramPath, value: result }, null, 2));
                } else {
                    ok(`${mod}.${paramPath} = ${JSON.stringify(result)}`);
                }
            } catch (err) {
                die(err.message);
            } finally {
                await be.close();
            }
        });

    // ── transaction ──────────────────────────────────

    globals
        .command('transaction <mod> <path> <amount>')
        .description('atomic increment on a numeric field')
        .action(async function (mod, paramPath, rawAmount) {
            const amount = parseFloat(rawAmount);
            if (isNaN(amount)) die(`"${rawAmount}" is not a valid number.`);

            const be = getBackend(this);
            try {
                const result = await be.globals.transaction(mod, paramPath, amount);
                const root   = this.optsWithGlobals();

                if (result === null) {
                    if (root.json) { console.log(JSON.stringify({ mod, path: paramPath, result: null })); return; }
                    die(`Transaction failed for "${mod}.${paramPath}".`);
                }

                if (root.json) {
                    console.log(JSON.stringify({ mod, path: paramPath, value: result }, null, 2));
                } else {
                    ok(`${mod}.${paramPath} = ${result}`);
                }
            } catch (err) {
                die(err.message);
            } finally {
                await be.close();
            }
        });

    // ── delete ───────────────────────────────────────

    globals
        .command('delete <mod>')
        .description('delete a module\'s global document')
        .action(async function (mod) {
            const be = getBackend(this);
            try {
                const removed = await be.globals.delete(mod);
                const root    = this.optsWithGlobals();

                if (root.json) {
                    console.log(JSON.stringify({ mod, deleted: removed }, null, 2));
                } else if (removed) {
                    ok(`Deleted global for "${mod}".`);
                } else {
                    warn(`No global document found for "${mod}".`);
                }
            } catch (err) {
                die(err.message);
            } finally {
                await be.close();
            }
        });
};
