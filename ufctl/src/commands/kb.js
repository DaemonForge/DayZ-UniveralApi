/**
 * ufctl kb commands (Commander-based)
 *
 * All commands go through the backend abstraction layer,
 * so they work identically against DirectBackend (MongoDB)
 * or ApiBackend (REST).
 *
 * Supports:
 *   kb add --kb <id> --file /path/to/file.md         (single file)
 *   kb add --kb <id> --file '/path/to/folder/*'       (glob / folder)
 *   kb add --kb <id> --content <text> --name <name>    (inline text)
 *   kb add --kb <id> --file '/folder/*' --refresh      (wipe + re-import)
 */

'use strict';

const fs   = require('fs');
const path = require('path');
const { resolveBackend }           = require('../lib/backend');
const { ok, info, warn, die, table, c } = require('../lib/output');

// Allowed text-based file extensions for folder imports
const TEXT_EXTENSIONS = new Set([
    '.txt', '.md', '.markdown', '.json', '.xml',
    '.csv', '.yaml', '.yml', '.html', '.htm',
    '.c', '.h', '.js', '.ts', '.py', '.lua',
    '.cfg', '.ini', '.conf', '.log', '.rst',
]);

// ──────────────────────────────────────────────────────
// Helpers
// ──────────────────────────────────────────────────────

function getBackend(cmd) {
    const root = cmd.optsWithGlobals();
    return resolveBackend(root);
}

/**
 * Expand a --file argument into an array of { filePath, name, fileType }.
 *
 * Supports:
 *   /path/to/file.md          → single file
 *   /path/to/folder/*         → all text files in folder (non-recursive)
 *   /path/to/folder/**        → all text files recursively
 *   /path/to/folder/          → same as folder/*
 */
function expandFilePaths(fileArg) {
    if (!fileArg) return [];

    // Check for glob: ends with /* or /** or trailing slash
    const isGlob       = fileArg.endsWith('/*');
    const isRecursive  = fileArg.endsWith('/**');
    const isDir        = fileArg.endsWith('/') || fileArg.endsWith('\\');

    if (isGlob || isRecursive || isDir) {
        let dirPath = fileArg;
        if (isRecursive) dirPath = fileArg.slice(0, -3);
        else if (isGlob) dirPath = fileArg.slice(0, -2);
        else dirPath = fileArg.slice(0, -1);

        dirPath = path.resolve(dirPath);
        if (!fs.existsSync(dirPath) || !fs.statSync(dirPath).isDirectory()) {
            die(`Directory not found: ${dirPath}`);
        }

        return collectFiles(dirPath, isRecursive);
    }

    // Single file
    const filePath = path.resolve(fileArg);
    if (!fs.existsSync(filePath)) die(`File not found: ${filePath}`);
    if (fs.statSync(filePath).isDirectory()) {
        // User passed a directory without trailing slash — treat as folder/*
        return collectFiles(filePath, false);
    }

    const ext = path.extname(filePath).toLowerCase();
    return [{
        filePath,
        name: path.basename(filePath),
        fileType: ext.replace('.', '') || 'txt',
    }];
}

/**
 * Recursively or non-recursively collect text files from a directory.
 */
function collectFiles(dirPath, recursive) {
    const results = [];

    function walk(dir, relPrefix) {
        const entries = fs.readdirSync(dir, { withFileTypes: true });
        for (const entry of entries) {
            const fullPath = path.join(dir, entry.name);
            if (entry.isDirectory()) {
                if (recursive) walk(fullPath, path.join(relPrefix, entry.name));
            } else if (entry.isFile()) {
                const ext = path.extname(entry.name).toLowerCase();
                if (TEXT_EXTENSIONS.has(ext)) {
                    const relName = relPrefix ? path.join(relPrefix, entry.name) : entry.name;
                    results.push({
                        filePath: fullPath,
                        name: relName.replace(/\\/g, '/'),   // normalise to forward slashes
                        fileType: ext.replace('.', '') || 'txt',
                    });
                }
            }
        }
    }

    walk(dirPath, '');

    if (results.length === 0) {
        die(`No supported text files found in: ${dirPath}\n  Supported: ${[...TEXT_EXTENSIONS].join(', ')}`);
    }
    return results;
}

// ──────────────────────────────────────────────────────
// list
// ──────────────────────────────────────────────────────

async function cmdList(opts, cmd) {
    const be = getBackend(cmd);
    try {
        const data = await be.kb.list();
        const root = cmd.optsWithGlobals();

        if (root.json) {
            console.log(JSON.stringify(data, null, 2));
            return;
        }

        if (!data || data.length === 0) {
            info('No knowledge bases found.');
            return;
        }

        const rows = [['KB ID', 'Name', 'Docs', 'Description']];
        for (const kb of data) {
            rows.push([
                kb.kbId || kb._id,
                kb.name || '',
                kb.documentCount ?? '?',
                (kb.description || '').substring(0, 50),
            ]);
        }
        table(rows);
    } finally {
        await be.close();
    }
}

// ──────────────────────────────────────────────────────
// create
// ──────────────────────────────────────────────────────

