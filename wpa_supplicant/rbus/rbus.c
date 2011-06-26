#include "utils/includes.h"

#include "utils/common.h"
#include "utils/eloop.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>


#include <rbus.h>
#include "rbus_wpas.h"
#include "../src/common/wpa_common.h"
#include "../wpa_supplicant_i.h"
#include "../config.h"
#include "../config_ssid.h"
#include "../bss.h"

#define end {""}

static struct rbus_child root_children[] = {
    {"iface", NULL, NULL},
    end
};

static struct rbus_child iface_children[] = {
    {"net", NULL},
    {"bss", NULL},
    end
};

static char zomg[] = "ZOMG\n";

char* read_state(struct rbus_t *rbus, char* buf) {
    return zomg;
};

char* read_iface_state(struct rbus_t *rbus, char* buf) {
    struct wpa_supplicant *wpa_s = rbus->native;

    return (char*)wpa_supplicant_state_txt(wpa_s->wpa_state);
};


static struct rbus_prop root_props[] = {
    {"status", read_state},
    {"debug", read_state},
/*
 *  {"debug/level",
 *  {"debug/timestamp",
 *  {"debug/showkeys",
 * NO ->  {"interfaces"
 *  {"eap_methods",
 *  */
    end
};

#define pr(sub, name) char* read_##sub##_##name(struct rbus_t *rbus, char* buf)

char* read_iface_scanning(struct rbus_t *rbus, char* buf) {
    struct wpa_supplicant *wpa_s = rbus->native;

    return wpa_s->scanning ? "1" : "0";
};

#define net_conf_int_prop(name) \
char* read_iface_##name (struct rbus_t *rbus, char* buf) { \
    static char ret[10]; \
    struct wpa_supplicant *wpa_s = rbus->native; \
    sprintf(ret, "%d", wpa_s->conf->name); \
    return ret; \
};

net_conf_int_prop(ap_scan);
net_conf_int_prop(bss_expiration_age);
net_conf_int_prop(bss_expiration_scan_count);

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

char *read_bss_ssid(struct rbus_t *rbus, char* buf) {
    struct wpas_rbus_bss *bss = rbus->native;

    struct wpa_bss *res = wpa_bss_get_id(bss->wpa_s, bss->id);

    if(res->ssid)
        return (char*)res->ssid;
    else
        return "NONE";
};

static struct rbus_prop bss_props[] = {
    {"ssid", read_bss_ssid},
/*
    {"bssid",
    {"privacy",
    {"mode",
    {"signal",
    {"freq",
    {"rates",
    {"wpa",
    {"rsn",
    {"ies",
*/
    end
};

/*
 * Callback from epool
 * */
static void process_watch_read(int sock, struct rbus_root *priv, IxpConn* conn)
{
        conn->read(conn);
}



/*
 * Callback from patched libixp
 *
 * called when new client registered
 * */
void register_fd(IxpConn* conn)  {
        eloop_register_sock((int)conn->fd, EVENT_TYPE_READ, (eloop_sock_handler)process_watch_read,
			    NULL, (void*)conn);

};

/*
 * Callback from patched libixp
 *
 * callaed when connection died
 * */
void unregister_fd(IxpConn* conn) {
        eloop_unregister_sock(conn->fd, EVENT_TYPE_READ);
};



/*
 * Init function called from notify.c
 * */
struct rbus_root * wpas_rbus_init(struct wpa_global *global)
{
    struct rbus_root *priv;
    int fd;

    priv = rbus_init("unix!/tmp/wpa_supplicant.9p");
    fd = priv->srv->conn->fd;

    wpa_printf(MSG_DEBUG, "rbus: init real bus here");

    rbus_callbacks(&register_fd, &unregister_fd);

    eloop_register_sock(fd, EVENT_TYPE_READ, (eloop_sock_handler)process_watch_read,
			    priv, priv->srv->conn);

    priv->rbus.childs = &root_children[0];

    struct rbus_child *child = priv->rbus.childs;

    while((child+1)->name[0]) {
        child->next = child+1;
        child++;
    }

    priv->rbus.props = &root_props[0];
    priv->rbus.root = priv;

    return priv;
}

void wpas_rbus_deinit(struct rbus_root *priv) {

    eloop_unregister_sock(priv->srv->conn->fd, EVENT_TYPE_READ);

    // FIXME: free resources, shutdown ixp, cleanup sockets
}


int wpas_rbus_register_interface(struct wpa_supplicant *wpa_s) {

    wpa_printf(MSG_ERROR, "interface add");

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

    printf("root: wpa_s %x, root: %x, rbus %x\n",
            (int)wpa_s,
            (int)priv->root,
            (int)priv
    );

    child = priv->root->rbus.childs;

    while (child->next)
        child = child->next;

    child->next = os_zalloc(sizeof(*child));
    child = child->next;

    strcpy(child->name, "iface");
    child->rbus = priv;

    return 0;

};

void wpas_rbus_register_network(struct wpa_supplicant *wpa_s, struct wpa_ssid *ssid) {

    wpa_printf(MSG_ERROR, "net add");

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

void wpas_rbus_register_bss(struct wpa_supplicant *wpa_s,
			   u8 bssid[ETH_ALEN], unsigned int id) {

    wpa_printf(MSG_ERROR, "bss add");

    struct rbus_t *priv;

    priv = os_zalloc(sizeof(*priv));


    struct wpas_rbus_bss * bss;
    bss =  os_zalloc(sizeof(struct wpas_rbus_bss));

    bss->id = id;
    bss->wpa_s = wpa_s;

    priv->native = bss;

    sprintf(priv->name, "bss%d", id);

    priv->root = wpa_s->global->rbus_root;

    priv->props = &bss_props[0];

    //wpa_s->rbus = priv; FIXME: wtf?

    struct rbus_child *child = wpa_s->rbus->childs;

    while (child->next)
        child = child->next;

    child->next = os_zalloc(sizeof(*child));
    child = child->next;

    strcpy(child->name, "bss");
    child->rbus = priv;

}

void wpas_rbus_unregister_bss(struct wpa_supplicant *wpa_s,
			   u8 bssid[ETH_ALEN], unsigned int id) {
    wpa_printf(MSG_ERROR, "bss remove %d", id);

    struct wpas_rbus_bss * bss;
    struct rbus_child *child = wpa_s->rbus->childs, *p=NULL;

    while (child) {
        printf("child %x\n", (int)child);
        if(strcmp(child->name, "bss"))
            goto skip;

        if(!child->rbus)
            goto skip;

        bss = child->rbus->native;

        if(bss->id != id)
            goto skip;

        printf("found %d\n", id);

        os_free(bss);
        os_free(child->rbus);

        if(p)
            p->next = child->next;

        os_free(child);

        return;

    skip:

        p = child;
        child = child->next;

    }

    wpa_printf(MSG_ERROR, "no bss to remove! wtf?");



}
