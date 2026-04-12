FROM ubuntu:20.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive
RUN apt update &&apt install -y \
build-essential \
g++ \
libc6-dev \
&& rm -rf /var/lib/apt/lists/*
WORKDIR /build
COPY client.cpp ./
COPY common.h ./
COPY server.cpp ./
RUN g++ -o tcp-server server.cpp -lpthread -static
RUN g++ -o tcp-client client.cpp -lpthread -static
FROM alpine:latest
RUN apk add --no-cache libstdc++
WORKDIR /app
COPY --from=builder /build/tcp-server .
COPY --from=builder /build/tcp-client .
EXPOSE 8888
CMD ["./tcp-server"]

