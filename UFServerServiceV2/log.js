/**
 * Logging module using Winston logger
 * 
 * Example usage of the logger:
 * 
 * // In your main application file:
 * const logger = require('./log.js').initializeLogger();
 * global.logger = logger; // Make it available globally if needed
 * 
 * // Basic logging at different levels
 * logger.error('Critical error occurred', { error: err });
 * logger.warn('Warning: resource usage high', { memoryUsage: process.memoryUsage() });
 * logger.info('Server started successfully', { port: 3000 });
 * logger.debug('Processing request', { method: 'GET', path: '/api/users' });
 */

const winston = require('winston');
const path = require('path');
const DailyRotateFile = require('winston-daily-rotate-file');
const { Writable } = require('stream');

/**
 * Initializes and configures the Winston logger
 * @returns {winston.Logger} Configured logger instance
 */
function initializeLogger() {
  // Ensure global SAVEPATH is set; default to current directory if not
  const savePath = global.SAVEPATH || '.';

  // Create an in-memory log history to cache log messages.
  // This global array will be used to send old logs when the console window opens.
  global.logHistory = [];

  // Create a writable stream that caches logs in the global.logHistory array.
  const logStream = new Writable({
    write(chunk, encoding, callback) {
      const message = chunk.toString();
      global.logHistory.push(message);
      // (Optional) Limit history size, for example to last 1000 messages:
      if (global.logHistory.length > 1000) {
        global.logHistory.shift();
      }
      callback();
    }
  });

  // Determine default log level:
  // - Dev (plain Node, or `electron .` via npm run start): default to 'debug'
  // - Packaged Electron app or pkg binary: use config or default to 'info'
  // process.defaultApp is true when running `electron .` (dev), undefined in packaged builds
  const isPackaged = Boolean(process.pkg || (process.versions?.electron && !process.defaultApp));
  const logLevel = isPackaged ? (global.config?.LogLevel || 'info') : 'debug';

  console.log(`Log level set to: ${logLevel}`); // Log the determined log level
  // Create Winston logger with base configuration
  const logger = winston.createLogger({
    level: logLevel,
    // Configure default log format with timestamp and JSON structure
    format: winston.format.combine(
      winston.format.timestamp({ format: () => new Date().toLocaleString() }),
      winston.format.json()
    ),
    // Set up transports (output destinations)
    transports: [
      // Console transport with colorized output for better readability
      new winston.transports.Console({
        format: winston.format.combine(
          winston.format.colorize(),
          winston.format.simple()
        )
      })
    ]
  });

  // Add a daily rotating file transport
  logger.add(new DailyRotateFile({
    filename: path.join(savePath, 'logs', 'UF-%DATE%.log'),
    datePattern: 'YYYY-MM-DD',
    zippedArchive: false,
    maxSize: '128m',
    maxFiles: '30d',
    level: 'info'
  }));

  // Add the stream transport to capture logs in the in-memory cache.
  logger.add(new winston.transports.Stream({ stream: logStream }));

  return logger;
}

module.exports = { initializeLogger };
