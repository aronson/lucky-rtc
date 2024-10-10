# Alpine is slim
FROM docker.io/library/alpine

# Set environment variables
ENV WONDERFUL_TOOLCHAIN=/opt/wonderful
ENV PATH=/opt/wonderful/bin:$PATH

# Install required packages
RUN apk add --no-cache \
    bash \
    python3 \
    git \
    make \
    ca-certificates \
    curl

# Create the /opt/wonderful directory
RUN mkdir -p /opt/wonderful

# Fail next command if pipe fails
SHELL ["/bin/bash", "-o", "pipefail", "-c"]

# Download and extract the bootstrap
RUN arch=$(arch) && curl -L https://wonderful.asie.pl/bootstrap/wf-bootstrap-$arch.tar.gz | tar xzvf - -C /opt/wonderful

# Synchronize and update wf's package manager
RUN wf-pacman -Syu
RUN wf-pacman -S --noconfirm target-gba thirdparty-blocksds-toolchain

# Set the default command
CMD ["/bin/bash"]
