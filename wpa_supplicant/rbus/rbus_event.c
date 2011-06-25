#include "utils/includes.h"
#include "utils/common.h"

#include "../wpa_supplicant_i.h"

#include "rbus.h"

char* wpas_rbus_prop_s[] = {
    "ap_scan",
    "scanning",
    "state",
    "current_bss",
    "current_network",
    "current_auth_mode",
    "bsss",
};

void wpas_rbus_signal_prop_changed(struct wpa_supplicant *wpa_s, enum wpas_rbus_prop prop) {
    wpa_printf(MSG_ERROR, "rbus: prop %d %s changed", prop, wpas_rbus_prop_s[prop]);

    rbus_event(&wpa_s->rbus->events, "prop %s\n", wpas_rbus_prop_s[prop]);

    rbus_event(&wpa_s->rbus->root->rbus.events, "net prop %s\n", wpas_rbus_prop_s[prop]);

}

