/**
 * Self-check for deleteObject filter casing (the `mod` vs `Mod` bug).
 * No DB needed. Run with: node test-object-delete.js
 *
 * Documents are stored with an uppercase `Mod` field (see newObject).
 * deleteObject used to filter on lowercase `mod`, so it never matched and
 * POST /Object/Delete always reported deletedCount 0.
 */
const assert = require('assert');

global.logger = { info(){}, warn(){}, error(){}, debug(){} };
global.config = { DBServer: 'mongodb://localhost:27017', DB: 'test' };

// Fake mongodb: one seeded doc, deleteOne does exact field-equality matching.
const docs = [{ ObjectId: 'obj1', Mod: 'testmod', data: { hp: 100 } }];
const fakeCollection = {
  deleteOne: async (filter) => {
    const i = docs.findIndex(d => Object.keys(filter).every(k => d[k] === filter[k]));
    if (i === -1) return { deletedCount: 0 };
    docs.splice(i, 1);
    return { deletedCount: 1 };
  }
};
const fakeDb = {
  collection: () => fakeCollection,
  command: async () => ({ ok: 1 })
};
class FakeMongoClient {
  constructor() {}
  async connect() {}
  db() { return fakeDb; }
  on() {}
}
require.cache[require.resolve('mongodb')] = {
  id: require.resolve('mongodb'),
  filename: require.resolve('mongodb'),
  loaded: true,
  exports: { MongoClient: FakeMongoClient }
};

const { deleteObject } = require('./models/object');

(async () => {
  // Wrong mod -> nothing deleted.
  let r = await deleteObject('obj1', 'othermod');
  assert.deepStrictEqual(r, { success: false, deleted: false, deletedCount: 0 });
  assert.strictEqual(docs.length, 1);

  // Correct mod -> must match the stored `Mod` field and delete the doc.
  r = await deleteObject('obj1', 'testmod');
  assert.deepStrictEqual(r, { success: true, deleted: true, deletedCount: 1 });
  assert.strictEqual(docs.length, 0, 'document must actually be removed');

  console.log('object-delete self-check passed');
})().catch(err => { console.error(err); process.exit(1); });
