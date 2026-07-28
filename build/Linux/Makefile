#
# Top-level convenience makefile for Linux builds.
# Builds the wxPlotting static library and the demo application.
#
# Usage:
#   make              # Debug (demo + library)
#   make CONF=Release
#   make lib          # library only
#   make clean
#

CONF ?= Debug

.PHONY: all lib demo clean

all: demo

lib:
	$(MAKE) -C wxPlotting CONF=$(CONF)

# Demo makefile builds the library as a subproject by default.
demo:
	$(MAKE) -C wxPlottingDemo CONF=$(CONF)

clean:
	$(MAKE) -C wxPlottingDemo CONF=$(CONF) clean
