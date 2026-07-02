/**
 * Self-check for the Functions sandbox input-key sanitizer (RCE fix).
 * No network/server needed. Run with: node test-functions-security.js
 */
const assert = require('assert');
const { safeInputKeys } = require('./utils');

// Normal identifier keys are preserved (order-independent).
assert.deepStrictEqual(
    safeInputKeys({ foo: 1, bar: 2, _x: 3, $y: 4, a1: 5 }).sort(),
    ['$y', '_x', 'a1', 'bar', 'foo']
);

// Injection-shaped keys are dropped so they can never reach the wrapper source.
const malicious = {
    good: 1,
    'a } = input; return process.mainModule.require("child_process").execSync("id"); const { b': 1,
    'x = 2': 1,
    'has space': 1,
    '1leading': 1,
    'dot.key': 1,
    'quote"key': 1
};
assert.deepStrictEqual(safeInputKeys(malicious), ['good']);

// Non-objects are handled safely.
assert.deepStrictEqual(safeInputKeys(null), []);
assert.deepStrictEqual(safeInputKeys(undefined), []);
assert.deepStrictEqual(safeInputKeys('string'), []);

// The bound destructuring produced from a malicious body must be valid JS with
// no injected statements (proves the escape is closed).
const keys = safeInputKeys(malicious);
const wrapped = `const { ${keys.join(', ')} } = input;`;
assert.strictEqual(wrapped, 'const { good } = input;');
// Should parse as a single, harmless declaration.
new Function('input', wrapped);

console.log('functions-security self-check passed');
