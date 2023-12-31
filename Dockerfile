# Use an official Alpine Linux runtime as a parent image
FROM alpine:latest

# Set the working directory to /app
WORKDIR /app

# Install necessary dependencies
RUN apk --update --no-cache add \
    --repository=http://dl-cdn.alpinelinux.org/alpine/v3.19/community \
    build-base \
    cmake \
    git \
    libusb-dev \
    hidapi-dev \
    libsndfile-dev \
    gcc-arm-none-eabi \
    newlib-arm-none-eabi \
    g++-arm-none-eabi \
    doxygen \
    clang-extra-tools \
    python3

WORKDIR /workspaces
