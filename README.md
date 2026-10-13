# Your seven nodes connected to the 2D display

Build and run instructions are in **CHANNEL_SETUP.md**. All required source,
headers and adapter files are included. Keep them together when compiling.
Each node is a separate executable; do not combine their main functions.

## What changed

- **Traffic L1:** added one simulator input server and one display status client.
  The input changes only `storedDirection`; the status uses `outputL1` and the
  actual light colour. Existing client arguments now accept remote paths.
- **Traffic L2:** the uploaded file was incomplete (undefined connection variables,
  missing controller/pedestrian/train workers and missing fields). Restored the
  L1-style connection/main-loop sections so it can run, receive its controller
  state, and wait for L1's start permission. Its original route selector and
  `StateOutput` switch are unchanged. Its train route selector now uses
  `trainDirectionNext`, separating a route group from train-presence status.
- **Pedestrian L1/L2:** added simulator input and display status threads.
  Random input latches requests, like buttons; zero releases do not erase them.
  Your existing server still decides which requests have been served.
- **Train:** fixed `snfame` to `sname`; added clients for L2 and display using the
  existing one-second train client function. Added an input server that changes
  only `train_detected`. Gate feedback and hardware faults are not fabricated.
- **Controller:** only its timing setting changed to 15 ticks at 1 second.
  Existing priority calculation and route selection remain unchanged.
- **Display:** retained the connected map, slower cars and sparse pedestrians
  from the latest revision. It sends simulated inputs and receives actual node
  states. It does not choose the traffic states. Preview mode is explicitly
  synthetic and does not drive any nodes.

The L2 repair is larger than the other edits because its missing connection
code was necessary for integration. `CHANGES.patch` shows every change against
these seven uploaded files. No source was deployed to your devices.

## Cycle timing

Both traffic nodes use your spelling and units:

```c
settings.time = 15;    // 15 ticks per selected state
settings.peroid = 1;   // 1 second per tick
```

| Part of a normal cycle | Duration |
| --- | ---: |
| Vehicle green | 12 seconds |
| Vehicle yellow | 2 seconds |
| Vehicle red | 1 second |
| Pedestrian WALK, if selected | 9 seconds |
| Pedestrian flashing red, if selected | 3 seconds |
| Pedestrian red | Until the next permitted command |

Traffic clients send `settings.time - 3` (12 ticks) to pedestrian nodes, so
WALK/flashing completes during vehicle green. With no pedestrian command the
pedestrian node stays red. An observed new train still shortens vehicle green
using your existing logic; it also sends PED_NONE to interrupt the earlier
pedestrian timer before switching to yellow. A failed link cannot guarantee the
remote pedestrian command was applied.

The 15 seconds is the timed colour sequence, not a strict start-to-start period:
button requests, the existing one-second controller calculation wait, network
replies and L1 permission add time while red. L2's start is permission-based,
not a guarantee of simultaneous execution on different machines.

## Simulator rates

- Car counts: 0–2 per movement, updated about every 8 seconds.
- Visible car arrivals: at most one every 2.5–5 seconds per intersection.
- Pedestrian request: one random crossing about every 20–40 seconds per node;
  delivered on the next two-second input update. One observed request animates
  one walker on WALK. It does not spawn people automatically each cycle.
- Train: first detection after 60–120 seconds, present 8–12 seconds, then a
  60–120 second clear interval. Only detection changes/reconnections are sent.
- Traffic/pedestrian display reports: every 250 ms plus reply time.
- Train reports to L1, L2, controller and display: every second plus reply time.

Vehicle/pedestrian/train positions are illustrative. Counts and signal states
are actual reports; individual vehicle identities and positions are not sent.
A browser opened after a pedestrian request was already cleared may miss that
walker. Train detection is displayed without inventing gate-down feedback.

## Checks completed and limits

All seven C nodes and the display transport passed host C99 syntax checking
with `-Wall -Wextra -Werror` using temporary QNX declarations. Host linking with
an IPC substitute found no missing function definitions. Targeted checks passed
for adapter field mapping, latched pedestrian requests, all-red outputs, the
12/2/1 transition split, and train detection/state conversion. Both uploaded
traffic route selectors and output switches were compared and preserved exactly.

There is no QNX compiler/runtime here. A full process smoke test could not run
because this environment blocked the host IPC substitute's sockets; no live
QNX/Qnet exchange is claimed as tested. Build and test on your QNX targets.
Existing application MsgSend calls can still block; the existing main-thread
reply timeout does not cancel those sends. This update keeps that existing
connection design rather than changing all your clients and servers.
