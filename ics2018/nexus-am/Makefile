include Makefile.check

AM_HOME ?= $(shell pwd)
# AM_HOME ?= $(/home/qin2mu4/Desktop/ics2018/nexus-am)
 
all:
	@$(MAKE) -C am

# clean everything
ALLMAKE = $(dir $(shell find . -mindepth 2 -name "Makefile"))
$(ALLMAKE):
	-@$(MAKE) -s -C $@ clean

clean: $(ALLMAKE)

.PHONY: all clean $(ALLMAKE)
