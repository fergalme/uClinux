# makefile for smblib
# Type make system, where system is ULTRIX, DU, DECOSF1, Solaris etc
AR = ar

INCLUDES = smblib.h smblib-priv.h

RFCNB = session.o rfcnb-util.o rfcnb-io.o

OBJS = smblib.o smblib-util.o file.o smb-errors.o exper.o smblib-api.o smbencrypt.o smbdes.o md4.o

VALIDATE = valid.o session.o rfcnb-util.o \
           rfcnb-io.o smblib-util.o smblib.o smbencrypt.o smbdes.o md4.o

.SUFFIXES: .c .o .h

dummy: all

smbvalid.a:	$(VALIDATE) 
	$(AR) rcs smbvalid.a $(VALIDATE)

all: smbvalid.a

.c.o:	$(INCLUDES)
	@echo Compiling $*.c
	$(CC) $(CFLAGS) -c $*.c

clean:
	rm -f *.o smbvalid.a

