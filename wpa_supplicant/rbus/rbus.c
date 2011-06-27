#include "utils/includes.h"

#include "utils/common.h"
#include "utils/eloop.h"

#include <rbus.h>
#include "rbus_wpas.h"

#define end {""}

static struct rbus_child root_children[] = {
    {"iface", NULL, NULL},
    end
};

static char zomg[] = "ZOMG\n";

char* read_state(struct rbus_t *rbus, char* buf) {
    return zomg;
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

