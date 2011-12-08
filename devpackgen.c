#include "u.h"
#include "../port/lib.h"
#include "mem.h"
#include "dat.h"
#include "fns.h"
#include "io.h"
#include "../port/error.h"
#include "../port/netif.h"

#include "etherif.h"

#include "./devpackgen.h"

static struct Etherpkt*
dup_epkt(struct Etherpkt *pkt)
{
	Etherpkt *duped;

	duped = malloc(sizeof(struct Etherpkt));
	if (duped == nil)
		return nil;

	memset(duped, 0, sizeof(struct Etherpkt));
	memmove(duped, pkt, sizeof(struct Etherpkt));

	return duped;
}

static Block*
dup_bp(Block *bp)
{
	Block *duped;
	
	duped = malloc(sizeof(Block));
	if (duped == nil)
		return nil;

	memset(duped, 0, sizeof(Block));
	memmove(duped, bp, sizeof(Block));

	duped->rp = (uchar *)dup_epkt((struct Etherpkt *)bp->rp);
	if (duped->rp == nil)
		return nil;
	duped->wp = (uchar *)((int)duped->rp + bp->lim);

	return duped;
}

static void
free_bp(Block *bp)
{
	freeb(bp);

	return;
}

void 
pg_record(Block *bp)
{
	print("rec");

	if (recorded_blocks[pgrecidx] != nil) {
		freeb(recorded_blocks[pgrecidx]);
	}

	recorded_blocks[pgrecidx++] = bp;

	if (pgrecidx >= PG_MAX_RECORD)
		pgrecidx =  0;

	return;
}

Block*
pg_get_next_record(void)
{
	Block *bp;

	bp = recorded_blocks[pgrunidx++];
	
	if (pgrunidx >= PG_MAX_RECORD)
		pgrunidx = 0;

	return bp;
}

static void
pg_init_buf(void)
{
	int i;
	
	for ( i=0; i < PG_MAX_RECORD; i++) {
		if (recorded_blocks[i])
			free_bp(recorded_blocks[i]);
		recorded_blocks[i] = nil;
	}

	pgrecidx = 0;
	pgrunidx = 0;
	pgmode = PGinit;

}

static void
pg_set_mode(char *cmd, int)
{

	if (0 == cmd)
		return;

	print("cmd-> %s", cmd);
	if (0 == strncmp(cmd, "rec", 3) ){
		print("pgsetmode : rec");
		pgmode = PGrec;

	} else if (0 == strncmp(cmd, "start", 5)) {
		print("pgsetmode : start");
		pgmode = PGstart;
	} else if( 0 == strncmp(cmd, "stop", 4)) {
		print("pgsetmode : stop");
		pgmode = PGstop;
	} else {
		print("pgsetmode : ????");
		return;
	}

}

enum {
	Qdir,
	Qcmd,
};

static Dirtab pgdir[] = {
	".",		{ Qdir, 0, QTDIR },		0,	DMDIR|055,
	"packgencmd",	{ Qcmd },			0,	0777,
};

static void
pgreset(void)
{
	pg_init_buf();
}

static void
pginit(void)
{
	pg_init_buf();
}

static Chan*
pgattach(char *spec)
{
	return devattach('G', spec);	
}

static Walkqid*
pgwalk(Chan *c, Chan *nc, char **name, int nname)
{
	return devwalk(c, nc, name, nname, pgdir, nelem(pgdir), devgen);
}

static int
pgstat(Chan *c, uchar *dp, int n)
{
	return devstat(c, dp, n, pgdir, nelem(pgdir), devgen);
}

static Chan*
pgopen(Chan *c, int omode)
{
	if (!iseve()) {
		error(Eperm);
	}

	return devopen(c, omode, pgdir, nelem(pgdir), devgen);
}

static void
pgclose(Chan *c)
{
	if (c->aux) {
		free(c->aux);
		c->aux = nil;
	}

	return;
}

static long
pgread(Chan *c, void *va, long n, vlong)
{
	int rn = 0;
	switch ((ulong)c->qid.path) {
		case Qdir:
			return devdirread(c, va, n, pgdir, nelem(pgdir), devgen);
			break;
		case Qcmd:
			/*  XXX: show mode */
			/* pg_copy_mode(off, va, n); */
			break;
		default:
			print("pgread : default");
			break;
	}

	return rn;
}

static long
pgwrite(Chan *c, void *va, long n, vlong)
{
	int rn = 0;
	switch ((ulong)c->qid.path) {
		case Qdir:
			error(Eisdir);
			break;
		case Qcmd:
			/*  XXX: set mode */
			pg_set_mode((char *)va, n);
			rn = n;
			break;
		default:
			print("pgwrite : default %s", (char *)va);
			break;
	}

	return rn;
}

Dev packgendevtab = {
	'G',
	"packgen",
	pgreset,
	devinit,
	devshutdown,
	pgattach,
	pgwalk,
	pgstat,
	pgopen,
	devcreate,
	pgclose,
	pgread,
	devbread,
	pgwrite,
	devbwrite,
	devremove,
	devwstat,
};
