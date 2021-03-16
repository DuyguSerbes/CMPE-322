/**
* @author Duygu Serves.
* @name CMPE 322 Project 1 Process
* Compile and run:
* g++ process.cpp -o process.
* ./watchdog processNum processPath.
*  Process.cpp of the project checks coming signal, handle them by printing signal value and terminate itself.
*/

#include<stdio.h>
#include <unistd.h>
#include <iostream>
#include <unistd.h>
#include <csignal>
#include <sstream>
#include <fstream>
#include <sys/stat.h>
#include <fcntl.h>
#include <map>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <bits/stdc++.h>
#include <string.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <csignal>
using namespace std;

int processNum; ///<Number of children given via terminal
fstream file; ///<Creates file to write
char const *path_name; ///<Output file path
int term_flag=1;  ///<Termination flag. It become 0 when SIGTERM signal is came.



/**
 * Take signals, and handle them by writing signal value.
 * If signal is SIGTERM, it prints the process is terminating.
 * To be terminated, it informs main function by making term_flag 0.
 * @param signal_num indicates which signal is delivered to the watchdog
 */
void signal_handler( int signal_num ) {

    fstream file(path_name,ios::out | ios::app);

    if (signal_num==15){
        file<<"P"<<processNum<< " received signal "<<signal_num<<", terminating gracefully"<<endl;
        term_flag=0;
    }
    else
        file<<"P"<<processNum<< " received signal "<<signal_num<<endl;

}
/**
 * This is the main function responsible to take argument (output file name and index number of process).
 * It prints waiting signal status after starting.
 * Signals defined here and coming signal sends to signal_handler function.
 * If term flag become 0 due to SIGTERM, it exits this process.
 * exit the program with exit(0)
 * @param argc Number of strings in array argv
 * @param argv Array of command-line argument strings
*/
int main(int argc, char *argv[]) {
    processNum = stoi(argv[1]);
    path_name=argv[2];
    fstream file(path_name,ios::out | ios::app);
    //freopen("./deneme.txt","w",stdout);
    file<< "P"<<processNum<<" is waiting for a signal"<<endl;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);
    signal(SIGILL, signal_handler);
    signal(SIGTRAP, signal_handler);
    signal(SIGFPE,signal_handler);
    signal(SIGSEGV,signal_handler);
    signal(SIGXCPU, signal_handler);

    while(1) {
        if (term_flag == 0) {
            file.close();
            exit(0);
        }
    }

};


