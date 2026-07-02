/**
 * Self-check for the query server-side-JS operator guard ($where etc.).
 * No DB needed. Run with: node test-query-guard.js
 */
const assert = require('assert');
const { findDangerousQueryOperator } = require('./utils');

// Clean queries the mod side actually uses -> must pass (null).
const clean = [
  { name: 'survivor' },
  { 'data.health': { $gt: 50, $lte: 100 } },
  { $or: [{ a: 1 }, { b: 2 }] },
  { $and: [{ x: { $in: [1, 2, 3] } }, { y: { $exists: true } }] },
  { content: { $regex: 'foo', $options: 'i' } },
  { $expr: { $eq: ['$a', '$b'] } },            // $expr without $function is fine
  { tags: { $elemMatch: { $gt: 5 } } },
  {}, [], null, 'string', 42
];
for (const q of clean) {
  assert.strictEqual(findDangerousQueryOperator(q), null, `should be clean: ${JSON.stringify(q)}`);
}

// Dangerous queries -> must be caught, wherever the operator hides.
assert.strictEqual(findDangerousQueryOperator({ $where: 'this.a==1' }), '$where');
assert.strictEqual(findDangerousQueryOperator({ a: { $where: 'x' } }), '$where');
assert.strictEqual(findDangerousQueryOperator({ $or: [{ ok: 1 }, { $where: '1' }] }), '$where');
assert.strictEqual(findDangerousQueryOperator({ $expr: { $function: { body: 'x', args: [], lang: 'js' } } }), '$function');
assert.strictEqual(findDangerousQueryOperator({ $group: { total: { $accumulator: {} } } }), '$accumulator');
assert.strictEqual(findDangerousQueryOperator([{ a: 1 }, { b: { $where: 'y' } }]), '$where');

// Depth guard: deep-but-clean returns null and does not throw.
let deep = { v: 1 };
for (let i = 0; i < 500; i++) deep = { nested: deep };
assert.strictEqual(findDangerousQueryOperator(deep), null);

console.log('query-guard self-check passed');
