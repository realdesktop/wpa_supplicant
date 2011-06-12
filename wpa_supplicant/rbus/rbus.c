#include "utils/includes.h"
#include "utils/common.h"

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

#include <ixp.h>

extern Ixp9Srv p9srv;

#include "rbus.h"

/*
 * Callback from epool
 * */
static void process_watch_read(int sock, struct wpas_rbus_priv *priv, IxpConn* conn)
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
struct wpas_rbus_priv * wpas_rbus_init(struct wpa_global *global)
{
    struct wpas_rbus_priv *priv;

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

    return priv;
}

void wpas_rbus_deinit(struct wpas_rbus_priv *priv) {

    eloop_unregister_sock(priv->srv->conn->fd, EVENT_TYPE_READ);

    // FIXME: free resources, shutdown ixp, cleanup sockets
}
