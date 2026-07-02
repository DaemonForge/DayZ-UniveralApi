/**
 * Self-check for aiClient.js compat-mode adapter (Responses API <-> Chat Completions).
 * No network or server needed. Run with: node test-ai-client.js
 */
const assert = require('assert');

global.config = { OpenAIApi: { ApiKey: 'test', BaseURL: 'http://localhost:9999/v1', DefaultModel: 'my-model' } };
const { createResponse, isCompatMode, getDefaultModel, getEmbeddingModel } = require('./aiClient');

assert.strictEqual(isCompatMode(), true);
assert.strictEqual(getDefaultModel(), 'my-model');
assert.strictEqual(getEmbeddingModel(), 'text-embedding-3-large');

function mockClient(reply) {
    const captured = {};
    return {
        captured,
        chat: { completions: { create: async (body) => { captured.body = body; return reply; } } },
        responses: { create: async () => { throw new Error('Responses API must not be called in compat mode'); } }
    };
}

(async () => {
    // 1) Full round-trip: instructions, messages, prior tool call + result, flat tools
    let client = mockClient({
        choices: [{ message: { content: 'final answer', tool_calls: undefined } }],
        usage: { total_tokens: 10 }
    });
    let res = await createResponse(client, {
        model: 'my-model',
        instructions: 'be terse',
        input: [
            { type: 'message', role: 'user', content: 'hi' },
            { type: 'function_call', call_id: 'call_1', name: 'lookup', arguments: '{"q":"x"}' },
            { type: 'function_call_output', call_id: 'call_1', output: 'result-x' },
            { type: 'message', role: 'assistant', content: [{ type: 'output_text', text: 'ok' }] }
        ],
        tools: [{ type: 'function', name: 'lookup', description: 'd', parameters: { type: 'object', properties: {} } }],
        reasoning: { effort: 'medium' } // must be dropped
    });
    const b = client.captured.body;
    assert.deepStrictEqual(b.messages[0], { role: 'system', content: 'be terse' });
    assert.deepStrictEqual(b.messages[1], { role: 'user', content: 'hi' });
    assert.deepStrictEqual(b.messages[2], {
        role: 'assistant', content: null,
        tool_calls: [{ id: 'call_1', type: 'function', function: { name: 'lookup', arguments: '{"q":"x"}' } }]
    });
    assert.deepStrictEqual(b.messages[3], { role: 'tool', tool_call_id: 'call_1', content: 'result-x' });
    assert.deepStrictEqual(b.messages[4], { role: 'assistant', content: 'ok' }); // array content flattened
    assert.strictEqual(b.tools[0].type, 'function');
    assert.strictEqual(b.tools[0].function.name, 'lookup');
    assert.strictEqual(b.reasoning, undefined);
    assert.strictEqual(b.response_format, undefined);
    assert.strictEqual(res.output_text, 'final answer');
    assert.deepStrictEqual(res.output, [{ type: 'message', role: 'assistant', content: 'final answer' }]);
    assert.strictEqual(res.status, 'completed');

    // 2) Tool-call reply converts to function_call output items
    client = mockClient({
        choices: [{ message: { content: null, tool_calls: [{ id: 'call_2', type: 'function', function: { name: 'lookup', arguments: '{"q":"y"}' } }] } }]
    });
    res = await createResponse(client, { model: 'my-model', input: 'plain string question' });
    assert.deepStrictEqual(client.captured.body.messages, [{ role: 'user', content: 'plain string question' }]);
    assert.deepStrictEqual(res.output[0], { type: 'function_call', id: 'call_2', call_id: 'call_2', name: 'lookup', arguments: '{"q":"y"}' });
    assert.strictEqual(res.output_text, '');

    // 3) Adapter output items feed back in cleanly (as the KB loop does)
    client = mockClient({ choices: [{ message: { content: 'done' } }] });
    await createResponse(client, {
        model: 'my-model',
        input: [
            { type: 'message', role: 'user', content: 'hi' },
            ...res.output,
            { type: 'function_call_output', call_id: 'call_2', output: 'kb result' }
        ]
    });
    assert.strictEqual(client.captured.body.messages.length, 3); // user, assistant tool_calls, tool

    // 4) Model returns text AND tool calls in one turn: message item first,
    //    and on feed-back they merge so the tool result directly follows the
    //    message carrying tool_calls (strict providers require this).
    client = mockClient({
        choices: [{ message: { content: 'let me check', tool_calls: [{ id: 'call_3', type: 'function', function: { name: 'lookup', arguments: '{}' } }] } }]
    });
    res = await createResponse(client, { model: 'my-model', input: 'question' });
    assert.deepStrictEqual(res.output.map(o => o.type), ['message', 'function_call']);
    assert.strictEqual(res.output_text, '');

    client = mockClient({ choices: [{ message: { content: 'done' } }] });
    await createResponse(client, {
        model: 'my-model',
        input: [
            { type: 'message', role: 'user', content: 'question' },
            ...res.output,
            { type: 'function_call_output', call_id: 'call_3', output: 'kb result' }
        ]
    });
    let msgs = client.captured.body.messages;
    assert.strictEqual(msgs.length, 3); // user, assistant(content+tool_calls), tool
    assert.strictEqual(msgs[1].role, 'assistant');
    assert.strictEqual(msgs[1].content, 'let me check');
    assert.strictEqual(msgs[1].tool_calls[0].id, 'call_3');
    assert.strictEqual(msgs[2].role, 'tool'); // directly follows tool_calls

    // 5) Sampling params survive translation (used by KB shorter-answers extraction)
    client = mockClient({ choices: [{ message: { content: 'extracted' } }] });
    await createResponse(client, { model: 'my-model', input: 'q', temperature: 0.3, max_output_tokens: 2000 });
    assert.strictEqual(client.captured.body.temperature, 0.3);
    assert.strictEqual(client.captured.body.max_tokens, 2000);
    assert.strictEqual(client.captured.body.max_output_tokens, undefined);

    console.log('aiClient self-check passed');
})().catch(err => { console.error('aiClient self-check FAILED:', err.message); process.exit(1); });
