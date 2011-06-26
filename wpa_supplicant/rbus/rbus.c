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


#include "rbus.h"
#include "../wpa_supplicant_i.h"
#include "../config_ssid.h"
#include "../bss.h"

extern Ixp9Srv p9srv;
struct rbus_root* RbusRoot = NULL;

static struct rbus_child root_children[] = {
    {"iface", NULL},
/* move to net    {"net", NULL}, */
    NULL,
};

static struct rbus_child iface_children[] = {
    {"net", NULL},
    {"bss", NULL},
    NULL,
};

static char zomg[] = "ZOMG\n";

char* read_state(struct rbus_t *rbus, char* buf) {
    return zomg;
};

char* read_iface_state(struct rbus_t *rbus, char* buf) {
    struct wpa_supplicant *wpa_s = rbus->native;

    return wpa_supplicant_state_txt(wpa_s->wpa_state);
};

char* read_iface_network(struct rbus_t *rbus, char* buf) {
    char ssid[256];
    struct wpa_supplicant *wpa_s = rbus->native;

    if(wpa_s->current_ssid) {
        sprintf(ssid, "%u", wpa_s->current_ssid->ssid);
        return ssid;
    } else {
        return "none";
    }
};

char* read_iface_name(struct rbus_t *rbus, char* buf) {
    struct wpa_supplicant *wpa_s = rbus->native;
    return wpa_s->ifname;
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
    NULL,
};

static struct rbus_prop iface_props[] = {
    {"state", read_iface_state},
    {"current_ssid", read_iface_network},
    {"ifname", read_iface_name},
    NULL,
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
    NULL,
};

char *read_bss_ssid(struct rbus_t *rbus, char* buf) {
    struct wpas_rbus_bss *bss = rbus->native;

    struct wpa_bss *res = wpa_bss_get_id(bss->wpa_s, bss->id);

    if(res->ssid)
        return res->ssid;
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
    NULL,
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

    priv = os_zalloc(sizeof(*priv));
    if (priv == NULL)
            return NULL;
    //priv->global = global;

    wpa_printf(MSG_DEBUG, "rbus: init real bus here");

    // accept
    IxpServer *srv = os_zalloc(sizeof(IxpServer));
    char *address;
    int fd;

    // FIXME: hardcoded
    address = "unix!/tmp/wpa_supplicant.9p";

    fd = ixp_announce(address);
    if(fd < 0) {
            err(1, "ixp_announce");
    }

    ixp_listen(srv, fd, &p9srv, ixp_serve9conn, NULL);

    eloop_register_sock(fd, EVENT_TYPE_READ, (eloop_sock_handler)process_watch_read,
			    priv, srv->conn);

    priv->srv = srv;

    priv->rbus.childs = &root_children[0];

    struct rbus_child *child = priv->rbus.childs;

    while((child+1)->name[0]) {
        child->next = child+1;
        child++;
    }

    priv->rbus.props = &root_props[0];
    priv->rbus.root = priv;

    RbusRoot = priv;

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
