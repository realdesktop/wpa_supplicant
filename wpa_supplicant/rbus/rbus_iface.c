#include "utils/includes.h"

#include "utils/common.h"

#include <rbus.h>
#include "rbus_wpas.h"
#include "../wpa_supplicant_i.h"
#include "../config.h"
#include "../bss.h"

#define end {""}

static struct rbus_child iface_children[] = {
    {"net", NULL},
    {"bss", NULL},
    end
};

char* read_iface_state(struct rbus_t *rbus, char* buf) {
    struct wpa_supplicant *wpa_s = rbus->native;

    return (char*)wpa_supplicant_state_txt(wpa_s->wpa_state);
};


#define pr(sub, name) char* read_##sub##_##name(struct rbus_t *rbus, char* buf)

char* read_iface_scanning(struct rbus_t *rbus, char* buf) {
    struct wpa_supplicant *wpa_s = rbus->native;

    return wpa_s->scanning ? "1" : "0";
};

#define iface_conf_int_prop(name) \
char* read_iface_##name (struct rbus_t *rbus, char* buf) { \
    static char ret[10]; \
    struct wpa_supplicant *wpa_s = rbus->native; \
    sprintf(ret, "%d", wpa_s->conf->name); \
    return ret; \
};

iface_conf_int_prop(ap_scan);
iface_conf_int_prop(bss_expiration_age);
iface_conf_int_prop(bss_expiration_scan_count);

char* read_iface_country(struct rbus_t *rbus, char* buf) {
    static char country[3];
    struct wpa_supplicant *wpa_s = rbus->native;

    country[0] = wpa_s->conf->country[0];
    country[1] = wpa_s->conf->country[1];
    country[2] = '\0';

    return country;
};

#define iface_str_prop(name) \
char* read_iface_##name(struct rbus_t *rbus, char* buf) { \
    struct wpa_supplicant *wpa_s = rbus->native; \
    return (char*)wpa_s->name; \
};

iface_str_prop(ifname);
iface_str_prop(driver);
iface_str_prop(confname);
iface_str_prop(bridge_ifname);

#define iface_num_prop(name) \
pr(iface, name) { \
    static char ret[256]; \
    struct wpa_supplicant *wpa_s = rbus->native; \
    if(wpa_s->name) { \
        sprintf(ret, "%u", wpa_s->name->id); \
        return ret; \
    } else { \
        return "none"; \
    }  \
};


iface_num_prop(current_ssid)
iface_num_prop(current_bss)

pr(iface, current_auth_mode) {
    struct wpa_supplicant *wpa_s = rbus->native;

    const char *eap_mode;
    const char *auth_mode;
    char eap_mode_buf[WPAS_DBUS_AUTH_MODE_MAX];

    if (wpa_s->wpa_state != WPA_COMPLETED) {
            auth_mode = "INACTIVE";
    } else if (wpa_s->key_mgmt == WPA_KEY_MGMT_IEEE8021X ||
        wpa_s->key_mgmt == WPA_KEY_MGMT_IEEE8021X_NO_WPA) {
            eap_mode = wpa_supplicant_get_eap_mode(wpa_s);
            os_snprintf(eap_mode_buf, WPAS_DBUS_AUTH_MODE_MAX,
                        "EAP-%s", eap_mode);
            auth_mode = eap_mode_buf;

    } else {
            auth_mode = wpa_key_mgmt_txt(wpa_s->key_mgmt,
                                         wpa_s->current_ssid->proto);
    }

    return (char*)auth_mode;

}


#define iprop(name) {#name, read_iface_##name}

static struct rbus_prop iface_props[] = {
    iprop(state),
    iprop(current_ssid),
    iprop(current_bss),
    iprop(scanning),
    iprop(ap_scan),
    iprop(bss_expiration_age),
    iprop(bss_expiration_scan_count),
    iprop(country), // FIXME: broken
    iprop(driver), // FIXME: broken
    iprop(ifname), // TODO: mpve out
    iprop(confname),
    iprop(bridge_ifname),
    iprop(current_auth_mode),

/*
 * TODO: implement
 *
 *  {"capabilities", <- tldr
 *  {"current_network", <- like current_ssid, different path
 *  {"blobs", <-- hmmm.. blobs array
 *  {"bsss" <- Hmmm.. array of paths, useless?
 *  {"networks" <- hmmm  array of paths. useless?
 *  {"process_credentials", wtf?
 *
 *  setters:
 *      ap_scan
 *      bss_expire_age
 *      bss_expire_count
 *      country
 *      process_credentials
 * */
    end
};

int wpas_rbus_register_interface(struct wpa_supplicant *wpa_s) {

    struct rbus_t *priv;

    priv = os_zalloc(sizeof(*priv));
    priv->native = wpa_s;
    strcpy(priv->name, wpa_s->ifname);
    priv->root = wpa_s->global->rbus_root;

    priv->childs = &iface_children[0];

    struct rbus_child *child = priv->childs;

    while((child+1)->name[0]) {
        child->next = child+1;
        child++;
    }

    priv->props = &iface_props[0];

    wpa_s->rbus = priv;

    child = priv->root->rbus.childs;

    while (child->next)
        child = child->next;

    child->next = os_zalloc(sizeof(*child));
    child = child->next;

    strcpy(child->name, "iface");
    child->rbus = priv;

    return 0;

};
