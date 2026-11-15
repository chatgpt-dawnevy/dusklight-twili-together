// Authoritative consumable ledger. The relay owns ordering; a client retry must
// never apply a pickup or expenditure twice. Quantities are integers, not save blobs.
'use strict';
const MAX_QUANTITY = 65535;
const MAX_RESOURCES = 128;
const MAX_CLIENTS = 64;
const RESOURCE = /^(rupees|arrows|seeds|oil|bomb:(normal|water|insect)|key:([0-9]|[12][0-9]|3[01])|bottle:[0-3]:[0-9]{1,3})$/;
function quantities(value, signed = false) {
    if (!value || Array.isArray(value) || typeof value !== 'object') throw new TypeError('invalid inventory');
    const entries = Object.entries(value);
    if (entries.length > MAX_RESOURCES) throw new RangeError('too many resources');
    for (const [key, n] of entries) {
        if (!RESOURCE.test(key) || !Number.isSafeInteger(n) || n > MAX_QUANTITY || n < (signed ? -MAX_QUANTITY : 0)) {
            throw new TypeError('invalid resource quantity');
        }
    }
    return entries;
}
class Inventory {
    constructor(initial) {
        this.counts = new Map(quantities(initial));
        this.revision = 0;
        this.clients = new Map();
    }
    snapshot(session) {
        return { revision: this.revision, acknowledged: this.clients.get(session)?.sequence ?? 0,
            counts: Object.fromEntries(this.counts) };
    }
    transact(session, sequence, delta) {
        if (typeof session !== 'string' || !/^[a-zA-Z0-9_-]{1,96}$/.test(session) ||
            !Number.isSafeInteger(sequence) || sequence < 1) throw new TypeError('invalid transaction identity');
        const entries = quantities(delta, true);
        const previous = this.clients.get(session);
        if (previous && sequence <= previous.sequence) return { ...this.snapshot(session), accepted: false, reason: 'duplicate' };
        if (sequence !== (previous?.sequence ?? 0) + 1) return { ...this.snapshot(session), accepted: false, reason: 'gap' };
        if (!previous && this.clients.size >= MAX_CLIENTS) throw new RangeError('too many inventory sessions');
        const candidate = new Map(this.counts);
        let accepted = true;
        for (const [key, delta] of entries) {
            const next = (candidate.get(key) ?? 0) + delta;
            if (next < 0 || next > MAX_QUANTITY) accepted = false;
            if (next === 0) candidate.delete(key);
            else candidate.set(key, next);
        }
        if (candidate.size > MAX_RESOURCES) accepted = false;
        // A rejected transaction is acknowledged too. Retrying it after a later
        // pickup must not unexpectedly consume the newly acquired resources.
        this.clients.set(session, { sequence });
        if (accepted) {
            this.counts = candidate;
            this.revision++;
        }
        return { ...this.snapshot(session), accepted, reason: accepted ? 'applied' : 'insufficient-or-overflow' };
    }
}
module.exports = { Inventory };
