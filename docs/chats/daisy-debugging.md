Yep — you can do this on macOS with zero Python deps. Two solid options:

Option A: screen + tee (minimal, ubiquitous)

This is the simplest. It won’t auto-reconnect, but it’s great for manual capture.

Makefile

BAUD ?= 115200
PORT ?= $(shell ls -t /dev/cu.usbmodem* 2>/dev/null | head -n 1)
LOGDIR ?= logs
APP ?= patchinit

listen:
	@test -n "$(PORT)" || (echo "No /dev/cu.usbmodem* found"; exit 1)
	@mkdir -p "$(LOGDIR)"
	@echo "Listening on $(PORT) @ $(BAUD)"
	@echo "Log: $(LOGDIR)/$(APP)_$$(date +%Y%m%d_%H%M%S).log"
	@screen "$(PORT)" "$(BAUD)" | tee -a "$(LOGDIR)/$(APP)_$$(date +%Y%m%d_%H%M%S).log"

ports:
	@ls -1 /dev/cu.usbmodem* 2>/dev/null || true

Notes:
	•	Quit screen: Ctrl-A then \ then y
	•	Because we generate the timestamp twice above, the filename could differ; if you care, use a single shell line (below).

Better (single timestamp)

listen:
	@test -n "$(PORT)" || (echo "No /dev/cu.usbmodem* found"; exit 1)
	@mkdir -p "$(LOGDIR)"
	@ts=$$(date +%Y%m%d_%H%M%S); \
	  log="$(LOGDIR)/$(APP)_$$ts.log"; \
	  echo "Listening on $(PORT) @ $(BAUD) -> $$log"; \
	  screen "$(PORT)" "$(BAUD)" | tee -a "$$log"

Option B: Auto-reconnect with only shell + stty + cat (no extra deps)

This one will recover after flashing/reboot by waiting for the port to reappear and reopening.

BAUD ?= 115200
PORT_GLOB ?= /dev/cu.usbmodem*
LOGDIR ?= logs
APP ?= patchinit

listen:
	@mkdir -p "$(LOGDIR)"
	@ts=$$(date +%Y%m%d_%H%M%S); \
	  log="$(LOGDIR)/$(APP)_$$ts.log"; \
	  echo "Auto-reconnect listen -> $$log"; \
	  while true; do \
	    port=$$(ls -t $(PORT_GLOB) 2>/dev/null | head -n 1); \
	    if [ -z "$$port" ]; then sleep 0.2; continue; fi; \
	    echo "[listen] opening $$port @ $(BAUD)"; \
	    # Configure the TTY (raw-ish). Some flags may vary by macOS version. \
	    stty -f "$$port" $(BAUD) raw -echo -icanon min 1 time 1 2>/dev/null || true; \
	    # Read until the device disappears; then loop and reconnect \
	    cat "$$port" 2>/dev/null | tee -a "$$log"; \
	    echo "[listen] disconnected; waiting..."; \
	    sleep 0.2; \
	  done

This works because:
	•	on disconnect, cat errors out → loop continues
	•	on reconnect, a new /dev/cu.usbmodem* appears → we reopen it

Small caveat

If your firmware calls StartLog(true), it may pause until the host opens the port — which is fine, but it means your logs start once the loop reconnects (expected).

⸻

My pick
	•	If you want reconnect after flashing: Option B.
	•	If you want interactive terminal (send characters back to the device): use screen (Option A), but you’ll need to manually re-run after flashing unless you wrap it.

If you tell me what your flash command is (OpenOCD vs DFU), I can add a make debug that does: flash && (wait for usbmodem) && listen cleanly.