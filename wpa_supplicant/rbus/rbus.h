#ifndef RBUS_H
#define RBUS_H

struct wpa_global;
typedef struct IxpServer IxpServer;

typedef int rbus_con;

struct wpas_rbus_priv {
        IxpServer *srv;
};

struct wpas_rbus_priv * wpas_rbus_init(struct wpa_global *global);
void wpas_rbus_deinit(struct wpas_rbus_priv *priv);

#endif
