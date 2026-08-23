#---------------------------------------------------------------------------------
.SUFFIXES:
.SECONDARY:
#---------------------------------------------------------------------------------
# The host tests deliberately need no cross-compiler: requiring devkitPPC to run
# them would put them out of reach on the machine where they are most useful.
# Named once, so the guard below and the rules further down cannot drift apart --
# `make test-player` on a laptop with no cross-compiler has to work too.
HOST_TESTS := test-obstacles test-player test-characters test-stars test-playfield

ifeq ($(filter test $(HOST_TESTS),$(MAKECMDGOALS)),)
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules
endif

#---------------------------------------------------------------------------------
TARGET      := moonlight-drift
BUILD       := build
SOURCES     := source ../magnolia/source ../magnolia/font
DATA        :=
INCLUDES    := ../magnolia ../magnolia/source ../magnolia/font source

CFLAGS      = -g -O2 -Wall $(MACHDEP) $(INCLUDE)
CXXFLAGS    = $(CFLAGS)
LDFLAGS     = -g $(MACHDEP) -Wl,-Map,$(notdir $@).map

LIBS        := -lgrrlib -lpngu -lfreetype -lpng -ljpeg -lz -lbrotlidec -lbrotlicommon -lbz2 -lfat -lasnd -lwiiuse -lbte -logc -lm
LIBDIRS     := $(PORTLIBS)

#---------------------------------------------------------------------------------
ifeq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

TOPDIR := $(CURDIR)/..

export OUTPUT   := $(CURDIR)/$(TARGET)
export VPATH    := $(foreach dir,$(SOURCES),$(TOPDIR)/$(dir))
export DEPSDIR  := $(CURDIR)

