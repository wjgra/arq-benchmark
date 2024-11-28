#!/usr/bin/env python3

import dateutil.parser as dparser
import matplotlib
import re
import sys

def parse_logfile(directory, first_match_str, second_match_str, output = False):
    """
    Parses an ARQ logfile to extract packet sequence numbers and corresponding timestamps.

    :param directory: a path to the logfile
    :param first_match_str: pattern string for extracting the sequence number
    :param second_match_str: pattern string for extracting the timestamp
    :param output: whether to print the found pairs
    :return: a list of pairs [sequence_number, timestamp]
    """
    logfile = open(directory, "r")

    time_series=[]

    for line in logfile:
        try:
            sequence_number = re.search(first_match_str, line).group(1)

            timestamp_str = re.search(second_match_str, line).group(1)

            timestamp = dparser.parse(timestamp_str)
        except:
            # Line is not one of the desired logs
            continue
        
        if output:
            print(f"SN: {sequence_number}, Timestamp: {timestamp}")
        time_series.append((sequence_number, timestamp))
        
    logfile.close()

    return time_series

def parse_server_logfile(directory, output = False):
    """
    As parse_logfile, but timestamps correspond to when packets are pushed to the input buffer (IB).
    Example server logline:
    Adding packet with SN 2 to IB at time 2024-11-27 19:16:12.309514278
    """
    return parse_logfile(directory, "Adding packet with SN (.+?) to IB", " to IB at time (.+?)$", output)

def parse_client_logfile(directory, output = False):
    """
    As parse_logfile, but timestamps correspond to when packets are pushed to the output buffer (OB).
    Example client logline:
    Pushed packet with SN 1 to OB at time 2024-11-27 19:16:12.350295886
    """
    return parse_logfile(directory, "Pushed packet with SN (.+?) to OB", " to OB at time (.+?)$", output)

def validate_time_series_data(server_time_series, client_time_series):
    # ARQ gives a reliable connection, so the number and sequence of packets should match exactly
    if len(server_time_series) != len(client_time_series):
        print(f"len(server_time_series) ({server_time_series}) != len(client_time_series) ({client_time_series})")
        return 1
    
    for server, client in zip(server_time_series, client_time_series):
        # Check sequence numbers
        if server[0] != client[0]:
            print(f"Server SN {server[0]} does not match client SN {client[0]}")
            return 1
        
        # Check OB timestamp does not precede IB timestamp
        if server[1] > client[1]:
            print(f"SN {client[0]}: OB timestamp ({client[1]}) precedes IB timestamp ({server[1]})")
            return 1
    
    return 0


def main():
    # Extract ARQ time series data
    server_time_series = parse_server_logfile("/home/wjgra/repos/arq-benchmark/logs/server_arqlog.txt")
    client_time_series = parse_client_logfile("/home/wjgra/repos/arq-benchmark/logs/client_arqlog.txt")

    if validate_time_series_data(server_time_series, client_time_series) != 0:
        print("Failed to validate time series data")
        return 1

if __name__ == "__main__":
    sys.exit(main())
