#
# Top-level convenience makefile for Linux builds.
# Builds the wxPlotting static library and demo applications.
#
# Usage:
#   make                 # Debug (interactive demo + library)
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

# Demo makefile builds the library as a subproject by default.
demo:
	$(MAKE) -C wxPlottingDemo CONF=$(CONF)

realtime:
	$(MAKE) -C wxPlottingRealTimeDemo CONF=$(CONF)

clean:
	$(MAKE) -C wxPlottingDemo CONF=$(CONF) clean
	$(MAKE) -C wxPlottingRealTimeDemo CONF=$(CONF) SUB=no clean
	$(MAKE) -C wxPlotting CONF=$(CONF) clean
