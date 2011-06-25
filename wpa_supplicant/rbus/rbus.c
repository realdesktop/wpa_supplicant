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

extern Ixp9Srv p9srv;
struct rbus_root* RbusRoot = NULL;

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
    priv->root = wpa_s->global->rbus_root;

    wpa_s->rbus = priv;

    return 0;

};
