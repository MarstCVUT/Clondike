OBJECTS = director-cli.o
CFLAGS += -g -fPIC
AR		= ar

all: director-cli.a test-bin

director-cli: $(OBJECTS)
	$(CC) $(OBJECTS) -L$(OPENSSL_FUNC) -I/home/user/dev/c/libn-1.0-pre6/include -L/home/user/dev/c/libnl-1.0-pre6/lib -L/usr/lib -lnl -lssl -lopensslfunc  -o ossld

director-cli.a: $(OBJECTS)
	$(AR) rcs libdirector-cli.a $(OBJECTS)

test-bin: test.o director-cli.a
	$(CC) test.o -L$(OPENSSL_FUNC) -I/home/user/dev/c/libn-1.0-pre6/include -L. -L/usr/lib -ldirector-cli  -o test-bin

clean:
	rm -f *.a *.so *.o