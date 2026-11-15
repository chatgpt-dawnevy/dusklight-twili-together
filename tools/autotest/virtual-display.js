// Never inherit the user's desktop when running game scenarios on Linux.
const { spawn } = require('node:child_process');

async function virtualDisplay({ platform = process.platform, spawnProcess = spawn, env = process.env } = {}) {
    if (platform !== 'linux') {
        throw new Error('Game tests require Linux Xvfb isolation; use a dedicated Linux test host.');
    }
    const child = spawnProcess(env.TWILI_XVFB || 'Xvfb', [
        '-displayfd', '3', '-screen', '0', '1800x900x24', '-nolisten', 'tcp',
    ], { stdio: ['ignore', 'ignore', 'pipe', 'pipe'] });
    let stderr = '';
    child.stderr.on('data', (data) => { stderr = (stderr + data).slice(-4096); });
    const stop = () => {
        if (child.exitCode === null) child.kill();
    };
    try {
        const display = await new Promise((resolve, reject) => {
            let data = '';
            const timer = setTimeout(() => { reject(new Error('Xvfb startup timed out')); }, 10000);
            const fail = (error) => { clearTimeout(timer); reject(error); };
            child.once('error', fail);
            child.once('exit', () => fail(new Error(`Xvfb exited before readiness: ${stderr}`)));
            child.stdio[3].on('data', (chunk) => {
                data += chunk;
                if (!data.includes('\n')) return;
                clearTimeout(timer);
                if (!/^\d+\n$/.test(data)) return reject(new Error('Invalid Xvfb display number'));
                resolve(`:${data.trim()}`);
            });
        });
        const isolated = { DISPLAY: display, SDL_VIDEODRIVER: 'x11', WAYLAND_DISPLAY: '' };
        return { env: isolated, stop, child };
    } catch (error) {
        stop();
        throw new Error(`No game was launched: virtual display unavailable (${error.message})`);
    }
}
module.exports = { virtualDisplay };
