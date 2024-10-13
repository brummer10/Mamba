
include libxputty/Build/Makefile.base

NOGOAL := install all features

PASS := features 

SUBDIR := src

.PHONY: $(SUBDIR) libxputty  recurse

$(MAKECMDGOALS) recurse: $(SUBDIR)

check-and-reinit-submodules :
ifeq (,$(filter $(NOGOAL),$(MAKECMDGOALS)))
ifeq (,$(findstring clean,$(MAKECMDGOALS)))
	@if git submodule status 2>/dev/null | egrep -q '^[-]|^[+]' ; then \
		echo "$(red)INFO: Need to reinitialize git submodules$(reset)"; \
		git submodule update --init; \
		echo "$(blue)Done$(reset)"; \
	else echo "$(blue) Submodule up to date$(reset)"; \
	fi
endif
endif

libxputty: check-and-reinit-submodules
ifeq (,$(filter $(NOGOAL),$(MAKECMDGOALS)))
ifeq (,$(wildcard ./libxputty/xputty/resources/play.png))
	@cp ./resources/*.png ./libxputty/xputty/resources/
endif
	@exec $(MAKE) --no-print-directory -j 1 -C $@ $(MAKECMDGOALS)
endif

$(SUBDIR): libxputty
ifeq (,$(filter $(PASS),$(MAKECMDGOALS)))
	@exec $(MAKE) --no-print-directory -j 1 -C $@ $(MAKECMDGOALS)
endif

clean:
	@rm -f ./libxputty/xputty/resources/play.png
	@rm -f ./libxputty/xputty/resources/pause.png
	@rm -f ./libxputty/xputty/resources/record.png
	@rm -f ./libxputty/xputty/resources/eject.png
	@rm -f ./libxputty/xputty/resources/minus.png
	@rm -f ./libxputty/xputty/resources/plus.png

features:
