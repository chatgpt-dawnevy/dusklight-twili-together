const { test } = require('node:test');
const assert = require('node:assert/strict');
const { EventEmitter } = require('node:events');
const { PassThrough } = require('node:stream');
const { virtualDisplay } = require('../virtual-display');
function fakeSpawn(ready) {
    let child;
    return {
        spawn(exe, args, options) {
            assert.ok(args.includes('-nolisten'));
            assert.equal(options.stdio[3], 'pipe');
            child = new EventEmitter();
            child.exitCode = null;
            child.stderr = new PassThrough();
            child.stdio = [null, null, child.stderr, new PassThrough()];
            child.kill = () => { child.killed = true; };
            queueMicrotask(() => ready(child));
            return child;
        },
        get child() { return child; },
    };
}
test('uses a newly allocated display, overrides Wayland and stops its own server', async () => {
    const fake = fakeSpawn(c => c.stdio[3].write('123\n'));
    const display = await virtualDisplay({ platform: 'linux', spawnProcess: fake.spawn,
        env: { DISPLAY: ':0', WAYLAND_DISPLAY: 'wayland-0' } });
    assert.deepEqual(display.env, { DISPLAY: ':123', SDL_VIDEODRIVER: 'x11', WAYLAND_DISPLAY: '' });
    display.stop();
    assert.equal(fake.child.killed, true);
});
test('fails closed when Xvfb is missing', async () => {
    const fake = fakeSpawn(c => c.emit('error', new Error('ENOENT')));
    await assert.rejects(virtualDisplay({ platform: 'linux', spawnProcess: fake.spawn }), /No game was launched/);
    assert.equal(fake.child.killed, true);
});
test('fails closed on malformed readiness', async () => {
    const fake = fakeSpawn(c => c.stdio[3].write('invalid\n'));
    await assert.rejects(virtualDisplay({ platform: 'linux', spawnProcess: fake.spawn }), /Invalid Xvfb/);
});
test('rejects hosts without isolation before spawning anything', async () => {
    await assert.rejects(virtualDisplay({ platform: 'win32', spawnProcess() { assert.fail('must not spawn'); } }), /isolation/);
});
