#!/usr/bin/env bash

if [ "$(id -u)" -ne 0 ]; then
        echo "This script must be run as root" >&2
        exit 1
fi

log_dir="../logs"

snw_log="snw.log"
sctp_log="sctp.log"

num_pkts=100

# Generate dummy SCTP data
../test_scripts/run.sh -n "${num_pkts}" -d "100ms 10ms distribution normal" -w 4 -l "random 1%" -t 50 -i 10 -p "dummy-sctp" -f "${sctp_log}"
# Generate ARQ data
../test_scripts/run.sh -n "${num_pkts}" -d "100ms 10ms distribution normal" -w 4 -l "random 1%" -t 50 -i 10 -p "stop-and-wait" -f "${snw_log}"

# Graphy the results
../test_scripts/graph.py "${sctp_log}" "${snw_log}"