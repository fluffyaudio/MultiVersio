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
    doxygen

WORKDIR /workspaces

# Clone the libDaisy repository
RUN git clone --recursive https://github.com/electro-smith/libDaisy.git libdaisy

# Set the working directory to the libDaisy folder
WORKDIR /workspaces/libdaisy

# Build libDaisy
RUN make

WORKDIR /workspaces

# Clone the DaisySP repository
RUN git clone --recursive https://github.com/electro-smith/DaisySP.git

WORKDIR /workspaces/DaisySP

RUN make

WORKDIR /workspaces

RUN git clone --recursive https://github.com/pichenettes/stmlib.git
