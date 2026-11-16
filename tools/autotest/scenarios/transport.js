// Transports and reconnecting (src/net/).

const { STAGES, COMMON_CVARS, waitStage, meetIn, connect } = require("../lib");

// The relay dies on A's signal and comes back: dummies return without a connect step.
const afterRestart = (other) => [
    { op: "waitNoDummies", timeoutSec: 60 },
    { op: "waitConnected", timeoutSec: 90 },
    ...meetIn(STAGES.linksHouse, other),
    { op: "quit" },
];

module.exports = [
    {
        name: "ws-idle-heartbeat",
        description: "one player stays connected while waiting alone through several relay heartbeats",
        timeoutSec: 65,
        cvars: COMMON_CVARS,
        rejectLog: [/\[net\] connection lost/, /connected as client [2-9]/],
        instances: [{
            name: "A",
            start: STAGES.forestTemple,
            steps: [
                waitStage(STAGES.forestTemple),
                ...connect,
                { op: "wait", sec: 35 },
                { op: "waitConnected", timeoutSec: 0 },
                { op: "quit" },
            ],
        }],
    },
    {
        name: "tcp-transport",
        description: "same-stage over tcp:// (NetService, length-prefixed JSON) instead of WebSocket",
        timeoutSec: 240,
        cvars: COMMON_CVARS,
        transport: "tcp",
        expectLog: [/\[net\] connected to tcp:\/\/127\.0\.0\.1:\d+ over tcp/],
        instances: ["A", "B"].map((name, i, names) => ({
            name,
            start: STAGES.linksHouse,
            steps: [...connect, ...meetIn(STAGES.linksHouse, names[1 - i]), { op: "quit" }],
        })),
    },
    {
        name: "auto-reconnect",
        description: "the relay restarts mid-session; both players reconnect and see each other again on their own",
        timeoutSec: 300,
        cvars: COMMON_CVARS,
        restartServer: { afterSignal: "restart-relay", downMs: 3000 },
        expectLog: [/\[net\] connection lost/, /\[net\] reconnecting to/],
        instances: [
            {
                name: "A",
                start: STAGES.linksHouse,
                steps: [
                    ...connect,
                    ...meetIn(STAGES.linksHouse, "B"),
                    { op: "signal", name: "restart-relay" },
                    ...afterRestart("B"),
                ],
            },
            {
                name: "B",
                start: STAGES.linksHouse,
                steps: [...connect, ...meetIn(STAGES.linksHouse, "A"), ...afterRestart("A")],
            },
        ],
    },
];
