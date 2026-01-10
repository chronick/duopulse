---
id: 36
slug: 36-segfault-led-broken-drift-test
title: "BUGFIX: Segfault in test_led_indicator.cpp broken-drift tests"
status: archived
created_date: 2026-01-04
updated_date: 2026-01-04
branch: bugfix/36-segfault-led-test
priority: low
---

# Task 36: BUGFIX - Segfault in LED Indicator Broken/Drift Tests

## Problem

Pre-existing segfault in `tests/test_led_indicator.cpp` affecting broken-drift tests:

```
tests/test_led_indicator.cpp:489: FAILED:
  {Unknown expression after the reported line}
due to a fatal error condition:
  SIGSEGV - Segmentation violation signal
```

## Affected Tests

- "High BROKEN + High DRIFT produces maximum irregularity" (line 462)
- "BROKEN parameter affects timing irregularity" (around line 355)

## Symptoms

- Test assertions PASS but segfault occurs immediately after
- Crash happens during test teardown or next test setup
- Issue exists independently of Task 34 changes (verified by reverting)

## Suspected Causes

1. Stack overflow from large `std::vector<float>` in test (1000 elements)
2. Memory corruption in LedIndicator affecting test cleanup
3. Catch2 internal issue with test ordering

## Workaround

Tests can be skipped with tag `[!mayfail]` or excluded from test run.

## Investigation Steps

- [ ] Add address sanitizer to build (`-fsanitize=address`)
- [ ] Check for uninitialized memory access
- [ ] Verify LedIndicator destructor (if any)
- [ ] Try reducing vector size in test
- [ ] Check for stack overflow

## Notes

Discovered during Task 34 implementation. Does not block V5 functionality - only affects test suite.

## Archive Note

Archived 2026-01-04 after attempting repro:
- `./build/test_runner "[led-feedback][broken-drift]" -a --success` passes without segfault.
- `./build/test_runner "[led-feedback][parameter-feedback]" -a --success` passes without segfault.
- `make test` fails on unrelated `GetMetricWeight` assertions in `tests/test_axis_biasing.cpp`, not a crash.

No crash reproduced in isolation. If the segfault reappears, retry with ASAN on host tests.
