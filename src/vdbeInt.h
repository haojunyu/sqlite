/*
** 2003 September 6
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This is the header file for information that is private to the
** VDBE.  This information used to all be at the top of the single
** source code file "vdbe.c".  When that file became too big (over
** 6000 lines long) it was split up into several smaller files and
** this header information was factored out.
*/

/*
** intToKey() and keyToInt() used to transform the rowid.  But with
** the latest versions of the design they are no-ops.
*/
#define keyToInt(X)   (X)
#define intToKey(X)   (X)

/*
** The makefile scans the vdbe.c source file and creates the following
** array of string constants which are the names of all VDBE opcodes.  This
** array is defined in a separate source code file named opcode.c which is
** automatically generated by the makefile.
*/
extern char *sqlite3OpcodeNames[];

/*
** SQL is translated into a sequence of instructions to be
** executed by a virtual machine.  Each instruction is an instance
** of the following structure.
*/
typedef struct VdbeOp Op;

/*
** Boolean values
*/
typedef unsigned char Bool;

/*
** A cursor is a pointer into a single BTree within a database file.
** The cursor can seek to a BTree entry with a particular key, or
** loop over all entries of the Btree.  You can also insert new BTree
** entries or retrieve the key or data from the entry that the cursor
** is currently pointing to.
** 
** Every cursor that the virtual machine has open is represented by an
** instance of the following structure.
**
** If the Cursor.isTriggerRow flag is set it means that this cursor is
** really a single row that represents the NEW or OLD pseudo-table of
** a row trigger.  The data for the row is stored in Cursor.pData and
** the rowid is in Cursor.iKey.
*/
struct Cursor {
  BtCursor *pCursor;    /* The cursor structure of the backend */
  i64 lastRecno;        /* Last recno from a Next or NextIdx operation */
  i64 nextRowid;        /* Next rowid returned by OP_NewRowid */
  Bool recnoIsValid;    /* True if lastRecno is valid */
  Bool keyAsData;       /* The OP_Column command works on key instead of data */
  Bool atFirst;         /* True if pointing to first entry */
  Bool useRandomRowid;  /* Generate new record numbers semi-randomly */
  Bool nullRow;         /* True if pointing to a row with no data */
  Bool nextRowidValid;  /* True if the nextRowid field is valid */
  Bool pseudoTable;     /* This is a NEW or OLD pseudo-tables of a trigger */
  Bool deferredMoveto;  /* A call to sqlite3BtreeMoveto() is needed */
  Bool intKey;          /* True if the table requires integer keys */
  Bool zeroData;        /* True if table contains keys only - no data */
  u8 bogusIncrKey;      /* Something for pIncrKey to point to if pKeyInfo==0 */
  i64 movetoTarget;     /* Argument to the deferred sqlite3BtreeMoveto() */
  Btree *pBt;           /* Separate file holding temporary table */
  int nData;            /* Number of bytes in pData */
  char *pData;          /* Data for a NEW or OLD pseudo-table */
  i64 iKey;             /* Key for the NEW or OLD pseudo-table row */
  u8 *pIncrKey;         /* Pointer to pKeyInfo->incrKey */
  KeyInfo *pKeyInfo;    /* Info about index keys needed by index cursors */
  int nField;           /* Number of fields in the header */

  /* Cached information about the header for the data record that the
  ** cursor is currently pointing to */
  Bool cacheValid;      /* True if the cache is valid */
  int payloadSize;      /* Total number of bytes in the record */
  u32 *aType;           /* Type values for all entries in the record */
  u32 *aOffset;         /* Cached offsets to the start of each columns data */
};
typedef struct Cursor Cursor;

/*
** A sorter builds a list of elements to be sorted.  Each element of
** the list is an instance of the following structure.
*/
typedef struct Sorter Sorter;
struct Sorter {
  int nKey;           /* Number of bytes in the key */
  char *zKey;         /* The key by which we will sort */
  int nData;          /* Number of bytes in the data */
  char *pData;        /* The data associated with this key */
  Sorter *pNext;      /* Next in the list */
};

/* 
** Number of buckets used for merge-sort.  
*/
#define NSORT 30

