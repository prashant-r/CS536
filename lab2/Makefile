#If you use threads, add -pthread here.
COMPILERFLAGS = -g -Wall -Wextra -Wno-sign-compare

#Any libraries you might need linked in.
#LINKLIBS = -lm

SENDEROBJECTS1 = obj/myping.o
RECEIVEROBJECTS1 = obj/mypingd.o
SENDEROBJECTS2 = obj/tcpclient.o
RECEIVEROBJECTS2 = obj/tcpserver.o
.PHONY: all clean

all : obj myping mypingd tcpserver tcpclient

myping: $(SENDEROBJECTS1)
	$(CC) $(COMPILERFLAGS) $^ -o $@ $(LINKLIBS)

mypingd: $(RECEIVEROBJECTS1)
	$(CC) $(COMPILERFLAGS) $^ -o $@ $(LINKLIBS)

tcpserver: $(RECEIVEROBJECTS2)
	$(CC) $(COMPILERFLAGS) $^ -o $@ $(LINKLIBS)

tcpclient: $(SENDEROBJECTS2)
	$(CC) $(COMPILERFLAGS) $^ -o $@ $(LINKLIBS)

clean :
	$(RM) -r obj/ myping mypingd tcpserver tcpclient output
obj/%.o: src/%.c
	$(CC) $(COMPILERFLAGS) -c -o $@ $<
obj:
	mkdir -p obj