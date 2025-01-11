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

arq_timeout="50" # ms
arq_protocol="dummy-sctp" # Options: "dummy-sctp", "stop-and-wait", "sliding-window" and "selective-repeat"

tx_delay="100ms 10ms distribution normal"
tx_loss="random 1%"

pkt_num="30"
pkt_interval="10"

logging_level="4"
log_dir="/home/wjgra/repos/arq-benchmark/logs"
log_file="arqlog.txt"

remain_on_exit="false"

usage() { echo "Usage: $0 [-d <tc delay arg string>] [-l <tc loss arg string>] [-n <number of pkts to tx>] [-i <interval between tx pkts> ] [-w <logging level>] [-f <log file>] [-t <ARQ timeout>] [-r <remain on exit>] [-h]" 1>&2; }

while getopts "d:l:n:i:w:f:t:p:rh" opt; do
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
        w)
            logging_level=${OPTARG}
            ;;
        f)
            log_file=${OPTARG}
            ;;
        t)
            arq_timeout=${OPTARG}
            ;;
        p)
            arq_protocol=${OPTARG}
            ;;
        r)
            remain_on_exit="true"
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

# Clean up old logs
client_log="${log_dir}/client_${log_file}"
server_log="${log_dir}/server_${log_file}"
rm -f ${client_log} || true
rm -f ${server_log} || true

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

common_opts="--logging ${logging_level} --client-addr ${client_addr} --server-addr ${server_addr} --arq-protocol ${arq_protocol} "

# Start server
tmux new-session -d -s "arq" -n "server" "stdbuf -o0 ip netns exec ${server_ns} ${wrap_cmd} ${base_dir}/build/arq-benchmark/launcher \
--launch-server ${common_opts} --tx-pkt-num ${pkt_num} --tx-pkt-interval ${pkt_interval} --arq-timeout ${arq_timeout} | tee ${server_log}"

# Start client
tmux new-window -t "arq" -n "client" "stdbuf -o0 ip netns exec ${client_ns} ${wrap_cmd} ${base_dir}/build/arq-benchmark/launcher \
--launch-client ${common_opts} | tee ${client_log}"

if [[ "${remain_on_exit}" == "true" ]]; then
    tmux set-option -g remain-on-exit on
else
    tmux set-option -g remain-on-exit off
fi

tmux set-option -t "arq" -g mouse
tmux attach-session -t "arq"

ip netns exec ${server_ns} tc qdisc del dev ${server_veth} root netem delay ${tx_delay}
