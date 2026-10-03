# QNX SampleCode — modular lab examples

17 self-contained `.c` files, one adapted module per uploaded source file. Each contains a source comment, reusable functions and a small optional demo. Related setup/work/cleanup functions stay together so a module is practical to copy. No extra shared header is required.

These are adaptations of the uploaded lab code, not verbatim teacher originals. The original files were left unchanged. Long IDE instructions, repeated diagnostic output and unused declarations were removed. Relevant QNX calls and the traffic-light transition sequences were retained.

## File map

| Sample file | Original uploaded file | Main reusable function(s) |
|---|---|---|
| [SampleCode_SystemInfo.c](SampleCode_SystemInfo.c) | `Lab1_Task1A.c` | `PrintSystemInfo` |
| [SampleCode_StructArray.c](SampleCode_StructArray.c) | `Lab1_Task6A.c` | `GiveValue / PrintScan` |
| [SampleCode_Threads.c](SampleCode_Threads.c) | `Lab2_Task1A.c` | `thread_ex + pthread_create/join demo` |
| [SampleCode_ThreadScheduling.c](SampleCode_ThreadScheduling.c) | `Lab2_Task1D.c` | `CreatePriorityThread` |
| [SampleCode_MessageQueueSend.c](SampleCode_MessageQueueSend.c) | `Lab2_Task3A_send.c` | `CreateMessageQueue / SendQueueText` |
| [SampleCode_MessageQueueReceive.c](SampleCode_MessageQueueReceive.c) | `Lab2_Task3A_recieve.c` | `ReceiveQueueText` |
| [SampleCode_Semaphore.c](SampleCode_Semaphore.c) | `Lab3_Task2A.c` | `user_thread / changer_thread + sem_init demo` |
| [SampleCode_NamedSemaphore.c](SampleCode_NamedSemaphore.c) | `Lab3_Task3A.c` | `user_thread / changer_thread + sem_open demo` |
| [SampleCode_ProducerConsumer.c](SampleCode_ProducerConsumer.c) | `Lab3_Task5B.c` | `BufferInit / BufferPut / BufferGet / BufferDestroy` |
| [SampleCode_TrafficStateMachine.c](SampleCode_TrafficStateMachine.c) | `Lab4_Task2C.c` | `SingleStep_TrafficLight_SM` |
| [SampleCode_TrafficQueueSend.c](SampleCode_TrafficQueueSend.c) | `Lab4_Task2D_Send.c` | `CreateTrafficQueue / SendTrafficInput` |
| [SampleCode_TrafficQueueReceive.c](SampleCode_TrafficQueueReceive.c) | `Lab4_Task2D_Recieve.c` | `ThreadReceive / ReadTrafficInput` |
| [SampleCode_TimerPulse.c](SampleCode_TimerPulse.c) | `Lab4_Task3B.c` | `StartPulseTimer / StopPulseTimer + tick state machine` |
| [SampleCode_NamedClient.c](SampleCode_NamedClient.c) | `Lab5_Task1_Client(1).c` | `SendNamedData + name_open/close demo` |
| [SampleCode_NamedServer.c](SampleCode_NamedServer.c) | `Lab5_Task1_Server(1).c` | `RunNamedServer` |
| [SampleCode_ChannelClient.c](SampleCode_ChannelClient.c) | `Lab5_Task2_Client (1).c` | `ConnectLocalServer / SendTrafficData` |
| [SampleCode_ChannelServer.c](SampleCode_ChannelServer.c) | `Lab5_Task2_Server (2).c` | `CreateLocalServer / RunLocalTrafficServer` |

## Copy into your project

1. Choose the module you need and copy its includes, types and functions above `#ifdef SAMPLECODE_DEMO`.
2. Use the demo at the bottom as the calling example. In your own project, keep your existing `main()` and leave `SAMPLECODE_DEMO` undefined.
3. Change queue/server names, input data or the marked processing block as needed. For L1 and L2 servers, use different names or different PID/CHID files.
4. Keep shared data alive until its worker threads finish. Initialise locks before starting threads and destroy them after joining.

These are independent examples, **not one combined executable**. Do not add all files to one Momentics executable: several intentionally retain the teacher's names (`app_data`, `user_thread`, `SingleStep_TrafficLight_SM`, etc.). Select one variant, or rename/move its declarations into your own project header when combining modules. Do not both `#include` a `.c` file and compile that same file separately.

The client and server deliberately repeat their protocol structs to remain standalone. When adapting a pair, change both copies together; in a larger project you can move these declarations to one common header.

## Build a standalone demo

In a configured QNX SDP shell, select your target in the usual way and build one file:

