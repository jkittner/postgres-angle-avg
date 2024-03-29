[![pre-commit.ci status](https://results.pre-commit.ci/badge/github/jkittner/postgres-angle-avg/master.svg)](https://results.pre-commit.ci/latest/github/jkittner/postgres-angle-avg/master)

# postgres-angle-avg

A C-Language function for PostgreSQL allowing to average angles in degree.

This can be useful for e.g. component averaging of wind directions (not taking the wind speed into account) directly in the database.

## Setup

### Compilation

This was tested against PostgreSQL 11, 12, 13 and 14. The function has to be compiled into a shared object.

On Linux:

compile the intermediate object file. The PostgreSQL development files (Header files) need to be present and included e.g. `postgresql-server-dev-12` on debian based OS.

```console
cc -fpic -c angle_avg.c -lm -I /usr/include/postgresql/14/server
```

create the shared object file

```console
cc -shared -o angle_avg.so angle_avg.o
```

For other platforms see [Compiling and Linking Dynamically-Loaded Functions](https://www.postgresql.org/docs/14/xfunc-c.html#DFUNC)

### SQL Function Creation

The `angle_avg.so` needs to be placed in a directory, which can be accessed by PostgreSQL e.g. `/usr/local/lib/funcs/angle_avg.so`.

Change the `angle_avg.sql` file so it matches the directory. Execute the `angle_avg.sql` file and now the function `avg_angle` is available.

### Example

```SQL
CREATE TABLE wind (wind_direction NUMERIC);
INSERT INTO wind VALUES (355), (15);
```

```SQL
SELECT wind_direction FROM wind;
```

```console
 wind_direction
----------------
            355
             15
(2 rows)
```

```SQL
SELECT avg_angle(wind_direction) FROM wind;
```

```console
     avg_angle
--------------------
 4.999999999999996
```
