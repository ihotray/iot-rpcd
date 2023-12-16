#include <lualib.h>
#include <lauxlib.h>
#include <iot/mongoose.h>
#include <iot/cJSON.h>
#include <iot/iot.h>
#include "rpc.h"
#include "mqtt.h"

static int s_signo;
static void signal_handler(int signo) {
    s_signo = signo;
}


void rpc_req_handler(struct mg_connection *c, struct mg_str topic, struct mg_str data) {

    struct mg_str pub_topic = mg_str_n(topic.ptr, topic.len - mg_str(IOT_RPCD_TOPIC_POSTFIX).len);
    struct rpc_private *priv = (struct rpc_private *)c->mgr->userdata;

    cJSON *root = cJSON_ParseWithLength(data.ptr, data.len);
    cJSON *proxy = cJSON_GetObjectItem(root, FIELD_PROXY);

    const char *error_msg = NULL, *response = NULL, *printed = NULL;
    lua_State *L = NULL;

    if (root) {
        char *c_topic = mg_mprintf("%.*s", (int)pub_topic.len, pub_topic.ptr);
        cJSON_AddItemToObject(root, FIELD_TOPIC, cJSON_CreateString(c_topic));
        free(c_topic);
    }
    //add param
    printed = cJSON_Print(root);

    if ( cJSON_IsString(proxy) ) {
        MG_DEBUG(("pub %s ->  %s", printed, cJSON_GetStringValue(proxy)));
        struct mg_mqtt_opts pub_opts;
        memset(&pub_opts, 0, sizeof(pub_opts));
        pub_opts.topic = mg_str(cJSON_GetStringValue(proxy));
        pub_opts.message = mg_str(printed);
        pub_opts.qos = MQTT_QOS, pub_opts.retain = false;
        mg_mqtt_pub(c, &pub_opts);
        goto done;
    }
    
    L = luaL_newstate();
    luaL_openlibs(L);

    MG_DEBUG(("open lua lib finished"));

    //check args
    cJSON *method = cJSON_GetObjectItem(root, FIELD_METHOD);
    if ( !cJSON_IsString(method) ) {
        MG_ERROR(("method is not string"));
        error_msg = "{\"code\": -10401}\n";
        goto done;
    }

    MG_DEBUG(("dofile %s, PARAM: %s", priv->cfg.opts->iot_rpc_lua, printed));

    if ( luaL_dofile(L, priv->cfg.opts->iot_rpc_lua) ) {
        MG_ERROR(("open %s failed", priv->cfg.opts->iot_rpc_lua));
        error_msg = "{\"code\": -10402}\n";
        goto done;
    }

    MG_DEBUG(("dofile %s finished", priv->cfg.opts->iot_rpc_lua));

    lua_getfield(L, -1, cJSON_GetStringValue(method));
    if (!lua_isfunction(L, -1)) {
        MG_ERROR(("method %s is not a function", cJSON_GetStringValue(method)));
        error_msg = "{\"code\": -10403}\n";
        goto done;
    }

    MG_DEBUG(("check method %s finished", cJSON_GetStringValue(method)));

    lua_pushstring(L, printed);

    MG_DEBUG(("push param finished"));

    if (lua_pcall(L, 1, 1, 0)) {//one param, one return values, zero error func
        MG_ERROR(("lua call %s failed", priv->cfg.opts->iot_rpc_lua));
        error_msg = "{\"code\": -10404}\n";
    }

    MG_DEBUG(("call lua finished"));

    response = lua_tostring(L, -1);
    if (!response) {
        MG_ERROR(("lua call no response"));
        error_msg = "{\"code\": -10405}\n";
    }

    MG_DEBUG(("get lua resp finished"));

done:

    if ( !cJSON_IsString(proxy) ) {
        if (!response) {
            response = error_msg;
        }

        MG_DEBUG(("pub %s ->  %.*s", response, (int) pub_topic.len, pub_topic.ptr));

        struct mg_mqtt_opts pub_opts;
        memset(&pub_opts, 0, sizeof(pub_opts));
        pub_opts.topic = pub_topic;
        pub_opts.message = mg_str(response);
        pub_opts.qos = MQTT_QOS, pub_opts.retain = false;
        mg_mqtt_pub(c, &pub_opts);
    }

    cJSON_Delete(root);
    if (printed)
        cJSON_free((void*)printed);

    if (L)
        lua_close(L);

}

int rpc_init(void **priv, void *opts) {

    struct rpc_private *p;
    int timer_opts = MG_TIMER_REPEAT | MG_TIMER_RUN_NOW;

    signal(SIGINT, signal_handler);   // Setup signal handlers - exist event
    signal(SIGTERM, signal_handler);  // manager loop on SIGINT and SIGTERM

    *priv = NULL;
    p = calloc(1, sizeof(struct rpc_private));
    if (!p)
        return -1;
    
    p->cfg.opts = opts;
    mg_log_set(p->cfg.opts->debug_level);

    mg_mgr_init(&p->mgr);

    p->mgr.userdata = p;

    mg_timer_add(&p->mgr, 2000, timer_opts, timer_mqtt_fn, &p->mgr);

    *priv = p;

    return 0;

}


void rpc_run(void *handle) {
    struct rpc_private *priv = (struct rpc_private *)handle;
    while (s_signo == 0) mg_mgr_poll(&priv->mgr, 1000);  // Event loop, 1000ms timeout
}

void rpc_exit(void *handle) {
    struct rpc_private *priv = (struct rpc_private *)handle;
    mg_mgr_free(&priv->mgr);
    free(handle);
}

int rpc_main(void *user_options) {

    struct rpc_option *opts = (struct rpc_option *)user_options;
	void *rpc_handle;
	int ret;

    ret = rpc_init(&rpc_handle, opts);
    if (ret)
        exit(EXIT_FAILURE);

    rpc_run(rpc_handle);

    rpc_exit(rpc_handle);

    return 0;

}