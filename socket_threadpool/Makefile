CFLAGS+=-Wall
LDFLAGS+=-pthread

server: server.o Threadpool.o
	gcc $^ -o $@ $(CFLAGS) $(LDFLAGS)

%.o:%.c
	gcc -c $< -o $@ $(CFLAGS) $(LDFLAGS)
clean:
	rm -rf *.o server
