/*----------------------------------------------------------------------------
 *
 * replag.c
 *     Look for PostgreSQL replication lag.
 *
 *----------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <libpq-fe.h>

#include "config.h"
#include "pg10.h"
#include "pg9x.h"


static const char *dbport;
static const char *dbhost;
static const char *dbname;
static const char *dbuser;


static void usage();
static void version();
static void parse_options(int argc, char **argv);
static PGconn *connectdb(const char *dbhost, const char *dbport,
                         const char *dbuser, const char *dbname);
int get_stream_replication(PGconn *db, char **standbys);
char *get_current_wal_lsn(PGconn *db);

int get_standby_replication(const char *standbys, const char *lsn);


int
main(int argc, char **argv)
{
    int			 version;
    int			 res;
    char		*query;
    char		*curlsn;
    char		*standbys = NULL;
    PGconn		*db;

    parse_options(argc, argv);

    db = connectdb(dbhost, dbport, dbuser, dbname);
    if (db == NULL)
        return -1;

    version = PQserverVersion(db);
    printf("server version: %d, client version: %d\n", version, PG_VERSION_NUM);

    printf("\n[MASTER]: %s\n", dbhost);
    res = get_stream_replication(db, &standbys);
    if (res != 0)
        goto err;

    curlsn = get_current_wal_lsn(db);
    if (curlsn == NULL) {
        goto err;
    }
    printf("current %s: %s\n\n",
           version > 100000 ? "wal lsn" : "xlog location", curlsn);

    res = get_standby_replication(standbys, curlsn);
    if (res != 0)
        goto err;

    res = 0;

 err:
    PQfinish(db);

    return res;
}

static void
usage()
{
    printf("%s looks PostgreSQL replication lag\n"
           "\n"
           "Usage:\n"
           "  %s [option]...\n"
           "\n"
           "Options:\n"
           "  -?, --help               show this page and exit\n"
           "  -V, --version            show version and exit\n"
           "  -h, --host=HOSTNAME      database server host\n"
           "  -p, --port=PORT          database server port\n"
           "  -U, --username=USERNAME  database user name\n"
           "  -d, --dbname=DBNAME      database name to connect to\n"
           "\n"
           "Report bugs to <" PACKAGE_BUGREPORT ">.\n",
           PACKAGE_NAME, PACKAGE_NAME);
}

static void
version()
{
    printf(PACKAGE_NAME " version " PACKAGE_VERSION "\n");
}

static void
parse_options(int argc, char **argv)
{
    int            c, idx;
    const char    *short_opts = "d:h:p:U:V?";
    struct option  long_opts[] = {
        {"dbname",       required_argument, NULL, 'd'},
        {"host",         required_argument, NULL, 'h'},
        {"port",         required_argument, NULL, 'p'},
        {"username",     required_argument, NULL, 'U'},
        {"help",         no_argument,       NULL,  1 },
        {"version",      no_argument,       NULL, 'V'},
        { NULL,          0,                 NULL,  0 }
    };

    while ((c = getopt_long(argc, argv, short_opts, long_opts, &idx)) != -1) {
        switch (c) {
        case 'd':
            dbname = optarg;
            break;
        case 'h':
            dbhost = optarg;
            break;
        case 'p':
            dbport = optarg;
            break;
        case 'U':
            dbuser = optarg;
            break;
        case 'V':
            version();
            break;
        case 1:
            usage();
            exit(0);
        case '?':
            if (strcmp(argv[optind - 1], "-?") == 0) {
                usage();
                exit(0);
            }
            /* fall through */
        default:
            fprintf(stderr,
                    "Try \"%s --help\" for more information\n",
                    PACKAGE_NAME);
            exit(1);
        }
    }

    /* If there has more argument, they may be dbname and username. */
    while (argc - optind >= 1) {
        if (!dbname) {
            dbname = argv[optind];
        } else if (!dbuser) {
            dbuser = argv[optind];
        }
        optind++;
    }
}

static PGconn *
connectdb(const char *dbhost, const char *dbport,
          const char *dbuser, const char *dbname)
{
#define PG_CONNECT_KEY_SIZE    9
    PGconn       *db;
    const char   *keys[PG_CONNECT_KEY_SIZE];
    const char   *vals[PG_CONNECT_KEY_SIZE];

    keys[0] = "host";                      vals[0] = dbhost;
    keys[1] = "port";                      vals[1] = dbport;
    keys[2] = "user";                      vals[2] = dbuser;
    keys[3] = "password";                  vals[3] = NULL;
    keys[4] = "dbname";                    vals[4] = dbname;
    keys[5] = "client_encoding";           vals[5] = "auto";
    keys[6] = "connect_timeout";           vals[6] = "10";
    keys[7] = "fallback_application_name"; vals[7] = PACKAGE_NAME;
    keys[8] = NULL;                        vals[8] = NULL;

    db = PQconnectdbParams(keys, vals, 0);
    if (PQstatus(db) != CONNECTION_OK) {
        fprintf(stderr, "%s", PQerrorMessage(db));
        PQfinish(db);
        return NULL;
    }

    return db;
}

