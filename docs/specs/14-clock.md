# Clock and Reset

[← Back to Index](00-index.md) | [← Boot Behavior](13-boot.md)

---

## 1. Clock Source Selection

| Condition | Clock Source | Behavior |
|-----------|--------------|----------|
| Gate In 1 unpatched | Internal Metro | Steps advance on internal clock |
| Gate In 1 patched | External only | Internal clock disabled |

External clock is **exclusive**: no timeout fallback, no parallel operation.

---

## 2. Reset Behavior

Reset (Gate In 2) always resets to step 0. Pattern regeneration occurs at bar boundaries.

---

[Next: Testing Requirements →](15-testing.md)
