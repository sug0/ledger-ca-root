# ****************************************************************************
#    CA Root — Ledger certificate authority application.
#    (c) 2026 Ledger SAS.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
# ****************************************************************************

# Per-user local configuration: paths to the SDK, toolchain and Python venv.
# Copy mkconfig.example to mkconfig and adjust; mkconfig is gitignored.
-include mkconfig

# GNU make >= 4.2 is required: older releases (macOS ships 3.81) never set
# .SHELLSTATUS, which makes the SDK's install_params step fail with a bogus error
make_version_ok := $(shell printf '%s\n' '$(MAKE_VERSION)' | awk -F. '{ print ($$1 > 4 || ($$1 == 4 && $$2 >= 2)) ? 1 : 0 }')
ifneq ($(make_version_ok),1)
$(error GNU make >= 4.2 required, found $(MAKE_VERSION); on macOS: brew install make, then run gmake)
endif

# Optional toolchain wiring from mkconfig: PREPEND_PATH entries (space
# separated) go first on PATH, LEDGER_VENV points at a Python virtualenv with
# ledgerblue/ledgered/pillow whose bin/ also goes first.
empty :=
space := $(empty) $(empty)
ifneq ($(PREPEND_PATH),)
export PATH := $(subst $(space),:,$(strip $(PREPEND_PATH))):$(PATH)
endif
ifneq ($(LEDGER_VENV),)
ifneq ($(wildcard $(LEDGER_VENV)/bin),)
export PATH := $(LEDGER_VENV)/bin:$(PATH)
else
$(warning LEDGER_VENV=$(LEDGER_VENV) has no bin/ directory, ignoring)
endif
endif

# Path to a ledger-secure-sdk checkout (API level 27): set it in mkconfig or
# in the environment
ifeq ($(BOLOS_SDK),)
$(error BOLOS_SDK is not set: copy mkconfig.example to mkconfig, or export BOLOS_SDK)
endif

# Device to build for by default; override on the command line (make TARGET=stax)
TARGET ?= nanos2

# NB: LDFLAGS/CFLAGS from the calling environment (e.g. Homebrew's native
# build hints) survive the SDK makefiles' MAKEFLAGS re-exec with the SDK's
# own link flags already appended and get them appended a second time,
# corrupting the link; ambient flag variables are not part of this build
LDFLAGS :=
CFLAGS :=
unexport LDFLAGS CFLAGS

# Curve variant of the CA key: each variant builds as its own app with a
# distinct name, so both can coexist on the device. Override on the command
# line (make CA_VARIANT=p256) or use the ed25519/p256 wrapper targets.
CA_VARIANT ?= ed25519

ifeq ($(filter $(CA_VARIANT),ed25519 p256),)
$(error CA_VARIANT must be ed25519 or p256, found '$(CA_VARIANT)')
endif

# Separate build tree per variant; the override also beats the SDK's own
# BUILD_DIR assignment below, so every derived directory follows it
override BUILD_DIR = build/$(CA_VARIANT)

include $(BOLOS_SDK)/Makefile.target

########################################
#        Mandatory configuration       #
########################################
# Application name and curve, per variant. The USE_* define switches the
# curve-specific code paths at compile time.
ifeq ($(CA_VARIANT),ed25519)
DEFINES += USE_ED25519
APPNAME = "CA Root Ed25519"
CURVE_APP_LOAD_PARAMS = ed25519
else
DEFINES += USE_P256
APPNAME = "CA Root P256"
CURVE_APP_LOAD_PARAMS = secp256r1
endif

# Application version
APPVERSION_M = 1
APPVERSION_N = 0
APPVERSION_P = 0
APPVERSION = "$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)"

# Application source files
APP_SOURCE_PATH += src

# Application icons following guidelines:
# https://developers.ledger.com/docs/embedded-app/design-requirements/#device-icon
ICON_NANOX = icons/app_ca_root_14px.gif
ICON_NANOSP = icons/app_ca_root_14px.gif
ICON_STAX = icons/app_ca_root_32px.gif
ICON_FLEX = icons/app_ca_root_40px.gif
ICON_APEX_P = icons/app_ca_root_32px_apex.png

ifeq ($(TARGET_NAME),$(filter $(TARGET_NAME),TARGET_NANOX TARGET_NANOS2))
    # With the Nano NBGL Design, the Home Screen icon is the reverse of the App icon:
    # It should be on white background, with rounded corners.
    # This definition allows SDK Makefiles to automatically generate it based on the App icon.
    # Please note that the icon is dynamically generated, and declared in the .gitignore to avoid storing it.
    ICON_HOME_NANO = glyphs/home_ca_root_14px.gif
endif

# Application allowed derivation curves.
# Possible curves are: secp256k1, secp256r1, ed25519 and bls12381g1
# Set per variant above, matching the CA key derivation.

