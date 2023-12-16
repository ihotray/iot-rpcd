#ifndef __IOT_RPC_H__
#define __IOT_RPC_H__

#include <iot/mongoose.h>


struct rpc_option {

    const char *iot_rpc_lua;

    const char *mqtt_serve_address;      //mqtt 服务端口
    int mqtt_keepalive;                  //mqtt 保活间隔

    int debug_level;

};

struct rpc_config {
    struct rpc_option *opts;
};

struct rpc_private {
    struct rpc_config cfg;

    struct mg_mgr mgr;

    struct mg_connection *mqtt_conn;
    uint64_t ping_active;
    uint64_t pong_active;

};

void rpc_req_handler(struct mg_connection *c, struct mg_str topic, struct mg_str data);
int rpc_main(void *user_options);


#endif