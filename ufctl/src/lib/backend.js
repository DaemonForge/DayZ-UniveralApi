/**
 * Backend abstraction layer
 *
 * Two implementations:
 *   DirectBackend  – talks to MongoDB using the same logic as the service models
 *   ApiBackend     – talks to the REST API (for remote use / when DB isn't local)
 *
 * resolveBackend() picks the right one based on the service config.json + CLI flags.
 */

'use strict';

const { resolveConnection } = require('./config');

// ─────────────────────────────────────────────────────
// Interface (documented here, implemented below)
// ─────────────────────────────────────────────────────
//
// Every backend must expose:
//
//   kb.list()                              → [{ kbId, name, description, documentCount }]
//   kb.get(kbId)                           → { kbId, name, description, ... } | null
//   kb.create(kbId, name, description)     → object
//   kb.delete(kbId)                        → boolean
//   kb.update(kbId, updates)               → boolean
//   kb.listDocuments(kbId)                 → [{ documentId, name, ... }]
//   kb.addDocument(kbId, name, content, contextHint, fileType) → object
//   kb.deleteDocument(kbId, documentId)    → boolean
//   kb.deleteAllDocuments(kbId)            → number (deleted count)
//
//   globals.list()                         → [{ id, mod }]
//   globals.get(mod)                       → { id, mod, data } | null
//   globals.set(mod, data)                 → boolean
//   globals.setParam(mod, element, value)  → any | null
//   globals.transaction(mod, element, amt) → number | null
//   globals.delete(mod)                    → boolean
//
//   index.getCollections()                 → [string]
//   index.getIndexes(collection)           → [{ name, key, unique, sparse, ... }]
//   index.getAllIndexes()                   → { collection: [indexes] }
//   index.getIndexStats(collection)         → [{ name, accesses }]
//   index.analyzeCollection(collection)     → { collection, documentCount, ... }
//   index.createIndex(coll, spec, opts)     → { success, indexName }
//   index.dropIndex(coll, indexName)        → { success }
//
//   data.scanInstalledMods()               → [{ modName, collections, totalDocuments }]
//   data.deleteModData(modName)            → { modName, collections, totalDeleted, errors }
//
//   close()                                → void  (cleanup connections)

// ─────────────────────────────────────────────────────
// DirectBackend  (MongoDB)
// ─────────────────────────────────────────────────────

class DirectBackend {
    constructor(dbUri, dbName, openaiApiKey) {
        this.dbUri = dbUri;
        this.dbName = dbName;
        this.openaiApiKey = openaiApiKey || '';
        this._kb = null;
        this._globals = null;
        this._index = null;
        this._data = null;
    }

    /** Lazy-load the KB model so it only connects on first use */
    _getKB() {
        if (!this._kb) {
            this._kb = require('./directKB')(this.dbUri, this.dbName, this.openaiApiKey);
        }
        return this._kb;
    }

    /** Lazy-load the Globals model */
    _getGlobals() {
        if (!this._globals) {
            this._globals = require('./directGlobals')(this.dbUri, this.dbName);
        }
        return this._globals;
    }

    /** Lazy-load the Index model */
    _getIndex() {
        if (!this._index) {
            this._index = require('./directIndex')(this.dbUri, this.dbName);
        }
        return this._index;
    }

    /** Lazy-load the Data model */
    _getData() {
        if (!this._data) {
            this._data = require('./directData')(this.dbUri, this.dbName);
        }
        return this._data;
    }

    get kb() {
        const m = this._getKB();
        return {
            list:              ()                                         => m.listKBs(),
            get:               (kbId)                                     => m.getKB(kbId),
            create:            (kbId, name, description)                  => m.createKB(kbId, name, description),
            delete:            (kbId)                                     => m.deleteKB(kbId),
            update:            (kbId, updates)                            => m.updateKB(kbId, updates),
            listDocuments:     (kbId)                                     => m.listDocuments(kbId),
            addDocument:       (kbId, name, content, contextHint, fileType) =>
                                   m.addDocument(kbId, name, content, contextHint || '', fileType || 'txt'),
            deleteDocument:    (kbId, documentId)                         => m.deleteDocument(kbId, documentId),
            deleteAllDocuments:(kbId)                                     => m.deleteAllDocuments(kbId),
        };
    }

    get globals() {
        const g = this._getGlobals();
        return {
            list:        ()                         => g.listGlobals(),
            get:         (mod)                      => g.getGlobal(mod),
            set:         (mod, data)                => g.setGlobal(mod, data),
            setParam:    (mod, element, value)       => g.setParam(mod, element, value),
            transaction: (mod, element, amount)      => g.transactionGlobal(mod, element, amount),
            delete:      (mod)                      => g.deleteGlobal(mod),
        };
    }

    get index() {
        const idx = this._getIndex();
        return {
            getCollections:     ()                            => idx.getCollections(),
            getIndexes:         (collection)                  => idx.getIndexes(collection),
            getAllIndexes:      ()                            => idx.getAllIndexes(),
            getIndexStats:      (collection)                  => idx.getIndexStats(collection),
            analyzeCollection:  (collection)                  => idx.analyzeCollection(collection),
            createIndex:        (collection, spec, opts)      => idx.createIndex(collection, spec, opts),
            dropIndex:          (collection, indexName)       => idx.dropIndex(collection, indexName),
        };
    }

