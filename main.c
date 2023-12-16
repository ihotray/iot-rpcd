#include <iot/mongoose.h>
#include <iot/iot.h>
#include "rpc.h"

static void usage(const char *prog) {
    fprintf(stderr,
            "IoT-SDK v.%s\n"
            "Usage: %s OPTIONS\n"
            "  -x PATH   - iot rpc lua script, default: '%s'\n"
            "  -s ADDDR  - mqtt server address, default: '%s'\n"
            "  -k n      - mqtt timeout, default: '%d'\n"
            "  -v LEVEL  - debug level, from 0 to 4, default: %d\n",
            MG_VERSION, prog, "/www/iot/iot-rpc.lua", MQTT_LISTEN_ADDR, 6, MG_LL_INFO);

    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {

    struct rpc_option opts = {
	.iot_rpc_lua = "/www/iot/iot-rpc.lua",
        .mqtt_serve_address = MQTT_LISTEN_ADDR,
        .mqtt_keepalive = 6,
        .debug_level = MG_LL_INFO
    };

    // Parse command-line flags
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-x") == 0) {
            opts.iot_rpc_lua = argv[++i];
        } else if (strcmp(argv[i], "-s") == 0) {
            opts.mqtt_serve_address = argv[++i];
        } else if (strcmp(argv[i], "-k") == 0) {
            opts.mqtt_keepalive = atoi(argv[++i]);
            if (opts.mqtt_keepalive < 6) {
                opts.mqtt_keepalive = 6;
            }
        } else if (strcmp(argv[i], "-v") == 0) {
            opts.debug_level = atoi(argv[++i]);
        } else {
            usage(argv[0]);
        }
    }

    MG_INFO(("IoT-SDK version  : v%s", MG_VERSION));
    MG_INFO(("rpc lua script   : %s", opts.iot_rpc_lua));

    rpc_main(&opts);
    
    return 0;
}
