# Base image: Ubuntu x86_64
FROM --platform=linux/amd64 ubuntu:22.04

ENV TZ=Asia/Seoul LANG=ko_KR.UTF-8 LANGUAGE=ko_KR.UTF-8

# Install dependencies
RUN export DEBIAN_FRONTEND=noninteractive && \
    apt-get update \
    # && echo "tzdata tzdata/Areas select Asia" | debconf-set-selections \
    # && echo "tzdata tzdata/Zones/Asia select Seoul" | debconf-set-selections \
    && apt-get install -y \
    locales tzdata \
    build-essential \
    gcc \
    gdb \
    vim \
    git \
    sudo \
    qemu-system-x86 \
    python3 \
    perl \
    tmux \
    curl \
    wget \
    bsdextrautils \
    && locale-gen ko_KR.UTF-8 && update-locale LANG=ko_KR.UTF-8 \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m -s /bin/bash jungle && usermod -aG sudo jungle
RUN echo "jungle ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/jungle

USER jungle
WORKDIR /home/jungle

# Automatically source pintos activate script if available in workspace
RUN echo 'for act in /workspaces/pintos_22.04_lab_docker/pintos/activate /workspaces/JungleWeek07/pintos/activate /workspace/pintos/activate; do if [ -f "$$act" ]; then source "$$act"; break; fi; done' >> /home/jungle/.bashrc