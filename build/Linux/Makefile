#
# Top-level convenience makefile for Linux builds.
# Builds the wxPlotting static library and demo applications.
#
# Usage:
#   make                 # Debug: library and both demos
#   make demo            # Interactive demo
#   make realtime        # Real-time demo
#   make all             # Both demos
#   make CONF=Release
#   make lib
#   make clean
#

CONF ?= Debug

.PHONY: all lib demo realtime clean

all: demo realtime

lib:
	$(MAKE) -C wxPlotting CONF=$(CONF)

# Build the library once, then the apps. Each demo makefile also builds the
# library as a subproject; skip that here so `make -j` does not archive the
# same objects twice (ranlib then reports a truncated member).
demo: lib
	$(MAKE) -C wxPlottingDemo CONF=$(CONF) SUB=no

realtime: lib
	$(MAKE) -C wxPlottingRealTimeDemo CONF=$(CONF) SUB=no

clean:
	$(MAKE) -C wxPlottingDemo CONF=$(CONF) clean
	$(MAKE) -C wxPlottingRealTimeDemo CONF=$(CONF) SUB=no clean
	$(MAKE) -C wxPlotting CONF=$(CONF) clean