    get data() {
        const d = this._getData();
        return {
            scanInstalledMods: ()         => d.scanInstalledMods(),
            deleteModData:     (modName)  => d.deleteModData(modName),
        };
    }

    async close() { /* MongoClient connections are per-call in the model */ }
}

// ─────────────────────────────────────────────────────
// ApiBackend  (REST)
// ─────────────────────────────────────────────────────

class ApiBackend {
    constructor(url, authKey) {
        const { ApiClient } = require('./apiClient');
        this.client = new ApiClient(url, authKey);
    }

    /** Unwrap the service envelope: { Status, data?, Error? } */
    _unwrap(res) {
        if (res.status >= 400) {
            const msg = res.body?.Error || res.body?.error || JSON.stringify(res.body);
            throw new Error(`Server ${res.status}: ${msg}`);
        }
        return res.body?.data ?? res.body;
    }

    /**
     * Unwrap a raw response (no envelope).
     * Globals routes return plain objects, not wrapped in { data }.
     */
    _unwrapRaw(res) {
        if (res.status >= 400) {
            const msg = res.body?.Error || res.body?.error || JSON.stringify(res.body);
            throw new Error(`Server ${res.status}: ${msg}`);
        }
        return res.body;
    }

    get kb() {
        const c = this.client;
        const u = this._unwrap.bind(this);
        return {
            list:              async ()                                          => u(await c.kbList()),
            get:               async (kbId)                                      => u(await c.kbGet(kbId)),
            create:            async (kbId, name, description)                   => u(await c.kbCreate(kbId, name, description)),
            delete:            async (kbId)                                      => { u(await c.kbDelete(kbId)); return true; },
            update:            async (kbId, updates)                             => { u(await c.kbUpdate(kbId, updates)); return true; },
            listDocuments:     async (kbId)                                      => u(await c.kbListDocuments(kbId)),
            addDocument:       async (kbId, name, content, contextHint, fileType) =>
                                   u(await c.kbAddDocument(kbId, name, content, contextHint, fileType)),
            deleteDocument:    async (kbId, documentId)                          => { u(await c.kbDeleteDocument(kbId, documentId)); return true; },
            deleteAllDocuments: async (kbId) => {
                // API doesn't have a bulk-delete; delete them one at a time
                const docs = u(await c.kbListDocuments(kbId));
                for (const d of docs) { u(await c.kbDeleteDocument(kbId, d.documentId)); }
                return docs.length;
            },
        };
    }

    get globals() {
        const cl = this.client;
        const u  = this._unwrapRaw.bind(this);
        return {
            list:        async () => {
                // The REST API has no list-globals endpoint.
                throw new Error(
                    'Globals list is not available via the REST API.\n' +
                    'Use direct mode (default) or omit --mode api.'
                );
            },
            get:         async (mod) => {
                const res = await cl.globalsLoad(mod);
                if (res.status === 203) return null;      // document not found
                return { mod, data: u(res) };
            },
            set:         async (mod, data) => {
                u(await cl.globalsSave(mod, data));
                return true;
            },
            setParam:    async (mod, element, value) => {
                const body = u(await cl.globalsUpdate(mod, element, value, 'set'));
                return body?.Status === 'Success' ? value : null;
            },
            transaction: async (mod, element, amount) => {
                const body = u(await cl.globalsTransaction(mod, element, amount));
                return body?.Value ?? null;
            },
            delete:      async () => {
                throw new Error(
                    'Globals delete is not available via the REST API.\n' +
                    'Use direct mode (default) or omit --mode api.'
                );
            },
        };
    }

    get index() {
        throw new Error(
            'Index management is not available via the REST API.\n' +
            'Use direct mode (default) or omit --mode api.'
        );
    }

    get data() {
        throw new Error(
            'Data management is not available via the REST API.\n' +
            'Use direct mode (default) or omit --mode api.'
        );
    }

    async close() { /* nothing to clean up */ }
}

// ─────────────────────────────────────────────────────
// Factory
// ─────────────────────────────────────────────────────

/**
 * Resolve the right backend based on service config + CLI flags.
 *
 * Default on Linux: DirectBackend (reads config.json for DB + OpenAI key).
 * Falls back to ApiBackend only when --mode api is explicitly requested.
 */
function resolveBackend(opts) {
    const conn = resolveConnection(opts);
    const mode = (conn.mode || '').toLowerCase();

    if (mode === 'api') {
        if (!conn.authKey) throw new Error('API mode requires an auth key (ServerAuth in config.json).');
        return new ApiBackend(conn.url, conn.authKey);
    }

    // Default: direct mode using service config values
    return new DirectBackend(conn.dbUri, conn.dbName, conn.openaiApiKey);
}

module.exports = { resolveBackend, DirectBackend, ApiBackend };
