# CAN V2 Roadmap / CAN V2 로드맵

---

## 1. Scope / 문서 범위

### English
This roadmap defines the current baseline, design direction, implemented features, constraints, and next steps for the **TC375 + iLLD CAN/CAN FD library**.

### 한국어
이 로드맵은 **TC375 + iLLD 기반 CAN/CAN FD 라이브러리**의 현재 기준선, 설계 방향, 구현 항목, 제약, 다음 단계를 정리한다.

---

## 2. Goal / 목표

### English
Build a practical CAN/CAN FD platform library for TC375 + iLLD with:
- no dynamic allocation,
- static-resource-oriented design,
- polling-first behavior,
- a simple public API surface,
- and a clean separation between platform/core/socket layers.

The library should be usable both:
- directly by application code, and
- indirectly by higher-level libraries built on top of it.

### 한국어
다음을 만족하는 TC375 + iLLD 기반 CAN/CAN FD 플랫폼 라이브러리를 구축한다.
- 동적 할당 없음
- 정적 자원 중심 설계
- polling 우선 동작
- 단순한 공개 API surface
- platform/core/socket 계층 간의 명확한 분리

이 라이브러리는
- application 코드가 직접 사용할 수도 있어야 하고,
- 그 위에 올라가는 상위 라이브러리들이 간접적으로 사용할 수도 있어야 한다.

---

## 3. Layering / 계층 구조

### English
Current intended layering:
1. BSP / Board Layer
2. MCU Driver Adapter Layer (TC3xx iLLD wrapper)
3. CAN Core Layer
4. CAN Socket Layer
5. Upper application/domain libraries (out of scope for this roadmap)

### 한국어
현재 의도하는 계층 구조:
1. BSP / Board Layer
2. MCU Driver Adapter Layer (TC3xx iLLD wrapper)
3. CAN Core Layer
4. CAN Socket Layer
5. 상위 application/domain 라이브러리 (이 로드맵 범위 밖)

---

## 4. Public Surface Direction / 공개 API 방향

### English
Recommended simple public surface:
- `can_core.h`
- `can_socket.h`
- `can_types.h`

Internal/platform-specific surface:
- `can_platform.h`
- `can_tc3xx*.h`
- `board/*`

Higher-level domain wrappers should depend on the public CAN library surface, not the other way around.

### 한국어
권장 단순 공개 surface:
- `can_core.h`
- `can_socket.h`
- `can_types.h`

내부/platform 전용 surface:
- `can_platform.h`
- `can_tc3xx*.h`
- `board/*`

상위 도메인 wrapper는 공개 CAN 라이브러리 surface를 의존해야 하며, 그 반대가 되어서는 안 된다.

---

## 5. Current Baseline / 현재 기준선

### English
The CAN library baseline now includes:
- board/platform/core/socket layering,
- frame helper APIs,
- owning and non-owning socket usage modes,
- and helper APIs for common send/receive flows.

The CAN library baseline does **not** include domain-specific protocol logic.

### 한국어
현재 CAN 라이브러리 기준선에는 다음이 포함된다.
- board/platform/core/socket 계층 구조
- frame helper API
- owning / non-owning socket 사용 방식
- 자주 쓰는 send/receive 흐름을 위한 helper API

현재 CAN 라이브러리 기준선에는 도메인 전용 프로토콜 로직은 포함되지 않는다.

---

## 6. Implemented Features / 현재 구현 완료 항목

### 6.1 Core Layer / Core 계층

### English
Implemented core-oriented features include:
- initialization/open/close/start/stop flows,
- send/receive and try-send/try-receive,
- timeout-aware send/receive,
- poll/event query,
- error-state query and recovery hooks,
- status and stats query/reset.

### 한국어
구현된 core 중심 기능:
- initialization/open/close/start/stop 흐름
- send/receive 및 try-send/try-receive
- timeout 기반 send/receive
- poll/event query
- error-state query 및 recovery hook
- status/stats 조회 및 reset

### 6.2 Socket Layer / Socket 계층

### English
Implemented socket features:
- `CANSocketInit()`
- `CANSocketInitOpenParams()`
- `CANSocketInitOpenParamsClassic500k()`
- `CANSocketInitOpenParamsFd500k2M()`
- `CANSocketOpen()` / `CANSocketClose()`
- `CANSocketBindCore()` / `CANSocketUnbind()`
- `CANSocketSendNow()` / `CANSocketReceiveNow()`
- `CANSocketWaitTxReady()` / `CANSocketWaitRxReady()`
- `CANSocketSendClassicStd()` / `CANSocketSendClassicExt()`
- `CANSocketSendFdStd()` / `CANSocketSendFdExt()`
- `CANSocketReceiveMatch()` / `CANSocketReceiveTimeoutMatch()`

