/**
 * ufctl index command (Commander-based)
 *
 * Interactive wizard that mirrors the Electron Index Optimizer UI:
 *   1. Scans all collections for indexes and usage stats
 *   2. Shows a summary with unused-index warnings
 *   3. Lets you pick indexes to create or drop interactively
 *
 * Usage:
 *   ufctl index                  — run the interactive wizard
 *   ufctl index --json           — dump full analysis as JSON (for scripting)
 */

'use strict';

const readline = require('readline');
const { resolveBackend } = require('../lib/backend');
const { ok, info, warn, die, c, table } = require('../lib/output');

/** Resolve the backend from Commander's merged opts */
function getBackend(cmd) {
    const root = cmd.optsWithGlobals();
    return resolveBackend(root);
}

// ────────────────────────────────────────────────────
// Readline helpers
// ────────────────────────────────────────────────────

function createRL() {
    return readline.createInterface({ input: process.stdin, output: process.stderr });
}

function ask(rl, question) {
    return new Promise(resolve => rl.question(question, answer => resolve(answer.trim())));
}

function formatKey(key) {
    return Object.entries(key).map(([k, v]) => `${k}:${v}`).join(', ');
}

function formatBytes(bytes) {
    if (bytes == null || bytes === 0) return '0 B';
    const units = ['B', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(1024));
    return (bytes / Math.pow(1024, i)).toFixed(i > 0 ? 1 : 0) + ' ' + units[i];
}

// ────────────────────────────────────────────────────
// Analysis
// ────────────────────────────────────────────────────

async function runAnalysis(be) {
    const collections = await be.index.getCollections();
    const results = [];

    for (const name of collections) {
        const analysis = await be.index.analyzeCollection(name);
        results.push(analysis);
    }

    return results;
}

function findUnusedIndexes(analyses) {
    const unused = [];
    for (const a of analyses) {
        for (const idx of a.indexes) {
            if (idx.name === '_id_') continue;
            if (idx.usageStats && idx.usageStats.ops === 0) {
                unused.push({ collection: a.collection, ...idx });
            }
        }
    }
    return unused;
}

function findBareCollections(analyses) {
    // Collections with only the default _id index and at least some documents
    return analyses.filter(a => a.indexes.length <= 1 && a.documentCount > 0);
}

// ────────────────────────────────────────────────────
// Display
// ────────────────────────────────────────────────────

function printSummary(analyses) {
    console.log('');
    ok('Index Analysis');
    console.log('');

    for (const a of analyses) {
        const customCount = a.indexes.filter(i => i.name !== '_id_').length;
        const label = customCount === 0
            ? c.dim(`(only _id)`)
            : `${customCount} custom index${customCount !== 1 ? 'es' : ''}`;

        console.log(`  ${c.bold(a.collection)}`);
        console.log(`    Docs: ${a.documentCount.toLocaleString()}  Size: ${formatBytes(a.totalSize)}  Indexes: ${label}`);

        for (const idx of a.indexes) {
            if (idx.name === '_id_') continue;
            const ops = idx.usageStats ? String(idx.usageStats.ops) : c.dim('n/a');
            const unusedTag = (idx.usageStats && idx.usageStats.ops === 0)
                ? c.yellow(' (unused)')
                : '';
            console.log(`    ${c.dim('•')} ${idx.name}  ${c.dim(formatKey(idx.key))}  ops: ${ops}${unusedTag}`);
        }
        console.log('');
    }
}

// ────────────────────────────────────────────────────
// Wizard: create indexes
// ────────────────────────────────────────────────────

async function wizardCreate(rl, be, analyses) {
    const collections = analyses.map(a => a.collection).sort();

    console.log('');
    info('Create a new index');
    console.log('  Available collections:');
    collections.forEach((name, i) => console.log(`    ${c.bold(String(i + 1))}. ${name}`));
    console.log('');

    const colChoice = await ask(rl, '  Collection number (or name): ');
    let collection;
    const asNum = parseInt(colChoice, 10);
    if (!isNaN(asNum) && asNum >= 1 && asNum <= collections.length) {
        collection = collections[asNum - 1];
    } else if (collections.includes(colChoice)) {
        collection = colChoice;
    } else {
        warn(`Invalid collection: "${colChoice}"`);
        return;
    }

    console.log('');
    info(`Creating index on ${c.bold(collection)}`);
    console.log('  Enter fields as JSON, e.g. {"Mod":1,"OID":1}');
    console.log('  (1 = ascending, -1 = descending)');
    const specRaw = await ask(rl, '  Index spec: ');

    let spec;
    try {
        spec = JSON.parse(specRaw);
    } catch {
        warn('Invalid JSON. Aborting.');
        return;
    }
    if (!spec || typeof spec !== 'object' || Array.isArray(spec) || Object.keys(spec).length === 0) {
        warn('Index spec must be a non-empty JSON object.');
        return;
    }

    const uniqueAnswer = await ask(rl, '  Unique index? (y/N): ');
    const unique = /^y(es)?$/i.test(uniqueAnswer);

    console.log('');
    console.log(`  Collection: ${c.bold(collection)}`);
    console.log(`  Spec:       ${c.bold(formatKey(spec))}`);
    console.log(`  Unique:     ${unique ? 'yes' : 'no'}`);

    const confirm = await ask(rl, '  Create this index? (y/N): ');
    if (!/^y(es)?$/i.test(confirm)) {
        info('Cancelled.');
        return;
    }

    try {
        const opts = {};
        if (unique) opts.unique = true;
        const result = await be.index.createIndex(collection, spec, opts);
        ok(`Index created: ${c.bold(result.indexName)}`);
    } catch (err) {
        warn(`Failed: ${err.message}`);
    }
}

