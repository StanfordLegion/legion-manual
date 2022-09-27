# Build image for C++

FROM ubuntu:22.04

MAINTAINER Elliott Slaughter <slaughter@cs.stanford.edu>

ARG LEGION_BRANCH=stable

ENV DEBIAN_FRONTEND noninteractive

COPY . /build

RUN apt-get update -qq && \
    apt-get install -qq build-essential cmake git python3 zlib1g-dev && \
    apt-get clean

RUN cd /build && \
    ./docker/test_cxx.sh $LEGION_BRANCH
