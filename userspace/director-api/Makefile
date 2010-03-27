OBJECTS = msg-common.o director-api.o npm-check.o node-connected.o immigration-request.o task-exitted.o generic_user_message.o
OPENSSL_FUNC =../openssl-func
CFLAGS += -g -I$(OPENSSL_FUNC) -fPIC
AR		= ar

all: director-api.a

director-api: $(OBJECTS)
	$(CC) $(OBJECTS) -L$(OPENSSL_FUNC) -I/home/user/dev/c/libn-1.0-pre6/include -L/home/user/dev/c/libnl-1.0-pre6/lib -L/usr/lib -lnl -lssl -lopensslfunc  -o ossld

director-api.a: $(OBJECTS)
	$(AR) rcs libdirector-api.a $(OBJECTS)


clean:
	rm -f *.a *.so *.o