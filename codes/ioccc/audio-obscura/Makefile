#!/usr/bin/env make
#
# prog - an example Makefile <<=== change "an example Makefile" to a suitable title for entry
#
# Please copy this file into your entry directory under the name: Makefile
# Modify the resulting Makefile as needed.


#############################
# shell used by this Makefile
#############################
#
SHELL= bash


#######################
# common tool locations
#######################
#
CC= cc
GINDENT= gindent
MV= mv
RM= rm
TRUE= true

# Common C compiler warnings to silence
#
# Example: CSILENCE= -Wno-some-thing -Wno-another-thing
#
# NOTE: If you add -Wno-stuff to CSILENCE, please update
#	CUNKNOWN in the next comment block.
#
# NOTE: Please don't add -Wno-unknown-warning-option to CSILENCE.
#
#
CSILENCE=-Wno-unsafe-buffer-usage # nah i'm not really into rust

# Attempt to silence unknown warnings
#
# If you add -Wno-stuff to CSILENCE, then please change CUNKNOWN to read:
#
#	CUNKNOWN= -Wno-unknown-warning-option
#
CUNKNOWN=

# Common C compiler warning flags
#
CWARN= -Wall -Wextra -pedantic ${CSILENCE} ${CUNKNOWN}

# The standard to compile against
#
# Your IOCCC submission is free to require older C standards, or
# even not specify a C standard at all.  We suggest trying
# for -std=gnu17, but that is not a requirement and you won't
# be penalized if you name CSTD empty or use another
# well known and reasonably widely implemented C standard.
#
CSTD= -std=gnu17

# Compiler bit architecture
#
# Example for 32-bitness: ARCH= -m32
# Example for 64-bitness: ARCH= -m64
#
# NOTE: Normally one should not specify a specific architecture.
#
ARCH=

# Defines that are needed to compile
#
# Example: -Dfoo -Dbar=baz
#
CDEFINE=

# Include files that are needed to compile
#
# Example: CINCLUDE= -include stdio.h
#
CINCLUDE=

# Other flags to pass to the C compiler
#
# Example: COTHER= -fno-math-errno
#
COTHER=

# Optimization
#
OPT= -O3

# Default flags for ANSI C compilation
#
CFLAGS= ${CSTD} ${CWARN} ${ARCH} ${CDEFINE} ${CINCLUDE} ${COTHER} ${OPT}

# Libraries needed to build
#
LDFLAGS=-lm

# C compiler to use
#
CC= cc

# Compiler add-ons or replacements for clang only
#
ifeq "$(findstring $(CLANG),${CC})" "$(CLANG)"
#
CSILENCE+=
#
CWARN+= -Weverything
#
endif

# Specific add-ons or replacements for gcc only
#
ifeq "$(findstring $(GCC),${CC})" "$(GCC)"
#
CSILENCE+=
#
CWARN+=
#
endif


###########################################
# Special Makefile variables for this entry
###########################################

# what to build
#
PROG= prog
OBJ= ${PROG}.o
TARGET= ${PROG}
# If you have alternate code, try something like:
# ALT_OBJ= ${PROG}.alt.o
# ALT_TARGET= ${PROG}.alt
#
# or so.
#
# Please remove these lines either way.
#
ALT_OBJ=
ALT_TARGET=

# list any data files supplied with the submission
#
DATA=


#################
# build the entry
#################
#
all: data ${TARGET}
	@${TRUE}

.PHONY: all data try clean clobber install

# how to compile
#
${PROG}: ${PROG}.c
	${CC} ${CFLAGS} $< -o $@ ${LDFLAGS}

# alternative executable
#
alt: data ${ALT_TARGET}
	@${TRUE}

${PROG}.alt: ${PROG}.alt.c
	${CC} ${CFLAGS} $< -o $@ ${LDFLAGS}



# data files
#
data: ${DATA}
	@${TRUE}

# both all and alt
#
everything: all alt
	@${TRUE}


# suggested way(s) to run the program
#
try: ${PROG} ${DATA}
	./${PROG}


###############
# utility rules
###############
#
clean:
	${RM} -f ${OBJ}
	@-if [[ -f indent.c ]]; then \
	    echo ${RM} -f indent.c; \
	    ${RM} -f indent.c; \
	fi

clobber: clean
	${RM} -f ${TARGET} *.dSYM
	@-if [[ -e prog.wav ]]; then \
	    ${RM} -f prog.wav; \
	    echo 'ate prog.wav'; \
	fi


######################################
# optional include of 1337 hacker rulz
######################################

-include 1337.mk ../1337.mk ../../1337.mk
