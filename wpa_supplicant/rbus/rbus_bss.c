#include "utils/includes.h"

#include "utils/common.h"

#include <rbus.h>
#include "rbus_wpas.h"
#include "../wpa_supplicant_i.h"
#include "../config.h"
#include "../bss.h"

#define end {""}

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


void wpas_rbus_register_bss(struct wpa_supplicant *wpa_s,
			   u8 bssid[ETH_ALEN], unsigned int id) {

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

    struct wpas_rbus_bss * bss;
    struct rbus_child *child = wpa_s->rbus->childs, *p=NULL;

    while (child) {
        if(strcmp(child->name, "bss"))
            goto skip;

        if(!child->rbus)
            goto skip;

        bss = child->rbus->native;

        if(bss->id != id)
            goto skip;

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