# Sprites and audio are linked into the binary rather than read from SD. Assets
# on a card can go missing, go stale, or disagree with the code -- all three
# have happened here. bin2s emits one assembly blob plus a header of symbols;
# -a 32 matches ASND's DMA alignment so audio plays straight out of the image
# with no copy.
BIN2S       := $(DEVKITPRO)/tools/bin/bin2s
ASSETFILES  := $(wildcard $(TOPDIR)/sprites/*.png) $(wildcard $(TOPDIR)/audio/*.pcm)

CFILES      := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(TOPDIR)/$(dir)/*.c)))
CPPFILES    := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(TOPDIR)/$(dir)/*.cpp)))

ifeq ($(strip $(CPPFILES)),)
    export LD  := $(CC)
else
    export LD  := $(CXX)
endif

export OFILES   := assets.o $(addsuffix .o,$(basename $(CPPFILES) $(CFILES)))
export INCLUDE  := $(foreach dir,$(INCLUDES),-I$(TOPDIR)/$(dir)) \
                   $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                   -I$(CURDIR) -I$(CURDIR)/$(BUILD) -I$(LIBOGC_INC)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib) -L$(LIBOGC_LIB)

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).dol: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

assets.s: $(ASSETFILES)
	@echo "embedding $(words $(ASSETFILES)) assets ..."
	@$(BIN2S) -a 32 -H assets.h $(ASSETFILES) > assets.s
	@echo "" >> assets.s

assets.h: assets.s

assets.o: assets.s
	@$(AS) -o $@ assets.s

# Every translation unit may include assets.h, so it must exist first.
$(filter-out assets.o,$(OFILES)): assets.h

-include $(DEPENDS)

#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------

.PHONY: $(BUILD) clean run deploy dolphin test $(HOST_TESTS)

# Stated explicitly: the host-test rules below would otherwise be the first
# targets in the file, and a bare `make` would run tests instead of building.
.DEFAULT_GOAL := $(BUILD)

# Host-side tests. Anything in source/ free of libogc runs on the machine you
# are sitting at, in a second, with no emulator in the loop -- which is where
# game rules belong, because a rule that is subtly wrong looks exactly like a
# rule that is right until someone plays far enough to notice.
#
# One binary per test file, each naming what it links. The list is the record of
# which modules are host-clean; a wildcard over source/ would try to link the
# rendering and fail with something far less informative.
#
# Note the plain mkdir rather than an order-only dependency on $(BUILD): here
# $(BUILD) is the target that cross-compiles the game, not merely a directory,
# so depending on it would drag devkitPPC into the one target that exists for
# not needing it.
HOSTCC     ?= cc
HOSTCFLAGS := -Wall -Wextra -O2 -I source -I tests -I ../magnolia/source

test: $(HOST_TESTS)

# obstacles.h reaches into the engine for Theme. theme.c is pure arithmetic and
# its header is guarded, so it links here without dragging libogc in.
test-obstacles:
	@mkdir -p $(BUILD)
	@$(HOSTCC) $(HOSTCFLAGS) -o $(BUILD)/$@ \
	    tests/test_obstacles.c source/obstacles.c ../magnolia/source/theme.c -lm
	@$(BUILD)/$@

test-player:
	@mkdir -p $(BUILD)
	@$(HOSTCC) $(HOSTCFLAGS) -o $(BUILD)/$@ tests/test_player.c source/player.c -lm
	@$(BUILD)/$@

# characters.c carries the roster's numbers and its sprite pointers, and the
# sprites are bin2s symbols that only exist after a console build. The stub is
# generated from characters.c itself rather than hand-written, so adding a
# character cannot leave it stale -- the roster is the list.
test-characters:
	@mkdir -p $(BUILD)
	@grep -ohE '[a-z0-9_]+_png' source/characters.c | sort -u | \
	    sed 's/^/const unsigned char /; s/$$/[1] = {0};/' > $(BUILD)/assets.h
	@$(HOSTCC) $(HOSTCFLAGS) -I $(BUILD) -o $(BUILD)/$@ \
	    tests/test_characters.c source/characters.c -lm
	@$(BUILD)/$@

test-stars:
	@mkdir -p $(BUILD)
	@$(HOSTCC) $(HOSTCFLAGS) -o $(BUILD)/$@ tests/test_stars.c source/stars.c -lm
	@$(BUILD)/$@

# The projection maths, split out of playfield.c so it can run without a
# console. playfield.c itself still cannot -- it asks libogc what video mode is
# running -- but the arithmetic it asks about can.
test-playfield:
	@mkdir -p $(BUILD)
	@$(HOSTCC) $(HOSTCFLAGS) -o $(BUILD)/$@ \
	    tests/test_playfield.c source/projection.c -lm
	@$(BUILD)/$@

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).dol

run:
	wiiload $(TARGET).dol

deploy: $(BUILD)
	@mkdir -p $(CURDIR)/sdcard/apps/$(TARGET)
	@cp $(BUILD)/$(TARGET).dol $(CURDIR)/sdcard/apps/$(TARGET)/boot.dol
	@cp $(CURDIR)/meta.xml $(CURDIR)/sdcard/apps/$(TARGET)/meta.xml
	@rm -rf $(CURDIR)/sdcard/apps/$(TARGET)/sprites
	@mkdir -p $(CURDIR)/sdcard/apps/$(TARGET)/sprites
	@echo "Deployed to sdcard/apps/$(TARGET)/"
	@echo "  boot.dol, meta.xml"
# Assets are committed, but warn rather than fail if a checkout is partial --
# the game runs without them, just with placeholder art and silence.
ifeq ($(wildcard $(CURDIR)/sprites/*.png),)
	@echo "  WARNING: sprites/ is empty -- characters will draw as white boxes."
	@echo "  Copy the PNGs into sprites/ before deploying."
else
	@cp $(CURDIR)/sprites/*.png $(CURDIR)/sdcard/apps/$(TARGET)/sprites/
	@echo "  sprites/ ($(words $(wildcard $(CURDIR)/sprites/*.png)) PNGs)"
endif
	@rm -rf $(CURDIR)/sdcard/apps/$(TARGET)/audio
	@mkdir -p $(CURDIR)/sdcard/apps/$(TARGET)/audio
ifeq ($(wildcard $(CURDIR)/audio/*.pcm),)
	@echo "  WARNING: audio/ is empty -- the game will run silently."
else
	@cp $(CURDIR)/audio/*.pcm $(CURDIR)/sdcard/apps/$(TARGET)/audio/
	@echo "  audio/ ($(words $(wildcard $(CURDIR)/audio/*.pcm)) PCM files)"
endif
	@echo "Copy sdcard/ contents to SD card root"

# Push straight to the folder Dolphin reads as its SD card. On MC1 that path
# comes from Dolphin's own config (WiiSDCardPath in Dolphin.ini); /mnt/c is the
# Windows drive as seen from WSL. Override DOLPHIN_SD for a different setup.
DOLPHIN_SD ?= /mnt/c/Dolphin/sdcard

dolphin: deploy
	@if [ ! -d /mnt/c ]; then \
		echo "No /mnt/c -- not running under WSL, nothing to sync."; \
	else \
		rm -rf "$(DOLPHIN_SD)/apps/$(TARGET)"; \
		mkdir -p "$(DOLPHIN_SD)/apps/$(TARGET)"; \
		cp -r $(CURDIR)/sdcard/apps/$(TARGET)/. "$(DOLPHIN_SD)/apps/$(TARGET)/"; \
		echo "Synced to $(DOLPHIN_SD)/apps/$(TARGET)/"; \
		echo "  boot.dol   $$(stat -c%s "$(DOLPHIN_SD)/apps/$(TARGET)/boot.dol") bytes"; \
		echo "  sprites    $$(ls "$(DOLPHIN_SD)/apps/$(TARGET)/sprites"/*.png 2>/dev/null | wc -l)"; \
		echo "  audio      $$(ls "$(DOLPHIN_SD)/apps/$(TARGET)/audio"/*.pcm 2>/dev/null | wc -l)"; \
		printf '\nIn Dolphin, open:\n  %s\n' 'C:\Dolphin\sdcard\apps\$(TARGET)\boot.dol'; \
	fi

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
