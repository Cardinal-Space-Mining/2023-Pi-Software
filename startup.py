from typing import Final
import subprocess
from io import IOBase
import os

processes: Final[list[list[str]]]  = [
    [r'make', '-C' , r'./path_planning/map_server', 'run'], 
    ['python3' , r'./path_planning/map_client/PathGUI.py' ]
]


class Proc:
    process: subprocess.Popen
    envocation: list[str]
    file_out: IOBase

    def __init__(self, args: list[str]) -> None:
        value: int = 0
        self.envocation = args
        while True:
            suggested_f_name = f"{args[0]}.{value}.run"
            try:
                if not os.path.exists(suggested_f_name):
                    self.file_out = open(suggested_f_name, 'wt')
                    break
                else:
                    value +=1
                    continue
            except OSError:
                value += 1
                continue
        self.process = subprocess.Popen(args=args, stdout=self.file_out, stderr= self.file_out)

    def __del__(self) -> None:
        if self.process.poll() is None:
            self.process.wait()
        self.file_out.close()

    def restart(self):
        value = 0
        while True:
            suggested_f_name = f"{self.envocation[0]}.{value}.run"
            try:
                if not os.path.exists(suggested_f_name):
                    self.file_out = open(suggested_f_name, 'wt')
                    break
                else:
                    value +=1
                    continue
            except OSError:
                value += 1
                continue
        self.process = subprocess.Popen(args=self.envocation, stdout=self.file_out)
        print(f"Restarting process: {self.envocation}")




def main():
    child_processes: Final[list[Proc]] = []
    for process in processes:
        child_processes.append(Proc(process))

    while len(child_processes) != 0:
        for process in child_processes:
            if process.process.poll() is  not None:
                if process.process.returncode != 0:
                    process.restart()
                else:
                    child_processes.remove(process)
                
    


if __name__ == "__main__":
    main()