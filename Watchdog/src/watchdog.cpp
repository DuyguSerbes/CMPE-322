/*! \mainpage WatchDog - CMPE 322 Project 1
 * This project build watchdog, process and executor.
 * \section Introduction
 * Watchdog is responsible for child production, execute them to given process.cpp, send PIDs of children and
 * itself through the FIFO(pipe) and taking care of child dead by producing new ones.
 * When first child is dead, all other children are terminated and all children created again.
 * When the other children except first children is dead, only dead child is created again.
 * All new children PIDs are sent to the pipe.
 *
 * Process is responsible to handle signal by writing signal value, and terminate itself as a result of SIGTERM.
 *
 * Executor takes instructions from user, take PIDs of children from pipe and send signals according to instructions.
 *
 * Compile and run with these comments:
 *
 * g++ executor.cpp -std=c++14 -o executor
 *
 * g++ watchdog.cpp -std=c++14 -o watchdog
 *
 * g++ process.cpp -std=c++14 -o process
 *
 * ./executor <Number_of_process> instructions.txt &
 *
 * ./watchdog <Number_of_process> process_out.txt watchdog_out.txt.
 *
 * \section Bonuscase
 * In order to deal abnormal request to terminate watchdog.cpp, termination flag is used. When SIGTERM signal is
 * received from watchdog. It waits 0.6 seconds to check first child status. Because executor.cpp starts kill process
 * watchdog then child 1, child 2 and so on when it reaches at the end of the instruction file. After waiting, watchdog
 * checks first child. If it is already dead, it understands which is a normal termination request. On the other hand,
 * after waiting, watchdog can see that first child is alive. In that case, watchdog regret the termination request and
 * don't pay attention to handle SIGTERM signal, it informs user about abnormal termination request and its cancellation.
 * It sends its own PIDs to the pipe to keep executor.cpp well.
 *
 * \section Conclusion
 *
 * The project provides 3 subparts which are watchdog itself, executor, and process. Watchdog and executor communication
 * is enabled via pipe. PIDs of watchdog and its children send to the executor via pipe. Executor takes inputs from
 * instruction file which is given from user ana executor send needed signal to the processes. It terminates itself when
 * all requests are send by reaching at the end of the instruction text. Watchdog executes its children to the process.cpp.
 * process.cpp handles coming signals by writing its value and terminate itself.
 * As a consequences, watchdog is responsible to keep children alive until executor finishes the all instructions and sends
 * required signals.
 *\author Duygu Serbes.
 *\name CMPE 322 Project 1 Watchdog.
 *
 *
 */

/**
* @author Duygu Serbes.
* @name CMPE 322 Project 1 Watchdog.
 *
* Run and compile with this comments:
*  g++ watchdog.cpp -o watchdog
* ./watchdog processNum processPath watchdogPath
*
*  Watchdog.cpp of the project controls part creates processes
* and tries to keep these processes alive so that the system continues to run smoothly.
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
#include <spawn.h>
#include <cstdio>



using namespace std;
int term_flag=1;  ///<  Check termination signal coming.
vector<pid_t> pidList; ///<  Hold PIDs number of  process starting from watchdog process itself.
struct timespec delta = {0 /*secs*/, 400000000 /*nanosecs*/}; ///<0.4 sec sleep


/**
 * Taking SIGTERM signal and informing main function using termination flag.
 * When SIGTERM signal is delivered term_flag become 0 from 1.
 * @param signal_num indicates which signal is delivered to the watchdog
 */
void signSIGTERM( int signal_num ) {

    if (signal_num==15){
        term_flag=0;
    }
}


