/**
 * AI Chat API Test Script
 * 
 * Tests the AI Chat endpoints to verify the Responses API integration.
 * Run with: node test-ai-chat.js
 * 
 * Requires: The service to be running (npm run start)
 */

const http = require('http');
const https = require('https');

// Configuration
const CONFIG = {
    host: '127.0.1.1',
    port: 443,
    protocol: 'https',
    // Get this from your config.json ServerAuthKeys
    serverAuthToken: 's~VxS2mgjFvHN~grJDAEQLzcQI5G~cdxLEUNGu8sr516YEBA',
    kbId: null // Set via --kb flag
};

// Colors for console output
const colors = {
    reset: '\x1b[0m',
    green: '\x1b[32m',
    red: '\x1b[31m',
    yellow: '\x1b[33m',
    cyan: '\x1b[36m',
    dim: '\x1b[2m'
};

function log(msg, color = 'reset') {
    console.log(`${colors[color]}${msg}${colors.reset}`);
}

function logJson(label, obj) {
    console.log(`${colors.cyan}${label}:${colors.reset}`);
    console.log(colors.dim + JSON.stringify(obj, null, 2) + colors.reset);
}

/**
 * Make an HTTP request to the AI Chat API
 */
function makeRequest(method, path, body = null) {
    return new Promise((resolve, reject) => {
        const options = {
            hostname: CONFIG.host,
            port: CONFIG.port,
            path: `/AI/Chat${path}`,
            method: method,
            rejectUnauthorized: false, // Allow self-signed certificates
            headers: {
                'Content-Type': 'application/json',
                'auth-key': CONFIG.serverAuthToken
            }
        };

        log(`\n${colors.yellow}>>> ${method} ${options.path}${colors.reset}`);
        if (body) {
            logJson('Request Body', body);
        }

        const client = CONFIG.protocol === 'https' ? https : http;
        const req = client.request(options, (res) => {
            let data = '';
            res.on('data', chunk => data += chunk);
            res.on('end', () => {
                log(`<<< Status: ${res.statusCode}`, res.statusCode === 200 || res.statusCode === 201 ? 'green' : 'red');
                try {
                    const json = JSON.parse(data);
                    logJson('Response', json);
                    resolve({ status: res.statusCode, data: json });
                } catch (e) {
                    log(`Raw Response: ${data}`, 'dim');
                    resolve({ status: res.statusCode, data: data });
                }
            });
        });

        req.on('error', (e) => {
            log(`Request Error: ${e.message}`, 'red');
            reject(e);
        });

        if (body) {
            req.write(JSON.stringify(body));
        }
        req.end();
    });
}

/**
 * Test 1: Create a new chat session
 */
async function testCreateChat() {
    log('\n========================================', 'cyan');
    log('TEST 1: Create Chat Session', 'cyan');
    log('========================================', 'cyan');

    const result = await makeRequest('POST', '/Create', {
        SystemMessage: 'You are a helpful assistant for testing. Keep responses very short (under 50 words).',
        ResponseFormat: 'string',
        Model: 'gpt-4o-mini',
        MaxHistory: 10
    });

    if (result.data.Status === 'Success' && result.data.ChatId) {
        log(`✓ Chat created successfully! ChatId: ${result.data.ChatId}`, 'green');
        return result.data.ChatId;
    } else {
        log(`✗ Failed to create chat: ${JSON.stringify(result.data)}`, 'red');
        return null;
    }
}

/**
 * Test 2: Create a chat with Knowledge Base
 */
async function testCreateChatWithKB(kbId = 'test-kb') {
    log('\n========================================', 'cyan');
    log('TEST 2: Create Chat with Knowledge Base', 'cyan');
    log('========================================', 'cyan');

    const result = await makeRequest('POST', '/Create', {
        SystemMessage: 'You are a helpful assistant with access to a knowledge base.',
        ResponseFormat: 'string',
        Model: 'gpt-4o-mini',
        MaxHistory: 10,
        KBId: kbId
    });

    if (result.data.Status === 'Success' && result.data.ChatId) {
        log(`✓ Chat with KB created! ChatId: ${result.data.ChatId}`, 'green');
        return result.data.ChatId;
    } else {
        log(`✗ Failed to create chat with KB: ${JSON.stringify(result.data)}`, 'red');
        return null;
    }
}