async function cmdCreate(opts, cmd) {
    if (!opts.id)   die('--id is required.');
    if (!opts.name) die('--name is required.');

    const be = getBackend(cmd);
    try {
        const data = await be.kb.create(opts.id, opts.name, opts.desc || '');
        const root = cmd.optsWithGlobals();

        if (root.json) {
            console.log(JSON.stringify(data, null, 2));
            return;
        }
        ok(`KB created: ${c.bold(opts.id)} (${opts.name})`);
    } finally {
        await be.close();
    }
}

// ──────────────────────────────────────────────────────
// delete-kb
// ──────────────────────────────────────────────────────

async function cmdDeleteKB(opts, cmd) {
    if (!opts.id) die('--id is required.');

    const be = getBackend(cmd);
    try {
        const ok_ = await be.kb.delete(opts.id);
        const root = cmd.optsWithGlobals();

        if (root.json) {
            console.log(JSON.stringify({ deleted: opts.id, ok: ok_ }, null, 2));
            return;
        }
        ok(`KB deleted: ${c.bold(opts.id)}`);
    } finally {
        await be.close();
    }
}

// ──────────────────────────────────────────────────────
// add  (add / replace files — single file, folder/*, or inline)
// ──────────────────────────────────────────────────────

async function cmdAdd(opts, cmd) {
    if (!opts.kb) die('--kb is required.');

    const root  = cmd.optsWithGlobals();
    const be    = getBackend(cmd);
    const kbId  = opts.kb;
    const contextHint = opts.contextHint || '';

    try {
        // Verify KB exists
        const kb = await be.kb.get(kbId);
        if (!kb) die(`KB "${kbId}" does not exist. Create it first: ufctl kb create --id ${kbId} --name "..."`);

        // Determine what we're adding
        let files = [];

        if (opts.file) {
            files = expandFilePaths(opts.file);
        } else if (opts.content) {
            if (!opts.name) die('--name is required when using --content.');
            files = [{
                filePath: null,  // inline content, no file
                name: opts.name,
                fileType: path.extname(opts.name).replace('.', '') || 'txt',
                _inlineContent: opts.content,
            }];
        } else {
            die('Either --file <path|folder/*> or --content <text> is required.');
        }

        // --refresh: wipe all existing documents first
        if (opts.refresh) {
            const deleted = await be.kb.deleteAllDocuments(kbId);
            info(`Refresh: removed ${deleted} existing document chunks from KB "${kbId}".`);
        }

        // Pre-fetch existing doc names for replace-by-name logic (unless --refresh)
        let existingDocs = [];
        if (!opts.refresh) {
            existingDocs = await be.kb.listDocuments(kbId) || [];
        }

        const results = [];
        let succeeded = 0;
        let failed = 0;

        for (const file of files) {
            try {
                // Read content
                let content;
                if (file._inlineContent) {
                    content = file._inlineContent;
                } else {
                    content = fs.readFileSync(file.filePath, 'utf-8');
                }
                if (!content || content.trim().length === 0) {
                    warn(`Skipping empty file: ${file.name}`);
                    continue;
                }

                // Replace-by-name: if a doc with same name exists, delete it first
                if (!opts.refresh) {
                    const existing = existingDocs.find(d => d.name === file.name);
                    if (existing) {
                        await be.kb.deleteDocument(kbId, existing.documentId);
                        if (files.length === 1) info(`Document "${file.name}" already exists — replacing.`);
                    }
                }

                const data = await be.kb.addDocument(kbId, file.name, content, contextHint, file.fileType);
                succeeded++;
                results.push(data);

                if (files.length === 1) {
                    // Single file — detailed output
                    if (root.json) {
                        console.log(JSON.stringify(data, null, 2));
                        return;
                    }
                    const embNote = data.hasEmbedding
                        ? c.green('embeddings generated')
                        : c.yellow('no embeddings (OpenAI API key not configured)');
                    ok(`Document added: ${c.bold(file.name)} → KB ${c.bold(kbId)} [${embNote}]`);
                } else {
                    // Multi-file — progress line
                    const emb = data.hasEmbedding ? '✓' : '-';
                    const idx = `${succeeded + failed}/${files.length}`;
                    const prefix = supportsStdoutTTY() ? `\r  [${idx}]` : `  [${idx}]`;
                    process.stdout.write(`${prefix} ${emb} ${file.name}\n`);
                }
            } catch (err) {
                failed++;
                warn(`Failed to add "${file.name}": ${err.message}`);
            }
        }

        // Multi-file summary
        if (files.length > 1) {
            if (root.json) {
                console.log(JSON.stringify({ succeeded, failed, results }, null, 2));
                return;
            }
            const embCount = results.filter(r => r.hasEmbedding).length;
            ok(`Added ${c.bold(succeeded)} document(s) to KB ${c.bold(kbId)}` +
               (embCount > 0 ? ` [${c.green(embCount + ' with embeddings')}]` : '') +
               (failed > 0 ? ` [${c.red(failed + ' failed')}]` : ''));
        }
    } finally {
        await be.close();
    }
}

