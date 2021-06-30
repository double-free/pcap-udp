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

def get_timestamp(csv_line):
    for field in csv_line:
        if field.isdigit():
            # timestamp is always the first digit field
            return int(field)

# an ugly workaround
def in_gap(bench_line) -> bool:
    timestamp = get_timestamp(bench_line) / 1e6

    # needs to be very precise
    if 1587611459.732 < timestamp < 1587611459.794:
        return True
    if 1587611460.094 < timestamp < 1587611460.6704:
        return True
    # 111016450 to 111016850
    if 1587611460.8134 < timestamp < 1587611461.215:
        return True
    # 111017150 to 111017610
    if 1587611461.516 < timestamp < 1587611461.974:
        return True
    # 111018620 to 111019110
    if 1587611462.986 < timestamp < 1587611463.470:
        return True
    # 111019360 to 1111019370
    if 1587611463.7217 < timestamp < 1587611463.7351:
        return True
    # 111020600 to 111020920
    if 1587611464.9602 < timestamp < 1587611465.2864:
        return True
    # 111021130 to 111021560
    if 1587611465.4955 < timestamp < 1587611465.921:
        return True

    return False

def compare(file, benchmark, comparator, max_err=1000) -> bool:
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

            prev_check_point = 0
            for idx, bench_line in enumerate(bench_reader):
                if idx - prev_check_point == 100000:
                    print(f"check point, line {idx}: {bench_line}")
                    prev_check_point = idx

                # very ugly workaround
                if in_gap(bench_line):
                    continue

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
    print(result)
