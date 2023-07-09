#Makefile for Thread Program
CC = gcc
CFLAGS = -g

LIBOBJS = thread.o 
# Source files for easy execution

LIBSRCS = thread.c

all: Test09 Test05
	echo Type "./Test09" to run test case.

#To make different test cases switch ./test03 to the name of the case you want to execute
Tests: Test09 Test05
	./Test09.c
	./Test05.c

thread.a: ${LIBOBJS} Makefile
	ar rcs thread.a ${LIBOBJS}

# Here we make the executable for the program

thread.o: thread.c thread.h Makefile
	${CC} ${CFLAGS} -c thread.c

# Test cases (how each test case is compiled)
Test09.o: Test09.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c Test09.c

Test09: Test09.o thread.a Makefile
	${CC} ${CFLAGS} Test09.o thread.a -o Test09

Test05.o: Test05.c ud_thread.h Makefile
	${CC} ${CFLAGS} -c Test05.c

Test05: Test05.o thread.a Makefile
	${CC} ${CFLAGS} Test05.o thread.a -o Test05

#Clears all executable files and .o files created
clean: 
	rm -f *.o thread.a Test01x Phase3test Test09 Test05