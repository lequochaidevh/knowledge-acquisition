# Use Ubuntu 22.04 as the base image
FROM ubuntu:22.04

# Declare the build-time argument (MUST be before COPY)
ARG PROJECT_ASSET

# Set environment variables for non-interactive installation
ENV DEBIAN_FRONTEND=noninteractive

# Update and install basic dependencies for GUI, OpenGL, EGL, and Qt
RUN apt-get update && apt-get install -y \
    libx11-6 \
    libxext-dev \
    libxrender-dev \
    libxtst-dev \
    libxi-dev \
    x11-apps \
    libgl1-mesa-glx \
    libgl1-mesa-dri \
    libxkbcommon0 \
    libegl1-mesa \
    libqt5core5a \
    libqt5gui5 \
    libqt5widgets5 \
    libqt5dbus5 \
    libqt5network5 \
    libgl1-mesa-dev \
    libxcb-* \
    libxkb* \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Set up environment for GUI
ENV DISPLAY=:0
RUN mkdir -p /root/.Xauthority

# Copy the entire yololabel directory into the container
COPY $PROJECT_ASSET /workspace/builder
WORKDIR /workspace
