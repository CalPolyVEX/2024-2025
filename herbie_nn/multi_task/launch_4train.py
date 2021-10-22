#This file will launch multiple neural network training sessions
#on the MPAC lab machines

import os, sys
import argparse

parser = argparse.ArgumentParser(description='Launch training jobs.')
parser.add_argument('-start', type=int, help='The hostname of the starting machine.')
parser.add_argument('-end', type=int, help='The hostname of the last machine.')
parser.add_argument('-kill', action='store_true', help='Kill the running processes.')

if len(sys.argv) == 1: # if no arguments are given, print the help
    parser.print_help(sys.stdout)
    sys.exit(1)

args = parser.parse_args()

start_host = int(args.start)

end_host = args.end
if end_host is None:
    end_host = start_host
end_host = int(end_host)

if end_host < start_host: # make sure the end host comes after the start
    start_host = end_host

# sys.exit(1)

local_dir = '/home/jseng/ue4/herbie_nn/multi_task'

for host_num in range(start_host,end_host+1):
    if args.kill is False:
        train_command = 'nohup /home/jseng/ue4/herbie_nn/multi_task/train_all.sh 1>/dev/null 2>/dev/null &'

        run_command = 'ssh 127x' + str(host_num).zfill(2) + '.csc.calpoly.edu \"' + train_command + '\"'
        print(run_command)
        os.system(run_command)
    else:
        kill_command = 'killall -u jseng'
        run_command = 'ssh 127x' + str(host_num).zfill(2) + '.csc.calpoly.edu \"' + kill_command + '\"'
        print(kill_command)
        os.system(run_command)
