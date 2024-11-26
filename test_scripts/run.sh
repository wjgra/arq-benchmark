#!/usr/bin/env bash

if [ "$(id -u)" -ne 0 ]; then
        echo "This script must be run as root" >&2
        exit 1
fi

wrap_cmd=${WRAP} # Allows scripts to be run under GDB, for instance
base_dir="$(dirname "$0")/.."

server_veth="arqveth0"
client_veth="arqveth1"

server_ns="arqns0"
client_ns="arqns1"

server_addr="10.0.0.1"
client_addr="10.0.0.2"

tx_delay="100ms"
tx_loss="random 1%"

pkt_num="10"
pkt_interval="10"

usage() { echo "Usage: $0 [-d <tc delay arg string>] [-l <tc loss arg string>] [-n <number of pkts to tx>] [-i <interval between tx pkts> ] [-h]" 1>&2; }

while getopts "d:l:n:i:h" opt; do
    case ${opt} in
        d)
            tx_delay=${OPTARG}
            echo "tx_delay set to ${tx_delay}"
            ;;
        l)
            tx_loss=${OPTARG}
            echo "tx_loss set to ${tx_delay}"
            ;;
        n) 
            pkt_num=${OPTARG}
            ;;
        i)
            pkt_interval=${OPTARG}
            ;;
        h) 
            usage
            exit 0
            ;;
        *) 
            usage
            exit 1
            ;;
    esac
done
shift $((OPTIND-1))

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
ip netns exec ${server_ns} tc qdisc add dev ${server_veth} root netem delay ${tx_delay} loss ${tx_loss}
ip netns exec ${client_ns} tc qdisc add dev ${client_veth} root netem delay ${tx_delay} loss ${tx_loss}

# Start server
tmux new-session -d -s "arq" -n "server" "ip netns exec ${server_ns} ${wrap_cmd} ${base_dir}/build/arq-benchmark/launcher --launch-server --logging 4 --client-addr ${client_addr} --server-addr ${server_addr} --tx-pkt-num ${pkt_num} --tx-pkt-interval ${pkt_interval}; read"

# Start client
tmux new-window -t "arq" -n "client" "ip netns exec ${client_ns} ${wrap_cmd} ${base_dir}/build/arq-benchmark/launcher --launch-client --logging 4 --client-addr ${client_addr} --server-addr ${server_addr}; read"

tmux set-option -t "arq" -g mouse
tmux attach-session -t "arq"

ip netns exec ${server_ns} tc qdisc del dev ${server_veth} root netem delay ${tx_delay}
