#!/usr/bin/env node
/**
 * ufctl – CLI companion for Universal Framework Server Service
 *
 * Linux-only tool that ships alongside the service binary.
 * Reads the SAME config.json as the service (/etc/ufserverservice/config.json)
 * for MongoDB connection, OpenAI API key, server auth tokens, etc.
 *
 * Architecture:
 *   Commands talk to a "backend" abstraction with two implementations:
 *     • DirectBackend  – reads/writes MongoDB directly  (default)
 *     • ApiBackend     – talks to the REST API over HTTPS  (--mode api)
 */

'use strict';

const { Command } = require('commander');
const { resolveBackend } = require('./lib/backend');
const { loadServiceConfig, findServiceConfig, saveServiceConfig } = require('./lib/config');
const { ok, info, warn, die, c, table } = require('./lib/output');
const registerKBCommands = require('./commands/kb');
const registerGlobalsCommands = require('./commands/globals');

const pkg = require('../package.json');

const program = new Command();

program
    .name('ufctl')
    .description('CLI companion for Universal Framework Server Service')
    .version(pkg.version)
    .option('--config <path>', 'path to service config.json (default: /etc/ufserverservice/config.json)')
    .option('--mode <mode>', 'force backend: direct (MongoDB, default) or api (REST)')
    .option('--db-uri <uri>', 'override MongoDB URI')
    .option('--db-name <name>', 'override database name')
    .option('--json', 'output raw JSON (for scripting)');

// ── config subcommand ───────────────────────────────

const configCmd = program
    .command('config')
    .description('view service configuration (read-only)');

configCmd
    .command('show')
    .description('print effective service configuration')
    .option('--reveal', 'show sensitive values (tokens, keys) unredacted')
    .action(function () {
        const root = this.optsWithGlobals();
        const local = this.opts();
        try {
            const cfg = loadServiceConfig(root.config);
            const display = { ...cfg };
            if (!local.reveal) {
                if (display.ServerAuth) display.ServerAuth = display.ServerAuth.map(() => '***');
                if (display.OpenAIApi?.ApiKey) display.OpenAIApi = { ...display.OpenAIApi, ApiKey: '***' };
                if (display.Discord?.Bot_Token) display.Discord = { ...display.Discord, Bot_Token: '***', Client_Secret: '***' };
            }
            delete display._configPath;
            console.log(JSON.stringify(display, null, 2));
            info(`Config file: ${cfg._configPath}`);
            if (!local.reveal) info('Sensitive values redacted. Use --reveal to show them.');
        } catch (err) {
            die(err.message);
        }
    });

configCmd
    .command('path')
    .description('print the path to the config file being used')
    .action(function () {
        const root = this.optsWithGlobals();
        const p = findServiceConfig(root.config);
        if (p) {
            console.log(p);
        } else {
            die('Service config.json not found.');
        }
    });

// ── config auth subcommands ─────────────────────────

const authCmd = configCmd
    .command('auth')
    .description('manage server auth tokens');

authCmd
    .command('list')
    .description('list all server auth tokens')
    .option('--reveal', 'show full token values')
    .action(function () {
        const root = this.optsWithGlobals();
        const local = this.opts();
        try {
            const cfg = loadServiceConfig(root.config);
            const tokens = cfg.ServerAuth || [];
            const labels = cfg.ServerAuthLabels || [];
            if (tokens.length === 0) {
                warn('No server auth tokens configured.');
                return;
            }
            if (root.json) {
                const entries = tokens.map((t, i) => ({
                    index: i,
                    label: labels[i] || '',
                    token: local.reveal ? t : '***'
                }));
                console.log(JSON.stringify(entries, null, 2));
                return;
            }
            const rows = [['#', 'Label', 'Token']];
            tokens.forEach((t, i) => {
                const label = labels[i] || c.dim('(none)');
                const display = local.reveal ? t : (t.slice(0, 6) + '...' + t.slice(-4));
                rows.push([String(i), label, display]);
            });
            table(rows);
            info(`${tokens.length} token(s). Use --reveal to show full values.`);
        } catch (err) {
            die(err.message);
        }
    });

