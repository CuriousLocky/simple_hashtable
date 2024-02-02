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
        
def run_test(server_s: int, server_w: int, client_num: int, entry: int, op: int) -> bool:
    prepare_dir()
    generate_trace(client_num, entry, op)
    outputs = []
    server_output_path = "./trace/server_output.txt"
    server_output = open(server_output_path, "w")
    server_arg_s = "-s {}".format(server_s)
    server_arg_w = "-w {}".format(server_w)
    server_process = subprocess.Popen([server_bin, server_arg_s, server_arg_w, "-v"], stdout=server_output)
    client_processes: list[subprocess.Popen] = []
    for i in range(client_num):
        output_path = "{}/{}{}.{}".format(trace_dir, output_file_prefix, i, output_file_suffix)
        trace_path = "{}/{}{}.{}".format(trace_dir, trace_file_prefix, i, trace_file_suffix)
        output_f = open(output_path, "a+")
        trace_f = open(trace_path, "r")
        client_processes.append(subprocess.Popen([client_bin], stdout=output_f, stdin=trace_f))
        outputs.append(output_f)
    
    for client_p in client_processes:
        client_p.wait()
    server_process.kill()
    
    for f in outputs:
        f.seek(0)
        lines: list[str] = f.readlines()
        for line in lines:
            if "failed" in line:
                return False

    return True

tests = [
    ([1, 1, 1, 10, 50],         "smoke"),
    ([1, 1, 1, 300, 1500],      "big-smoke"),
    ([1, 2, 1, 300, 1500],      "collaborate"),
    ([1, 1, 5, 300, 1500],      "client-compete"),
    ([1, 8, 10, 300, 1500],    "linear"),
    ([2, 2, 5, 300, 1500],      "mess"),
    ([5, 5, 10, 500, 5000],     "big-mess"),
    ([64, 16, 40, 1000, 10000],   "race"),
]

for test in tests:
    name:str = test[1]
    args:list[int] = test[0]
    print("running {}...".format(name))
    pass_test = run_test(args[0], args[1], args[2], args[3], args[4])
    if pass_test:
        print("\tpass")
    else:
        print("\tfailed")
        break
    
    