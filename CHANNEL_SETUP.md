# Channel setup: which program connects where

## 1. Build all seven executables

Keep this folder's `.c`, `.h` and `.inc` files together. In the QNX SDP build
environment for your target, run:

```sh
make
```

The Makefile produces `controller`, `l1`, `l2`, `ped1`, `ped2`, `train`, `display`.
The display needs `-lsocket`. The five traffic/pedestrian/train executables also
compile `SampleCode_DisplayTransport.c`; do not omit it in a Momentics project.
The controller and display compile their own single C file only.
Use the same protocol definitions and compatible byte order on every target.

## 2. What a channel name means

The **server** creates its local named channel with:

```c
name_attach(NULL, "Pedestrian", 0);
```

The **client** connects using the server's name:

```c
name_open("Pedestrian", 0);  // Server on the same QNX machine.
name_open("/net/PED1_HOST/dev/name/local/Pedestrian", 0); // Remote Qnet machine.
```

Replace `PED1_HOST` with the pedestrian machine's actual Qnet hostname.
The remote path always names the machine running the **server**. It is not
an IP URL, a source file path, or a thread address. Do not put `/net/...` into
`name_attach`: servers keep their short local name.

You do not manually create channels in the shell. Starting the server program
calls `name_attach`; the name then appears in `/dev/name/local`. `MsgSend`
sends a request and blocks for `MsgReply`; both ends must agree on the struct.

Official QNX reference:
https://www.qnx.com/developers/docs/7.1/com.qnx.doc.neutrino.lib_ref/topic/n/name_attach.html

## 3. Channel map

### Existing control channels

| Server executable | Local attachment name | Client executable |
| --- | --- | --- |
| `ped1` | `Pedestrian` | `l1` |
| `ped2` | `Pedestrian_L2` | `l2` |
| `controller` | `L1_To_Controller` | `l1` |
| `controller` | `L2_To_Controller` | `l2` |
| `controller` | `Train_To_Controller` | `train` |
| `l1` | `Train_To_Traffic` | `train` |
| `l2` | `Train_To_Traffic_L2` | `train` |
| `l2` | `L1_To_L2` | `l1` |

L2's train name has an `_L2` suffix so L1 and L2 can also run on one machine.
Update the train client destination accordingly; all supplied defaults match.

### Status reports to the display

| Server executable | Local attachment name | Client executable |
| --- | --- | --- |
| `display` | `Display_From_L1` | `l1` |
| `display` | `Display_From_L2` | `l2` |
| `display` | `Display_From_Pedestrian_L1` | `ped1` |
| `display` | `Display_From_Pedestrian_L2` | `ped2` |
| `display` | `Train_To_Display` | `train` |

### Random inputs from the display

| Server executable | Local attachment name | Client executable | Input |
| --- | --- | --- | --- |
| `l1` | `Simulator_To_L1` | `display` | Car counts |
| `l2` | `Simulator_To_L2` | `display` | Car counts |
| `ped1` | `Simulator_To_Pedestrian_L1` | `display` | Button presses |
| `ped2` | `Simulator_To_Pedestrian_L2` | `display` | Button presses |
| `train` | `Simulator_To_Train` | `display` | Train detected 0/1 |

The controller has no direct display channel: it receives the simulated demand
through L1/L2 and chooses their states in the existing way.

## 4. First test: all processes on one QNX machine

Open seven terminals and run one executable in each. Starting in this order
is convenient; clients retry failed connections every two seconds:

```sh
./display
./controller
./ped1
./ped2
./l2
./l1
./train
```

These are separate terminal commands, not one sequential shell script: each
program runs indefinitely. No arguments are needed for this local test.
Then open `http://QNX_MACHINE_IP:8080` on your computer's browser. Use live mode;
**Preview animation** uses synthetic data even when other programs are running.

## 5. Separate QNX machines: use these launch arguments

Qnet must already be configured/running on the machines. TCP/IP ping alone
does not establish Qnet. On each QNX machine check the actual available peers:

```sh
ls /net
ls /dev/name/local
```

After a remote server has started, check its name, for example:

```sh
ls /net/PED1_HOST/dev/name/local
```

If `/net` or a peer is missing, configure Qnet using your lab image/instructions
before debugging this application. Keep the lab image's network setup rather
than guessing an io-pkt driver command for different hardware.

The labels below are placeholders. Replace each with its actual Qnet hostname;
multiple roles may use the same hostname.

**Display machine:** its five arguments point to the five input servers.

```sh
./display \
 /net/L1_HOST/dev/name/local/Simulator_To_L1 \
 /net/L2_HOST/dev/name/local/Simulator_To_L2 \
 /net/PED1_HOST/dev/name/local/Simulator_To_Pedestrian_L1 \
 /net/PED2_HOST/dev/name/local/Simulator_To_Pedestrian_L2 \
 /net/TRAIN_HOST/dev/name/local/Simulator_To_Train
```

**Controller machine:** all three servers use their local short names.

```sh
./controller
```

**Pedestrian L1 and L2 machines:** their one argument points to the display.

```sh
./ped1 /net/DISPLAY_HOST/dev/name/local/Display_From_Pedestrian_L1
./ped2 /net/DISPLAY_HOST/dev/name/local/Display_From_Pedestrian_L2
```

**L1 machine:** pedestrian, controller, L2, display — in that order.

```sh
./l1 \
 /net/PED1_HOST/dev/name/local/Pedestrian \
 /net/CONTROLLER_HOST/dev/name/local/L1_To_Controller \
 /net/L2_HOST/dev/name/local/L1_To_L2 \
 /net/DISPLAY_HOST/dev/name/local/Display_From_L1
```

**L2 machine:** pedestrian, controller, display — in that order.

```sh
./l2 \
 /net/PED2_HOST/dev/name/local/Pedestrian_L2 \
 /net/CONTROLLER_HOST/dev/name/local/L2_To_Controller \
 /net/DISPLAY_HOST/dev/name/local/Display_From_L2
```

**Train machine:** L1, controller, L2, display — in that order.

```sh
./train \
 /net/L1_HOST/dev/name/local/Train_To_Traffic \
 /net/CONTROLLER_HOST/dev/name/local/Train_To_Controller \
 /net/L2_HOST/dev/name/local/Train_To_Traffic_L2 \
 /net/DISPLAY_HOST/dev/name/local/Train_To_Display
```

In the browser, use `http://DISPLAY_MACHINE_IP:8080`. This uses the display
machine's IP for HTTP. The C clients above use Qnet hostnames for named channels.

## 6. Check a connection

1. Check the server is running and its exact name exists in `/dev/name/local`.
2. On the client machine, check `/net/SERVER_HOST/dev/name/local` if remote.
3. Check argument order and letter case; restarting with corrected arguments
   changes `sname` without editing source.
4. Watch the browser show five live status sources. Input ACKs verify the
   reverse direction; live status alone does not prove input delivery.
5. Car counts should change about every 8 seconds. Pedestrian requests are
   intentionally sparse (20–40 seconds per node), and the first train waits
   60–120 seconds. A quiet scene initially is expected.

An attachment-name conflict normally means another copy of that server is
already running. This package uses unique names for all local test processes.
For public-channel permission failures, follow the user/ability configuration
of your QNX lab image. Exact permissions depend on the QNX version and policy.