int
get_stream_replication(PGconn *db, char **standbys)
{
    int			i;
    int			ntuples;
    int			nfields;
    char	   *query;
    PGresult   *result;
    PQprintOpt  printopt = { 0 };

    if (PQserverVersion(db) >= 100000)
        query = PG10_STAT_REPLICATION;
    else
        query = PG9X_STAT_REPLICATION;

    result = PQexec(db, query);
    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        fprintf(stderr, "%s", PQresultErrorMessage(result));
        return -1;
    }

    ntuples = PQntuples(result);
    if (ntuples == 0) {
        printf("INFO: there is no stream replication\n");
        PQclear(result);
        return 1;
    }

    printopt.header = 1;
    printopt.align = 1;
    printopt.fieldSep = "|";
    PQprint(stdout, result, &printopt);

    if (standbys) {
        size_t	len = 1;
        size_t  pos = 0;
        char   *clients;
        clients = (char *) malloc(1);

        for (i = 0; i < ntuples; i++) {
            char   *addr = PQgetvalue(result, i, 0);
            size_t	addrlen = strlen(addr);
            len += addrlen + 1;
            clients = (char *) realloc(clients, len);
            if (clients == NULL) {
                fprintf(stderr, "ERROR: out of memory\n");
                exit(1);
            }

            strncpy(clients + pos, addr, addrlen);
            pos += addrlen;
            clients[pos++] = '\0';
        }
        clients[pos] = '\0';
        *standbys = clients;
    }

    PQclear(result);
    return 0;
}

char *
get_current_wal_lsn(PGconn *db)
{
    PGresult	*result;
    const char	*query;
    const char	*xlogname;

    if (PQserverVersion(db) >= 100000) {
        query = "SELECT pg_current_wal_lsn()";
        xlogname = "wal lsn";
    } else {
        query = "SELECT pg_current_xlog_location()";
        xlogname = "xlog location";
    }

    result = PQexec(db, query);
    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        fprintf(stderr, "ERROR: fetch current %s failed\n", xlogname);
        PQclear(result);
        return NULL;
    }

    return strdup(PQgetvalue(result, 0, 0));
}

int
get_standby_replication(const char *standbys, const char *lsn)
{
    char		*client = (char *) standbys;
    PQprintOpt   printopt = { 0 };

    printopt.header = 1;
    printopt.align = 1;
    printopt.fieldSep = "|";

    while (*client) {
        char		*query;
        char		*replsn;
        char		*lagsize;
        PGconn		*sdb;
        PGresult	*result;

        sdb = connectdb(client, dbport, dbuser, dbname);
        if (sdb == NULL) {
            printf("WARNING: cannot connect standby \"%s\"\n", client);
            continue;
        } else {
            printf("[STANDBY]: %s\n", client);
        }

        if (PQserverVersion(sdb) >= 100000)
            query = PG10_STANDBY_REPLICATION_HEALTH;
        else
            query = PG9X_STANDBY_REPLICATION_HEALTH;

        result = PQexec(sdb, query);
        if (PQresultStatus(result) != PGRES_TUPLES_OK) {
            fprintf(stderr, "%s", PQresultErrorMessage(result));
            PQclear(result);
            return -1;
        }


        replsn = PQgetvalue(result, 0, 3);
        PQprint(stdout, result, &printopt);

        /*
         * If there has a wal/xlog replay delay, calculate the wal/xlog
         * replay size.
         */
        if (strcmp(PQgetvalue(result, 0, 5), "0") != 0) {
            PGresult	*lsnres;
            char		 sql[1024] = { 0 };

            if (PQserverVersion(sdb) >= 100000)
                query = PG10_WAL_DELAY_SIZE;
            else
                query = PG9X_XLOG_DELAY_SIZE;

            snprintf(sql, sizeof(sql), query, lsn, replsn);

            lsnres = PQexec(sdb, sql);
            if (PQresultStatus(lsnres) != PGRES_TUPLES_OK) {
                fprintf(stderr, "%s", PQresultErrorMessage(lsnres));
            } else {
                PQprint(stdout, lsnres, &printopt);
            }
            PQclear(lsnres);
        }

        PQclear(result);
        PQfinish(sdb);

        client += strlen(client) + 1;
    }

    return 0;
}
