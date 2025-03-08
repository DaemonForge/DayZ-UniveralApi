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
 * 
 * // Logging with metadata
 * logger.info('User authentication', { 
 *   userId: '123',
 *   action: 'login',
 *   ipAddress: '192.168.1.1'
 * });
 * 
 * // Usage in error handling
 * try {
 *   // some code that might throw
 * } catch (error) {
 *   logger.error('Failed to process request', {
 *     error: error.message,
 *     stack: error.stack,
 *     context: 'userService'
 *   });
 * }
 */
const winston = require('winston');
const path = require('path');

/**
 * Initializes and configures the Winston logger
 * @returns {winston.Logger} Configured logger instance
 */
function initializeLogger() {
    // Generate current date for log filename
    const date = new Date().toISOString().slice(0, 10);
    // Create log file path using global save path or default to current directory
    const logfilename = path.join(global.SAVEPATH || '.', 'logs', `api-${date}.log`);
    
    // Create Winston logger with base configuration
    const logger = winston.createLogger({
        // Use log level from global config or default to 'info'
        level: global.config?.LogLevel || 'info',
        // Configure default log format with timestamp and JSON structure
        format: winston.format.combine(
            winston.format.timestamp(),
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
    
    // Add file transport if enabled in configuration
    if (global.config?.LogToFile) {
        logger.add(new winston.transports.File({ 
            filename: logfilename,
            format: winston.format.combine(
                winston.format.timestamp(),
                winston.format.json()
            )
        }));
    }
    
    return logger;
}


module.exports = { initializeLogger };
