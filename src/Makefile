
	# check CPU and supported optimization flags
	ifeq ($(UNAME), Linux)
		ifneq ($(shell cat /proc/cpuinfo | grep sse3 ) , )
			SSE_CFLAGS = -msse3 -mfpmath=sse
		else ifneq ($(shell cat /proc/cpuinfo | grep sse2 ) , )
			SSE_CFLAGS = -msse2 -mfpmath=sse
		else ifneq ($(shell cat /proc/cpuinfo | grep sse ) , )
			SSE_CFLAGS = -msse -mfpmath=sse
			else ifneq ($(shell cat /proc/cpuinfo | grep ARM ) , )
			ifneq ($(shell cat /proc/cpuinfo | grep ARMv7 ) , )
				ifneq ($(shell cat /proc/cpuinfo | grep vfpd32 ) , )
					SSE_CFLAGS = -march=armv7-a -mfpu=vfpv3 
				else ifneq ($(shell cat /proc/cpuinfo | grep vfpv3 ) , )
					SSE_CFLAGS = -march=armv7-a -mfpu=vfpv3
				endif
			else
				ARMCPU = "YES"
			endif
		else
			SSE_CFLAGS =
		endif
	endif

	# set bundle name
	OLDNAME = MidiKeyBoard
	OLDEXEC_NAME  = $(shell echo $(OLDNAME) | tr A-Z a-z)
	NAME = Mamba
	EXEC_NAME  = $(shell echo $(NAME) | tr A-Z a-z)
	BUILD_DIR = build
	VER = 0.6

	LIB_DIR := ../libxputty/libxputty/
	HEADER_DIR := $(LIB_DIR)include/
	PREFIX ?= /usr
	BIN_DIR ?= $(PREFIX)/bin
	SHARE_DIR ?= $(PREFIX)/share
	DESKAPPS_DIR ?= $(SHARE_DIR)/applications
	PIXMAPS_DIR ?= $(SHARE_DIR)/pixmaps

	# set compile flags
	CXXFLAGS += -O2 -D_FORTIFY_SOURCE=2 -Wall $(SSE_CFLAGS) 
	LDFLAGS += -I./ -I../libxputty/libxputty/include/ `pkg-config --cflags --libs jack cairo x11 sigc++-2.0 liblo` -lm -pthread
	# invoke build files
	OBJECTS = $(OLDNAME).cpp NsmHandler.cpp xkeyboard.c
	## output style (bash colours)
	BLUE = "\033[1;34m"
	RED =  "\033[1;31m"
	NONE = "\033[0m"

.PHONY : $(HEADER_DIR)*.h all clean install uninstall 

all : check $(NAME)
	@mkdir -p ./$(BUILD_DIR)
	@mv ./$(EXEC_NAME) ./$(BUILD_DIR)
	@if [ -f ./$(BUILD_DIR)/$(EXEC_NAME) ]; then echo $(BLUE)"build finish, now run make install"; \
	else echo $(RED)"sorry, build failed"; fi
	@echo $(NONE)

check :
ifdef ARMCPU
	@echo $(RED)ARM CPU DEDECTED, please check the optimization flags
	@echo $(NONE)
endif

clean :
	@rm -f ./$(BUILD_DIR)/$(EXEC_NAME)
	@rm -rf ./$(BUILD_DIR)
	@echo ". ." $(BLUE)", clean up"$(NONE)

install :
ifneq ("$(wildcard ./$(BUILD_DIR))","")
	cp ./$(BUILD_DIR)/$(EXEC_NAME) $(DESTDIR)$(BIN_DIR)/$(EXEC_NAME)
	cp $(NAME).desktop $(DESTDIR)$(DESKAPPS_DIR)
	cp $(NAME).png $(DESTDIR)$(PIXMAPS_DIR)
	@echo ". ." $(BLUE)", done"$(NONE)
else
	@echo ". ." $(BLUE)", you must build first"$(NONE)
endif

uninstall :
	@rm -rf $(DESTDIR)$(BIN_DIR)/$(OLDEXEC_NAME)
	@rm -rf $(DESTDIR)$(DESKAPPS_DIR)/$(OLDNAME).desktop
	@rm -rf $(DESTDIR)$(PIXMAPS_DIR)/$(OLDNAME).png

	@rm -rf $(DESTDIR)$(BIN_DIR)/$(EXEC_NAME)
	@rm -rf $(DESTDIR)$(DESKAPPS_DIR)/$(NAME).desktop
	@rm -rf $(DESTDIR)$(PIXMAPS_DIR)/$(NAME).png

	@echo ". ." $(BLUE)", done"$(NONE)

$(NAME) :
	$(CXX) $(CXXFLAGS) $(OBJECTS) -L. ../libxputty/libxputty/libxputty.a -o $(EXEC_NAME) $(LDFLAGS)

doc:
	#pass
