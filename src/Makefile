
	# check CPU and supported optimization flags
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

	# set bundle name
	OLDNAME = MidiKeyBoard
	OLDEXEC_NAME  = $(shell echo $(OLDNAME) | tr A-Z a-z)
	NAME = Mamba
	EXEC_NAME  = $(shell echo $(NAME) | tr A-Z a-z)
	BUILD_DIR = build
	VER = 2.6

	LIBSCALA_DIR := ../libscala-file/
	LIB_DIR := ../libxputty/libxputty/
	HEADER_DIR := $(LIB_DIR)include/
	LOCALIZE_DIR := ../libxputty/xputty/dialogs/
	LXPUTTY := -L. $(LIB_DIR)libxputty.a
	PREFIX ?= /usr
	BIN_DIR ?= $(PREFIX)/bin
	SHARE_DIR ?= $(PREFIX)/share
	DESKAPPS_DIR ?= $(SHARE_DIR)/applications
	PIXMAPS_DIR ?= $(SHARE_DIR)/pixmaps
	MAN_DIR ?= $(SHARE_DIR)/man/man1
	LOCAL_DIR ?= $(SHARE_DIR)/locale

	# set compile flags
	
	CXXFLAGS += -std=gnu++17 -O3 -D_FORTIFY_SOURCE=2 -Wall -fstack-protector -funroll-loops \
	-ffast-math -fomit-frame-pointer -MMD $(SSE_CFLAGS)
	ifneq ($(CXX), clang)
	CXXFLAGS += -fstrength-reduce -fschedule-insns
	endif
	DEBUG_CXXFLAGS += -g -D DEBUG
	LDFLAGS += -Wl,-z,noexecstack -Wl,--no-undefined \
	`pkg-config --libs jack cairo x11 sigc++-2.0 liblo smf fluidsynth` -lm -pthread -lasound -lstdc++
	INCFLAGS = -I./ -I../libxputty/libxputty/include/ -I../libscala-file/ \
	`pkg-config --cflags jack cairo x11 sigc++-2.0 liblo smf fluidsynth`\
	-DVERSION=\"$(VER)\"
	# invoke build files
	OBJECTS = $(OLDNAME).cpp $(NAME).cpp XAlsa.cpp XJack.cpp NsmHandler.cpp XSynth.cpp MidiMapper.cpp
	SOBJECTS = $(LIBSCALA_DIR)scala_kbm.cpp $(LIBSCALA_DIR)scala_scl.cpp
	COBJECTS = xmkeyboard.c xcustommap.c
	OBJ_FILES = $(addprefix ./$(BUILD_DIR)/,$(notdir $(OBJECTS:.cpp=.o)))
	SOBJ_FILES = $(addprefix ./$(BUILD_DIR)/,$(notdir $(SOBJECTS:.cpp=.o)))
	COBJ_FILES = $(addprefix ./$(BUILD_DIR)/,$(notdir $(COBJECTS:.c=.o)))
	LOCALIZE = $(LOCALIZE_DIR)xfile-dialog.c $(LOCALIZE_DIR)xmessage-dialog.c $(LOCALIZE_DIR)xsavefile-dialoge.c
	## output style (bash colours)
	BLUE = "\033[1;34m"
	RED =  "\033[1;31m"
	NONE = "\033[0m"

.PHONY : all debug nls gettext updatepot po clean install uninstall 

all : check $(NAME)
	@if [ -f ./$(BUILD_DIR)/$(EXEC_NAME) ]; then \
		$(B_ECHO) "=================== DONE =======================$(reset)"; \
	else \
		$(R_ECHO) "sorry, build failed$(reset)"; \
	fi

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
	@mkdir -p ./$(BUILD_DIR)
ifdef ARMCPU
	@(R_ECHO) "ARM CPU DEDECTED, please check the optimization flags $(reset)"
endif

clean :
	@rm -f ./$(BUILD_DIR)/$(EXEC_NAME)
	@rm -rf ./$(BUILD_DIR)
	@$(ECHO) ". . , clean up$(reset)"

install :
ifneq ("$(wildcard ./$(BUILD_DIR))","")
	@$(B_ECHO) "Install Mamba$(reset)"
	$(QUIET)mkdir -p $(DESTDIR)$(BIN_DIR)
	$(QUIET)cp ./$(BUILD_DIR)/$(EXEC_NAME) $(DESTDIR)$(BIN_DIR)/$(EXEC_NAME)
	$(QUIET)mkdir -p $(DESTDIR)$(DESKAPPS_DIR)
	$(QUIET)cp $(NAME).desktop $(DESTDIR)$(DESKAPPS_DIR)
	$(QUIET)mkdir -p $(DESTDIR)$(PIXMAPS_DIR)
	$(QUIET)cp $(NAME).svg $(DESTDIR)$(PIXMAPS_DIR)
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
	$(QUIET)update-desktop-database || true
	@$(B_ECHO) "=================== DONE =======================$(reset)"
else
	@$(B_ECHO) ". . , you must build first$(reset)"
endif

uninstall :
	@rm -rf $(DESTDIR)$(BIN_DIR)/$(EXEC_NAME)
	@rm -rf $(DESTDIR)$(DESKAPPS_DIR)/$(NAME).desktop
	@rm -rf $(DESTDIR)$(PIXMAPS_DIR)/$(NAME).svg
	@rm -rf $(DESTDIR)$(PIXMAPS_DIR)/$(NAME).png
	@rm -rf $(addprefix $(SHARE_DIR)/, $(MSGOBJS))
	@$(B_ECHO) "=================== DONE =======================$(reset)"

./$(BUILD_DIR)/%.o: %.cpp
	@$(ECHO) "Building object file $@ $(reset)"
	$(QUIET)$(CXX) $(CXXFLAGS)  $(INCFLAGS) -c $< -o $@

./$(BUILD_DIR)/%.o: $(LIBSCALA_DIR)%.cpp
	@$(ECHO) "Building object file $@ $(reset)"
	$(QUIET)$(CXX) $(CXXFLAGS)  $(INCFLAGS) -c $< -o $@

./$(BUILD_DIR)/%.o: %.c
	@$(ECHO) "Building object file $@ $(reset)"
	$(QUIET)$(CXX) $(CXXFLAGS)  $(INCFLAGS) -c $< -o $@

-include $(OBJ_FILES:%.o=%.d)
-include $(SOBJ_FILES:%.o=%.d)
-include $(COBJ_FILES:%.o=%.d)

$(NAME) : $(OBJ_FILES) $(SOBJ_FILES) $(COBJ_FILES)
	@$(B_ECHO) "Compiling $(NAME) $(reset)"
	$(QUIET)$(CXX) $(CXXFLAGS) -o ./$(BUILD_DIR)/$(EXEC_NAME) $(OBJ_FILES) $(SOBJ_FILES) $(COBJ_FILES) $(INCFLAGS) $(LDFLAGS) $(LXPUTTY)

doc:
	#pass
