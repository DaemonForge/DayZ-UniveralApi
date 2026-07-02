// aiClient.js
// Shared OpenAI client factory with support for OpenAI-compatible providers
// (Vultr Serverless Inference, Cloudflare Workers AI, Ollama, vLLM, etc.)
// via config.OpenAIApi.BaseURL.
//
// When BaseURL is empty, everything behaves exactly as before (api.openai.com,
// Responses API). When BaseURL is set, chat requests are translated to the
// Chat Completions API — the standard implemented by compatible providers,
// which do not implement the Responses API.
const { OpenAI } = require('openai').default;

function aiConfig() {
    return (global.config && global.config.OpenAIApi) || {};
}

// A custom BaseURL means "OpenAI-compatible provider mode"
function isCompatMode() {
    return !!aiConfig().BaseURL;
}

function getDefaultModel() {
    return aiConfig().DefaultModel || 'gpt-4o-mini';
}

function getEmbeddingModel() {
    return aiConfig().EmbeddingModel || 'text-embedding-3-large';
}

function createClient() {
    const cfg = aiConfig();
    return new OpenAI({
        // openai v6 throws on a missing/empty apiKey; compat providers
        // (Ollama, vLLM) often need none, so send a placeholder they ignore.
        apiKey: cfg.ApiKey || (cfg.BaseURL ? 'not-needed' : cfg.ApiKey),
        ...(cfg.BaseURL ? { baseURL: cfg.BaseURL } : {})
    });
}

/**
 * Executes a Responses-API-shaped request ({ model, instructions, input,
 * tools, ... }) and returns a Responses-shaped result
 * ({ output, output_text, status, usage }).
 *
 * Against OpenAI this calls the Responses API directly. In compat mode it
 * translates the request to Chat Completions and the completion back into
 * Responses output items, so callers don't need to know the difference.
 */
async function createResponse(client, reqBody) {
    if (!isCompatMode()) {
        return client.responses.create(reqBody);
    }

    const messages = [];
    if (reqBody.instructions) {
        messages.push({ role: 'system', content: reqBody.instructions });
    }

    const input = typeof reqBody.input === 'string'
        ? [{ type: 'message', role: 'user', content: reqBody.input }]
        : (reqBody.input || []);

    // Chat Completions groups tool calls on a single assistant message,
    // so consecutive function_call items are merged into one. If the calls
    // directly follow an assistant text message (a model that "thinks out
    // loud" before calling a tool), attach them to that message: strict
    // providers require tool results to immediately follow the message
    // carrying tool_calls.
    let pendingToolCalls = [];
    const flushToolCalls = () => {
        if (pendingToolCalls.length === 0) return;
        const last = messages[messages.length - 1];
        if (last && last.role === 'assistant' && !last.tool_calls) {
            last.tool_calls = pendingToolCalls;
        } else {
            messages.push({ role: 'assistant', content: null, tool_calls: pendingToolCalls });
        }
        pendingToolCalls = [];
    };

    for (const item of input) {
        if (item.type === 'function_call') {
            pendingToolCalls.push({
                id: item.call_id,
                type: 'function',
                function: { name: item.name, arguments: item.arguments || '{}' }
            });
            continue;
        }
        flushToolCalls();
        if (item.type === 'function_call_output') {
            messages.push({ role: 'tool', tool_call_id: item.call_id, content: item.output || '' });
        } else if (item.type === 'message') {
            const content = Array.isArray(item.content)
                ? item.content.map(c => c.text || '').join('')
                : item.content;
            messages.push({ role: item.role, content });
        }
        // Other item types (e.g. reasoning) have no Chat Completions equivalent.
    }
    flushToolCalls();

    const ccBody = {
        model: reqBody.model,
        messages,
        ...(reqBody.tools && reqBody.tools.length > 0 ? {
            // Responses API tools are flat; Chat Completions nests them under 'function'
            tools: reqBody.tools.map(t => ({
                type: 'function',
                function: { name: t.name, description: t.description, parameters: t.parameters }
            })),
            tool_choice: 'auto'
        } : {}),
        ...(reqBody.temperature !== undefined ? { temperature: reqBody.temperature } : {}),
        // 'max_tokens' is the name every compat provider accepts
        ...(reqBody.max_output_tokens !== undefined ? { max_tokens: reqBody.max_output_tokens } : {})
        // reasoning and text.format are intentionally dropped: they are not
        // portable across open-source providers. JSON output is already
        // enforced by the schema system message + parse/retry loop in aiChat.
    };

    const completion = await client.chat.completions.create(ccBody);
    const msg = (completion.choices && completion.choices[0] && completion.choices[0].message) || {};

    // Message item goes before any function_call items so that when these
    // items are fed back as input (the KB loop does this), the conversion
    // above can re-attach the tool calls to the text message they came with.
    const output = [];
    if (msg.content) {
        output.push({ type: 'message', role: 'assistant', content: msg.content });
    }
    if (Array.isArray(msg.tool_calls)) {
        for (const tc of msg.tool_calls) {
            output.push({
                type: 'function_call',
                id: tc.id,
                call_id: tc.id,
                name: tc.function && tc.function.name,
                arguments: (tc.function && tc.function.arguments) || '{}'
            });
        }
    }

    return {
        output,
        output_text: (Array.isArray(msg.tool_calls) && msg.tool_calls.length > 0) ? '' : (msg.content || ''),
        status: 'completed',
        usage: completion.usage
    };
}

module.exports = { createClient, createResponse, isCompatMode, getDefaultModel, getEmbeddingModel };