```sh
qcc -std=gnu99 -Wall -Wextra -DSAMPLECODE_DEMO SampleCode_Threads.c -o sample_threads
```

Or in Momentics: create/use a QNX C project, include only the selected file, and add the preprocessor symbol `SAMPLECODE_DEMO`. To compile reusable functions without the example `main()`:

```sh
qcc -std=gnu99 -Wall -Wextra -c SampleCode_ThreadScheduling.c
```

These samples follow the supplied QNX 7-era APIs. Build and run the QNX-specific examples on your installed target/SDP. The portable Linux checks below are not QNX runtime validation.

## Run the paired examples

| Pair | Start first | Start second | Finish |
|---|---|---|---|
| MessageQueueSend / MessageQueueReceive | Sender creates `/test_queue` and queues five messages plus `done` | Receiver drains messages | After receiver prints `done`, press Enter in sender to remove the queue |
| TrafficQueueSend / TrafficQueueReceive | Sender creates `/traffic_v3` | Receiver starts its input thread/state loop | Type `e`/`n` in sender; `q` stops both after the receiver has opened the queue |
| NamedServer / NamedClient | Server registers `myname` | Client sends five integer requests | Demo server exits when its client disconnects; pass `0` to `RunNamedServer` to keep it running |
| ChannelServer / ChannelClient | Server writes `/tmp/myServer.info` | Client reads PID/CHID and connects | Type `e`/`n`; `q` replies and then stops the server |

For the lab's traditional QNX message queues, the target needs the `mqueue` service. Traditional queue functions use libc; do not switch these cross-node examples to the alternate `libmq`/`mq` implementation. QNX 7 remote queues use `/net/<hostname>/<queue-name>` and need Qnet. Sender and receiver must refer to the **same** queue on the same host. Queue capacity limits depend on the target; the text queue retains the teacher's 100-message capacity.

The named client defaults to a same-node connection for an easy first test. To connect across nodes, pass the server's full Qnet name as its command-line argument:

```sh
./sample_named_client /net/VM_x86_Target01/dev/name/local/myname
```

Replace `VM_x86_Target01` with the actual server hostname. Qnet must already work between the nodes. This keeps the teacher's `name_attach → name_open → MsgSend → MsgReceive → MsgReply` sequence.

The PID/CHID pair uses `ND_LOCAL_NODE`: it connects separate processes on the same node, not only threads. Merely putting a remote pathname in that client's file lookup would not change its connection target.

## What changed, and why

- **SampleCode_SystemInfo.c:** Extracted the useful calls; added missing headers and hostname termination.
- **SampleCode_StructArray.c:** Kept card/GiveValue/PrintScan; condensed printing and checked array bounds.
- **SampleCode_Threads.c:** Combined duplicate workers into one parameterised thread function.
- **SampleCode_ThreadScheduling.c:** Moved attribute setup into CreatePriorityThread; default stack avoids the hard-coded 8000 bytes.
- **SampleCode_MessageQueueSend.c:** Extracted create/send calls, checked errors, and removed long fixed sleeps.
- **SampleCode_MessageQueueReceive.c:** Added buffer-size/termination checks before using received text.
- **SampleCode_Semaphore.c:** Kept user/changer logic; moved lock into data, removed unused fields, protected loop condition.
- **SampleCode_NamedSemaphore.c:** Kept user/changer logic; moved lock into data, removed unused fields, protected loop condition.
- **SampleCode_ProducerConsumer.c:** Extracted BufferInit/Put/Get/Destroy; removed unused condvar and unrelated scheduling setup.
- **SampleCode_TrafficStateMachine.c:** Kept switch/transitions/sleeps; passed input by value instead of racing a global scanf thread.
- **SampleCode_TrafficQueueSend.c:** Extracted create/send calls; added q/EOF shutdown so cleanup is reachable.
- **SampleCode_TrafficQueueReceive.c:** Added mutex-protected snapshots, error flag and q shutdown; kept separate receiver/state loop.
- **SampleCode_TimerPulse.c:** Extracted Start/StopPulseTimer; fixed error printing, priority query, interval comments and cleanup.
- **SampleCode_NamedClient.c:** Extracted one send/reply operation; kept padded header with fixed-width data and no pointer.
- **SampleCode_NamedServer.c:** Kept name_attach/receive/reply loop; separated native pulses from custom application messages.
- **SampleCode_ChannelClient.c:** Extracted connect/send functions; checked file/input errors and added q shutdown.
- **SampleCode_ChannelServer.c:** Extracted setup/server loop; kept message-driven state steps, added validation and q shutdown.

### Important details for explaining the adaptations

