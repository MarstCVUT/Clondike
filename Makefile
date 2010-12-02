###################################################################
#                           Makefile                              #
#                                                                 #
# Description: Main makefile for clondike project                 #
#                                                                 #
# Author: Jan Capek                                               #
# Created: 11/15/2004                                             #
#                                                                 #
# Last modified:                                                  #
#                                                                 #
###################################################################
TCMI_VER = 1-06
KKC_VER = 1-02
CLONDIKE_VER = 0-08

SUBDIRS     := src test doc

SRCSTATDIR  := src-stats

SLOCTOOL = sloccount
SLOCFILE = sloc

# build everything except for docs
.PHONY: all
all:
	for dir in $(filter-out doc,$(SUBDIRS)); do $(MAKE) -C $$dir; done

# explicit documentation target
.PHONY: doc
doc:
	$(MAKE) -C doc

install:
	scripts/install.sh

.PHONY: clean
clean:
	rm -f *~ 
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done

.PHONY: stat

cscope.out:
	cscope -Fcscope.out -R -k -b -s$(PWD)/src

# calculates code statistics and creates a .dat file for each
# directory. Used for plotting the data.
stat: clean
	@for dir in $(filter-out doc,$(SUBDIRS)); do\
           $(SLOCTOOL) --details $$dir  > $(SLOCFILE)-$$dir-`date -I`.txt; \
           $(SLOCTOOL)           $$dir >> $(SLOCFILE)-$$dir-`date -I`.txt; \
	   stats_file=doc/$${dir}_stats.dat; \
	   rm -f $$stats_file; \
	   echo printing into $$stats_file; \
	   for f in $(SRCSTATDIR)/*$$dir*; do \
		oldifs=$$IFS; IFS='-'; set -- $${f%.txt}; \
		echo -n $$4-$$5-$$6; IFS=$$oldifs; \
		cat $$f | perl -n -e 'print " $$1\n" if($$_=~/.*ansic:\s+(\d+).*/);'; \
	   done >> $$stats_file; \
	done


