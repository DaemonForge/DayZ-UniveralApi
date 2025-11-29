#!/usr/bin/env node
/**
 * Headless entry point for Universal Framework Service
 * Used for Linux/non-GUI deployments via pkg
 */

const path = require('path');

// Set global flags before loading app
global.isElectron = false;
global.SAVEPATH = process.env.UF_SAVE_PATH || './';
global.APIVERSION = require('./package.json').version;
global.rootPath = __dirname;

// Handle graceful shutdown
process.on('SIGINT', () => {
  console.log('\nReceived SIGINT. Shutting down gracefully...');
  process.exit(0);
});

process.on('SIGTERM', () => {
  console.log('\nReceived SIGTERM. Shutting down gracefully...');
  process.exit(0);
});

// Start the application without GUI
require('./app');