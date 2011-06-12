/*
 * Stub fs, originally copied from m9u
 *
 * */

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include <string.h>
#include <time.h>



typedef union IxpFileIdU IxpFileIdU;
typedef short bool;

union IxpFileIdU {
	char*		buf;
	void*		ref;
};


#include <ixp.h>
#include <ixp_srvutil.h>

typedef int64_t		vlong;
#define QID(t, i) (((vlong)((t)&0xFF)<<32)|((i)&0xFFFFFFFF))

#define structptr(ptr, type, offset) \
            ((type*)((char*)(ptr) + (offset)))
#define structmember(ptr, type, offset) \
            (*structptr(ptr, type, offset))

#ifndef nelem
#  define nelem(ary) (sizeof(ary) / sizeof(*ary))
#endif


static IxpPending	events;

char    buffer[8092];
char*   _buffer;
char*   const _buf_end = buffer + sizeof buffer;

void
rbus_event(const char *format, ...) {
	va_list ap;

	va_start(ap, format);
	vsnprintf(buffer, sizeof buffer, format, ap);
	va_end(ap);

	ixp_pending_write(&events, buffer, strlen(buffer));
}


enum {
    FsRoot,

    FsFRctl,
    FsFEvent,
};

typedef char* (*MsgFunc)(void*, IxpMsg*);
typedef char* (*BufFunc)(void*);


typedef struct ActionTab ActionTab;
struct ActionTab {
	MsgFunc		msg;
	BufFunc		read;
	size_t		buffer;
	size_t		size;
	int		max;
};

static char ctl_buf[] = "ctl ya";

char *readctl(void) {
    return ctl_buf;
};

ActionTab actiontab[] = {
    [FsFRctl] = {
        .msg = (MsgFunc)0,
        .read = (BufFunc)readctl,
    },
};

static IxpDirtab dirtab_root[]=	 {
    {".",	P9_QTDIR,	FsRoot,		0500|P9_DMDIR },
    {"ctl",	P9_QTAPPEND,	FsFRctl,	0600|P9_DMAPPEND },
    {"event",	P9_QTFILE,	FsFEvent,	0600 },
    {NULL},
};

static IxpDirtab* dirtab[] = {
	[FsRoot] = dirtab_root,
};

static IxpFileId*
lookup_file(IxpFileId *parent, char *name)
{
	IxpFileId *ret, *file, **last;
	IxpDirtab *dir;

	if(!(parent->tab.perm & P9_DMDIR))
		return NULL;
	dir = dirtab[parent->tab.type];
	last = &ret;
	ret = NULL;
	for(; dir->name; dir++) {
#               define push_file(nam, id_, vol)   \
			file = ixp_srv_getfile(); \
			file->id = id_;           \
			file->volatil = vol;      \
			*last = file;             \
			last = &file->next;       \
			file->tab = *dir;         \
			file->tab.name = strdup(nam)
		if(!name && !(dir->flags & FLHide) || name && !strcmp(name, dir->name)) {
			push_file(file->tab.name, 0, 0);
			file->p.ref = parent->p.ref;
			file->index = parent->index;
			/* Special considerations: */
			if(name)
				goto LastItem;
		}
#		undef push_file
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
	f->tab = dirtab[FsRoot][0];
	f->tab.name = strdup("/");
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


	switch(f->tab.type) {
	case FsFEvent:
		ixp_pending_pushfid(&events, r->fid);
		break;
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
	ActionTab *t;
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
		t = &actiontab[f->tab.type];
		if(f->tab.type < nelem(actiontab)) {
			if(t->read)
				buf = t->read(f->p.ref);
			else if(t->buffer && t->max)
				buf = structptr(f->p.ref, char, t->buffer);
			else if(t->buffer)
				buf = structmember(f->p.ref, char*, t->buffer);
			else
				goto done;
			n = t->size ? structmember(f->p.ref, int, t->size) : strlen(buf);
			ixp_srv_readbuf(r, buf, n);
			ixp_respond(r, NULL);
			found++;
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
