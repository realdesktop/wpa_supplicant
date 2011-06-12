#ifndef RBUS_H
#define RBUS_H

struct wpa_global;
typedef struct IxpServer IxpServer;

typedef int rbus_con;

struct wpas_rbus_priv {
        IxpServer *srv;
};

enum wpas_rbus_prop {
	WPAS_RBUS_PROP_AP_SCAN,
	WPAS_RBUS_PROP_SCANNING,
	WPAS_RBUS_PROP_STATE,
	WPAS_RBUS_PROP_CURRENT_BSS,
	WPAS_RBUS_PROP_CURRENT_NETWORK,
	WPAS_RBUS_PROP_CURRENT_AUTH_MODE,
	WPAS_RBUS_PROP_BSSS,
};


struct wpas_rbus_priv * wpas_rbus_init(struct wpa_global *global);
void wpas_rbus_deinit(struct wpas_rbus_priv *priv);


void wpas_rbus_signal_prop_changed(struct wpa_supplicant *wpa_s, enum wpas_rbus_prop prop);

#endif
