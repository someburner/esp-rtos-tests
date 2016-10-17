VERBOSE=1

TARGET = rconf
# .RECIPEPREFIX +=

#############
#
# Build tools
#
#############

CC = gcc $(COMPILEROPTIONS)
LD = ld
OBJCOPY = objcopy
OBJDUMP = objdump

###############
#
# Files and libs
#
###############

CFILES = rconf.c

COMPILEROPTIONS = -I.

CFLAGS += -Wextra -Wundef -Werror

V ?= $(VERBOSE)
ifeq ("$(V)","1")
Q :=
vecho := @true
else
Q := @
vecho := @echo
endif

############
#
# Tasks
#
############

OBJFILES = $(CFILES:%.c=%.o)

DEPFILES = $(CFILES:%.c=%.d)

ALLOBJFILES += $(OBJFILES)

DEPENDENCIES = $(DEPFILES)

# link object files, create binary
$(TARGET): $(ALLOBJFILES)
	@echo "... linking"
	@${CC} $(LINKEROPTIONS) -o $(TARGET) $(ALLOBJFILES)
	@${sh} ./$(TARGET)
	@rm -f main.o
	@rm -f main.d

-include $(DEPENDENCIES)

# compile c filesf
$(OBJFILES): %.o:%.c
		$(vecho) "... compile rconf $@"
		@${CC} ${CFLAGS} ${FLAGS} -g -c -o $@ $<

# make dependencies
$(DEPFILES): %.d:%.c
		@echo "... depend $@"; \
		rm -f $@; \
		${CC} $(COMPILEROPTIONS) -M $< > $@.$$$$; \
		sed 's,\($*\)\.o[ :]*, /\1.o $@ : ,g' < $@.$$$$ > $@; \
		rm -f $@.$$$$

clean:
	@echo ... removing files
	@rm -f rconf.o
	@rm -f rconf.d
	@rm -f rconf

all: $(TARGET) $(clean)
