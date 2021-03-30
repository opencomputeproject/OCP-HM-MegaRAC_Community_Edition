#------- Comment/uncomment the following line to enable/disable debugging---------
DEBUG = n

#---------------------- Change according to your files ----------------------------
SRC 		= port80.c			# Source File
TARGET = bioscode 			# Output file

CFLAGS += -I${SPXINC}/snoop
CFLAGS += -I${SPXINC}/global

# Add the Corresponding library
LIBS+= -L${SPXLIB}/snoop -lsnoop
LIBS+= -L${SPXLIB}/dbgout -ldbgout

include ${TOOLDIR}/rules/Rules.make.apps

#----------------------------------------------------------------------------------------
