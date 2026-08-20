#---------------------------------------------------------------------------------
.SUFFIXES:
.SECONDARY:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules

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

CFILES      := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(TOPDIR)/$(dir)/*.c)))
CPPFILES    := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(TOPDIR)/$(dir)/*.cpp)))

ifeq ($(strip $(CPPFILES)),)
    export LD  := $(CC)
else
    export LD  := $(CXX)
endif

export OFILES   := $(addsuffix .o,$(basename $(CPPFILES) $(CFILES)))
export INCLUDE  := $(foreach dir,$(INCLUDES),-I$(TOPDIR)/$(dir)) \
                   $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                   -I$(CURDIR)/$(BUILD) -I$(LIBOGC_INC)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib) -L$(LIBOGC_LIB)

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).dol: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

-include $(DEPENDS)

#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------

.PHONY: $(BUILD) clean run deploy dolphin

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
