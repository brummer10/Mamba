
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
	VER = 2.1

	LIB_DIR := ../libxputty/libxputty/
	HEADER_DIR := $(LIB_DIR)include/
	LOCALIZE_DIR := ../libxputty/xputty/dialogs/
	PREFIX ?= /usr
	BIN_DIR ?= $(PREFIX)/bin
	SHARE_DIR ?= $(PREFIX)/share
	DESKAPPS_DIR ?= $(SHARE_DIR)/applications
	PIXMAPS_DIR ?= $(SHARE_DIR)/pixmaps
	MAN_DIR ?= $(SHARE_DIR)/man/man1
	LOCAL_DIR ?= $(SHARE_DIR)/locale

	# set compile flags
	CXXFLAGS += -O2 -D_FORTIFY_SOURCE=2 -Wall -fstack-protector -funroll-loops -ffast-math -fomit-frame-pointer \
	-fstrength-reduce -fschedule-insns $(SSE_CFLAGS)
	DEBUG_CXXFLAGS += -g -D DEBUG
	LDFLAGS += -Wl,-z,noexecstack -Wl,--no-undefined -I./ -I../libxputty/libxputty/include/ \
	`pkg-config --cflags --libs jack cairo x11 sigc++-2.0 liblo smf fluidsynth` -lm -pthread -lasound \
	-DVERSION=\"$(VER)\"
	# invoke build files
	OBJECTS = $(OLDNAME).cpp $(NAME).cpp XAlsa.cpp XJack.cpp NsmHandler.cpp xkeyboard.c xcustommap.c XSynth.cpp
	LOCALIZE = $(LOCALIZE_DIR)xfile-dialog.c $(LOCALIZE_DIR)xmessage-dialog.c $(LOCALIZE_DIR)xsavefile-dialoge.c
	## output style (bash colours)
	BLUE = "\033[1;34m"
	RED =  "\033[1;31m"
	NONE = "\033[0m"

.PHONY : $(HEADER_DIR)*.h all debug nls gettext updatepot po clean install uninstall 

all : check $(NAME)
	@mkdir -p ./$(BUILD_DIR)
	@mv ./$(EXEC_NAME) ./$(BUILD_DIR)
	@if [ -f ./$(BUILD_DIR)/$(EXEC_NAME) ]; then echo $(BLUE)"build finish, now run make install"; \
	else echo $(RED)"sorry, build failed"; fi
	@echo $(NONE)

debug: CXXFLAGS = $(DEBUG_CXXFLAGS) 
debug: all

nls: LDFLAGS += -DENABLE_NLS -DGETTEXT_PACKAGE=\"$(EXEC_NAME)\" -DLOCAL_DIR=\"$(LOCAL_DIR)\"
nls: gettext all 

    #@localisation
MSGLANGS=$(notdir $(wildcard po/*po))
MSGOBJS=$(addprefix locale/,$(MSGLANGS:.po=/LC_MESSAGES/$(EXEC_NAME).mo))
LANGS=$(addprefix locale/,$(MSGLANGS:.po=/LC_MESSAGES/))
LANGW=$(words  $(MSGLANGS))

gettext: $(MSGOBJS)

locale/%/LC_MESSAGES/$(EXEC_NAME).mo: po/%.po
	mkdir -p $(dir $@)
	msgfmt -c -o $@ po/$*.po

updatepot:
	@mkdir -p ./po/
	xgettext --keyword=_ --language=C++ --add-comments --sort-output --package-name=$(EXEC_NAME) --package-version=$(VER) -o po/$(EXEC_NAME).pot $(OBJECTS) $(LOCALIZE)
	for POFILE in $(MSGLANGS) ; do msgmerge --update po/$$POFILE po/$(EXEC_NAME).pot ; done

po:
	msginit --input=po/$(EXEC_NAME).pot --locale=$(LANG) --output=po/$(basename $(LANG)).po
	echo $(LANG)

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
	mkdir -p $(DESTDIR)$(BIN_DIR)
	cp ./$(BUILD_DIR)/$(EXEC_NAME) $(DESTDIR)$(BIN_DIR)/$(EXEC_NAME)
	mkdir -p $(DESTDIR)$(DESKAPPS_DIR)
	cp $(NAME).desktop $(DESTDIR)$(DESKAPPS_DIR)
	mkdir -p $(DESTDIR)$(PIXMAPS_DIR)
	cp $(NAME).svg $(DESTDIR)$(PIXMAPS_DIR)
	@if [ ${LANGW} -gt 1 ]; then \
		for lang in $(LANGS) ; \
		do \
			if [ -f $$lang/$(EXEC_NAME).mo ]; then \
				mkdir -p $(DESTDIR)$(SHARE_DIR)/$$lang; \
				cp $$lang/$(EXEC_NAME).mo $(DESTDIR)$(SHARE_DIR)/$$lang; \
			fi \
		done \
	elif [ ${LANGW} -gt 0 ]; then \
			if [ -f $(LANGS)/$(EXEC_NAME).mo ]; then \
				mkdir -p $(DESTDIR)$(SHARE_DIR)/$(LANGS); \
				cp $(LANGS)/$(EXEC_NAME).mo $(DESTDIR)$(SHARE_DIR)/$(LANGS); \
			fi \
	fi
	update-desktop-database || true
	@echo ". ." $(BLUE)", done"$(NONE)
else
	@echo ". ." $(BLUE)", you must build first"$(NONE)
endif

uninstall :
	@rm -rf $(DESTDIR)$(BIN_DIR)/$(EXEC_NAME)
	@rm -rf $(DESTDIR)$(DESKAPPS_DIR)/$(NAME).desktop
	@rm -rf $(DESTDIR)$(PIXMAPS_DIR)/$(NAME).svg
	@rm -rf $(DESTDIR)$(PIXMAPS_DIR)/$(NAME).png
	@rm -rf $(addprefix $(SHARE_DIR)/, $(MSGOBJS))
	@echo ". ." $(BLUE)", done"$(NONE)

$(NAME) :
	$(CXX) $(CXXFLAGS) $(OBJECTS) -L. ../libxputty/libxputty/libxputty.a -o $(EXEC_NAME) $(LDFLAGS)

doc:
	#pass
