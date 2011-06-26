/*
 * Stub fs, originally copied from m9u
 *
 * */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <string.h>
#include <time.h>

#include "rbus.h"

typedef int64_t		vlong;
#define QID(t, i) (((vlong)((t)&0xFF)<<32)|((i)&0xFFFFFFFF))

#define structptr(ptr, type, offset) \
            ((type*)((char*)(ptr) + (offset)))
#define structmember(ptr, type, offset) \
            (*structptr(ptr, type, offset))

#ifndef nelem
#  define nelem(ary) (sizeof(ary) / sizeof(*ary))
#endif


/* static IxpPending	events; */

char    buffer[8092];

extern struct rbus_root* RbusRoot;


void
rbus_event(IxpPending *events, const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	vsnprintf(buffer, sizeof buffer, format, ap);
	va_end(ap);

	ixp_pending_write(events, buffer, strlen(buffer));
}


enum {
    FsObject,
    FsProp,
    FsList,
    FsEvent,
};

typedef char* (*MsgFunc)(void*, IxpMsg*);
typedef char* (*BufFunc)(void*);


static IxpFileId*
lookup_file(IxpFileId *parent, char *name)
{
	IxpFileId *ret, *file, **last;

	if(!(parent->tab.perm & P9_DMDIR))
		return NULL;

	//dir = parent->p.rbus->dirtab;

	last = &ret;
	ret = NULL;


#       define push_file(nam, id_, vol)   \
            file = ixp_srv_getfile(); \
            file->id = id_;           \
            file->volatil = vol;      \
            *last = file;             \
            last = &file->next;       \
            file->tab = parent->tab; \
            file->tab.name = strdup(nam)

        if(!name) {
            push_file(".", 0, 0);
            file->index = 0;
        }

        if( !parent->tab.type ) {

            if(!name || !strcmp(name, "events")) {
                push_file("events", 1, 0);
                file->tab.perm = P9_OREAD;
                file->tab.qtype = P9_QTFILE | P9_QTAPPEND;
                file->tab.type = FsEvent;

                file->p.rbus = parent->p.rbus;
            }

            struct rbus_prop *prop = parent->p.rbus->props;

            for(; prop && prop->name[0]; prop++) {

                    if(!name || name && !strcmp(name, prop->name)) {

                            push_file(prop->name, (int)prop, 1);

                            file->p.ref = parent->p.ref;
                            file->p.rbus = parent->p.rbus;

                            file->index = (int)prop;
                            file->tab.perm = 0600;
                            file->tab.qtype = P9_QTFILE;
                            file->tab.type = FsProp;

                            /* Special considerations: */
                            if(name)
                                    goto LastItem;
                    }
            }
        }

        struct rbus_child *child = parent->p.rbus->childs;
        char *cname;
	for(; child; child = child->next) {

                if(!child->rbus != !parent->tab.type)
                        continue;

                if(child->rbus)
                        cname = child->rbus->name;
                else
                        cname = child->name;

                if(child->rbus && strcmp(parent->tab.name, child->name))
                        continue;


		if(!name || name && !strcmp(name, cname)) {

                        push_file(cname, (int)child, 1);
                        if(child->rbus) {
                            file->p.rbus = child->rbus;
                            file->tab.type = FsObject;
                        } else  {
                            file->p.rbus = parent->p.rbus;
                            file->tab.type = FsList;
                        }


			file->index = (int)child;
                        file->tab.perm = 0700|P9_DMDIR;
                        file->tab.qtype = P9_QTDIR;

			/* Special considerations: */
			if(name)
				goto LastItem;
		}
	}


LastItem:
	*last = NULL;

	return ret;
}

static void
dostat(IxpStat *s, IxpFileId *f) {
	s->type = 0;
	s->dev = 0;
	s->qid.path = QID(f->tab.type, f->id);
	s->qid.version = 0;
	s->qid.type = f->tab.qtype;
	s->mode = f->tab.perm;
	s->atime = time(NULL);
	s->mtime = s->atime;
	s->length = 0;
	s->name = f->tab.name;
	s->uid = "";
	s->gid = "";
	s->muid = "";
}


void
fs_attach(Ixp9Req *r) {
	IxpFileId *f;

	f = ixp_srv_getfile();
	f->tab = (IxpDirtab){"/", P9_QTDIR, FsObject, 0500|P9_DMDIR};
        f->p.rbus = &RbusRoot->rbus;
	r->fid->aux = f;
	r->fid->qid.type = f->tab.qtype;
	r->fid->qid.path = QID(f->tab.type, 0);
	r->ofcall.rattach.qid = r->fid->qid;
	ixp_respond(r, NULL);
}

void
fs_walk(Ixp9Req *r)
{
        ixp_srv_walkandclone(r, lookup_file);
}

void
fs_open(Ixp9Req *r)
{
        IxpFileId *f;

	f = r->fid->aux;

	if(!ixp_srv_verifyfile(f, lookup_file)) {
		ixp_respond(r, "Enofile");
		return;
	}

	if(f->tab.qtype & P9_QTAPPEND) {
		ixp_pending_pushfid(&f->p.rbus->events, r->fid);
        }

	ixp_respond(r, NULL);
}

void
fs_clunk(Ixp9Req *r)
{
	ixp_respond(r, NULL);
}


void
fs_stat(Ixp9Req *r) {
	IxpMsg m;
	IxpStat s;
	int size;
	char *buf;
	IxpFileId *f;

	f = r->fid->aux;

	if(!ixp_srv_verifyfile(f, lookup_file)) {
		ixp_respond(r, "enofile");
		return;
	}

	dostat(&s, f);
	size = ixp_sizeof_stat(&s);
	r->ofcall.rstat.nstat = size;
	buf = ixp_emallocz(size);

	m = ixp_message(buf, size, MsgPack);
	ixp_pstat(&m, &s);

	r->ofcall.rstat.stat = (unsigned char*)m.data;
	ixp_respond(r, NULL);
}

void
fs_read(Ixp9Req *r)
{
	char *buf;
	IxpFileId *f;
	//ActionTab *t;
	int n, found;

	f = r->fid->aux;
        found = 0;

	if(!ixp_srv_verifyfile(f, lookup_file)) {
		ixp_respond(r, "enofile");
		return;
	}

	if(f->tab.perm & P9_DMDIR && f->tab.perm & 0400) {
		ixp_srv_readdir(r, lookup_file, dostat);
		return;
	}else{
		if(f->pending) {
			ixp_pending_respond(r);
			return;
		}

                struct rbus_prop *prop = f->p.rbus->props;

                for(; prop && prop->name[0]; prop++) {
                        if(strcmp(prop->name, f->tab.name))
                            continue;

                        buf = prop->read(f->p.rbus, buf);
                        n = strlen(buf);

                        ixp_srv_readbuf(r, buf, n);
			ixp_respond(r, NULL);
			found++;

                        break;

                }

	done:
		switch(f->tab.type) {
		default:
			if(found)
				return;
		}
	}
}


void
fs_write(Ixp9Req *r)
{
    	ixp_respond(r, NULL);
}

void
fs_wstat(Ixp9Req *r)
{
	ixp_respond(r, NULL); /* pretend it worked */
}

Ixp9Srv p9srv = {
	.open=fs_open,
	.clunk=fs_clunk,
	.walk=fs_walk,
	.read=fs_read,
	.stat=fs_stat,
	.write=fs_write,
	.wstat=fs_wstat,
	.attach=fs_attach
};
