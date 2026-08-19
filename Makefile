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

.PHONY: $(BUILD) clean run deploy

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
	@mkdir -p $(CURDIR)/sdcard/apps/$(TARGET)/sprites
	@echo "Deployed to sdcard/apps/$(TARGET)/"
	@echo "  boot.dol, meta.xml"
# sprites/ is deliberately not in git, so a fresh clone has none. Warn rather
# than fail the deploy -- the game runs without them, just with placeholder art.
ifeq ($(wildcard $(CURDIR)/sprites/*.png),)
	@echo "  WARNING: sprites/ is empty -- characters will draw as white boxes."
	@echo "  Copy the PNGs into sprites/ before deploying."
else
	@cp $(CURDIR)/sprites/*.png $(CURDIR)/sdcard/apps/$(TARGET)/sprites/
	@echo "  sprites/ ($(words $(wildcard $(CURDIR)/sprites/*.png)) PNGs)"
endif
	@echo "Copy sdcard/ contents to SD card root"

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
