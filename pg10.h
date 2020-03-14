/*----------------------------------------------------------------------------
 *
 * pg10.h
 *     This file define the SQL used after PostgreSQL 10.
 *
 *----------------------------------------------------------------------------
 */
#ifndef PG10_H
#define PG10_H

#define PG10_STAT_REPLICATION                  "\
SELECT \
    client_addr, \
    state, \
    sent_lsn, \
    write_lsn, \
    flush_lsn, \
    replay_lsn, \
    sync_state, \
    sync_priority \
FROM \
    pg_stat_replication \
;"

#define PG10_STANDBY_REPLICATION_HEALTH      "\
SELECT \
    pg_is_in_recovery() AS recovery, \
    pg_is_wal_replay_paused() AS replay_paused, \
    pg_last_wal_receive_lsn() AS last_recv_lsn, \
    pg_last_wal_replay_lsn() AS last_replay_lsn, \
    pg_last_xact_replay_timestamp() AS last_replay_ts, \
    CASE \
        WHEN pg_last_wal_receive_lsn() = pg_last_wal_replay_lsn() THEN 0 \
        ELSE EXTRACT (EPOCH FROM now() - pg_last_xact_replay_timestamp()) \
    END AS wal_delay \
;"

#define PG10_WAL_DELAY_SIZE \
    "SELECT pg_size_pretty(pg_wal_lsn_diff('%s', '%s')) AS wal_delay_size"

#endif    /* PG10_H */
