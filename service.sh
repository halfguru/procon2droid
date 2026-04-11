#!/system/bin/sh

# wait for the boot
while [ "$(getprop sys.boot_completed)" != "1" ]; do
    sleep 5
done

MODDIR="/data/adb/modules/procon2droid"
ENABLE_BIN="$MODDIR/system/bin/enable"
RUMBLE_BIN="$MODDIR/system/bin/rumble"

# make binaries executable
chmod +x "$ENABLE_BIN" "$RUMBLE_BIN"

while true; do
    if lsusb | grep -q "057e:2069"; then

        # no rumble, controller was plugged in
        if ! pgrep -f "$RUMBLE_BIN" > /dev/null; then
            echo "Pro Controller Detected: Enabling features..."
            "$ENABLE_BIN"
            "$RUMBLE_BIN" &
            echo -1000 > /proc/$!/oom_score_adj
        fi
    else
        # cleanup
        if pgrep -f "$RUMBLE_BIN" > /dev/null; then
            echo "Pro Controller Removed: Cleaning up..."
            pkill -f "$RUMBLE_BIN"
        fi
    fi

    sleep 5
done