// ────────────────────────────────────────────────────
// Wizard: drop unused indexes
// ────────────────────────────────────────────────────

async function wizardDrop(rl, be, unused) {
    console.log('');
    info(`${unused.length} unused index${unused.length !== 1 ? 'es' : ''} detected (zero operations since tracking started):`);
    console.log('');
    unused.forEach((idx, i) => {
        console.log(`    ${c.bold(String(i + 1))}. ${idx.collection} → ${idx.name}  ${c.dim(formatKey(idx.key))}`);
    });
    console.log('');

    const choice = await ask(rl, '  Enter number(s) to drop (comma-separated), or "skip": ');
    if (!choice || /^s(kip)?$/i.test(choice)) {
        info('Skipped.');
        return;
    }

    const nums = choice.split(',').map(s => parseInt(s.trim(), 10)).filter(n => !isNaN(n));
    const toDrop = nums
        .map(n => unused[n - 1])
        .filter(Boolean);

    if (toDrop.length === 0) {
        warn('No valid selections.');
        return;
    }

    console.log('');
    console.log('  Will drop:');
    for (const d of toDrop) {
        console.log(`    ${c.red('✗')} ${d.collection} → ${d.name}`);
    }
    const confirm = await ask(rl, `  Confirm drop ${toDrop.length} index${toDrop.length !== 1 ? 'es' : ''}? (y/N): `);
    if (!/^y(es)?$/i.test(confirm)) {
        info('Cancelled.');
        return;
    }

    for (const d of toDrop) {
        try {
            await be.index.dropIndex(d.collection, d.name);
            ok(`Dropped ${d.name} from ${d.collection}`);
        } catch (err) {
            warn(`Failed to drop ${d.name}: ${err.message}`);
        }
    }
}

// ────────────────────────────────────────────────────
// Main wizard loop
// ────────────────────────────────────────────────────

async function runWizard(be) {
    info('Scanning collections...');
    const analyses = await runAnalysis(be);

    if (analyses.length === 0) {
        info('No collections found.');
        return;
    }

    printSummary(analyses);

    const unused = findUnusedIndexes(analyses);
    const bare = findBareCollections(analyses);

    // Warnings
    if (bare.length > 0) {
        warn(`${bare.length} collection(s) with documents but no custom indexes:`);
        for (const b of bare) {
            console.log(`  ${c.yellow('•')} ${b.collection} (${b.documentCount.toLocaleString()} docs)`);
        }
        console.log('');
    }

    if (unused.length > 0) {
        warn(`${unused.length} unused index${unused.length !== 1 ? 'es' : ''} found.`);
        console.log('');
    }

    // Interactive loop
    const rl = createRL();
    try {
        let running = true;
        while (running) {
            console.log('  Actions:');
            console.log(`    ${c.bold('1')}. Create a new index`);
            if (unused.length > 0) {
                console.log(`    ${c.bold('2')}. Drop unused indexes (${unused.length})`);
            }
            console.log(`    ${c.bold('q')}. Quit`);
            console.log('');

            const action = await ask(rl, '  Choice: ');

            if (action === '1') {
                await wizardCreate(rl, be, analyses);
            } else if (action === '2' && unused.length > 0) {
                await wizardDrop(rl, be, unused);
                // Re-check unused after drops
                const refreshed = await runAnalysis(be);
                unused.length = 0;
                unused.push(...findUnusedIndexes(refreshed));
            } else if (/^q(uit)?$/i.test(action)) {
                running = false;
            } else {
                warn('Invalid choice.');
            }
            console.log('');
        }
    } finally {
        rl.close();
    }
}

// ────────────────────────────────────────────────────
// Register with Commander
// ────────────────────────────────────────────────────

function registerIndexCommands(program) {
    program
        .command('index')
        .description('interactive index optimizer — analyze, create, and drop indexes')
        .action(async function () {
            const be = getBackend(this);
            const root = this.optsWithGlobals();
            try {
                if (root.json) {
                    // Non-interactive: dump full analysis as JSON
                    const analyses = await runAnalysis(be);
                    const unused = findUnusedIndexes(analyses);
                    const bare = findBareCollections(analyses);
                    console.log(JSON.stringify({
                        collections: analyses,
                        unusedIndexes: unused,
                        bareCollections: bare.map(b => b.collection),
                    }, null, 2));
                    return;
                }
                await runWizard(be);
            } catch (err) {
                die(err.message);
            } finally {
                await be.close();
            }
        });
}

module.exports = registerIndexCommands;
