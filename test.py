import random
import os
import subprocess

trace_dir = "./trace"
trace_file_prefix = "test_trace"
trace_file_suffix = "trace"
output_file_prefix = "output"
output_file_suffix = "txt"
server_bin = "./bin/server"
client_bin = "./bin/client"

def generate_trace(num: int, entry: int, op: int):
    for t in range(num):
        (min_entry, max_entry) = (entry * t, entry * (t + 1))
        entries = list(range(min_entry, max_entry))
        repeat_num = int(op / entry)
        trace = entries * repeat_num
        random.shuffle(trace)
        trace_path = "{}/{}{}.{}".format(trace_dir, trace_file_prefix, t, trace_file_suffix)
        entry_count = {}
        lines: list[str] = []
        for num in trace:
            count = entry_count.get(num, 0)
            count += 1  
            if count == 1:
                lines.append("INSERT {} {}\n".format(num, num))
            elif count == repeat_num:
                lines.append("DELETE {} {}\n".format(num, num))
            else:
                lines.append("READ {} {}\n".format(num, num))
            entry_count[num] = count
        lines.append("exit\n")
        
        with open(trace_path, "w") as trace_file:
            trace_file.writelines(lines)
            
def prepare_dir():
    if not os.path.exists(trace_dir):
        os.makedirs(trace_dir)
    for file in os.scandir(trace_dir):
        os.unlink(file.path)
        
def run_test(server_s: int, server_w: int, client_num: int, entry: int, op: int):
    prepare_dir()
    generate_trace(client_num, entry, op)
    outputs = []
    server_arg_s = "-s {}".format(server_s)
    server_arg_w = "-w {}".format(server_w)
    server_process = subprocess.Popen([server_bin, server_arg_s, server_arg_w])
    client_processes: list[subprocess.Popen] = []
    for i in range(client_num):
        output_path = "{}/{}{}.{}".format(trace_dir, output_file_prefix, i, output_file_suffix)
        trace_path = "{}/{}{}.{}".format(trace_dir, trace_file_prefix, i, trace_file_suffix)
        output_f = open(output_path, "w")
        trace_f = open(trace_path, "r")
        client_processes.append(subprocess.Popen([client_bin], stdout=output_f, stdin=trace_f))
        outputs.append(output_f)
    
    for client_p in client_processes:
        client_p.wait()
    server_process.kill()
        
run_test(1, 1, 1, 3, 9)