### 한국어
구현된 socket 기능:
- `CANSocketInit()`
- `CANSocketInitOpenParams()`
- `CANSocketInitOpenParamsClassic500k()`
- `CANSocketInitOpenParamsFd500k2M()`
- `CANSocketOpen()` / `CANSocketClose()`
- `CANSocketBindCore()` / `CANSocketUnbind()`
- `CANSocketSendNow()` / `CANSocketReceiveNow()`
- `CANSocketWaitTxReady()` / `CANSocketWaitRxReady()`
- `CANSocketSendClassicStd()` / `CANSocketSendClassicExt()`
- `CANSocketSendFdStd()` / `CANSocketSendFdExt()`
- `CANSocketReceiveMatch()` / `CANSocketReceiveTimeoutMatch()`

### 6.3 Frame Helper Layer / Frame Helper 계층

### English
Implemented frame helpers:
- `CANFrameInit()`
- `CANFrameSetData()`
- `CANFrameInitClassicStd()` / `CANFrameInitClassicExt()`
- `CANFrameInitFdStd()` / `CANFrameInitFdExt()`

### 한국어
구현된 frame helper:
- `CANFrameInit()`
- `CANFrameSetData()`
- `CANFrameInitClassicStd()` / `CANFrameInitClassicExt()`
- `CANFrameInitFdStd()` / `CANFrameInitFdExt()`

---

## 7. Design Decisions / 설계 결정 사항

### 7.1 Resource Model / 자원 모델

### English
The library remains:
- static-resource-oriented,
- allocation-free,
- and polling-first.

Dynamic callback/event-queue style APIs may be added later, but they are not the baseline.

### 한국어
이 라이브러리는 계속해서 다음 원칙을 유지한다.
- 정적 자원 중심
- 동적 할당 없음
- polling 우선

callback/event-queue 스타일 API는 나중에 추가될 수 있으나, 현재 기준선은 아니다.

### 7.2 Socket Ownership Model / Socket 소유권 모델

### English
`CANSocket` supports both:
- non-owning bind to an external `CANCore`
- owning open/close using embedded core storage

This dual-mode design is intentional and is part of the library itself.

### 한국어
`CANSocket`은 다음 두 경로를 모두 지원한다.
- 외부 `CANCore`에 대한 non-owning bind
- 내부 core 저장소를 사용하는 owning open/close

이 이중 모드 설계는 의도된 것이며, 라이브러리 자체의 기능에 포함된다.

### 7.3 Receive Path Policy / 수신 경로 정책

### English
Current intended combinations:
- `FIFO0 + filter disabled` : supported
- `FIFO0 + filter enabled` : unsupported
- `BUFFER + filter enabled` : supported
- `BUFFER + filter disabled` : unsupported

`CAN_RX_PATH_DEFAULT` is currently treated as the default FIFO0-style path.

### 한국어
현재 의도된 조합:
- `FIFO0 + filter disabled` : 지원
- `FIFO0 + filter enabled` : 미지원
- `BUFFER + filter enabled` : 지원
- `BUFFER + filter disabled` : 미지원

현재 `CAN_RX_PATH_DEFAULT`는 기본 FIFO0 성격의 경로로 취급한다.

### 7.4 Helper Semantics / Helper 의미

### English
Socket helper APIs are intended to reduce boilerplate, not to hide all transport semantics.

Examples:
- `CANSocketSendClassicStd()` builds a temporary frame and sends it.
- `CANSocketReceiveTimeoutMatch()` searches for a matching frame and may consume non-matching frames while searching.

### 한국어
socket helper API는 boilerplate를 줄이기 위한 것이지, transport 의미 전체를 감추기 위한 것은 아니다.

예:
- `CANSocketSendClassicStd()`는 임시 frame을 구성한 뒤 송신한다.
- `CANSocketReceiveTimeoutMatch()`는 매칭되는 frame을 찾는 동안 non-matching frame을 소비할 수 있다.

---

## 8. Validation Status / 검증 상태

### English
The CAN library has validation coverage for:
- 2-board real CAN communication on TC375 Lite Kit,
- socket poll/send/receive flows,
- timeout-aware behavior,
- open/close and helper-driven paths,
- frame-helper self-tests.

