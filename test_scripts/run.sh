#!/usr/bin/env bash

WRAP=${WRAP} # Allows scripts to be run under GDB, for instance

BASE_DIR=".."

VETH_NAME="arqveth"
NS_NAME="arqns"

server_veth="arqveth0"
client_veth="arqveth1"

server_ns="arqns0"
client_ns="arqns1"

server_addr="10.0.0.1"
client_addr="10.0.0.2"

tx_delay="100ms"

# Clean up old namespaces
ip netns delete ${server_ns} || true
ip netns delete ${client_ns} || true

# Create namespaces and veth pair
ip netns add ${server_ns}
ip netns add ${client_ns}
ip link add name ${server_veth} netns ${server_ns} type veth peer name ${client_veth} netns ${client_ns} mtu 1500

ip netns exec ${server_ns} ip addr add ${server_addr} dev ${server_veth}
ip netns exec ${client_ns} ip addr add ${client_addr} dev ${client_veth}

# ip netns exec ${server_ns} ip link set lo up
# ip netns exec ${client_ns} ip link set lo up
ip netns exec ${server_ns} ip link set ${server_veth} up
ip netns exec ${client_ns} ip link set ${client_veth} up

ip netns exec ${server_ns} ip route add ${client_addr} dev ${server_veth}
ip netns exec ${client_ns} ip route add ${server_addr} dev ${client_veth}

# Simulate network conditions
ip netns exec ${server_ns} tc qdisc add dev ${server_veth} root netem delay ${tx_delay}

# Start server
tmux new-session -d -s "arq" -n "server" -c "${BASE_DIR}" "ip netns exec ${server_ns} ${WRAP} build/arq-benchmark/launcher --launch-server --logging 4 --client-addr ${client_addr} --server-addr ${server_addr}; read"

# Start client
tmux new-window -t "arq" -n "client" -c "${BASE_DIR}" "ip netns exec ${client_ns} ${WRAP} build/arq-benchmark/launcher --launch-client --logging 4 --client-addr ${client_addr} --server-addr ${server_addr}; read"

tmux set-option -t "arq" -g mouse
tmux attach-session -t "arq"

ip netns exec ${server_ns} tc qdisc del dev ${server_veth} root netem delay ${tx_delay}
