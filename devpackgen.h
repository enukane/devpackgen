
/*  packet record & gen */
#define	PG_MAX_RECORD	128
enum {
	PGinit	= 0,
	PGrecstop = 1,
	PGrec	= 2,
	PGstart	= 3,
	PGstop	= 4,
};

int pgmode = PGinit;
int pgrecidx = 0;
int pgrunidx = 0;

Block	*recorded_blocks[PG_MAX_RECORD];

void pg_record(Block *bp);
Block* pg_get_next_record(void);

