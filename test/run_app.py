#!/usr/bin/env python3

import argparse
import subprocess

def run(executable, pcap_file, max_parallel=10):
    cmds = [[executable, f'{pcap_file}{i}', f'pcap{i}']
            for i in range(499, 513)]    # totaly 512 files

    idx = 0
    while idx < len(cmds):
        procs = [subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                    for cmd in cmds[idx : idx+max_parallel]]
        idx += max_parallel
        for proc in procs:
            outs, errs = proc.communicate()
            print(f"execution {proc.args} returns {proc.returncode}")
            if proc.returncode != 0:
                if outs:
                    print(str(outs))
                if errs:
                    print(str(errs))



if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument("-e", "--executable",
                        help="path to the md parser", required=True)
    parser.add_argument("-p", "--pcap", help="path to the pcap file", required=True)
    args = parser.parse_args()
    run(args.executable, args.pcap)