- **Semaphore versus mutex:** Lab3 Task2A and Task3A actually call `sem_wait/sem_post`, despite their original mutex comments. Their samples retain semaphores. Task5B uses `empty/full` semaphores plus a mutex; its unused condition variable was removed. No new condition-variable algorithm was substituted.
- **What a named semaphore shares:** processes opening the same semaphore name share that lock. Their ordinary C variables remain separate; shared memory or IPC is still needed to exchange data. The unnamed example uses `pshared=0` for threads in one process.
- **Named semaphore ownership:** the standalone demo now creates `/my_sem` exclusively, closes it, and unlinks it after its threads finish. Another participant opens it with `sem_open("/my_sem", 0)` and only closes it. A stale queue/semaphore name after a forced stop can cause `EEXIST`; remove it only after confirming no other instance uses it.
- **Shared input:** the queue receiver locks both updates and snapshots. Its mutex is released before the state machine sleeps. The standalone state-machine sample receives an input value directly, replacing the original unsynchronised keyboard thread. The queue sample keeps the latest input; it is not an event-latching or request-counting design.
- **Timing:** the sleep-based state machine still blocks for one or two seconds per call. Lab5 Task2 still advances only when a message arrives, and replies after that step finishes. For periodic advancement, use the timer pattern explicitly; it has not been silently added to the channel server.
- **Timer interval:** Lab4 Task3B's actual code used 1.0 seconds, although its comments said 1.5. The demo retains 1.0; `StartPulseTimer(&timer, 1, 500000000L)` selects 1.5. Green states take two timer ticks. Timer deletion, connection detach and channel destruction are now explicit.
- **Native pulses versus messages:** the named server receives into a union and decodes kernel pulses using the actual `struct _pulse`. Its custom header is used only for application messages. Pulses have no application `ClientID` and must not be replied to as ordinary messages. Disconnect pulses release the server-side connection; unblock pulses release the supplied receive ID.
- **32/64-bit layout:** the named pair retains the teacher's padded header idea but removes the unused pointer, uses fixed-width integers and checks field offsets/sizes at compile time. No choice of processor bitness is required in the source. Both new files must be used together; compatibility with an unmodified teacher binary is not promised. Matching byte order is still required, and actual mixed-target Qnet operation has not been tested here. The local PID/CHID pair deliberately retains the native header and requires compatible builds.
- **Return values:** most wrappers follow `0`/`-1` with `errno`; connect/create functions return a descriptor or `-1`. `CreatePriorityThread` follows pthread convention: `0` or an error number, printed with `strerror(err)`. Its stack size defaults to the target default instead of assuming 8000 bytes is suitable.
- **Blocking:** `MsgSend` waits for the reply; queue sends wait when full and receives wait when empty; buffer put/get wait on their semaphores. These examples do not add timeouts, reconnection, cancellation recovery or guaranteed deadlines.

The original semaphore labs also credit John Fehr's *Protecting Your Data in a Multi-Threaded App*. That credit is retained in both adapted semaphore files.

## Validation performed

- All **10 modules without QNX-only headers** passed GCC GNU C99 checks with `-Wall -Wextra -Werror`, both with and without the demo enabled.
- Ran the structure-array example: every 13th card printed all four kings, including card 52.
- Ran both semaphore examples: each produced the expected result of 1000.
- Ran the producer/consumer example: received A through O in order.
- Checked all eight traffic-state transitions and both green-state hold conditions in a scratch harness that skips sleeps.
- Compiled the named protocol's layout assertions on this host.
- No `qcc`, QNX headers, QNX target or Qnet was available here. The seven QNX-specific modules were reviewed but **not compiled or executed on QNX**. Message queue integration and explicit real-time priority behaviour also need target testing.

## QNX references checked for the API details

These references supplement the uploaded source attribution; the uploads remain the basis of the adaptations.

- [name_attach: channel flags and connect handling](https://qdn.qnx.com/developers/docs/7.1/com.qnx.doc.neutrino.lib_ref/topic/n/name_attach.html)
- [ChannelCreate: disconnect and unblock pulses](https://qnx.com/developers/docs/7.1/com.qnx.doc.neutrino.lib_ref/topic/c/channelcreate.html)
- [sigevent: SIGEV_PULSE_INIT](https://qdn.qnx.com/developers/docs/7.1/com.qnx.doc.neutrino.lib_ref/topic/s/sigevent.html)
- [Message information and received length](https://www.qnx.com/developers/docs/7.1/com.qnx.doc.neutrino.lib_ref/topic/m/_msg_info.html)
- [QNX message queue implementations](https://qnx.com/developers/docs/7.1/com.qnx.doc.neutrino.technotes/topic/managing_mq_mqueue.html)
