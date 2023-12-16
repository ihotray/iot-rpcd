PROG ?= iot-rpcd
DEFS ?= -llua -liot-base-nossl -liot-json
EXTRA_CFLAGS ?= -Wall -Werror
CFLAGS += $(DEFS) $(EXTRA_CFLAGS)

all: $(PROG)

SRCS = main.c mqtt.c rpc.c

$(PROG):
	$(CC) $(SRCS) $(CFLAGS) -o $@


clean:
	rm -rf $(PROG) *.o
