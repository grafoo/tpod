# tpod
the tiny podcatcher

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
    make \
    sqlite3
```

### opensuse
```
sudo zypper addrepo http://download.opensuse.org/repositories/devel:/libraries:/c_c++/openSUSE_Factory/devel:libraries:c_c++.repo # needed for libmrss
sudo zypper refresh
sudo zypper install \
    alsa-devel \
    gcc \
    libao-devel \
    libjansson-devel \
    libmrss-devel \
    make \
    scons # makes it easier to include non package mpg123 \
    sqlite3 # for interacting with the database \
    sqlite3-devel
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
- test flexbox for css ( https://developer.mozilla.org/en-US/docs/Web/CSS/CSS_Flexible_Box_Layout/Using_CSS_flexible_boxes )


# bugs
- when sending sigint while playback is running libao will leave the fd of alsa in a bad state
    ```
    ALSA lib pcm_dmix.c:1079:(snd_pcm_dmix_open) unable to open slave
    ao_alsa ERROR: Unable to open ALSA device 'default' for playback => File descriptor in bad state
    ```

# shoutcast/icecast

Icy-MetaData:1
e.g. curl -H "Icy-MetaData:1" http://neo.m2stream.fr:8000/m280-128.mp3 -D header

further reading on the protocol: http://www.smackfu.com/stuff/programming/shoutcast.html
