CREATE OR REPLACE FUNCTION complex_angle_accum(float8[], DOUBLE PRECISION)
RETURNS float8[]
     AS '$libdir/angle_avg', 'complex_angle_accum'
     LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION complex_angle_avg(float8[])
RETURNS DOUBLE PRECISION
     AS '$libdir/angle_avg', 'complex_angle_avg'
     LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION complex_angle_combine(float8[], float8[])
RETURNS float8[]
     AS '$libdir/angle_avg', 'complex_angle_combine'
     LANGUAGE C STRICT;

CREATE OR REPLACE AGGREGATE avg_angle(DOUBLE PRECISION)
(
    SFUNC = complex_angle_accum,
    STYPE = float8[],
    FINALFUNC = complex_angle_avg,
    initcond = '{0,0,0}',
    COMBINEFUNC = complex_angle_combine,
    PARALLEL = SAFE
);
