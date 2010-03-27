###################################################################
#                           debug.make                            #
#                                                                 #
# Description: Sets up the EXTRA_CFLAGS for debugging, these      #
# 	       variables are used by the Kbuild system as         #
#              additional CFLAGS                                  #
#                                                                 #
# Author: Jan Capek                                               #
# Created: 03/28/2005                                             #
#                                                                 #
# Last modified:                                                  #
#                                                                 #
###################################################################

# Special flags that handle debuging
ifeq ($(DEBUG),y)
 EXTRA_CFLAGS += -D__MDBG_ON__
endif
