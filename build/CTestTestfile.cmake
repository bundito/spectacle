# CMake generated Testfile for 
# Source directory: /home/bundito/projects/spectacle
# Build directory: /home/bundito/projects/spectacle/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(appstreamtest "/usr/bin/cmake" "-DAPPSTREAMCLI=/usr/bin/appstreamcli" "-DINSTALL_FILES=/home/bundito/projects/spectacle/build/install_manifest.txt" "-P" "/usr/share/ECM/kde-modules/appstreamtest.cmake")
subdirs(src)
subdirs(dbus)
subdirs(desktop)
subdirs(icons)
subdirs(doc)
