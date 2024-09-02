import math

import psycopg.errors
import pytest
from psycopg.sql import SQL


@pytest.fixture(scope='session')
def docker_compose_file() -> str:
    return 'docker-compose.yml'


@pytest.fixture(scope='session')
def postgresql(docker_ip, docker_services):
    def _test_postgresql(dsn):
        try:
            psycopg.connect(dsn)
        except psycopg.DatabaseError:
            return False
        return True

    port = docker_services.port_for('db', 5432)
    host = docker_ip
    dsn = f'postgresql://postgres:test@{host}:{port}/postgres'

    docker_services.wait_until_responsive(
        timeout=60,
        pause=1,
        check=lambda: _test_postgresql(dsn),
    )

    return dsn


@pytest.fixture(scope='session')
def db(postgresql, docker_services):
    with psycopg.connect(postgresql, autocommit=True) as conn:
        yield conn


@pytest.fixture(scope='session', autouse=True)
def initialize_sql_function(db: psycopg.Connection) -> None:
    with open('angle_avg.sql') as f:
        sql = f.read()
    db.execute(sql)


@pytest.mark.parametrize(
    ('angles', 'expected'),
    (
        ([15], 15),
        ([360, 10], 5),
        ([10, 20, 30], 20),
        ([340, 10], 355),
        ([350, 10], 360),
        ([15, None], 15),
    ),
)
def test_calculate_avg_angle(angles, expected, db):
    values = [SQL('(%s)')] * len(angles)
    value_query = SQL(', ').join(values)
    query = SQL('SELECT avg_angle(val) FROM (VALUES {}) AS t(val)').format(
        value_query,
    )
    with db.cursor() as cur:
        cur.execute(query, angles)
        a, = cur.fetchone()

    assert a == pytest.approx(expected)


@pytest.mark.parametrize(
    'angles',
    (
        ([15, 0, float('nan')]),
        ([350, 10, float('nan')]),
    ),
)
def test_calculate_avg_angle_with_nans(angles, db):
    values = [SQL('(%s)')] * len(angles)
    value_query = SQL(', ').join(values)
    query = SQL('SELECT avg_angle(val) FROM (VALUES {}) AS t(val)').format(
        value_query,
    )
    with db.cursor() as cur:
        cur.execute(query, angles)
        a, = cur.fetchone()

    assert math.isnan(a)


def test_calculate_avg_angle_from_null(db):
    query = SQL('SELECT avg_angle(NULL)')
    with db.cursor() as cur:
        cur.execute(query)
        a, = cur.fetchone()

    assert a is None


def test_calculate_avg_angle_multiple_values_null(db):
    query = SQL('SELECT avg_angle(val) FROM (VALUES (NULL), (NULL)) AS t(val)')
    with db.cursor() as cur:
        with pytest.raises(psycopg.errors.UndefinedFunction) as exc:
            cur.execute(query)

    msg, = exc.value.args
    assert 'No function matches the given name and argument types' in msg


@pytest.mark.parametrize('angles', ([-10, 20, 30], [10, 20, 365]))
def test_calculate_avg_angle_invalid_angles(angles, db):
    values = [SQL('(%s)')] * len(angles)
    value_query = SQL(', ').join(values)
    query = SQL('SELECT avg_angle(val) FROM (VALUES {}) AS t(val)').format(
        value_query,
    )
    with db.cursor() as cur:
        with pytest.raises(psycopg.errors.InternalError) as exc:
            cur.execute(query, angles)

    msg, = exc.value.args
    assert 'Invalid value encountered' in msg
