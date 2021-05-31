# Watchdog

## Overview

In this project, watchdog system is implemented.  The project consists of two parts; process definition and process control.The process definition part specifies the process-specific information such as process id, processname, and signals to be handled by each process.  The process control part creates processesand tries to keep these processes alive so that the system continues to run smoothly.

## Implementation
Run the executor.cpp in background
```
./executor 5 instruction_path &
```
For C++:
```
g++ process.cpp -std=c++14 -o process
g++ watchdog.cpp -std=c++14 -o watchdog
./watchdog 5 process_output watchdog_output
```