This means the CAN library is beyond an experimental skeleton and is already usable as a real transport library baseline.

### 한국어
현재 CAN 라이브러리는 다음 항목에 대한 검증 범위를 가진다.
- TC375 Lite Kit 2-board 실물 CAN 통신
- socket poll/send/receive 흐름
- timeout 기반 동작
- open/close 및 helper 기반 경로
- frame helper self-test

즉, 현재 CAN 라이브러리는 단순 실험용 골격을 넘어 실제 transport 라이브러리 기준선으로 사용할 수 있는 상태이다.

---

## 9. Known Constraints / 현재 제약 사항

### 9.1 Platform Lifecycle / Platform 수명주기

### English
`CANSocketOpen()` / `CANSocketClose()` currently call platform init/deinit internally.
This is convenient for simple owning usage, but it is not yet the final answer for broader multi-socket ownership inside one process.

### 한국어
`CANSocketOpen()` / `CANSocketClose()`는 현재 내부에서 platform init/deinit을 호출한다.
이는 단순한 owning 사용에는 편리하지만, 한 프로세스 안에서 여러 socket을 일반화해 다루는 최종 해법은 아직 아니다.

### 9.2 Polling Baseline / Polling 기준선

### English
The current library is intentionally polling-first.
Interrupt/callback/event-queue driven patterns are not yet the main public model.

### 한국어
현재 라이브러리는 의도적으로 polling-first 기준선을 유지한다.
interrupt/callback/event-queue 기반 패턴은 아직 주 공개 모델이 아니다.

### 9.3 Documentation Name Consistency / 문서 이름 정합성

### English
Doxygen-facing naming, public header naming, and implementation symbol naming must remain aligned.
Documentation refactors must not accidentally rename the actual C API unless a real API rename is intended.

### 한국어
Doxygen용 이름, 공개 헤더 이름, 구현 심볼 이름은 계속 정합성을 유지해야 한다.
문서 정리를 하다가 실제 C API가 의도치 않게 변경되면 안 된다.

---

## 10. Next Steps / 다음 단계

### 10.1 Short Term / 단기

### English
1. Keep the CAN public surface stable around `can_core.h`, `can_socket.h`, and `can_types.h`.
2. Maintain regression coverage for open/close, helper send paths, and frame-helper tests.
3. Clean up Doxygen and roadmap documents to reflect the CAN library only.

### 한국어
1. `can_core.h`, `can_socket.h`, `can_types.h` 중심의 공개 surface를 안정화한다.
2. open/close, helper send 경로, frame-helper test에 대한 회귀 검증을 유지한다.
3. Doxygen 및 roadmap 문서를 CAN 라이브러리 자체 기준으로 정리한다.

### 10.2 Mid Term / 중기

### English
1. Revisit platform lifecycle generalization for broader multi-socket ownership.
2. Decide whether additional convenience APIs are needed for common application patterns.
3. Expand validation for Classical CAN and CAN FD combinations.

### 한국어
1. 더 넓은 multi-socket ownership을 위해 platform lifecycle 일반화를 다시 검토한다.
2. 자주 쓰는 application 패턴을 위한 추가 convenience API가 필요한지 판단한다.
3. Classical CAN / CAN FD 조합에 대한 검증 범위를 넓힌다.

### 10.3 Long Term / 장기

### English
1. Improve portability across boards and platform variants.
2. Add more optional observability/debug hooks if needed.
3. Consider future event-driven or callback-based extensions without breaking the polling-first baseline.

### 한국어
1. 보드 및 플랫폼 변형에 대한 이식성을 높인다.
2. 필요 시 관측성/디버깅용 선택적 hook를 추가한다.
3. polling-first 기준선을 깨지 않   는 범위에서 event-driven/callback 기반 확장을 검토한다.

---

## 11. Summary / 요약

### English
The CAN V2 roadmap should now be understood as a roadmap for the **CAN library itself**, centered on:
- platform/core/socket layering,
- helper APIs,
- static-resource and polling-first design,
- and a stable public transport library surface.

Upper-layer domain wrappers may evolve independently on top of this library.

### 한국어
이제 CAN V2 로드맵은 다음을 중심으로 하는 **CAN 라이브러리 자체의 로드맵**으로 이해해야 한다.
- platform/core/socket 계층 구조
- helper API
- 정적 자원 + polling-first 설계
- 안정적인 공개 transport 라이브러리 surface

상위 도메인 wrapper는 이 라이브러리 위에서 별도로 독립적으로 발전할 수 있다.
