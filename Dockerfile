ARG VERSION=16
FROM postgres:${VERSION}-bullseye

ARG VERSION=16

RUN : \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        build-essential \
        gcc \
        postgresql-server-dev-$VERSION \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /usr/local/lib/funcs

COPY ./angle_avg.c .
COPY ./angle_avg.sql .

RUN : \
    && cc -fPIC -Werror -Wall -c angle_avg.c -lm -I /usr/include/postgresql/${VERSION}/server \
    && cc -shared -o angle_avg.so angle_avg.o
