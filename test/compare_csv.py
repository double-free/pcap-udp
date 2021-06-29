#!/usr/bin/python3

import argparse
import csv


def compare_trade(trade1, trade2):
    if trade1[2:] != trade2[2:]:
        return False
    return True


def compare_order(order1, order2):
    channel_id = str(int(order2[6]) >> 16)

    if order1[6] == channel_id and order1[7:] == order2[7:]:
        return True
    return False


def compare_snapshot(snapshot1, snapshot2):
    if snapshot1[5:] != snapshot2[5:]:
        return False
    return True

# an ugly workaround
def in_gap(bench_line) -> bool:
    for field in bench_line:
        if field.isdigit():
            # timestamp is always the first digit field
            timestamp = int(field)
            break

    # needs to be very precise
    if 1587611459.732 < timestamp / 1e6 < 1587611459.794:
        return True
    return False

def compare(file, benchmark, comparator, max_err=1) -> bool:
    # may be out of order, so the compare shall between two containers
    # each container holds the recent n record
    my_recent_lines = []
    bench_recent_lines = []

    def compare_containers(lines1, lines2) -> bool:
        if len(lines1) != len(lines2):
            return False
        for line1 in lines1:
            matched = False
            for line2 in lines2:
                if comparator(line1, line2):
                    matched = True
                    break
            if matched == False:
                return False
        return True

    with open(benchmark, newline='') as bench_csv:
        bench_reader = csv.reader(bench_csv, delimiter=',')
        with open(file, newline='') as my_csv:
            my_reader = csv.reader(my_csv, delimiter=',')
            # skip header
            next(my_reader)
            next(bench_reader)
            my_cached_lines = []
            bench_cached_lines = []
            for idx, bench_line in enumerate(bench_reader):
                # very ugly workaround
                # if in_gap(bench_line):
                #     continue

                my_line = next(my_reader)
                if comparator(my_line, bench_line) == True:
                    continue

                my_cached_lines.append(my_line)
                bench_cached_lines.append(bench_line)

                if len(my_cached_lines) > max_err or len(bench_cached_lines) > max_err:
                    for line1, line2 in zip(my_cached_lines, bench_cached_lines):
                        print(f"find difference in below lines: ")
                        print(f"mine:  {line1}")
                        print(f"bench: {line2}")
                    return False

                if compare_containers(my_cached_lines, bench_cached_lines):
                    # clear both cache
                    my_cached_lines = []
                    bench_cached_lines = []

    return True


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-f", "--file", help="input a csv file", required=True)
    parser.add_argument("-b", "--benchmark",
                        help="benchmark csv file", required=True)
    parser.add_argument(
        "-m", "--mode", help="trade, order or snapshot", required=True)
    args = parser.parse_args()

    m = {"order": compare_order, "trade": compare_trade,
         "snapshot": compare_snapshot}

    if args.mode not in m.keys():
        print("unsupported mode: ", args.mode)
        exit(1)

    result = compare(args.file, args.benchmark, m[args.mode])
