#!/bin/bash -e
# Identify the currently logged-in user to push notifications to the correct screen
USER=$(w -h | grep -v "tmux" | head -n1 | awk '{print $1}')
USER_ID=$(id -u "$USER")

# Variable to track if the alert is currently active
ALERT_ACTIVE=0
# Infinite loop to constantly monitor the charging status
while true; do
    # Check the status of the ACAD power port (0 means disconnected/loose)
    if [ "$(cat /sys/class/power_supply/ACAD/online)" = "0" ]; then
        # Only trigger the alert if it isn't already showing
        if [ $ALERT_ACTIVE -eq 0 ]; then
            export XDG_RUNTIME_DIR=/run/user/$USER_ID
            
            # Generate the warning image using ffmpeg
            ffmpeg -f lavfi -i color=c=0x7f0000:s=1920x1080:d=1 \
            -vf \
            "drawtext=fontfile=/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf:text=\
            'WARNING\: CHARGER LOOSE!':fontcolor=white:fontsize=80:x=(w-text_w)/2:y=(h-text_h)/2" \
            -vframes 1 /tmp/warning.png -y

            # Open the warning image fullscreen via feh under the logged-in user
            feh --fullscreen /tmp/warning.png &
            FEH_PID=$!

            # Send the system critical notification
            sudo -u "$USER" DISPLAY=:0 \
                DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/$USER_ID/bus notify-send \
                -u critical "POWER CRITICAL" \
                "The charger has been disconnected or is loose! Please plug it back in."

            ALERT_ACTIVE=1
        fi
    else
        # If the charger is plugged back in (online = 1) and alert is active
        if [ $ALERT_ACTIVE -eq 1 ]; then
            # Automatically close the fullscreen image
            kill $FEH_PID 2>/dev/null
            ALERT_ACTIVE=0
        fi
    fi
    
    # Wait for 3 second before checking the status again
    sleep 3
done