# Forwards every target to the course project so `make build` etc. work from the repo root.
TARGETS := build flash serial debug gdb-server size symbols disasm reset clean

.DEFAULT_GOAL := build
.PHONY: $(TARGETS)

$(TARGETS):
	@$(MAKE) --no-print-directory -C microcontroller-embedded-c-programming $@
