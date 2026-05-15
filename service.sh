#!/system/bin/sh

# Magisk late_start service: monitors for Switch 2 Pro Controller (057e:2069)
# and runs enable (one-shot init) then rumble (persistent proxy).

MODDIR="${MODDIR:-/data/adb/modules/procon2droid}"
ENABLE_BIN="$MODDIR/system/bin/enable"
RUMBLE_BIN="$MODDIR/system/bin/rumble"
PIDFILE="$MODDIR/rumble.pid"
LOGFILE="$MODDIR/service.log"

log() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') $*" >> "$LOGFILE"
}

# Sanity checks
if [ ! -x "$ENABLE_BIN" ]; then
    log "ERROR: enable binary not found or not executable: $ENABLE_BIN"
    exit 1
fi

if [ ! -x "$RUMBLE_BIN" ]; then
    log "ERROR: rumble binary not found or not executable: $RUMBLE_BIN"
    exit 1
fi

# Ensure binaries are executable (in case module was pushed manually)
chmod +x "$ENABLE_BIN" "$RUMBLE_BIN"

# Clean up rumble if this script is killed
cleanup() {
    if [ -f "$PIDFILE" ]; then
        PID=$(cat "$PIDFILE")
        if kill -0 "$PID" 2>/dev/null; then
            log "Cleanup: stopping rumble ($PID)"
            kill "$PID"
        fi
        rm -f "$PIDFILE"
    fi
}
trap cleanup TERM INT EXIT

log "Service started. Monitoring for 057e:2069..."

while true; do
    CONTROLLER_PRESENT=false
    if command -v lsusb >/dev/null 2>&1; then
        if lsusb | grep -q "057e:2069"; then
            CONTROLLER_PRESENT=true
        fi
    fi

    if [ "$CONTROLLER_PRESENT" = true ]; then
        if [ ! -f "$PIDFILE" ] || ! kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
            log "Controller detected. Running enable + rumble..."

            "$ENABLE_BIN" >> "$LOGFILE" 2>&1
            ENABLE_STATUS=$?

            if [ $ENABLE_STATUS -ne 0 ]; then
                log "WARNING: enable exited with status $ENABLE_STATUS"
            fi

            "$RUMBLE_BIN" >> "$LOGFILE" 2>&1 &
            RUMBLE_PID=$!
            echo "$RUMBLE_PID" > "$PIDFILE"
            echo -1000 > /proc/$RUMBLE_PID/oom_score_adj
            log "Rumble started (pid=$RUMBLE_PID)"
        fi
    else
        if [ -f "$PIDFILE" ]; then
            PID=$(cat "$PIDFILE")
            if kill -0 "$PID" 2>/dev/null; then
                log "Controller removed. Stopping rumble (pid=$PID)..."
                kill "$PID"
            fi
            rm -f "$PIDFILE"
        fi
    fi

    sleep 5
done
