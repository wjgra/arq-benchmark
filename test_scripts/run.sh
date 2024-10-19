#!/usr/bin/env bash

WRAP=${WRAP} # Allows scripts to be run under GDB, for instance

BASE_DIR=".."

VETH_NAME="arqveth"
NS_NAME="arqns0"

# Create veth link
ip link delete ${VETH_NAME}0 || true
ip link delete ${VETH_NAME}1 || true
ip link add dev ${VETH_NAME}0 type veth peer ${VETH_NAME}1 mtu 1500

ip addr add 10.0.0.1 dev ${VETH_NAME}0
# ip addr add 10.0.0.2 dev ${VETH_NAME}1

# Create network namespaces
ip netns add ${NS_NAME}
ip netns exec ${NS_NAME} ip link set dev ${VETH_NAME}1 up
ip netns exec ${NS_NAME} ip addr add 10.0.0.2 dev ${VETH_NAME}1
ip netns exec ${NS_NAME} ip route add default via 10.0.0.1 dev ${VETH_NAME}1

iptables -t nat -A POSTROUTING -o ${VETH_NAME}0 - j MASQUERADE
iptables -A FORWARD  -i ${VETH_NAME}0 -J ACCEPT
iptables -A FORWARD  -o ${VETH_NAME}0 -J ACCEPT


# Simulate network conditions
# tc...
# tc qdisc add dev arqveth0 root netem delay 1000ms

# Bring link up
ip link set dev ${VETH_NAME}0 up
# ip link set dev ${VETH_NAME}1 up

tmux new-session -d -s "arq" -n "server" -c "${BASE_DIR}" "ip netns exec ${NS_NAME} ${WRAP} build/arq-benchmark/launcher --launch-server --logging 4 --client-addr 10.0.0.1 --server-addr 10.0.0.2; read"

tmux new-window -t "arq" -n "client" -c "${BASE_DIR}" "${WRAP} build/arq-benchmark/launcher --launch-client --logging 4 --client-addr 10.0.0.1 --server-addr 10.0.0.2; read"

tmux set-option -t "arq" -g mouse
tmux attach-session -t "arq"

tc qdisc del dev arqveth0 root netem delay 100ms