# Application allowed derivation paths.
# The CA key lives at m/8888'/0' (SLIP-0010 ed25519 / BIP32 P-256). The
# registry holds the beginning of the authorized path, and the OS grants
# derivations that extend it; production apps always derive strictly deeper
# than their registered entry (e.g. Solana registers 44'/501' and derives
# 44'/501'/0'/0'), so the prefix is registered rather than the exact path.
PATH_APP_LOAD_PARAMS = "8888'"

# Setting to allow building variant applications
# - <VARIANT_PARAM> is the name of the parameter which should be set
#   to specify the variant that should be build.
# - <VARIANT_VALUES> a list of variant that can be build using this app code.
VARIANT_PARAM = CA_VARIANT
VARIANT_VALUES = ed25519 p256

# Enabling DEBUG flag will enable PRINTF for speculos
#DEBUG = 1

# Enabling DEBUG_OVER_USB flag will enable PRINTF over USB
# This will force DISABLE_OS_IO_STACK_USE and add USB CDC profile
# The log can be displayed using a COM port terminal
#DEBUG_OVER_USB = 1

########################################
#     Application custom permissions   #
########################################
# See SDK `include/appflags.h` for the purpose of each permission
#HAVE_APPLICATION_FLAG_DERIVE_MASTER = 1
#HAVE_APPLICATION_FLAG_GLOBAL_PIN = 1
#HAVE_APPLICATION_FLAG_BOLOS_SETTINGS = 1
#HAVE_APPLICATION_FLAG_LIBRARY = 1

########################################
# Application communication interfaces #
########################################
#ENABLE_NFC = 1
ENABLE_NBGL_FOR_NANO_DEVICES = 1

########################################
#        Stack protection features     #
########################################
ENABLE_STACK_PROTECTOR = 1

########################################
#          Features disablers          #
########################################
# These advanced settings allow to disable some feature that are by
# default enabled in the SDK `Makefile.standard_app`.
#DISABLE_STANDARD_APP_FILES = 1
#DISABLE_DEFAULT_IO_SEPROXY_BUFFER_SIZE = 1 # To allow custom size declaration
#DISABLE_STANDARD_APP_DEFINES = 1 # Will set all the following disablers
#DISABLE_STANDARD_SNPRINTF = 1
#DISABLE_STANDARD_USB = 1
#DISABLE_STANDARD_WEBUSB = 1
#DISABLE_DEBUG_LEDGER_ASSERT = 1
#DISABLE_DEBUG_THROW = 1

include $(BOLOS_SDK)/Makefile.standard_app

# Recent clang lowers unaligned 4-byte accesses to __aeabi_* runtime calls
# that the SDK's prebuilt libraries don't provide; the SDK's libgcc.a does
LDLIBS += -lgcc

# NB: the SDK's load-offline calls `--offline` without a filename, which newer
# ledgerblue (>= 0.1.58) rejects; here it regenerates the root bin/app.apdu
load-offline: all
	python3 -m ledgerblue.loadApp $(APP_LOAD_PARAMS) --offline bin/app.apdu

########################################
#            Variant targets           #
########################################
# Plain make builds both variants; an explicit CA_VARIANT=... on the command
# line targets just that one.
ifeq ($(origin CA_VARIANT),command line)
.DEFAULT_GOAL := all
else
.DEFAULT_GOAL := both
endif

both: ed25519 p256

ed25519:
	+$(MAKE) CA_VARIANT=ed25519

p256:
	+$(MAKE) CA_VARIANT=p256

# Host unit tests (cmake + Unity): configure, build and run ctest. JOBS
# defaults to the machine's processor count; the SDK path is passed on
# explicitly because mkconfig sets it as a make variable, not in the
# environment cmake would inherit.
JOBS ?= $(shell getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)

unit-test:
	env BOLOS_SDK="$(BOLOS_SDK)" cmake -S unit-tests -B unit-tests/build
	cmake --build unit-tests/build -j $(JOBS)
	ctest --test-dir unit-tests/build --output-on-failure

# End-to-end tests for the current variant. The backend (speculos emulator
# or a real device over USB) must be given explicitly; both need a python
# venv, see tests/README.md.
SPECULOS_VENV ?= $(HOME)/.venvs/speculos
PYTEST_ARGS ?=

test: $(CA_VARIANT)
	@test -n "$(BACKEND)" || { echo "usage: make test BACKEND=speculos|device [CA_VARIANT=ed25519|p256]"; exit 2; }
	CA_VARIANT=$(CA_VARIANT) $(SPECULOS_VENV)/bin/python -m pytest tests/ -q --backend $(BACKEND) $(PYTEST_ARGS)

# Full cleanup across variants: the SDK's clean only removes $(BUILD_DIR)
cleanall:
	rm -fr build bin debug

.PHONY: both ed25519 p256 cleanall test unit-test