/**
 * Test 3: Send a message to a chat
 */
async function testSendMessage(chatId, message = 'Hello! What is 2 + 2?') {
    log('\n========================================', 'cyan');
    log('TEST 3: Send Message', 'cyan');
    log('========================================', 'cyan');

    const result = await makeRequest('POST', `/Send/${chatId}`, {
        Message: message
    });

    if (result.data.Status === 'Pending' && result.data.MessageId) {
        log(`✓ Message sent! MessageId: ${result.data.MessageId} (Pending)`, 'green');
        return result.data.MessageId;
    } else if (result.data.Status === 'Success') {
        log(`✓ Message processed immediately!`, 'green');
        log(`Response: ${result.data.Message}`, 'cyan');
        return null; // No polling needed
    } else {
        log(`✗ Failed to send message: ${JSON.stringify(result.data)}`, 'red');
        return null;
    }
}

/**
 * Test 4: Poll for message status
 */
async function testMessageStatus(messageId, maxPolls = 30) {
    log('\n========================================', 'cyan');
    log('TEST 4: Poll Message Status', 'cyan');
    log('========================================', 'cyan');

    for (let i = 0; i < maxPolls; i++) {
        log(`\nPoll ${i + 1}/${maxPolls}...`, 'dim');
        
        const result = await makeRequest('POST', `/MessageStatus/${messageId}`);

        if (result.data.Status === 'Success') {
            log(`✓ AI Response received!`, 'green');
            log(`Message: ${result.data.Message}`, 'cyan');
            return result.data;
        } else if (result.data.Status === 'Pending' || result.data.Status === 'Wait') {
            log(`Status: ${result.data.Status} - waiting...`, 'yellow');
            await new Promise(r => setTimeout(r, 1000));
        } else if (result.data.Status === 'ToolCall') {
            log(`Tool Call requested: ${result.data.ToolName}`, 'yellow');
            return result.data;
        } else {
            log(`✗ Unexpected status: ${result.data.Status}`, 'red');
            return result.data;
        }
    }

    log(`✗ Timeout waiting for response after ${maxPolls} polls`, 'red');
    return null;
}

/**
 * Test 5: Get chat history
 */
async function testGetHistory(chatId) {
    log('\n========================================', 'cyan');
    log('TEST 5: Get Chat History', 'cyan');
    log('========================================', 'cyan');

    const result = await makeRequest('POST', `/Read/${chatId}`);

    if (result.data.Status === 'Success') {
        log(`✓ Chat history retrieved! Messages: ${result.data.Messages?.length || 0}`, 'green');
        return result.data;
    } else {
        log(`✗ Failed to get history: ${JSON.stringify(result.data)}`, 'red');
        return null;
    }
}

/**
 * Test 6: Send message with context
 */
async function testSendWithContext(chatId) {
    log('\n========================================', 'cyan');
    log('TEST 6: Send Message with Context', 'cyan');
    log('========================================', 'cyan');

    const result = await makeRequest('POST', `/Send/${chatId}`, {
        Message: 'Based on the player info, what should I do next?',
        Context: [
            {
                Description: 'Player Status',
                Context: [
                    'Health: 75%',
                    'Hunger: 50%',
                    'Location: Chernarus, near Elektro'
                ]
            },
            {
                Description: 'Inventory',
                Context: [
                    'M4A1 Rifle (30 rounds)',
                    'Can of beans',
                    'Bandage x2'
                ]
            }
        ]
    });

    if (result.data.Status === 'Pending' && result.data.MessageId) {
        log(`✓ Message with context sent! MessageId: ${result.data.MessageId}`, 'green');
        return result.data.MessageId;
    } else if (result.data.Status === 'Success') {
        log(`✓ Message processed!`, 'green');
        return null;
    } else {
        log(`✗ Failed: ${JSON.stringify(result.data)}`, 'red');
        return null;
    }
}

