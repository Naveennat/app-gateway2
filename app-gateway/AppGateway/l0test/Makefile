# Standalone Makefile for AppGateway l0tests
# - Builds local test binary appgateway_l0test
# - Links against WPEFramework from PREFIX
# - Tries to link local plugin .so if available, falls back to installed one
# - Provides run and coverage targets
#
# Usage:
#   make [all|build]       # Build the test binary
#   make run               # Run with proper LD_LIBRARY_PATH; hints about APPGATEWAY_RESOLUTIONS_PATH
#   make coverage          # Generate coverage.info and ./coverage/index.html (requires lcov + genhtml)
#   make clean             # Clean artifacts
#   make env-check         # Print detected environment and presence of libs
#
# Note:
# - This Makefile does not modify or rely on AppGateway/CMakeLists.txt.
# - PREFIX defaults to the provided dependencies install location but can be overridden.

# ---------- Configuration ----------
PREFIX ?= /home/kavia/workspace/code-generation/dependencies/install

CXX       ?= g++
CXXFLAGS  ?= -std=gnu++11 -O0 -g --coverage -fPIC -D__CORE_MESSAGING__ -D__CORE_NO_WCHAR_SUPPORT__ -DMODULE_NAME=Plugin_AppGateway
# Add include directories:
#   - Local project headers
#   - Thunder/WPEFramework headers
CPPFLAGS  ?=
INC_DIRS  := \
  ../../helpers \
  ../resolutions \
  ../../Supporting_Files \
  ../../interfaces \
  $(PREFIX)/include/WPEFramework
CPPFLAGS  += $(addprefix -I,$(INC_DIRS))

# Linker flags and libraries
LDFLAGS   ?= --coverage -Wl,-rpath,'$$ORIGIN:$(PREFIX)/lib' -L$(PREFIX)/lib

# Required Thunder libs (conditionally included if present)
REQUIRED_LIBS := WPEFrameworkCore WPEFrameworkMessaging WPEFrameworkTracing WPEFrameworkPlugins
LDLIBS :=

# Detect and include each library only if present to avoid link errors
ifneq ($(wildcard $(PREFIX)/lib/libWPEFrameworkCore.*),)
  LDLIBS += -lWPEFrameworkCore
else
  $(warning Missing libWPEFrameworkCore in $(PREFIX)/lib)
endif
ifneq ($(wildcard $(PREFIX)/lib/libWPEFrameworkMessaging.*),)
  LDLIBS += -lWPEFrameworkMessaging
else
  $(warning Missing libWPEFrameworkMessaging in $(PREFIX)/lib)
endif
ifneq ($(wildcard $(PREFIX)/lib/libWPEFrameworkTracing.*),)
  LDLIBS += -lWPEFrameworkTracing
else
  $(warning Missing libWPEFrameworkTracing in $(PREFIX)/lib)
endif
ifneq ($(wildcard $(PREFIX)/lib/libWPEFrameworkPlugins.*),)
  LDLIBS += -lWPEFrameworkPlugins
else
  $(warning Missing libWPEFrameworkPlugins in $(PREFIX)/lib)
endif

# Common POSIX libs often required by Thunder/Core
LDLIBS += -ldl -lpthread

# Plugin shared object detection
LOCAL_PLUGIN_SO     := ../../../../build/appgatewayl0test/AppGateway/libWPEFrameworkAppGateway.so
INSTALLED_PLUGIN_SO := $(PREFIX)/lib/plugins/libWPEFrameworkAppGateway.so

ifeq ($(wildcard $(LOCAL_PLUGIN_SO)),)
  ifeq ($(wildcard $(INSTALLED_PLUGIN_SO)),)
    PLUGIN_SO := $(INSTALLED_PLUGIN_SO)
    $(warning libWPEFrameworkAppGateway.so not found at $(LOCAL_PLUGIN_SO) or $(INSTALLED_PLUGIN_SO). Will still attempt to link with $(PLUGIN_SO).)
  else
    PLUGIN_SO := $(INSTALLED_PLUGIN_SO)
  endif
else
  PLUGIN_SO := $(LOCAL_PLUGIN_SO)
endif

# ---------- Sources and Targets ----------
TARGET := appgateway_l0test

SRCS := \
  AppGatewayTest.cpp \
  AppGateway_Init_DeinitTests.cpp \
  AppGateway_JsonRpcResolveTests.cpp \
  Resolver_Configure_And_ResolveTests.cpp \
  Responder_BehaviorTests.cpp \
  ContextUtils_ConversionTests.cpp

OBJS := $(SRCS:.cpp=.o)

.PHONY: all build run coverage clean env-check

all: $(TARGET)
build: all

# Link final binary
$(TARGET): $(OBJS)
	@echo "Linking $@"
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS) $(PLUGIN_SO)

# Compile objects
%.o: %.cpp
	@echo "Compiling $<"
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

# Run with proper library path; respect APPGATEWAY_RESOLUTIONS_PATH if already set
run: $(TARGET)
	@echo "Running ./$(TARGET)"
	@if [ -z "$$APPGATEWAY_RESOLUTIONS_PATH" ]; then \
	  echo "Hint: APPGATEWAY_RESOLUTIONS_PATH is not set."; \
	  echo "      To override resolutions, set it like:"; \
	  echo "      export APPGATEWAY_RESOLUTIONS_PATH=$$(pwd)/../resolutions"; \
	fi
	@LD_LIBRARY_PATH="$(PREFIX)/lib:." APPGATEWAY_RESOLUTIONS_PATH="$$APPGATEWAY_RESOLUTIONS_PATH" ./$(TARGET)

# Generate coverage report (requires lcov and genhtml)
coverage: $(TARGET)
	@echo "Capturing coverage (lcov) and generating report (genhtml)..."
	@lcov -c -o coverage.info -d . --ignore-errors empty && genhtml -o coverage coverage.info
	@echo "Coverage report generated at ./coverage/index.html"

# Clean artifacts
clean:
	@echo "Cleaning artifacts..."
	@rm -f $(OBJS) $(TARGET) coverage.info
	@rm -rf coverage

# Environment diagnostics
env-check:
	@echo "Environment check:"
	@echo "  PREFIX     : $(PREFIX)"
	@echo "  Using CXX  : $(CXX)"
	@echo "  CXXFLAGS   : $(CXXFLAGS)"
	@echo "  CPPFLAGS   : $(CPPFLAGS)"
	@echo "  LDFLAGS    : $(LDFLAGS)"
	@echo "  LDLIBS     : $(LDLIBS)"
	@echo "  PLUGIN_SO  : $(PLUGIN_SO)"
	@if [ -f "$(PLUGIN_SO)" ]; then echo "  Plugin .so : FOUND ($(PLUGIN_SO))"; else echo "  Plugin .so : NOT FOUND ($(PLUGIN_SO))"; fi
	@echo "  Required libs in $(PREFIX)/lib:"
	@for l in $(REQUIRED_LIBS); do \
	  if ls "$(PREFIX)/lib/lib$$l."* >/dev/null 2>&1; then \
	    echo "    [OK] $$l"; \
	  else \
	    echo "    [MISSING] $$l"; \
	  fi; \
	done
