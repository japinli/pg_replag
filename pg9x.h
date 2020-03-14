/*----------------------------------------------------------------------------
 *
 * pg9x.h
 *     This file define the SQL used before PostgreSQL 10.
 *
 *----------------------------------------------------------------------------
 */
#ifndef PG9X_H
#define PG9X_H

#define PG9X_STAT_REPLICATION			"\
SELECT \
    client_addr, \
    state, \
    sent_location, \
	write_location, \
    flush_location, \
    replay_location, \
    sync_state, \
    sync_priority \
FROM \
    pg_stat_replication \
;"

#define PG9X_STANDBY_REPLICATION_HEALTH		"\
SELECT \
    pg_is_in_recovery() AS recovery, \
    pg_is_xlog_replay_paused() AS replay_paused, \
    pg_last_xlog_receive_location() AS last_recv_lsn, \
    pg_last_xlog_replay_location() AS last_replay_lsn, \
    pg_last_xact_replay_timestamp() AS last_replay_ts, \
    CASE \
        WHEN pg_last_xlog_receive_location() = pg_last_xlog_replay_location() THEN 0 \
        ELSE EXTRACT(EPOCH FROM now() - pg_last_xact_replay_timestamp()) \
    END AS xlog_delay \
;"

#define PG9X_XLOG_DELAY_SIZE    \
    "SELECT pg_size_pretty(pg_xlog_location_diff('%s', '%s')) AS xlog_delay_size"

#endif    /* PG9X_H */
