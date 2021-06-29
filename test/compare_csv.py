#!/usr/bin/python3

import argparse
import csv


class CompareResult:
    def __init__(self):
        self.success = 0
        self.failure = 0

    def __str__(self):
        return f'success: {self.success}, failure: {self.failure}'


def compare_trade(trade1, trade2):
    if trade1[2:] != trade2[2:]:
        return False
    return True


def compare_order(order1, order2):
    pass


def compare_snapshot():
    pass


def compare(file, benchmark, comparator, max_err=10) -> CompareResult:
    result = CompareResult()
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
        my_cached_lines = []
        bench_cached_lines = []
        with open(file, newline='') as my_csv:
            my_reader = csv.reader(my_csv, delimiter=',')
            for idx, bench_line in enumerate(bench_reader):
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
    result = CompareResult()
    print(result)
    result = compare(args.file, args.benchmark, m[args.mode])
    print(result)
