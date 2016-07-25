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
    libsqlite0-dev \
    make
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
