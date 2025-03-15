const express = require('express');
const router = express.Router();
const vm = require('vm');
const { requirePlayerOrServerAuth, requireServerAuth } = require('../auth/utils');
const discordClient = require('../discord/bot.js');
const { saveFunction, getFunction, deleteFunction } = require('../models/functions');
const messagesModel = require('../models/messages');
const logger = global.logger;
const { Message, MessageEmbed, MessageActionRow, MessageButton, MessageSelectMenu, Interaction, CommandInteraction, Guild, GuildMember, Role, Channel, TextChannel, DMChannel, VoiceChannel, StageChannel, Permissions, Collection, Constants, SnowflakeUtil, Util, MessageCollector, ReactionCollector, User } = require('discord.js');


const safeDiscord = {Message, MessageEmbed, MessageActionRow, MessageButton, MessageSelectMenu, Interaction, CommandInteraction, Guild, GuildMember, Role, Channel, TextChannel, DMChannel, VoiceChannel, StageChannel, Permissions, Collection, Constants, SnowflakeUtil, Util, MessageCollector, ReactionCollector, User  };
/**
 * Creates a MongoDB collection instance for a given mod for sandbox DB access.
 * This function returns the mod-specific collection (named 'Functions_<mod>') along with the connected client.
 *
 * @param {string} mod - The mod identifier.
 * @returns {Promise<{ collection: any, client: any }>} The mod-specific collection and the client instance.
 */
async function getMongoCollection(mod) {
    const { MongoClient } = require('mongodb');
    const client = new MongoClient(global.config.DBServer);
    await client.connect();
    const db = client.db(global.config.DB);
    const collection = db.collection(`Functions_${mod}`);
    return { collection, client };
}

/**
 * Creates a safe restricted resource using a Proxy.
 * If access to the resource is not allowed, any property access will throw an error.
 *
 * @param {any} resource - The resource to restrict.
 * @param {boolean} allowed - Flag indicating whether access is permitted.
 * @param {string} resourceName - The name of the resource (used in error messages).
 * @returns {any} The original resource if allowed; otherwise, a restricted proxy.
 */
function createRestrictedResource(resource, allowed, resourceName) {
    if (allowed) return resource;
    return new Proxy({}, {
        get() {
            throw new Error(`${resourceName} access not enabled`);
        },
    });
}
/**
 * Wraps the provided logger so that every log message is prefixed with "[mod.functionName]".
 *
 * @param {object} logger - The original logger object.
 * @param {string} mod - The mod identifier.
 * @param {string} functionName - The function name.
 * @returns {object} A wrapped logger that prepends the prefix.
 */
function createPrefixedLogger(logger, mod, functionName) {
    const prefix = `[${mod}.${functionName}]`;
    return {
      info: (...args) => logger.info(prefix, ...args),
      error: (...args) => logger.error(prefix, ...args),
      warn: (...args) => logger.warn(prefix, ...args),
      debug: (...args) => logger.debug(prefix, ...args)
    };
  }


/**
 * Wraps a user-submitted code snippet with dynamic input destructuring.
 *
 * This function takes an input object and a user-provided code snippet, and then generates
 * an asynchronous function wrapper. The wrapper automatically destructures the keys from the
 * input object so that the user code can reference each property directly.
 *
 * @param {Object} input - The input object containing key-value pairs (e.g., { foo: 'value1', bar: 'value2' }).
 * @param {string} userSnippet - The user-provided code snippet to execute.
 * @returns {string} A string representing an asynchronous function that takes (input, env)
 * and destructures the input, then executes the user code snippet.
 *
 * @example
 * // Given an input object and a snippet:
 * const input = { foo: 'Hello', bar: 'World' };
 * const snippet = "return foo + ' ' + bar;";
 * const wrappedCode = wrapUserCode(input, snippet);
 *
 * // The wrappedCode will be a string like:
 * // (async function(input, env) {
 * //   const { foo, bar } = input;
 * //   return foo + ' ' + bar;
 * // })
 *
 * // This wrapped function can then be evaluated and executed in your sandbox.
 */
function wrapUserCode(input, userSnippet) {
    const keys = Object.keys(input);
    return `
      (async function(input, env) {
        // Create a spread object from input.
        const { ${keys.join(', ')} } = input;
        // Execute the user's code snippet.
        ${userSnippet}
      })
    `;
  }

/**
 * Prepares the sandbox environment for function execution.
 * This sandbox includes only the permitted resources (e.g., DB access, Discord bot) based on the mod's configuration.
 *
 * @param {object} req - The Express request object.
 * @param {string} mod - The mod identifier.
 * @returns {object} The sandbox environment containing restricted resources and the function input.
 */
