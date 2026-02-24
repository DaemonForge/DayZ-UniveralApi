/**
 * ufctl configuration
 *
 * Reads the SAME config.json used by the service binary.
 * On Linux this lives at /etc/ufserverservice/config.json
 * (with a symlink at /var/lib/ufserverservice/config.json).
 *
 * The CLI flags --config, --db-uri, --db-name can override
 * values at runtime but nothing is written back.
 */

'use strict';

const fs   = require('fs');
const path = require('path');

// ──────────────────────────────────────────────────────
// Service config search paths (same order the service uses)
// ──────────────────────────────────────────────────────

const SEARCH_PATHS = [
    '/etc/ufserverservice/config.json',
    '/var/lib/ufserverservice/config.json',
];

/**
 * Find the service config.json.
 * Priority: --config flag → env var → well-known Linux paths → cwd fallback
 */
function findServiceConfig(flagPath) {
    // Explicit path from --config flag or env var
    const explicit = flagPath || process.env.UF_CONFIG;
    if (explicit) {
        if (fs.existsSync(explicit)) return explicit;
        throw new Error(`Config file not found: ${explicit}`);
    }

    for (const p of SEARCH_PATHS) {
        try { if (fs.existsSync(p)) return p; } catch { /* skip */ }
    }

    // Dev fallback: check cwd (for running on Windows during dev)
    const cwdConfig = path.join(process.cwd(), 'config.json');
    if (fs.existsSync(cwdConfig)) return cwdConfig;

    return null;
}

/**
 * Load and parse the service config.json.
 * Returns the raw config object with DBServer, DB, OpenAIApi, ServerAuth, etc.
 */
function loadServiceConfig(flagPath) {
    const p = findServiceConfig(flagPath);
    if (!p) {
        throw new Error(
            'Service config.json not found.\n' +
            'Looked in:\n' +
            SEARCH_PATHS.map(s => `  • ${s}`).join('\n') + '\n' +
            'Use --config <path> to specify the location.'
        );
    }
    const raw = fs.readFileSync(p, 'utf-8');
    const cfg = JSON.parse(raw);
    cfg._configPath = p;
    return cfg;
}

/**
 * Resolve the effective connection / config values.
 * CLI flags override config.json values.
 */
function resolveConnection(flags) {
    const cfgPath = flags?.config || null;
    let svcCfg;
    try {
        svcCfg = loadServiceConfig(cfgPath);
    } catch (err) {
        // If user explicitly passed --config, that's a hard error
        if (cfgPath) throw err;
        // Otherwise fall through to defaults (dev / no config scenario)
        svcCfg = {};
    }

    const dbUri  = flags?.dbUri  || process.env.UFCTL_DB_URI  || svcCfg.DBServer || 'mongodb://localhost:27017';
    const dbName = flags?.dbName || process.env.UFCTL_DB_NAME || svcCfg.DB       || 'DayZ';

    // API mode fallback
    const port = svcCfg.Port || 443;
    const url  = flags?.url  || process.env.UFCTL_URL || `https://localhost:${port}`;
    const authKey = flags?.authKey || process.env.UFCTL_AUTH_KEY
        || (svcCfg.ServerAuth && svcCfg.ServerAuth.length > 0 ? svcCfg.ServerAuth[0] : '');

    // OpenAI key for embedding generation
    const openaiApiKey = process.env.OPENAI_API_KEY || svcCfg.OpenAIApi?.ApiKey || '';

    return {
        dbUri,
        dbName,
        url,
        authKey,
        openaiApiKey,
        mode: flags?.mode || process.env.UFCTL_MODE || '',
        _svcCfg: svcCfg,
    };
}

module.exports = { findServiceConfig, loadServiceConfig, resolveConnection };
