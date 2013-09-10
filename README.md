##ABOUT

RozoFS is a scale-out NAS file system. RozoFS aims to provide an open source high performance and high availibility scale out storage software appliance  for  intensive disk IO data center scenario. It comes as a free software, licensed under the GNU GPL v2. RozoFS provides an easy way to scale to petabytes storage but using erasure coding it was designed to provide very high availability levels with optimized raw capacity usage on heterogenous commodity hardwares.

RozoFS provide a native open source POSIX filesystem, build on top of a usual out-band scale-out storage architecture. The Rozo specificity lies in the way data is stored. The data to be stored is translated into several chunks named projections using Mojette Transform and distributed across storage devices in such a way that it can be retrieved even if several pieces are unavailable. On the other hand, chuncks are meaningless alone. Redundancy schemes based on coding techniques like the one used by RozoFS allow to achieve signiﬁcant storage savings as compared to simple replication.

**Note**: [xxx] means optional, \<xxx\> means requiered.

##INSTALLATION

###Requirements
* cmake
* libfuse-dev (>= 2.9.0)
* libattr1-dev
* uuid-dev
* libconfig-dev
* libreadline-dev
* python2.7-dev
* libpthread
* libcrypt
* swig

###Quick start
Using default values will compile rozofs in Release mode and install it on _/usr/local_.
```
mkdir build
cd build
cmake -G "Unix Makefiles" [-DCMAKE_INSTALL_PREFIX=/foo/bar -DCMAKE_BUILD_TYPE=Debug] ../
make
[sudo] make install
```
Options (CMAKE_INSTALL_PREFIX, CMAKE_BUILD_TYPE...) of generated build tree can be modified with:
```
make edit_cache
```
###Uninstall

```
make uninstall
```

##RUNNING

###Requirements
* a running portmap/rpcbind see portmap(8)/rpcbind(8)
* extend attributes on used file system for exportd see mount(8) and fstab(5)

###Start

1. install exportd on a host
2. install storaged on multiple hosts
3. fill in _export.conf_ ( _/etc/rozofs_ or _/usr/local/etc/rozofs_ according to CMAKE_INSTALL_PREFIX)
4. fill in all _storage.conf_
5. 
``
[sudo] exportd
``
6. 
``
[sudo] storaged
``
7. wherever you want: `` [sudo] rozofsmount -H <the_host_running_exportd> -E  <the export_path> -P <passwd> <the_mount_dir> ``

###Stop
1. ``[sudo] umount <_the_mount_dir_>``
2. ``[sudo] killall exportd``
3. ``[sudo] killall storaged``

##BUGS
See https://github.com/rozofs/rozofs/issues.
