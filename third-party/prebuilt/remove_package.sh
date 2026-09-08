# 1. Define your target path
export CUSTOM_TARGET_DIR="${LOCAL_MINOR_ROOT}"

if [ -z "$PKG_REMOVE" ]; then
    echo "Variable is empty!"
fi
if [ ! -f "$PKG_REMOVE" ]; then
    echo "PKG_REMOVE has a value and is not exist."
    exit 0
fi

export PACKAGE_NAME="{$PKG_REMOVE}""

# 2. Automatically delete every file and link listed inside the deb package
dpkg -c $PACKAGE_NAME | awk '{print $6}' | while read f; do
    if [ -f "$CUSTOM_TARGET_DIR/$f" ] || [ -L "$CUSTOM_TARGET_DIR/$f" ]; then
        rm "$CUSTOM_TARGET_DIR/$f"
    fi
done