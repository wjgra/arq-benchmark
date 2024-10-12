#! /usr/bin/env bash

WRAP=${WRAP}

BASE_DIR=".."

tmux new-session -d -s "arq" -n "server" -c "${BASE_DIR}" "${WRAP} build/arq-benchmark/launcher --launch-server --logging 4; read"

tmux new-window -t "arq" -n "client" -c "${BASE_DIR}" "${WRAP} build/arq-benchmark/launcher --launch-client --logging 4; read"

tmux set-option -t "arq" -g mouse
tmux attach-session -t "arq"