export PATH=/home/bundito/projects/testing/bin:$PATH

# LD_LIBRARY_PATH only needed if you are building without rpath
# export LD_LIBRARY_PATH=/home/bundito/projects/testing/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH

export XDG_DATA_DIRS=/home/bundito/projects/testing/share:${XDG_DATA_DIRS:-/usr/local/share/:/usr/share/}
export XDG_CONFIG_DIRS=/home/bundito/projects/testing/etc/xdg:${XDG_CONFIG_DIRS:-/etc/xdg}

export QT_PLUGIN_PATH=/home/bundito/projects/testing/lib/x86_64-linux-gnu/plugins:$QT_PLUGIN_PATH
export QML2_IMPORT_PATH=/home/bundito/projects/testing/lib/x86_64-linux-gnu/qml:$QML2_IMPORT_PATH
