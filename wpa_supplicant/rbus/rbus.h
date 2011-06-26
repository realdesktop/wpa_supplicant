#ifndef RBUS_H
#define RBUS_H

struct wpa_global;
struct wpa_supplicant;

struct rbus_root;
struct rbus_t;

#include <ixp.h>

typedef union IxpFileIdU IxpFileIdU;
typedef struct IxpPending	IxpPending;
typedef short bool;

union IxpFileIdU {
	void*		ref;
        struct rbus_t* rbus;
};



#include <ixp_srvutil.h>


typedef struct IxpServer IxpServer;


struct rbus_child {
        char name[32];
        struct rbus_t *rbus;
        struct rbus_child *next;
};


typedef char* (*rbus_prop_read)(struct rbus_t*, char*);

struct rbus_prop {
        char name[32];
        rbus_prop_read read;
};

struct wpas_rbus_net {
        struct wpa_supplicant *wpa_s;
        struct wpa_ssid *ssid;
};


struct rbus_t {
        char name[32];
        void *native;
        struct rbus_root *root;
        IxpPending events;
        struct rbus_child *childs;
        struct rbus_prop *props;
};

struct rbus_root {
        IxpServer *srv;
        struct rbus_t rbus;
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

void rbus_event(IxpPending *events, const char *format, ...);

#endif
