/**
 * ufctl data commands (Commander-based)
 *
 * Simplified data manager — CLI equivalent of the Electron "Data Manager" window.
 * Scans all collections to find installed mods and their document counts.
 * Allows deleting all data for a specific mod across all collections.
 *
 * Supports:
 *   data list                                     — scan & list all installed mods
 *   data delete <mod>                             — delete all data for a mod
 *   data delete <mod> --yes                       — skip confirmation
 */

'use strict';

const { resolveBackend } = require('../lib/backend');
const { ok, info, warn, die, c, table } = require('../lib/output');
const readline = require('readline');

/** Resolve the backend from Commander's merged opts */
function getBackend(cmd) {
    const root = cmd.optsWithGlobals();
    return resolveBackend(root);
}

/**
 * Prompt for confirmation on stdin.
 * Returns true if user types 'y' or 'yes'.
 */
function confirm(question) {
    return new Promise(resolve => {
        const rl = readline.createInterface({
            input: process.stdin,
            output: process.stderr, // Write prompt to stderr so --json stdout stays clean
        });
        rl.question(question, answer => {
            rl.close();
            resolve(/^y(es)?$/i.test(answer.trim()));
        });
    });
}

function registerDataCommands(program) {
    const data = program
        .command('data')
        .description('mod data manager — scan & delete mod data across collections');

    // ── list ─────────────────────────────────────────

    data
        .command('list')
        .description('scan all collections and list installed mods with document counts')
        .action(async function () {
            const be = getBackend(this);
            const root = this.optsWithGlobals();
            try {
                const mods = await be.data.scanInstalledMods();

                if (root.json) {
                    console.log(JSON.stringify(mods, null, 2));
                    return;
                }

                if (!mods || mods.length === 0) {
                    info('No mod data found in the database.');
                    return;
                }

                // Summary table
                const rows = [['Mod', 'Total Docs', 'Collections']];
                for (const mod of mods) {
                    const collNames = Object.entries(mod.collections)
                        .filter(([, count]) => count > 0)
                        .map(([name, count]) => `${name}(${count})`)
                        .join(', ');
                    rows.push([
                        mod.modName,
                        String(mod.totalDocuments),
                        collNames || c.dim('(empty)'),
                    ]);
                }
                ok(`Found ${c.bold(String(mods.length))} installed mod(s)`);
                table(rows);

                // Grand total
                const totalDocs = mods.reduce((sum, m) => sum + m.totalDocuments, 0);
                info(`${totalDocs.toLocaleString()} total document(s) across all mods`);
            } catch (err) {
                die(err.message);
            } finally {
                await be.close();
            }
        });

    // ── delete ───────────────────────────────────────

    data
        .command('delete <mod>')
        .description('delete all data for a mod across all collections')
        .option('--yes', 'skip confirmation prompt')
        .action(async function (mod) {
            const opts = this.opts();
            const root = this.optsWithGlobals();

            // Confirmation unless --yes flag
            if (!opts.yes) {
                warn(`This will permanently delete ALL data for mod "${mod}" across ALL collections.`);
                info('Collections affected: Objects, Players (subdocuments), Messages, MessagesMeta,');
                info('  PlayerMessagesStatus, AIChats, AIMessages, AIAssistantThreads');
                info('Note: Globals are NOT deleted (use "ufctl globals delete" instead).');
                console.log('');
                const confirmed = await confirm(`  Type "yes" to confirm deletion of "${mod}": `);
                if (!confirmed) {
                    info('Cancelled.');
                    return;
                }
            }

            const be = getBackend(this);
            try {
                const result = await be.data.deleteModData(mod);

                if (root.json) {
                    console.log(JSON.stringify(result, null, 2));
                    return;
                }

                if (result.totalDeleted === 0 && result.errors.length === 0) {
                    warn(`No data found for mod "${mod}".`);
                    return;
                }

                ok(`Deleted data for "${c.bold(mod)}"`);

                // Show per-collection breakdown
                const rows = [['Collection', 'Deleted']];
                for (const [coll, count] of Object.entries(result.collections)) {
                    if (count > 0) {
                        rows.push([coll, String(count)]);
                    }
                }
                if (rows.length > 1) {
                    table(rows);
                }

                info(`${result.totalDeleted} total document(s) deleted`);

                if (result.errors.length > 0) {
                    warn(`${result.errors.length} error(s):`);
                    for (const e of result.errors) {
                        console.log(`  ${c.red(e.collection)}: ${e.error}`);
                    }
                }
            } catch (err) {
                die(err.message);
            } finally {
                await be.close();
            }
        });
}

module.exports = registerDataCommands;
