#   The following are locations and directory names.  Change these to fit
#   your needs.  The output .elf file winds up in the $BINDIR directory.
#   Use your own programmer.  I used the OpenOCD software with an STLINK V2
#   USB programmer.
#

GCC_PREFIX=arm-none-eabi
LIB_INC="-I../libopencm3/include"

# Standard locations of source, object, include, binary and dependent files.

SRCDIR:=./src
OBJDIR:=./obj
INCDIR:=./inc
BINDIR:=./bin

#   Files.

SRCS:= main.c uart.c ir.c ps2.c
OBJS:= $(addprefix $(OBJDIR)/,$(SRCS:.c=.o)) 
SRCS:= $(addprefix $(SRCDIR)/,$(SRCS))

#   Flags and definitions.

TARGET=irkey.elf
MAP=irkey.map
CC=$(GCC_PREFIX)-gcc
DEVICE=STM32F1
GCC_INC=$(LIB_INC) -I$(INCDIR)
GCC_LINK_INC=-L../libopencm3/lib

.PHONY: $(DEPDIR)/%.d

GCC_OPT=-Os -std=c99 -g -mthumb -mcpu=cortex-m3 -msoft-float \
-mfix-cortex-m3-ldrd -Wextra -Wshadow -Wimplicit-function-declaration \
-Wredundant-decls -Wmissing-prototypes -Wstrict-prototypes -fno-common \
-ffunction-sections -fdata-sections  -MD -Wall -Wundef -D$(DEVICE) 

GCC_LINK_OPT1=--static -nostartfiles -T./stm32-h103.ld -mthumb -mcpu=cortex-m3 \
-msoft-float -mfix-cortex-m3-ldrd -Wl,-Map=$(MAP) -Wl,--gc-sections 
 
GCC_LINK_OPT2=-lopencm3_stm32f1 -Wl,--start-group -lc -lgcc -lnosys \
-Wl,--end-group 

#  Create result directories, if they don't exist already

$(shell mkdir -p $(OBJDIR) >/dev/null)
$(shell mkdir -p $(BINDIR) >/dev/null)

DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.d

all: $(BINDIR)/$(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.c 
	$(CC) $(GCC_OPT) $(GCC_INC)  -c  -o $@ $<

$(BINDIR)/$(TARGET): $(OBJS)
	$(CC) $(GCC_LINK_OPT1) $(OBJS) $(GCC_LINK_INC) $(GCC_LINK_OPT2)  -o $@

.PHONY: clean	

clean:
	rm -f $(BINDIR)/* $(OBJDIR)/* $(MAP)
	
