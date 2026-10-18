#!/bin/sh
# Biko3 native runtime launcher (installed as ~/.local/bin/biko3).
#
# InputPlumber's default KONKR profile exposes the built-in controller as a
# UHID "Valve Steam Deck Controller". That node only appears as /dev/hidraw
# unless the kernel hid-steam driver is bound, so SDL cannot use it. Ask
# InputPlumber for the normal Xbox uinput target instead; the game then sees a
# standard SDL gamepad. Failure is non-fatal (USB/Bluetooth pads still work).
set -u

ROOT="${BIKO3_PREFIX:-${HOME}/.local/share/biko3}"
GAME="${BIKO3_GAME:-${ROOT}/game}"

if command -v busctl >/dev/null 2>&1; then
  busctl --system call org.shadowblip.InputPlumber \
    /org/shadowblip/InputPlumber/CompositeDevice0 \
    org.shadowblip.Input.CompositeDevice SetTargetDevices \
    as 3 xbox-elite keyboard mouse >/dev/null 2>&1 || true
  # Let the virtual input node appear before SDL enumerates controllers.
  sleep 0.4
fi

exec "${ROOT}/biko3" "${GAME}"
