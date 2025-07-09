#!/bin/bash

# Install Dependencies for g++-14 Plugin Development

# Check if running as root
if [ "$EUID" -ne 0 ]; then
    echo "Please run this script as root or with sudo"
    exit 1
fi

# Update package lists
echo "Updating package lists..."
apt-get update -qq

# Install GCC 14 and related packages
echo "Installing GCC 14 and development tools..."
apt-get install -y \
    gcc-14 \
    g++-14 \
    gcc-14-plugin-dev \
    make \
    cmake \
    build-essential \
    binutils-dev \
    libgmp-dev \
    libmpfr-dev \
    libmpc-dev \
    flex \
    bison \
    libisl-dev \
    git

# Set g++-14 as default (optional)
echo "Setting g++-14 as default..."
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-14 100 --slave /usr/bin/g++ g++ /usr/bin/g++-14

# Verify installation
echo "Verifying installation..."
g++-14 --version
gcc-plugin --version

echo ""
echo "Installation complete!"
echo "You can now develop GCC plugins with g++-14"
