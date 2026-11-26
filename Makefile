# Requires devkitARM + libctru (devkitPro), plus makerom and bannertool on PATH
ifeq ($(strip $(DEVKITARM)),)
$(error Set DEVKITARM, e.g. export DEVKITARM=/opt/devkitpro/devkitARM)
endif
include $(DEVKITARM)/3ds_rules

TARGET   := doors3ds
NAME     := Hallway 100
DESC     := First-person survival horror
AUTHOR   := You
CTRULIB  := $(DEVKITPRO)/libctru
ARCH     := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS   := -g -Wall -O3 -ffast-math -mword-relocations -ffunction-sections $(ARCH) \
            -I$(CTRULIB)/include -Isource -D__3DS__ -std=gnu11
LDFLAGS  := -specs=3dsx.specs -g $(ARCH) -L$(CTRULIB)/lib
LIBS     := -lctru -lm
CC       := arm-none-eabi-gcc
SRC      := source/main.c source/game.c source/game.h

all: $(TARGET).3dsx $(TARGET).cia

$(TARGET).elf: $(SRC)
	$(CC) $(CFLAGS) source/main.c source/game.c $(LDFLAGS) $(LIBS) -o $@

$(TARGET).smdh: assets/icon.png
	smdhtool --create "$(NAME)" "$(DESC)" "$(AUTHOR)" $< $@

$(TARGET).3dsx: $(TARGET).elf $(TARGET).smdh
	3dsxtool $< $@ --smdh=$(TARGET).smdh

banner.bnr: assets/banner.png assets/audio.wav
	bannertool makebanner -i assets/banner.png -a assets/audio.wav -o $@

icon.icn: assets/icon.png
	bannertool makesmdh -s "$(NAME)" -l "$(DESC)" -p "$(AUTHOR)" -i assets/icon.png -o $@

$(TARGET).cia: $(TARGET).elf banner.bnr icon.icn app.rsf
	makerom -f cia -o $@ -elf $(TARGET).elf -rsf app.rsf -icon icon.icn -banner banner.bnr -exefslogo -target t

clean:
	rm -f *.elf *.3dsx *.smdh *.cia *.bnr *.icn
