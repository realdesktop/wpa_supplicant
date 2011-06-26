#ifndef RBUS_WPAS_H
#define RBUS_WPAS_H

#define WPAS_DBUS_AUTH_MODE_MAX 64

struct wpa_global;
struct wpa_supplicant;

struct rbus_root;

struct wpas_rbus_net {
        struct wpa_supplicant *wpa_s;
        struct wpa_ssid *ssid;
};

struct wpas_rbus_bss {
        struct wpa_supplicant *wpa_s;
        int id;
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


struct rbus_root * wpas_rbus_init(struct wpa_global *global);
void wpas_rbus_deinit(struct rbus_root *priv);


void wpas_rbus_signal_prop_changed(struct wpa_supplicant *wpa_s, enum wpas_rbus_prop prop);

int wpas_rbus_register_interface(struct wpa_supplicant *wpa_s);
void wpas_rbus_register_network(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid);

void wpas_rbus_register_bss(struct wpa_supplicant *wpa_s,
			   unsigned char bssid[6], unsigned int id);

#endif
