# tpod
a tiny podcast fetcher/player

# dependencies
- libao
- libcurl
- mpg123
- mongoose-6.5
- libnxml
- libmrss

## toolchain
stuff needed to build tpod

### void linux
```
sudo xbps-install -S \
    gcc \
    libao-devel \
    libcurl-devel \
    libmrss-devel \
    libnxml-devel \
    make \
    mpg123-devel \
    sqlite-devel
```

### debian
```
sudo apt-get install \
    gcc \
    libao-dev \
    libcurl4-nss-dev \
    libjansson-dev \
    libmpg123-dev \
    libmrss0-dev \
    libsqlite3-dev \
    make
```

### opensuse
```
sudo zypper addrepo http://download.opensuse.org/repositories/devel:/libraries:/c_c++/openSUSE_Factory/devel:libraries:c_c++.repo # needed for libmrss
sudo zypper refresh
sudo zypper install libmrss-devel \
    alsa-devel \
    libao-devel \
    sqlite3-devel \
    libjansson-devel \
    sqlite3 # for interacting with the database \
    scons # makes it easier to include non package mpg123
```
it seems that there is now standard package for mpg123, so this needs to be built manually.
the source can be found on e.g. `https://www.mpg123.de/download/mpg123-1.23.6.tar.bz2`
configure it with so targets will be installed in ./dep

start with non default install path of libs:
```
LD_LIBRARY_PATH=./dep/lib64/ ./tpod -s
```

# todo
- investigate why mongoose-devel won't work
- update debian dependencies

# bugs
- when sending sigint while playback is running libao will leave the fd of alsa in a bad state
    ```
    ALSA lib pcm_dmix.c:1079:(snd_pcm_dmix_open) unable to open slave
    ao_alsa ERROR: Unable to open ALSA device 'default' for playback => File descriptor in bad state
    ```
sudo zypper install gcc make