/**
 * Test 7: Delete chat
 */
async function testDeleteChat(chatId) {
    log('\n========================================', 'cyan');
    log('TEST 7: Delete Chat', 'cyan');
    log('========================================', 'cyan');

    const result = await makeRequest('POST', `/Delete/${chatId}`);

    if (result.data.Status === 'Success') {
        log(`✓ Chat deleted successfully!`, 'green');
        return true;
    } else {
        log(`✗ Failed to delete: ${JSON.stringify(result.data)}`, 'red');
        return false;
    }
}

/**
 * Run all tests
 */
async function runAllTests() {
    log('\n╔══════════════════════════════════════════╗', 'cyan');
    log('║   AI CHAT API TEST SUITE                 ║', 'cyan');
    log('║   Testing OpenAI Responses API           ║', 'cyan');
    log('╚══════════════════════════════════════════╝', 'cyan');

    log(`\nTarget: ${CONFIG.protocol}://${CONFIG.host}:${CONFIG.port}`, 'dim');
    
    if (CONFIG.serverAuthToken === 'YOUR_SERVER_AUTH_TOKEN_HERE') {
        log('\n⚠️  WARNING: You need to set your server auth token in CONFIG!', 'red');
        log('Get it from your config.json ServerAuthKeys array.', 'yellow');
        return;
    }

    try {
        // Test 1: Create basic chat
        const chatId = await testCreateChat();
        if (!chatId) {
            log('\n❌ Cannot continue without a valid chat session', 'red');
            return;
        }

        // Test 3: Send a message
        const messageId = await testSendMessage(chatId);
        
        // Test 4: Poll for response (if needed)
        if (messageId) {
            await testMessageStatus(messageId);
        }

        // Test 6: Send with context
        const contextMsgId = await testSendWithContext(chatId);
        if (contextMsgId) {
            await testMessageStatus(contextMsgId);
        }

        // Test 5: Get history
        await testGetHistory(chatId);

        // Test 7: Cleanup
        await testDeleteChat(chatId);

        // Test with Knowledge Base if --kb flag was provided
        if (CONFIG.kbId) {
            log('\n========================================', 'yellow');
            log('KNOWLEDGE BASE TESTS', 'yellow');
            log('========================================', 'yellow');
            
            const kbChatId = await testCreateChatWithKB(CONFIG.kbId);
            if (kbChatId) {
                // Send a message that should trigger KB lookup for DayZ server info
                const kbMsgId = await testSendMessage(kbChatId, 'What are the rules on this DayZ server? Are there any restricted areas or safe zones?');
                if (kbMsgId) {
                    await testMessageStatus(kbMsgId);
                }
                await testGetHistory(kbChatId);
                await testDeleteChat(kbChatId);
            }
        }

        log('\n========================================', 'green');
        log('✓ ALL TESTS COMPLETED!', 'green');
        log('========================================', 'green');

    } catch (err) {
        log(`\n❌ Test failed with error: ${err.message}`, 'red');
        console.error(err);
    }
}

// Check for command line args for quick tests
const args = process.argv.slice(2);
if (args[0] === '--help') {
    console.log(`
AI Chat API Test Script
=======================

Usage: node test-ai-chat.js [options]

Options:
  --help          Show this help
  --token TOKEN   Set the auth token
  --host HOST     Set the host (default: localhost)
  --port PORT     Set the port (default: 3000)

Examples:
  node test-ai-chat.js --token abc123
  node test-ai-chat.js --host myserver.com --port 443 --token xyz
`);
    process.exit(0);
}

// Parse args
for (let i = 0; i < args.length; i++) {
    if (args[i] === '--token' && args[i+1]) {
        CONFIG.serverAuthToken = args[++i];
    } else if (args[i] === '--host' && args[i+1]) {
        CONFIG.host = args[++i];
    } else if (args[i] === '--port' && args[i+1]) {
        CONFIG.port = parseInt(args[++i]);
    } else if (args[i] === '--kb' && args[i+1]) {
        CONFIG.kbId = args[++i];
    }
}

// Run tests
runAllTests();