authCmd
    .command('add')
    .description('generate and add a new server auth token')
    .argument('[label]', 'label / server ID for the new token', '')
    .action(function (label) {
        const root = this.optsWithGlobals();
        try {
            const cfg = loadServiceConfig(root.config);
            if (!Array.isArray(cfg.ServerAuth)) cfg.ServerAuth = [];
            if (!Array.isArray(cfg.ServerAuthLabels)) cfg.ServerAuthLabels = [];

            // Generate a 48-char random token (same as the service)
            const chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.!~';
            let token = '';
            for (let i = 0; i < 48; i++) {
                token += chars.charAt(Math.floor(Math.random() * chars.length));
            }

            cfg.ServerAuth.push(token);
            cfg.ServerAuthLabels.push(label || '');

            // Keep labels array in sync
            while (cfg.ServerAuthLabels.length < cfg.ServerAuth.length) cfg.ServerAuthLabels.push('');
            if (cfg.ServerAuthLabels.length > cfg.ServerAuth.length) {
                cfg.ServerAuthLabels = cfg.ServerAuthLabels.slice(0, cfg.ServerAuth.length);
            }

            saveServiceConfig(cfg);

            const idx = cfg.ServerAuth.length - 1;
            ok(`Auth token #${idx} added${label ? ` (label: ${c.bold(label)})` : ''}`);
            console.log();
            console.log(`  ${c.bold('Token:')}  ${c.yellow(token)}`);
            console.log(`  ${c.bold('Label:')}  ${label || c.dim('(none)')}`);
            console.log();
            info('Paste this token into the DayZ server\'s $profile:UF/UFramework.json → "ServerAuth"');
            warn('Restart the service for the new token to take effect.');
        } catch (err) {
            die(err.message);
        }
    });

authCmd
    .command('remove')
    .description('remove a server auth token by index or label')
    .argument('<index-or-label>', 'numeric index or label text to match')
    .action(function (target) {
        const root = this.optsWithGlobals();
        try {
            const cfg = loadServiceConfig(root.config);
            if (!Array.isArray(cfg.ServerAuth) || cfg.ServerAuth.length === 0) {
                die('No auth tokens to remove.');
            }
            if (!Array.isArray(cfg.ServerAuthLabels)) cfg.ServerAuthLabels = [];

            let idx = -1;
            // Try numeric index first
            const asNum = parseInt(target, 10);
            if (!isNaN(asNum) && asNum >= 0 && asNum < cfg.ServerAuth.length) {
                idx = asNum;
            } else {
                // Search by label (case-insensitive)
                const lower = target.toLowerCase();
                idx = cfg.ServerAuthLabels.findIndex(l => l && l.toLowerCase() === lower);
            }

            if (idx < 0) {
                die(`No auth token found matching "${target}".`);
            }

            const removed = cfg.ServerAuth[idx];
            const removedLabel = cfg.ServerAuthLabels[idx] || '';
            cfg.ServerAuth.splice(idx, 1);
            cfg.ServerAuthLabels.splice(idx, 1);

            saveServiceConfig(cfg);

            ok(`Removed auth token #${idx}${removedLabel ? ` (${removedLabel})` : ''}: ${removed.slice(0, 6)}...${removed.slice(-4)}`);
            info(`${cfg.ServerAuth.length} token(s) remaining.`);
            warn('Restart the service for this change to take effect.');
        } catch (err) {
            die(err.message);
        }
    });

// ── kb subcommand ───────────────────────────────────

registerKBCommands(program);

// ── globals subcommand ──────────────────────────────

registerGlobalsCommands(program);

// ── run ─────────────────────────────────────────────

program.parseAsync(process.argv).catch(err => {
    die(err.message || String(err));
});
