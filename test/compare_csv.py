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
        print("compare trade failed: ", trade1)
        return False
    return True


def compare_order(order1, order2):
    pass


def compare_snapshot():
    pass


def compare(file, benchmark, comparator) -> CompareResult:
    result = CompareResult()
    with open(benchmark, newline='') as bench_csv:
        bench_reader = csv.reader(bench_csv, delimiter=',')
        with open(file, newline='') as my_csv:
            my_reader = csv.reader(my_csv, delimiter=',')
            for idx, bench_line in enumerate(bench_reader):
                my_line = next(my_reader)
                if comparator(my_line, bench_line) == False:
                    print(f"find difference in line {idx}: ")
                    print(f"mine:  {my_line}")
                    print(f"bench: {bench_line}")
                    return False

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
