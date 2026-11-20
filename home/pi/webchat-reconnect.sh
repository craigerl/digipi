#!/bin/bash
# Reconnect WebChat once this radio service's Direwolf owns KISS port 8001.
# aprsd does not reopen a KISS socket after Direwolf is replaced.
# Exit 0 always so this hook cannot fail the radio service.
#
# systemd runs this as ExecStartPost. Detach immediately so the radio unit
# can become active before WebChat starts. webchat.sh starts the TNC if it
# does not already see a radio service active.

if [ "${WEBCHAT_RECONNECT_CHILD:-}" != 1 ]; then
  WEBCHAT_RECONNECT_CHILD=1 setsid "$0" "$@" >>/tmp/webchat-reconnect.log 2>&1 </dev/null &
  exit 0
fi

unit="${1:-}"
echo "$(date -Is) reconnect wait unit=${unit}"

for _ in $(seq 1 20); do
  if [ -n "$unit" ] && systemctl is-active --quiet "$unit"; then
    mainpid=$(systemctl show -p MainPID --value "$unit" 2>/dev/null || true)
    kid=$(pgrep -P "$mainpid" -x direwolf 2>/dev/null | head -1 || true)
    if [ -n "$kid" ] && ss -ltnp 2>/dev/null | grep ':8001 ' | grep -q "pid=${kid},"; then
      echo "$(date -Is) ${unit} direwolf ${kid} listening; try-restart webchat"
      systemctl try-restart webchat.service || true
      exit 0
    fi
  fi
  sleep 1
done

echo "$(date -Is) webchat not restarted: ${unit:-unknown} Direwolf is not listening on 8001" >&2
exit 0
