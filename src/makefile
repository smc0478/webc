ROOTDIR = ..
CC = gcc
SRCDIR = $(ROOTDIR)/src

OBJS = request.o response.o http.o net.o dict.o webc.o
OBJDIR = $(ROOTDIR)/obj
OBJFILES = $(patsubst %.o,$(OBJDIR)/%.o,$(OBJS))
INCLUDEDIR = $(ROOTDIR)/include
INCLUDES = $(OBJS:.o=.h)
LIBDIR = $(ROOTDIR)/lib
LIBNAME = libwebc

all: config include $(LIBDIR)/$(LIBNAME).so $(LIBDIR)/$(LIBNAME).a

config:
	mkdir -p $(OBJDIR)
	mkdir -p $(LIBDIR)
	mkdir -p $(INCLUDEDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -fpic -o $@ -c $<

$(LIBDIR)/$(LIBNAME).so: $(OBJFILES)
	$(CC) -shared -o $@ $^

$(LIBDIR)/$(LIBNAME).a: $(OBJFILES)
	ar -rcs $@ $(OBJFILES)

include: $(INCLUDES)
	cp $^ $(INCLUDEDIR)

clean:
	rm -rf $(OBJDIR) $(LIBDIR)