/*
** Number of bytes of string storage space available to each stack
** layer without having to malloc.  NBFS is short for Number of Bytes
** For Strings.
*/
#define NBFS 32

/*
** Internally, the vdbe manipulates nearly all SQL values as Mem
** structures. Each Mem struct may cache multiple representations (string,
** integer etc.) of the same value.  A value (and therefore Mem structure)
** has the following properties:
**
** Each value has a manifest type. The manifest type of the value stored
** in a Mem struct is returned by the MemType(Mem*) macro. The type is
** one of SQLITE_NULL, SQLITE_INTEGER, SQLITE_REAL, SQLITE_TEXT or
** SQLITE_BLOB.
*/
struct Mem {
  i64 i;              /* Integer value */
  int n;              /* Number of characters in string value, including '\0' */
  u16 flags;          /* Some combination of MEM_Null, MEM_Str, MEM_Dyn, etc. */
  u8  type;           /* One of MEM_Null, MEM_Str, etc. */
  u8  enc;            /* TEXT_Utf8, TEXT_Utf16le, or TEXT_Utf16be */
  double r;           /* Real value */
  char *z;            /* String or BLOB value */
  char zShort[NBFS];  /* Space for short strings */
};
typedef struct Mem Mem;

/* One or more of the following flags are set to indicate the validOK
** representations of the value stored in the Mem struct.
**
** If the MEM_Null flag is set, then the value is an SQL NULL value.
** No other flags may be set in this case.
**
** If the MEM_Str flag is set then Mem.z points at a string representation.
** Usually this is encoded in the same unicode encoding as the main
** database (see below for exceptions). If the MEM_Term flag is also
** set, then the string is nul terminated. The MEM_Int and MEM_Real 
** flags may coexist with the MEM_Str flag.
**
** Multiple of these values can appear in Mem.flags.  But only one
** at a time can appear in Mem.type.
*/
#define MEM_Null      0x0001   /* Value is NULL */
#define MEM_Str       0x0002   /* Value is a string */
#define MEM_Int       0x0004   /* Value is an integer */
#define MEM_Real      0x0008   /* Value is a real number */
#define MEM_Blob      0x0010   /* Value is a BLOB */

/* Whenever Mem contains a valid string or blob representation, one of
** the following flags must be set to determine the memory management
** policy for Mem.z.  The MEM_Term flag tells us whether or not the
** string is \000 or \u0000 terminated
*/
#define MEM_Term      0x0020   /* String rep is nul terminated */
#define MEM_Dyn       0x0040   /* Need to call sqliteFree() on Mem.z */
#define MEM_Static    0x0080   /* Mem.z points to a static string */
#define MEM_Ephem     0x0100   /* Mem.z points to an ephemeral string */
#define MEM_Short     0x0200   /* Mem.z points to Mem.zShort */

/* The following MEM_ value appears only in AggElem.aMem.s.flag fields.
** It indicates that the corresponding AggElem.aMem.z points to a
** aggregate function context that needs to be finalized.
*/
#define MEM_AggCtx    0x0400  /* Mem.z points to an agg function context */

struct VdbeFunc {
  FuncDef *pFunc;
  int nAux;
  struct AuxData {
    void *pAux;
    void (*xDelete)(void *);
  } apAux[0];
};
typedef struct VdbeFunc VdbeFunc;

/*
** The "context" argument for a installable function.  A pointer to an
** instance of this structure is the first argument to the routines used
** implement the SQL functions.
**
** There is a typedef for this structure in sqlite.h.  So all routines,
** even the public interface to SQLite, can use a pointer to this structure.
** But this file is the only place where the internal details of this
** structure are known.
**
** This structure is defined inside of vdbe.c because it uses substructures
** (Mem) which are only defined there.
*/
struct sqlite3_context {
  FuncDef *pFunc;   /* Pointer to function information.  MUST BE FIRST */
  VdbeFunc *pVdbeFunc;  /* Auxilary data, if created. */
  Mem s;            /* The return value is stored here */
  void *pAgg;       /* Aggregate context */
  u8 isError;       /* Set to true for an error */
  u8 isStep;        /* Current in the step function */
  int cnt;          /* Number of times that the step function has been called */
};

