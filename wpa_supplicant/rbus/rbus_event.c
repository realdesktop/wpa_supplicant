#include "utils/includes.h"
#include "utils/common.h"

#include "../wpa_supplicant_i.h"

#include "rbus_wpas.h"
#include <rbus.h>

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

    char *buf = "(wtf?)", *name;

    name = wpas_rbus_prop_s[prop];

    struct rbus_prop *rprop = wpa_s->rbus->props;

    for(; rprop && rprop->name[0]; rprop++) {
            if(!strcmp(name, rprop->name)) {
                    buf = rprop->read(wpa_s->rbus, name);
            }
    };

    wpa_printf(MSG_ERROR, "rbus: prop %d %s changed = %s", prop, name, buf);

    rbus_event(&wpa_s->rbus->events, "prop %s %s\n", wpas_rbus_prop_s[prop], buf);

}