function supportsStdoutTTY() {
    return process.stdout.isTTY;
}

// ──────────────────────────────────────────────────────
// remove (delete document by name)
// ──────────────────────────────────────────────────────

async function cmdRemove(opts, cmd) {
    if (!opts.kb)   die('--kb is required.');
    if (!opts.name) die('--name is required.');

    const be = getBackend(cmd);
    try {
        const docs = await be.kb.listDocuments(opts.kb);
        const doc = (docs || []).find(d => d.name === opts.name);
        if (!doc) die(`Document "${opts.name}" not found in KB "${opts.kb}".`);

        await be.kb.deleteDocument(opts.kb, doc.documentId);
        const root = cmd.optsWithGlobals();

        if (root.json) {
            console.log(JSON.stringify({ deleted: opts.name, documentId: doc.documentId }, null, 2));
            return;
        }
        ok(`Document removed: ${c.bold(opts.name)} from KB ${c.bold(opts.kb)}`);
    } finally {
        await be.close();
    }
}

// ──────────────────────────────────────────────────────
// update (KB settings: name, description, shorterAnswers, extractModel)
// ──────────────────────────────────────────────────────

async function cmdUpdate(opts, cmd) {
    if (!opts.id) die('--id is required.');

    const be = getBackend(cmd);
    try {
        // Verify KB exists
        const kb = await be.kb.get(opts.id);
        if (!kb) die(`KB "${opts.id}" not found.`);

        const updates = {};
        if (opts.name !== undefined)           updates.name           = opts.name;
        if (opts.desc !== undefined)           updates.description    = opts.desc;
        if (opts.shorterAnswers !== undefined) updates.shorterAnswers = opts.shorterAnswers;
        if (opts.extractModel !== undefined)   updates.extractModel   = opts.extractModel;

        if (Object.keys(updates).length === 0) {
            die('Nothing to update. Use --name, --desc, --shorter-answers, or --extract-model.');
        }

        const success = await be.kb.update(opts.id, updates);
        const root = cmd.optsWithGlobals();

        if (root.json) {
            console.log(JSON.stringify({ kbId: opts.id, updated: updates, success }, null, 2));
            return;
        }

        if (success) {
            const parts = [];
            if (updates.name !== undefined)           parts.push(`name="${updates.name}"`);
            if (updates.description !== undefined)    parts.push(`description="${updates.description}"`);
            if (updates.shorterAnswers !== undefined) parts.push(`shorterAnswers=${updates.shorterAnswers}`);
            if (updates.extractModel !== undefined)   parts.push(`extractModel="${updates.extractModel}"`);
            ok(`KB ${c.bold(opts.id)} updated: ${parts.join(', ')}`);
        } else {
            warn('No changes were made (values may already match).');
        }
    } finally {
        await be.close();
    }
}

// ──────────────────────────────────────────────────────
// Register with Commander
// ──────────────────────────────────────────────────────

function registerKBCommands(program) {
    const kb = program
        .command('kb')
        .description('Knowledge Base management');

    kb.command('list')
        .description('list all knowledge bases')
        .action(cmdList);

    kb.command('create')
        .description('create a new KB')
        .requiredOption('--id <kbId>', 'unique KB identifier (alphanumeric + underscores)')
        .requiredOption('--name <name>', 'human-readable name')
        .option('--desc <description>', 'optional description')
        .action(cmdCreate);

    kb.command('delete-kb')
        .description('delete a KB and all its documents')
        .requiredOption('--id <kbId>', 'KB identifier to delete')
        .action(cmdDeleteKB);

    kb.command('update')
        .description('update KB settings (name, description, shorterAnswers, extractModel)')
        .requiredOption('--id <kbId>', 'KB identifier to update')
        .option('--name <name>', 'new name')
        .option('--desc <description>', 'new description')
        .option('--shorter-answers <bool>', 'enable/disable shorter answers (true/false)', parseBool)
        .option('--extract-model <model>', 'extraction model (e.g. gpt-4o-mini)')
        .action(cmdUpdate);

    kb.command('add')
        .description('add document(s) to a KB from file, folder, or inline text')
        .requiredOption('--kb <kbId>', 'target KB identifier')
        .option('--file <path>', 'file path, folder/*, or folder/** for recursive')
        .option('--name <name>', 'document name (defaults to filename)')
        .option('--content <text>', 'inline content string (requires --name)')
        .option('--context-hint <hint>', 'context hint for retrieval')
        .option('--refresh', 'remove all existing documents before importing')
        .action(cmdAdd);

    kb.command('remove')
        .description('remove a document from a KB by name')
        .requiredOption('--kb <kbId>', 'target KB identifier')
        .requiredOption('--name <name>', 'document name to remove')
        .action(cmdRemove);
}

/** Parse boolean string for Commander option */
function parseBool(val) {
    if (val === 'true' || val === '1' || val === 'yes') return true;
    if (val === 'false' || val === '0' || val === 'no') return false;
    throw new Error(`Invalid boolean value: "${val}". Use true/false.`);
}

module.exports = registerKBCommands;
