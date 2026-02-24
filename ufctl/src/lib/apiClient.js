/**
 * ufctl API client
 * 
 * Thin HTTP wrapper around the UF Server Service REST API.
 * Uses only Node.js built-ins (no external deps) so the CLI stays
 * dependency-free and easy to package with `pkg`.
 * 
 * This module is designed to be reusable—a future web admin UI can
 * swap the transport (fetch in browser) while keeping the same shape.
 */

'use strict';

const https = require('https');
const http  = require('http');
const { URL } = require('url');

class ApiClient {
    /**
     * @param {string} baseUrl  - e.g. "https://myserver:443"
     * @param {string} authKey  - server auth token
     */
    constructor(baseUrl, authKey) {
        this.baseUrl = baseUrl.replace(/\/+$/, '');
        this.authKey = authKey;
    }

    // ────────────────────────────────────────────────
    // Low-level request
    // ────────────────────────────────────────────────

    /**
     * Make an HTTP(S) request.  Returns { status, body (parsed JSON or string) }.
     */
    request(method, urlPath, body) {
        return new Promise((resolve, reject) => {
            const full = new URL(urlPath, this.baseUrl);
            const isHttps = full.protocol === 'https:';
            const lib = isHttps ? https : http;

            const headers = {
                'auth-key': this.authKey,
                'Accept': 'application/json',
            };

            let payload;
            if (body !== undefined) {
                payload = JSON.stringify(body);
                headers['Content-Type'] = 'application/json';
                headers['Content-Length'] = Buffer.byteLength(payload);
            }

            const opts = {
                method,
                hostname: full.hostname,
                port: full.port || (isHttps ? 443 : 80),
                path: full.pathname + full.search,
                headers,
                rejectUnauthorized: false  // allow self-signed certs
            };

            const req = lib.request(opts, res => {
                const chunks = [];
                res.on('data', c => chunks.push(c));
                res.on('end', () => {
                    const raw = Buffer.concat(chunks).toString('utf-8');
                    let parsed;
                    try { parsed = JSON.parse(raw); }
                    catch { parsed = raw; }
                    resolve({ status: res.statusCode, body: parsed });
                });
            });

            req.on('error', reject);
            if (payload) req.write(payload);
            req.end();
        });
    }

    // ────────────────────────────────────────────────
    // Convenience verbs
    // ────────────────────────────────────────────────

    get(path)          { return this.request('GET', path); }
    post(path, body)   { return this.request('POST', path, body); }
    put(path, body)    { return this.request('PUT', path, body); }
    del(path)          { return this.request('DELETE', path); }

    // ────────────────────────────────────────────────
    // KB endpoints
    // ────────────────────────────────────────────────

    /** List all knowledge bases */
    kbList() {
        return this.get('/KB/');
    }

    /** Get a single KB by ID */
    kbGet(kbId) {
        return this.get(`/KB/${encodeURIComponent(kbId)}`);
    }

    /** Create a KB */
    kbCreate(kbId, name, description) {
        return this.post('/KB/', { kbId, name, description });
    }

    /** Delete a KB */
    kbDelete(kbId) {
        return this.del(`/KB/${encodeURIComponent(kbId)}`);
    }

    /** Update KB settings (name, description, shorterAnswers, extractModel) */
    kbUpdate(kbId, updates) {
        return this.put(`/KB/${encodeURIComponent(kbId)}`, updates);
    }

    /** List documents inside a KB */
    kbListDocuments(kbId) {
        return this.get(`/KB/${encodeURIComponent(kbId)}/documents`);
    }

    /** Get a single document */
    kbGetDocument(kbId, documentId) {
        return this.get(`/KB/${encodeURIComponent(kbId)}/documents/${encodeURIComponent(documentId)}`);
    }

    /** Add (or replace) a text document */
    kbAddDocument(kbId, name, content, contextHint, fileType) {
        return this.post(`/KB/${encodeURIComponent(kbId)}/documents/text`, {
            name,
            content,
            contextHint: contextHint || '',
            fileType: fileType || 'txt'
        });
    }

    /** Update an existing document */
    kbUpdateDocument(kbId, documentId, name, content, contextHint) {
        return this.put(`/KB/${encodeURIComponent(kbId)}/documents/${encodeURIComponent(documentId)}`, {
            name,
            content,
            contextHint
        });
    }

    /** Delete a document */
    kbDeleteDocument(kbId, documentId) {
        return this.del(`/KB/${encodeURIComponent(kbId)}/documents/${encodeURIComponent(documentId)}`);
    }

    // ────────────────────────────────────────────────
    // Globals endpoints
    // ────────────────────────────────────────────────

    /** Load (get) the global data for a module */
    globalsLoad(mod) {
        return this.post(`/Globals/Load/${encodeURIComponent(mod)}`, {});
    }

    /** Save (upsert) global data for a module */
    globalsSave(mod, data) {
        return this.post(`/Globals/Save/${encodeURIComponent(mod)}`, data);
    }

    /** Update a single field via the Update route */
    globalsUpdate(mod, element, value, operation = 'set') {
        return this.post(`/Globals/Update/${encodeURIComponent(mod)}`, {
            Element: element,
            Value: value,
            Operation: operation,
        });
    }

    /** Atomic increment via the Transaction route */
    globalsTransaction(mod, element, value) {
        return this.post(`/Globals/Transaction/${encodeURIComponent(mod)}`, {
            Element: element,
            Value: value,
        });
    }
}

module.exports = { ApiClient };
