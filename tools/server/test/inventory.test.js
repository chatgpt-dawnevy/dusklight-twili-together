const { test } = require('node:test');
const assert = require('node:assert/strict');
const { Inventory } = require('../inventory');
test('two simultaneous expenditures cannot overdraw the shared stock', () => {
    const inventory = new Inventory({ rupees: 100 });
    assert.equal(inventory.transact('alice', 1, { rupees: -80 }).accepted, true);
    assert.equal(inventory.transact('bob', 1, { rupees: -80 }).accepted, false);
    assert.equal(inventory.snapshot('bob').counts.rupees, 20);
});
test('retry after reconnect does not duplicate a pickup', () => {
    const inventory = new Inventory({ arrows: 0 });
    inventory.transact('alice', 1, { arrows: 10 });
    assert.equal(inventory.transact('alice', 1, { arrows: 10 }).reason, 'duplicate');
    assert.equal(inventory.snapshot('alice').counts.arrows, 10);
});
test('missing transactions are retried in order without losing quantities', () => {
    const inventory = new Inventory({ arrows: 0 });
    assert.equal(inventory.transact('alice', 2, { arrows: -1 }).reason, 'gap');
    inventory.transact('alice', 1, { arrows: 10 });
    inventory.transact('alice', 2, { arrows: -1 });
    assert.equal(inventory.snapshot('alice').counts.arrows, 9);
});
test('failed purchase is atomic and cannot be resurrected by a retry', () => {
    const inventory = new Inventory({ rupees: 5, 'bomb:normal': 0 });
    assert.equal(inventory.transact('alice', 1, { rupees: -30, 'bomb:normal': 10 }).accepted, false);
    assert.deepEqual(inventory.snapshot('alice').counts, { rupees: 5, 'bomb:normal': 0 });
    inventory.transact('bob', 1, { rupees: 50 });
    assert.equal(inventory.transact('alice', 1, { rupees: -30, 'bomb:normal': 10 }).reason, 'duplicate');
    assert.equal(inventory.snapshot('alice').counts.rupees, 55);
});
test('keys from separate dungeons remain separate and a late joiner gets current quantities', () => {
    const inventory = new Inventory({ 'key:1': 2, 'key:2': 3 });
    inventory.transact('alice', 1, { 'key:1': -1 });
    inventory.transact('bob', 1, { 'key:2': -2 });
    assert.deepEqual(inventory.snapshot('newcomer'), { revision: 2, acknowledged: 0, counts: { 'key:1': 1, 'key:2': 1 } });
});
test('malformed requests leave both stock and acknowledgement unchanged', () => {
    const inventory = new Inventory({ rupees: 10 });
    for (const delta of [{ rupees: NaN }, { rupees: 0.5 }, { unknown: 1 }, [], null, { rupees: 65536 }]) {
        assert.throws(() => inventory.transact('alice', 1, delta));
        assert.deepEqual(inventory.snapshot('alice'), { revision: 0, acknowledged: 0, counts: { rupees: 10 } });
    }
});
test('overflow refuses the whole transaction and snapshots cannot mutate stock', () => {
    const inventory = new Inventory({ rupees: 65535 });
    assert.equal(inventory.transact('alice', 1, { rupees: 1 }).accepted, false);
    const state = inventory.snapshot('alice');
    state.counts.rupees = 0;
    assert.equal(inventory.snapshot('alice').counts.rupees, 65535);
});
