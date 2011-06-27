#include "utils/includes.h"

#include "utils/common.h"

#include <rbus.h>
#include "rbus_wpas.h"
#include "../wpa_supplicant_i.h"
#include "../config.h"

#define end {""}

#define pr(sub, name) char* read_##sub##_##name(struct rbus_t *rbus, char* buf)

#define iprop(name) {#name, read_iface_##name}


char *read_net_enabled(struct rbus_t *rbus, char* buf) {
    struct wpas_rbus_net *net = rbus->native;

    return net->ssid->disabled ? "0" : "1";
};

char *read_net_prop(struct rbus_t *rbus, char *prop) {
    struct wpas_rbus_net *net = rbus->native;

    return wpa_config_get(net->ssid, prop);
};

static struct rbus_prop net_props[] = {
/*
 * DYNAMIC properties list, zomg
 * WTF? {"properties,
 * {"enabled",
 *
 * */
    {"enabled", read_net_enabled},
    end
};

void wpas_rbus_register_network(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid) {

    struct rbus_t *priv;

    priv = os_zalloc(sizeof(*priv));


    struct wpas_rbus_net * net;
    net =  os_zalloc(sizeof(struct wpas_rbus_net));

    net->ssid = ssid;
    net->wpa_s = wpa_s;

    priv->native = net;

    if(ssid->ssid) {
        strcpy(priv->name, ssid->ssid); // FIXME: wrong!
    } else {
        strcpy(priv->name, "NONE");
    }

    priv->root = wpa_s->global->rbus_root;

    priv->props = os_zalloc(sizeof(struct rbus_prop) * 32); // FIXME

    char **props = wpa_config_get_all(net->ssid, 1);
    char **iterator;
    struct rbus_prop *prop = priv->props;

    iterator = props;
    while (*iterator) {
        strcpy(prop->name, *iterator);
        prop->read = read_net_prop;

        iterator += 2;

        prop++;
    }

    memcpy(prop, &net_props[0], sizeof(struct rbus_prop));

    //wpa_s->rbus = priv; FIXME: wtf?

    struct rbus_child *child = wpa_s->rbus->childs;

    while (child->next)
        child = child->next;

    child->next = os_zalloc(sizeof(*child));
    child = child->next;

    strcpy(child->name, "net");
    child->rbus = priv;

};
