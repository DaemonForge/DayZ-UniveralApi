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
const { loadServiceConfig, findServiceConfig } = require('./lib/config');
const { ok, info, warn, die, c } = require('./lib/output');
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
    .action(function () {
        const root = this.optsWithGlobals();
        try {
            const cfg = loadServiceConfig(root.config);
            // Redact sensitive values
            const display = { ...cfg };
            if (display.ServerAuth) display.ServerAuth = display.ServerAuth.map(() => '***');
            if (display.OpenAIApi?.ApiKey) display.OpenAIApi = { ...display.OpenAIApi, ApiKey: '***' };
            if (display.Discord?.Bot_Token) display.Discord = { ...display.Discord, Bot_Token: '***', Client_Secret: '***' };
            console.log(JSON.stringify(display, null, 2));
            info(`Config file: ${cfg._configPath}`);
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

// ── kb subcommand ───────────────────────────────────

registerKBCommands(program);

// ── globals subcommand ──────────────────────────────

registerGlobalsCommands(program);

// ── run ─────────────────────────────────────────────

program.parseAsync(process.argv).catch(err => {
    die(err.message || String(err));
});
