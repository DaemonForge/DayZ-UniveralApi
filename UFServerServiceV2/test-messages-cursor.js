/**
 * Self-check for the message-queue compound-cursor pagination (data-loss fix).
 * No DB needed. Run with: node test-messages-cursor.js
 *
 * Proves that paging with buildAfterCursorQuery + (createdAt,_id) advance
 * delivers every message exactly once even when many share a createdAt
 * millisecond and a limit splits them - the bug the old `$gt: time` cursor had.
 */
const assert = require('assert');

// messages.js calls createLogger(global.logger,...) at require time.
global.logger = { info(){}, warn(){}, error(){}, debug(){} };
global.config = { DBServer: 'mongodb://localhost:27017', DB: 'test' };

const { buildAfterCursorQuery, resolveCursor } = require('./models/messages');

// ---- buildAfterCursorQuery shape ----
assert.deepStrictEqual(
  buildAfterCursorQuery('M', 'Q', 100, null),
  { Mod: 'M', Queue: 'Q', createdAt: { $gt: 100 } }
);
assert.deepStrictEqual(
  buildAfterCursorQuery('M', 'Q', 100, 5),
  { Mod: 'M', Queue: 'Q', $or: [ { createdAt: { $gt: 100 } }, { createdAt: 100, _id: { $gt: 5 } } ] }
);

// ---- resolveCursor ----
const D = n => new Date(n);
assert.deepStrictEqual(resolveCursor(null, D(0)), { time: D(0), id: null });
// lastRead newer than reset -> use lastRead + id
let r = resolveCursor({ lastRead: D(500), lastReadId: 'X' }, D(100));
assert.strictEqual(r.time.getTime(), 500); assert.strictEqual(r.id, 'X');
// reset newer than lastRead -> reset boundary, no id
r = resolveCursor({ lastRead: D(100), lastReadId: 'X' }, D(500));
assert.strictEqual(r.time.getTime(), 500); assert.strictEqual(r.id, null);
// legacy record (no lastReadId) -> id null
r = resolveCursor({ lastRead: D(500) }, D(100));
assert.strictEqual(r.time.getTime(), 500); assert.strictEqual(r.id, null);

// ---- Minimal in-memory emulation of Mongo find(filter).sort().limit() ----
// createdAt values are plain numbers here (stand-ins for Date ms); _id integers.
function simulateFind(dataset, filter, sortDir, limit) {
  let match;
  if (filter.$or) {
    const gtT = filter.$or[0].createdAt.$gt;
    const eqT = filter.$or[1].createdAt;
    const gtId = filter.$or[1]._id.$gt;
    match = m => (m.createdAt > gtT) || (m.createdAt === eqT && m._id > gtId);
  } else {
    const gtT = filter.createdAt.$gt;
    match = m => m.createdAt > gtT;
  }
  const rows = dataset.filter(match)
    .sort((a, b) => (a.createdAt - b.createdAt) || (a._id - b._id));
  if (sortDir === -1) rows.reverse();
  return rows.slice(0, limit);
}

// Dataset: 5 messages share t=100, 2 share t=200, 3 share t=300.
const dataset = [
  { _id: 1, createdAt: 100 }, { _id: 2, createdAt: 100 }, { _id: 3, createdAt: 100 },
  { _id: 4, createdAt: 100 }, { _id: 5, createdAt: 100 },
  { _id: 6, createdAt: 200 }, { _id: 7, createdAt: 200 },
  { _id: 8, createdAt: 300 }, { _id: 9, createdAt: 300 }, { _id: 10, createdAt: 300 }
];

// Drive the exact read loop the model uses: FIFO, limit 3, advance pointer to
// the (createdAt,_id) max of each batch.
function pageAll(sortDir) {
  const collected = [];
  let cursor = { time: 0, id: null };
  for (let guard = 0; guard < 100; guard++) {
    const filter = buildAfterCursorQuery('M', 'Q', cursor.time, cursor.id);
    const batch = simulateFind(dataset, filter, sortDir, 3);
    if (batch.length === 0) break;
    collected.push(...batch);
    const newest = sortDir === 1 ? batch[batch.length - 1] : batch[0];
    cursor = { time: newest.createdAt, id: newest._id };
  }
  return collected;
}

const fifo = pageAll(1).map(m => m._id);
assert.deepStrictEqual(fifo, [1,2,3,4,5,6,7,8,9,10], 'FIFO must deliver every message once, in order');

// Sanity: the OLD (timestamp-only) cursor would have lost messages.
function pageAllOld() {
  const collected = [];
  let t = 0;
  for (let guard = 0; guard < 100; guard++) {
    const batch = simulateFind(dataset, { createdAt: { $gt: t } }, 1, 3);
    if (batch.length === 0) break;
    collected.push(...batch);
    t = batch[batch.length - 1].createdAt; // advance to newest timestamp only
  }
  return collected;
}
const old = pageAllOld().map(m => m._id);
assert.ok(old.length < 10, 'old cursor is expected to lose messages (regression guard)');

console.log(`messages-cursor self-check passed (new=${fifo.length}/10, old lost ${10 - old.length})`);