/**
 * This is the main function responsible for producing child process, sending process information to the pipe through.
 * the FIFO pipe, taking care of child dead considering whether or not the head child is dead.
 * @param argc Number of strings in array argv.
 * @param argv Array of command-line argument strings.
 * \return the program exit code.
*/
int main(int argc, char *argv[]) {

    int pid;
    int term_pid; ///<  PID of terminated child
    int processNum = stoi(argv[1]);///< ProcessNum: Number of process to be created
    char const *watchdog_path = argv[3];  ///< watchdog_path: Watchdog output path
    char const *process_path = argv[2]; ///< process_path:  Process output path
    int unnamedPipe;
    char *myfifo = (char *) "/tmp/myfifo";  ///< myfifo: Location of FIFO
    mkfifo(myfifo, 0644);
    char temp[30];   ///<  temp: Pipe buffer array
    int status;
    char const *arg1 ;
    char const *arg2 = process_path;

    /**
    * Remove previous written files in the same path to prevent any conflict.
    */

    remove(process_path);
    remove(watchdog_path);

    /**
    * Get parent process(watchog) information
    * Create the named file (FIFO) under /tmp/myfifo directory.
    * Watchdog writes the newly produces PIDs to the pipe includes its own PID.
    * Watchdog is the first process to be sent with index 0.
    * P0 <process ID> is sent by finding process ID with getpid().
    */

    pidList.push_back(getpid());
    sprintf(temp, "P0 %d", getpid());
    unnamedPipe = open(myfifo, O_WRONLY);
    write(unnamedPipe, temp, 30);

    /**
    * Create the output file in the given path via command-line argument
    */

    fstream w_out(watchdog_path,ios::out | ios::app);

    /**
    * Evaluation of SIGTERM signal
    */

    signal(SIGTERM, signSIGTERM);


    /**
    * Create the child process using fork() and execl() to make them process.cpp..
    * Send PIDs of children through pipe in the format P<index> <process ID>.
    * Watchdog prints newly produced children information to the output file.
    * It includes process index and PID.
    */

    for (int i = 0; i < processNum; i++) //
    {

        pid = fork();

        sprintf(temp, "P%d %d", i + 1, pid);

        if (pid == 0) {
            string str = to_string(i + 1);
            arg1= str.c_str();
            execl("./process", "process", arg1, arg2);
            exit(0);
        } else {
            w_out << "P" << i + 1 << " is started and it has a pid of " << pid << endl;
            pidList.push_back(pid);
            nanosleep(&delta, &delta);
            unnamedPipe = open(myfifo, O_WRONLY);
            write(unnamedPipe, temp, 30);
        }
    }

    /**
    * Infinite while loop checks any child termination.
    * The while loop consists of two parts according to status of termination flag(term_flag).
    * When termination flag is 1, it means watchdog process don't have any SIGTERM signal at that moment.
    * When termination flag is 0, it means watchdog process have to deal with SIGTERM signal considering its normality.
    * It includes process index and PID.
    */
    while (1){
        here:
        /**
         * When there is no SIGTERM signal, the loop controls children exit status.
         * If one of them is dead, term_pid has the values of PID of dead child.
         * Watchdog takes care of child dead.
         * If head child is dead which has index 1, all processes must be terminated and created with new PIDs.
         * If one of the children is dead except head child, only dead child created with new PIDs.
         * The new PIDs of children always send to the pipe.
        */
        if (term_flag == 1) {

            for (int i = 1; i <= processNum; i++) {
                pid_t process_end = waitpid(pidList[i], &status, WNOHANG | WUNTRACED);

                if (process_end == pidList[i]) {
                    term_pid = pidList[i];
                    break;

                }
            }
            /**
            * When one of children is dead except first child( head child)
            * Watchdog prints which child is dead and informs recreation of that child.
            * The newly produced child PIDs and index is sent to the pipe.
            * The watchdog prints new PIDs os that child.
            * The child exec to process.cpp
            */

            if (term_pid != pidList[0] && term_pid != pidList[1] && term_flag == 1) {
                for (int i = 2; i <= processNum; i++) {
                    if (term_pid == pidList[i]) {
                        w_out << "P" << i << " is killed" << endl;
                        w_out << "Restarting P" << i << endl;
                        pid = fork();
                        sprintf(temp, "P%d %d", i, pid);
                        if (pid == 0) {
                            string str = to_string(i);
                            arg1 = str.c_str();
                            execl("./process", "process", arg1, arg2);
                            exit(0);
                        } else {
                            w_out << "P" << i << " is started and it has a pid of " << pid << endl;
                            write(unnamedPipe, temp, 30);
                            pidList[i] = pid;
                        }

                    }

                }
            }

            /**
            * When first child (head child) is dead
            * Watchdog prints that P1 is killed and all process must be killed.
            * The all children except P1 which is already dead are killed.
            * The all children creates again with fork() and execl().
            * The new PIDs of children send to the pipe.
            * Watchdog prints the PIDs number.
            */
            else if (term_pid == pidList[1] && term_flag == 1) {

                /**
                * Indicates P1 is dead all process must be killed.
                */
                w_out << "P1 is killed, all processes must be killed " << endl;
                w_out<< "Restarting all processes" << endl;

                /**
                * Kill all children(P2, P3,..)
                */

                for (int i = 2; i <= processNum; i++) {
                    kill(pidList[i], SIGTERM);
                    wait(NULL);
                }
                /**
                * Fork all children(P1, P2, P3,..)
                * Exec all children to given process via terminal (P1, P2, P3,..)
                * Print all new PIDs of children to the output file.
                */
                for (int i = 1; i < processNum + 1; i++) {
                    pid = fork();
                    nanosleep(&delta, &delta);
                    sprintf(temp, "P%d %d", i, pid);

                    if (pid == 0) {
                        string str = to_string(i);
                        arg1 = str.c_str();
                        execl("./process", "process", arg1, arg2);
                    }

                    else {
                        w_out << "P" << i << " is started and it has a pid of " << pid << endl;
                        pidList[i] = pid;
                        write(unnamedPipe, temp, 30);
                    }

                }

            }


        }
        /**
         *
         * When there is SIGTERM signal, watchdog checks which is normal termination or abnormal termination.
         * Watchdog checks the first child is terminated or not.
         * Because executor send SIGTERM first P1 after watchdog
         * So watchdog sleeps a little bit to give time to P1 to be terminated.
         * After waiting if P1 is still alive, watchdog understands its abnormal attack to kill watchdog.
         * Watchdog regrets to handle SIGTERM, informs user about abnomal attack.
         * Current PID of watchdog is again send to pipe.
         * After waiting of PID, P1 is dead, watchdog understand its a normal termination.
         * Inform user about gracefully termination and return 0.
         *
         */


        else if(term_flag==0){
            nanosleep(&delta, &delta);
            nanosleep(&delta, &delta);
            pid_t process_head = waitpid(pidList[1], &status, WNOHANG | WUNTRACED);

            /**
             * SIGTERM is came. Check status of first child.
             * If it is dead. Normal termination.
             */

            if (term_flag == 0 && process_head== pidList[1]) {

                w_out << "Watchdog is terminating gracefully" << endl;
                w_out.close();
                close(unnamedPipe);
                return 0;
            }

            /**
             * If P1 is still alive.
             * Regret termination inform about abnormal termination attack.
             * Send PID of watchdog to the pipe.
             */

            else{
                w_out<<"Abnormal attack to kill watchdog!"<<endl;
                w_out<<"Termination of watchdog cancelled"<<endl;
                sprintf(temp, "P0 %d", pidList[0]);
                write(unnamedPipe, temp, 30);
                term_flag=1;
                goto here;


            }

        }
    }

}

