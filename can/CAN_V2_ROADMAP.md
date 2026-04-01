# TC375 CAN/CAN FD Library - API Boundary and V2 Roadmap

## 1. Goal

This library provides a static-resource, polling-first CAN platform layer on top of
TC375 + iLLD.

Design goals:

- C-based API
- no dynamic allocation
- static resource ownership
- predictable polling-first behavior
- incremental layering toward async-like usability
- preserve minimal stable core while allowing additive higher layers

---

## 2. Layering

1. BSP / Board Layer
2. MCU Driver Adapter Layer (TC3xx iLLD wrapper)
3. CAN Core Layer
4. Higher Utility Layer
   - CANOperation
   - CANExecutor
   - CANContext

---

## 3. Stability Boundary

### 3.1 Stable Surface (v1)

The following are treated as stable/minimal core APIs:

- CANCoreInit
- CANCoreInitOpenParams
- CANCoreOpen / CANCoreClose
- CANCoreStart / CANCoreStop
- CANCoreSend / CANCoreReceive
- CANCoreTrySend / CANCoreTryReceive
- CANCoreSendTimeout / CANCoreReceiveTimeout
- CANCoreGetLastStatus
- CANCoreGetStats / CANCoreResetStats

### 3.2 Extension Surface (v2-preparation)

The following are implemented but treated as additive extension layers:

- CANCoreQueryEvents
- CANCoreGetErrorState
- CANCoreRecover

Higher composition layers:

- CANOperation
- CANExecutor
- CANContext
- CANContextSubmitSend / Receive / Poll helpers

These do not replace the v1 core.
They are layered on top of the stable core.

---

## 4. Current Runtime Model

- no dynamic allocation
- caller-owned storage
- static slot arrays
- polling-first
- timeout uses caller-provided runtime hooks
- higher layers do not own CANCore or transport state

---

## 5. RX Path Policy

Current intended policy:

- CAN_RX_PATH_DEFAULT == CAN_RX_PATH_FIFO0
- FIFO0  : accept-all oriented path
- BUFFER : exact / explicit filter oriented path

Supported combinations:

- FIFO0  + filter disabled : supported
- FIFO0  + filter enabled  : unsupported
- BUFFER + filter enabled  : supported
- BUFFER + filter disabled : unsupported

---

## 6. Current Higher-Layer Model

### 6.1 CANOperation

Represents one pending unit of work:

- SEND
- RECEIVE
- POLL

Usage:

1. Prepare
2. Submit
3. RunOnce until completed

### 6.2 CANExecutor

Runs multiple operations using caller-provided static slots.

Current behavior:

- submit by pointer
- round-robin progression
- PollOne / RunOne / DispatchOne
- bounded drain with max_steps

### 6.3 CANContext

Thin facade over:

- CANCore
- CANExecutor
- CANOperation

Shortens user-facing call sequences while preserving static ownership.

---

## 7. Recommended Usage Tiers

### Tier 1: minimal synchronous use
- CANCoreOpen
- CANCoreStart
- CANCoreSend / CANCoreReceive

### Tier 2: timeout-aware polling
- CANCorePoll
- CANCoreSendTimeout
- CANCoreReceiveTimeout

### Tier 3: composable async-like building blocks
- CANOperation
- CANExecutor

### Tier 4: shortest facade
- CANContextSubmitSend
- CANContextSubmitReceive
- CANContextSubmitPoll
- CANContextPollOne / RunOne / DispatchOne / Poll

---

## 8. Optional Driver Ops Roadmap

### 8.1 QueryEvents
Status:
- implemented in core
- partial-real connected on TC3xx

Purpose:
- polling hint surface
- ready mask provider for higher layers

### 8.2 GetErrorState
Planned purpose:
- bus-off state
- error passive
- warning state
- TEC / REC snapshot

### 8.3 Recover
Planned purpose:
- software-assisted recovery entry point
- platform-specific bus-off/node reactivation handling

If optional op is not supported by a driver/platform, core returns:
- CAN_STATUS_EUNSUPPORTED

---

## 9. V2 Next Steps

### Step 1
Document and freeze API boundary.

### Step 2
Define CANCoreErrorState behavior and Recover contract.
Connect TC3xx optional ops.

### Step 3
Introduce multi-op service layer draft above CANExecutor/CANContext.

Candidate direction:
- service owns no memory
- caller provides operation arrays
- bounded polling budget
- no callbacks yet
- callback/reactor remains future work

---

## 10. Non-Goals for Current Stage

Not part of the current stable scope:

- callback-driven reactor
- dynamic allocation based scheduling
- handler queue ownership
- OS-dependent asynchronous backend abstraction
- hidden thread-based runtime

---

## Public API Boundary

### recommended public surface now
- can_core.h
- can_socket.h

### advanced composable surface
- can_operation.h
- can_executor.h
- can_context.h
- can_service.h

### platform / board specific surface
- can_platform.h
- board_*.h
- can_tc3xx*.h

Notes:
- For simple socket-like use, prefer `CANSocket`.
- For composable multi-op flows, use `CANService` or `CANContext`.
- `can_tc3xx*.h` remains platform-specific, not general public API.

---

## 11. Summary

Current state already supports:

- stable minimal CAN core
- timeout-aware polling
- operation abstraction
- static executor
- context facade
- helper-based shorter usage path
- partial real-port verification on TC3xx

Current recommended shortest public entry is:
- CANCore for minimal control
- CANSocket for shortest socket-like usage

V2 should remain additive and must not break the minimal stable core surface.