SRCS = getd.c
TARGET	= getd

ENV = -g
ARCHIVE = libget$(ENV).a
LIBS = $(ARCHIVE)
INCLUDES = -I. -I..

CC = gcc

OBJS = $(addsuffix .o,$(basename $(SRCS)))


CFLAGS	= $(INCLUDES)
ARFLAGS = rU

RM = /bin/rm -f

.SILENT:

DEPENDS = $(addprefix .d_, $(basename $(SRCS)))

LP = (
RP = )
ARCHIVE_OBJS = $(addsuffix $(RP),$(addprefix $(ARCHIVE)$(LP),$(notdir $(OBJS))))

.SUFFIXES: .c

$(TARGET): $(LIBS)
	echo "Creating $@"
	$(CC) $(CFLAGS) -o $@ $(LIBS) -lnanomsg

$(ARCHIVE): $(ARCHIVE_OBJS)
	echo "Generating" $(ARCHIVE)
	ranlib $(ARCHIVE)

.PHONY: clean

clean:
	$(RM) $(ARCHIVE)

.c.o:
	echo "Compiling" $<
	$(CC) -c $(CFLAGS) $<

#
# default rule to put all .o files in the archive and remove them
#

(%.o) : %.o
	$(AR) $(ARFLAGS) $@ $<
	$(RM) $<

#
# The following two rules make the dependence file for the C source
# files. The C files depend upon the corresponding dependence file. The
# dependence file depends upon the source file's actual dependences. This way
# both the dependence file and the source file are updated on any change.
# The depend.sed sed command file sets up the dependence file appropriately.
#

.d_%.l: %.l
	echo "$(basename $<).c: $<" > $@

.d_%.y: %.y
	echo "$(basename $<).c: $<" > $@

.d_%: %.c
	echo  "Updating dependences for" $< "..."
	$(CPP) -MM -MT '$(ARCHIVE)($(basename $<).o)' $(INCLUDES) -MF $@ $<


#
# This includes all of the dependence files. If the file does not exist,
# GNU Make will use one of the above rules to create it.
#

include $(DEPENDS)

# DO NOT DELETE THIS LINE -- make depend depends on it.
