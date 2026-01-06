/**
 * One-time script to ensure all KB documents have embeddings
 * 
 * Run with: node ensure-embeddings.js
 * 
 * This script:
 * 1. Lists all KBs
 * 2. Finds any documents/chunks missing embeddings
 * 3. Generates embeddings for them
 * 
 * This runs independently of the server.
 */

// Initialize globals needed by modules
global.SAVEPATH = './';

// Initialize logger
const logModule = require('./log');
global.logger = logModule.initializeLogger();

// Load config
global.config = require('./configLoader');

const kbModel = require('./models/kb');
const kbController = require('./controllers/kb');

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

async function main() {
    log('\n╔══════════════════════════════════════════╗', 'cyan');
    log('║   ENSURE ALL KB EMBEDDINGS               ║', 'cyan');
    log('║   One-time embedding generation script   ║', 'cyan');
    log('╚══════════════════════════════════════════╝', 'cyan');

    try {
        // Step 1: List all KBs
        log('\n1. Listing all Knowledge Bases...', 'dim');
        const kbs = await kbModel.listKBs();
        
        if (kbs.length === 0) {
            log('No Knowledge Bases found.', 'yellow');
            process.exit(0);
        }
        
        log(`Found ${kbs.length} KB(s):`, 'green');
        kbs.forEach(kb => log(`  - ${kb.kbId}: ${kb.name} (${kb.documentCount} docs)`, 'dim'));

        // Step 2: Check each KB for missing embeddings
        log('\n2. Checking for missing embeddings...', 'dim');
        
        let totalDocsChecked = 0;
        let totalMissing = 0;
        let totalFixed = 0;
        let totalFailed = 0;

        for (const kb of kbs) {
            log(`\n   Checking KB: ${kb.kbId}`, 'cyan');
            
            const missingChunks = await kbModel.getDocumentsMissingEmbeddings(kb.kbId);
            
            if (missingChunks.length === 0) {
                log(`   ✓ All documents have embeddings`, 'green');
                continue;
            }

            // Group by documentId
            const byDoc = {};
            for (const chunk of missingChunks) {
                if (!byDoc[chunk.documentId]) byDoc[chunk.documentId] = [];
                byDoc[chunk.documentId].push(chunk);
            }

            const docCount = Object.keys(byDoc).length;
            totalDocsChecked += docCount;
            totalMissing += docCount;
            
            log(`   Found ${docCount} document(s) missing embeddings (${missingChunks.length} chunks)`, 'yellow');

            // Step 3: Generate embeddings for each document
            for (const [documentId, chunks] of Object.entries(byDoc)) {
                try {
                    chunks.sort((a, b) => a.chunkIndex - b.chunkIndex);
                    const contents = chunks.map(c => c.content);
                    
                    log(`   Generating ${contents.length} embedding(s) for: ${chunks[0].name}...`, 'dim');
                    
                    const embeddings = await kbController.generateEmbeddings(contents);
                    await kbModel.updateDocumentEmbeddings(kb.kbId, documentId, embeddings);
                    
                    totalFixed++;
                    log(`   ✓ ${chunks[0].name} - ${embeddings.length} embedding(s) created`, 'green');
                } catch (err) {
                    totalFailed++;
                    log(`   ✗ ${chunks[0]?.name || documentId} - Failed: ${err.message}`, 'red');
                }
            }
        }

        // Summary
        log('\n========================================', 'cyan');
        log('SUMMARY', 'cyan');
        log('========================================', 'cyan');
        
        if (totalMissing === 0) {
            log('✓ All documents in all KBs have embeddings!', 'green');
        } else {
            log(`Documents checked: ${totalDocsChecked}`, 'dim');
            log(`Missing embeddings: ${totalMissing}`, 'yellow');
            log(`Successfully fixed: ${totalFixed}`, 'green');
            if (totalFailed > 0) {
                log(`Failed: ${totalFailed}`, 'red');
            }
        }
        
        log('\n✓ Done!', 'green');
        process.exit(totalFailed > 0 ? 1 : 0);
        
    } catch (err) {
        log(`\n✗ Error: ${err.message}`, 'red');
        console.error(err.stack);
        process.exit(1);
    }
}

main();
