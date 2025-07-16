#!/bin/echo docker build . -f
# -*- coding: utf-8 -*-
# SPDX-License-Identifier: Apache-2.0

ARG OS_NAME=debian
ARG OS_VERSION_CODENAME=bookworm

FROM ${OS_NAME}:${OS_VERSION_CODENAME} AS builder

RUN echo "# log: Setup system" \
   && set -x \
   && apt-get update -y \
   && apt-get install -y --no-install-recommends sudo make \
   && date -u

ENV project=z-wave-nvm-migration-tool
ARG workdir=/usr/local/opt/${project}
WORKDIR ${workdir}
ARG HELPER="./helper.mk"

COPY ${HELPER} ${workdir}

RUN echo "# log: Setup ${project}" \
  && set -x \
  && ${HELPER} setup \
  && date -u

COPY . ${workdir}

RUN echo "# log: Build ${project}" \
  && set -x \
  && ${HELPER} \
  && date -u

FROM debian:bookworm
ENV project=z-wave-nvm-migration-tool
ARG workdir=/usr/local/opt/${project}
WORKDIR ${workdir}
COPY --from=builder ${workdir}/dist/ ${workdir}/dist/

RUN echo "# log: Install to system" \
  && set -x  \
  && apt-get update \
  && dpkg -i ./dist/*.deb \
  || apt install -f -y --no-install-recommends \
  && dpkg -L ${project} \
  && ${project} \
  && rm -rf dist \
  && apt-get clean -y \
  && rm -rf /var/lib/{apt,dpkg,cache,log}/ \
  && df -h \
  && date -u


ENTRYPOINT [ "/usr/bin/z-wave-nvm-migration-tool" ]
CMD [ "--help" ]

