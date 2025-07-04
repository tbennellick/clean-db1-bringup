
# Add LinkServer to PATH if it exists and not already in PATH
LINKSERVER_PATH="/usr/local/LinkServer"
if [[ -d "${LINKSERVER_PATH}" ]] && [[ ":${PATH}:" != *":${LINKSERVER_PATH}:"* ]]; then
    export PATH="${PATH}:${LINKSERVER_PATH}"
    echo "Added LinkServer to PATH"
fi

echo "LinkServer in PATH: $(command -v LinkServer >/dev/null 2>&1 && echo "Yes" || echo "No")"