/*
** An Agg structure describes an Aggregator.  Each Agg consists of
** zero or more Aggregator elements (AggElem).  Each AggElem contains
** a key and one or more values.  The values are used in processing
** aggregate functions in a SELECT.  The key is used to implement
** the GROUP BY clause of a select.
*/
typedef struct Agg Agg;
typedef struct AggElem AggElem;
struct Agg {
  int nMem;            /* Number of values stored in each AggElem */
  AggElem *pCurrent;   /* The AggElem currently in focus */
  HashElem *pSearch;   /* The hash element for pCurrent */
  Hash hash;           /* Hash table of all aggregate elements */
  FuncDef **apFunc;    /* Information about aggregate functions */
};
struct AggElem {
  char *zKey;          /* The key to this AggElem */
  int nKey;            /* Number of bytes in the key, including '\0' at end */
  Mem aMem[1];         /* The values for this AggElem */
};

/*
** A Set structure is used for quick testing to see if a value
** is part of a small set.  Sets are used to implement code like
** this:
**            x.y IN ('hi','hoo','hum')
*/
typedef struct Set Set;
struct Set {
  Hash hash;             /* A set is just a hash table */
  HashElem *prev;        /* Previously accessed hash elemen */
};

/*
** A Keylist is a bunch of keys into a table.  The keylist can
** grow without bound.  The keylist stores the ROWIDs of database
** records that need to be deleted or updated.
*/
typedef struct Keylist Keylist;
struct Keylist {
  int nKey;         /* Number of slots in aKey[] */
  int nUsed;        /* Next unwritten slot in aKey[] */
  int nRead;        /* Next unread slot in aKey[] */
  Keylist *pNext;   /* Next block of keys */
  i64 aKey[1];      /* One or more keys.  Extra space allocated as needed */
};

/*
** A Context stores the last insert rowid, the last statement change count,
** and the current statement change count (i.e. changes since last statement).
** Elements of Context structure type make up the ContextStack, which is
** updated by the ContextPush and ContextPop opcodes (used by triggers)
*/
typedef struct Context Context;
struct Context {
  int lastRowid;    /* Last insert rowid (from db->lastRowid) */
  int lsChange;     /* Last statement change count (from db->lsChange) */
  int csChange;     /* Current statement change count (from db->csChange) */
};

/*
** An instance of the virtual machine.  This structure contains the complete
** state of the virtual machine.
**
** The "sqlite3_stmt" structure pointer that is returned by sqlite3_compile()
** is really a pointer to an instance of this structure.
*/
struct Vdbe {
  sqlite *db;         /* The whole database */
  Vdbe *pPrev,*pNext; /* Linked list of VDBEs with the same Vdbe.db */
  FILE *trace;        /* Write an execution trace here, if not NULL */
  int nOp;            /* Number of instructions in the program */
  int nOpAlloc;       /* Number of slots allocated for aOp[] */
  Op *aOp;            /* Space to hold the virtual machine's program */
  int nLabel;         /* Number of labels used */
  int nLabelAlloc;    /* Number of slots allocated in aLabel[] */
  int *aLabel;        /* Space to hold the labels */
  Mem *aStack;        /* The operand stack, except string values */
  Mem *pTos;          /* Top entry in the operand stack */
  Mem **apArg;        /* Arguments to currently executing user function */
  Mem *aColName;      /* Column names to return */
  char **azColName;   /* Becomes the 4th parameter to callbacks */
  void **azColName16; /* UTF-16 encoded equivalent of azColName */
  int nCursor;        /* Number of slots in apCsr[] */
  Cursor **apCsr;     /* One element of this array for each open cursor */
  Sorter *pSort;      /* A linked list of objects to be sorted */
  FILE *pFile;        /* At most one open file handler */
  int nField;         /* Number of file fields */
  char **azField;     /* Data for each file field */
  int nVar;           /* Number of entries in apVar[] */
  Mem *apVar;         /* Values for the OP_Variable opcode. */
  char *zLine;            /* A single line from the input file */
  int nLineAlloc;         /* Number of spaces allocated for zLine */
  int magic;              /* Magic number for sanity checking */
  int nMem;               /* Number of memory locations currently allocated */
  Mem *aMem;              /* The memory locations */
  Agg agg;                /* Aggregate information */
  int nCallback;          /* Number of callbacks invoked so far */
  Keylist *pList;         /* A list of ROWIDs */
  int keylistStackDepth;  /* The size of the "keylist" stack */
  Keylist **keylistStack; /* The stack used by opcodes ListPush & ListPop */
  int contextStackDepth;  /* The size of the "context" stack */
  Context *contextStack;  /* Stack used by opcodes ContextPush & ContextPop*/
  int pc;                 /* The program counter */
  int rc;                 /* Value to return */
  unsigned uniqueCnt;     /* Used by OP_MakeRecord when P2!=0 */
  int errorAction;        /* Recovery action to do in case of an error */
  int inTempTrans;        /* True if temp database is transactioned */
  int returnStack[100];   /* Return address stack for OP_Gosub & OP_Return */
  int returnDepth;        /* Next unused element in returnStack[] */
  int nResColumn;         /* Number of columns in one row of the result set */
  char **azResColumn;     /* Values for one row of result */ 
  int popStack;           /* Pop the stack this much on entry to VdbeExec() */
  char *zErrMsg;          /* Error message written here */
  u8 resOnStack;          /* True if there are result values on the stack */
  u8 explain;             /* True if EXPLAIN present on SQL command */
  u8 autoCommitOn;        /* True if autocommit got turned on by this program */
};