function getSandboxEnv(req, mod) {
    // Retrieve mod-specific configuration from global settings (if available)
    const modConfig = (global.config.functions && global.config.functions[mod]) || {};
    const allowDB = modConfig.AllowDB;
    const allowDSBot = modConfig.AllowDiscordBot;
    const allowMsg = modConfig.AllowMsgQueue;
    const functionName = req.params?.FunctionName;
    return {    
        // Provide DB access as a helper. User functions can call env.db.getCollection().
        // The returned object includes a close() method that is auto-called after 5 seconds.
        db: createRestrictedResource({
            async getCollection() {
              const { collection, client } = await getMongoCollection(mod);
              let closed = false;
              // Wrap close to ensure it's only called once.
              const originalClose = async () => {
                if (!closed) {
                  closed = true;
                  await client.close();
                }
              };
              // Automatically close connection after 3.5 seconds if not explicitly closed.
              const timeoutId = setTimeout(() => {
                logger.warn("Auto-closing DB connection, collection access not explicitly closed", {mod, functionName});
                originalClose().catch(err => logger.error("Auto-closing DB connection failed:", err));
              }, 3500);
              return {
                collection,
                close: async () => {
                  clearTimeout(timeoutId);
                  return originalClose();
                }
              };
            }
          }, allowDB, 'DB'),
        // Indicates whether the request comes from a server (set by authentication middleware)
        isServer: req.params?.isServer,
        // GUID identifies the caller; it is null for server requests.
        GUID: req.params?.GUID || null,
        // Global logger for logging purposes.
        logger: createPrefixedLogger(global.logger, mod, functionName),
        // Provide restricted access to the Discord bot.
        dsbot: createRestrictedResource(discordClient, allowDSBot, 'Discord Bot'),
        msg: createRestrictedResource(async function(queue, message) {
                const actor = req.params?.isServer ? "Server" : req.params?.GUID;
                return await messagesModel.insertMessage(mod, queue, actor, message);
            }, allowMsg, 'Message Queue'),
        // Input provided by the caller, available to the executed function.
        fetch: fetch,
        setTimeout: setTimeout,
        setInterval: setInterval,
        discordJS: safeDiscord,
        input: req.body
    };
}

/**
 * Registers a new function.
 * Endpoint: POST /Functions/:Mod/RegisterFunction
 * This route saves a new function definition into the database.
 */
router.post('/Functions/:Mod/RegisterFunction', requireServerAuth, runRegisterFunction);

/**
 * Handles the registration of a new function.
 *
 * @param {object} req - The Express request object.
 * @param {object} res - The Express response object.
 */
async function runRegisterFunction(req, res) {
    const { Mod } = req.params;
    const { code, functionName, inputSchema, permission } = req.body;
    
    // Validate required fields.
    if (!code || !functionName) {
        logger.warn('Missing required fields for function registration', { mod: Mod });
        return res.status(400).json({ Status: 'Error', Error: 'Missing required fields' });
    }

    try {
        // Save the function using the model helper.
        await saveFunction(Mod, { functionName, code, inputSchema, permission });
        logger.info('Function registered successfully', { mod: Mod, functionName });
        res.json({ Status: 'Success' });
    } catch (error) {
        logger.error('Error registering function', { error });
        res.json({ Status: 'Error', Error: error.message });
    }
}

/**
 * Calls an existing function.
 * Endpoint: POST /Functions/Call/:Mod/:FunctionName
 * This route retrieves the function definition, validates the input against an optional schema, and executes it in a sandbox.
 */
router.post('/Functions/Call/:Mod/:FunctionName', requirePlayerOrServerAuth, runFunction);

/**
 * Handles the execution of a registered function in a secure sandbox.
 *
 * @param {object} req - The Express request object.
 * @param {object} res - The Express response object.
 */
