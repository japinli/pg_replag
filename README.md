# pg_replag

pg_replag is a tool for looking for PostgreSQL replication lag.

## Limits

* Only support the top stream replication, the cascade mode is not supported.
* Make sure that the master can connect the standby nodes to fetch information.

## Reference

[1] https://severalnines.com/database-blog/what-look-if-your-postgresql-replication-lagging