/*
** The following are allowed values for Vdbe.magic
*/
#define VDBE_MAGIC_INIT     0x26bceaa5    /* Building a VDBE program */
#define VDBE_MAGIC_RUN      0xbdf20da3    /* VDBE is ready to execute */
#define VDBE_MAGIC_HALT     0x519c2973    /* VDBE has completed execution */
#define VDBE_MAGIC_DEAD     0xb606c3c8    /* The VDBE has been deallocated */

/*
** Function prototypes
*/
void sqlite3VdbeCleanupCursor(Cursor*);
void sqlite3VdbeSorterReset(Vdbe*);
void sqlite3VdbeAggReset(Agg*);
void sqlite3VdbeKeylistFree(Keylist*);
void sqliteVdbePopStack(Vdbe*,int);
int sqlite3VdbeCursorMoveto(Cursor*);
#if !defined(NDEBUG) || defined(VDBE_PROFILE)
void sqlite3VdbePrintOp(FILE*, int, Op*);
#endif
int sqlite3VdbeSerialTypeLen(u32);
u32 sqlite3VdbeSerialType(Mem*);
int sqlite3VdbeSerialPut(unsigned char*, Mem*);
int sqlite3VdbeSerialGet(const unsigned char*, u32, Mem*);

int sqlite2BtreeKeyCompare(BtCursor *, const void *, int, int, int *);
int sqlite3VdbeIdxKeyCompare(Cursor*, int , const unsigned char*, int*);
int sqlite3VdbeIdxRowid(BtCursor *, i64 *);
int sqlite3MemCompare(const Mem*, const Mem*, const CollSeq*);
int sqlite3VdbeRecordCompare(void*,int,const void*,int, const void*);
int sqlite3VdbeIdxRowidLen(int,const u8*);
int sqlite3VdbeExec(Vdbe*);
int sqlite3VdbeList(Vdbe*);
int sqlite3VdbeChangeEncoding(Mem *, int);
int sqlite3VdbeMemCopy(Mem*, const Mem*);
int sqlite3VdbeMemNulTerminate(Mem*);
int sqlite3VdbeMemSetStr(Mem*, const char*, int, u8, int);
void sqlite3VdbeMemSetInt64(Mem*, long long int);
void sqlite3VdbeMemSetDouble(Mem*, double);
void sqlite3VdbeMemSetNull(Mem*);
int sqlite3VdbeMemMakeWriteable(Mem*);
int sqlite3VdbeMemDynamicify(Mem*);
int sqlite3VdbeMemStringify(Mem*, int);
int sqlite3VdbeMemIntegerify(Mem*);
int sqlite3VdbeMemRealify(Mem*);
int sqlite3VdbeMemFromBtree(BtCursor*,int,int,int,Mem*);
#ifndef NDEBUG
void sqlite3VdbeMemSanity(Mem*, u8);
#endif
