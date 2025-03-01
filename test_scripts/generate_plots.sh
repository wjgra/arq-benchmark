#!/usr/bin/env bash

if [ "$(id -u)" -ne 0 ]; then
        echo "This script must be run as root" >&2
        exit 1
fi

log_dir="../logs"

snw_log="snw.log"
sctp_log="sctp.log"

num_pkts=100

protocols=("stop-and-wait" "go-back-n" "selective-repeat")
logfiles=("snw.log" "gbn.log" "sr.log")

# protocols=("dummy-sctp" "go-back-n")
# logfiles=("dummy-sctp.log" "gbn.log")

for (( i=0; i<${#protocols[@]}; i++ )); do
        # SNW delay grows linearly
        # ../test_scripts/run.sh -n "${num_pkts}" -d "100ms" -w 0 -l "random 0%" -t 200 -i 100 -p "${protocols[$i]}" -f "${logfiles[$i]}"

        # Number of packets tx'd exceeds Rxer's processing capability? SCTP is better here
        ../test_scripts/run.sh -n "${num_pkts}" -d "100ms" -w 0 -l "random 0%" -t 200 -i 200 -p "${protocols[$i]}" -f "${logfiles[$i]}"

        # ../test_scripts/run.sh -n "${num_pkts}" -d "5ms" -w 0 -l "random 1%" -t 10 -i 25 -p "${protocols[$i]}" -f "${logfiles[$i]}"
done

# Graph the results
../test_scripts/graph.py "${logfiles[@]}"