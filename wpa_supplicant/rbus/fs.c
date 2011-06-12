/*
 * Stub fs, originally copied from m9u
 *
 * */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <ixp.h>

void
fs_attach(Ixp9Req *r)
{
	r->fid->qid.type = P9_QTDIR;
	r->fid->qid.path = 0;
	r->ofcall.rattach.qid = r->fid->qid;
	ixp_respond(r, NULL);
}

void
fs_walk(Ixp9Req *r)
{
	r->ofcall.rwalk.nwqid = 0;

	ixp_respond(r, NULL);
}

void
fs_open(Ixp9Req *r)
{
	ixp_respond(r, NULL);
}

void
fs_clunk(Ixp9Req *r)
{
	ixp_respond(r, NULL);
}


void
fs_stat(Ixp9Req *r)
{
	ixp_respond(r, NULL);
}

void
fs_read(Ixp9Req *r)
{
        ixp_respond(r, NULL);
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