async function runFunction(req, res) {
    const { Mod, FunctionName } = req.params;
    logger.debug('Processing function call', { mod: Mod, functionName: FunctionName });
    try {
        // Retrieve the function definition from the database.
        const funcRecord = await getFunction(Mod, FunctionName);
        if (!funcRecord) {
            logger.warn('Function not found', { mod: Mod, functionName: FunctionName });
            return res.status(404).json({ Status: 'Error', Error: 'Function not found' });
        }
        const { permission, inputSchema } = funcRecord;
        
        // Enforce permission: if the function is server-only and the request isn't from the server, reject the call.
        if (permission === 0 && !req.isServer) {
            logger.warn('Permission denied for function call', { mod: Mod, functionName: FunctionName });
            return res.status(403).json({ Status: 'Rejected', Error: 'Permission denied, this function is set to only run from the server' });
        }
        
        // Validate input using Ajv if an inputSchema is provided.
        if (inputSchema && Object.keys(inputSchema).length > 0) {
            const Ajv = require('ajv');
            const ajv = new Ajv();
            const validate = ajv.compile(inputSchema);
            const valid = validate(req.body);
            if (!valid) {
                logger.warn('Input validation failed', { mod: Mod, functionName: FunctionName, errors: validate.errors });
                return res.json({
                    Status: 'Rejected',
                    Error: `Input validation failed: ${JSON.stringify(validate.errors)}`
                });
            }
        }
        
        // Prepare the sandbox environment with only the allowed resources.
        const sandbox = getSandboxEnv(req, Mod);
        // Expose a separate variable for convenience.
        sandbox.env = sandbox;

        let result;
        try {
            // Wrap the user function code with dynamic input destructuring.
            // This creates a string that represents an async function that looks like:
            // (async function(input, env) {
            //   const { foo, bar } = input;
            //   // ...user's code...
            // })
            const wrappedCode = wrapUserCode(req.body, funcRecord.code);
            
            // Create a new VM Script from the wrapped code.
            // We then immediately invoke the function with (input, env) in our sandbox.
            const script = new vm.Script(`(${wrappedCode})(input, env)`);
            const context = vm.createContext(sandbox);
            // Execute the code with a timeout to prevent runaway scripts.
            result = script.runInContext(context, { timeout: 3100 });
        } catch (execError) {
            logger.error('Error executing function', { mod: Mod, functionName: FunctionName, error: execError });
            return res.json({ Status: 'Error', Error: execError.message });
        }

        logger.info('Function executed successfully', { mod: Mod, functionName: FunctionName });
        res.json({ Status: 'Success', Return: result });
    } catch (error) {
        logger.error('Error calling function', { mod: Mod, functionName: FunctionName, error });
        res.json({ Status: 'Error', Error: error.message });
    }
}

/**
 * Deletes a registered function.
 * Endpoint: DELETE /Functions/Delete/:Mod/:FunctionName
 * This route removes a function definition from the database.
 */
router.post('/Functions/Delete/:Mod/:FunctionName', requireServerAuth, runDeleteFunction);

/**
 * Handles the deletion of a function.
 *
 * @param {object} req - The Express request object.
 * @param {object} res - The Express response object.
 */
async function runDeleteFunction(req, res) {
    const { Mod, FunctionName } = req.params;
    logger.debug('Processing function deletion', { mod: Mod, functionName: FunctionName });
    try {
        // Delete the function using the model helper.
        const deletionResult = await deleteFunction(Mod, FunctionName);
        if (deletionResult.deletedCount === 0) {
            logger.warn('Function not found for deletion', { mod: Mod, functionName: FunctionName });
            return res.status(404).json({ Status: 'Error', Error: 'Function not found' });
        }
        logger.info('Function deleted successfully', { mod: Mod, functionName: FunctionName });
        res.json({ Status: 'Success' });
    } catch (error) {
        logger.error('Error deleting function', { mod: Mod, functionName: FunctionName, error });
        res.json({ Status: 'Error', Error: error.message });
    }
}

/**
 * Checks if a function is registered.
 * Endpoint: GET /Functions/Check/:Mod/:FunctionName
 * This route retrieves a function definition from the database and returns metadata if found.
 */
router.post('/Functions/Check/:Mod/:FunctionName', requireServerAuth, runCheckFunction);

/**
 * Handles checking if a function is registered.
 *
 * @param {object} req - The Express request object.
 * @param {object} res - The Express response object.
 */
async function runCheckFunction(req, res) {
    const { Mod, FunctionName } = req.params;
    logger.debug('Checking if function is registered', { mod: Mod, functionName: FunctionName });
    try {
        // Retrieve the function definition from the database.
        const funcRecord = await getFunction(Mod, FunctionName);
        if (!funcRecord) {
            logger.warn('Function not found', { mod: Mod, functionName: FunctionName });
            return res.status(404).json({ Status: 'Error', Error: 'Function not found' });
        }

        // Return basic metadata about the registered function.
        logger.info('Function is registered', { mod: Mod, functionName: FunctionName });
        res.json({Status: 'Success', Error: ''});
    } catch (error) {
        logger.error('Error checking function', { mod: Mod, functionName: FunctionName, error });
        res.json({ Status: 'Error', Error: error.message });
    }
}

module.exports = router;
