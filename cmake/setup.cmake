set (WDIR "" CACHE FORCE "Working Directory")
set (PREFIX "" CACHE FORCE "Prefix")
set (SETUP "" CACHE FORCE "Path to setup.py")
set (DEBIAN "" CACHE FORCE "Debian layout")
set (MANIFEST "" CACHE FORCE "Manifest file")

set (PYTHON "" CACHE FORCE "python exec")

set (INSTALL_ROOT $ENV{DESTDIR})
set (EXTRA_ARGS "")

if (INSTALL_ROOT)
    set (INSTALL_ROOT_ARGS "--root=$ENV{DESTDIR}")
    if (DEBIAN)
        set (EXTRA_ARGS "--install-layout=deb")
    endif (DEBIAN)
    #set (EXTRA_ARGS "--install-layout=deb")
else (INSTALL_ROOT)
    set (INSTALL_ROOT_ARGS "")
endif (INSTALL_ROOT)

#cmake_policy (SET CMP0012 NEW)

execute_process (COMMAND python ${SETUP} install ${EXTRA_ARGS} --prefix=${PREFIX} --record=${MANIFEST} ${INSTALL_ROOT_ARGS} WORKING_DIRECTORY ${WDIR})
