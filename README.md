# tpod
a tiny podcast fetcher/player

# dependencies
- libao
- libcurl
- mpg123

## toolchain
stuff needed to build tpod

### void linux
sudo xbps-install -S \
    gcc \
    libao-devel \
    libcurl-devel \
    make \
    mpg123-devel

### debian
sudo apt-get install \
    gcc \
    libao-dev \
    libcurl4-nss-dev
    libmpg123-dev \
    make \
