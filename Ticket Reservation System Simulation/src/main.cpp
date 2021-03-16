/*
*This program provides a possible solution for teather ticket reservation system.
 *
 * Author: Duygu Serbes
 *
 * Name: CMPE 322 Project 2 - Sync-Ticket.
 *
 * This programs have three teller, named as A, B, and C. A has priority on B and C to take care of clients.
 * B has priority on C.
 *
 * The system has 3 theater information, which are OdaTıyatrosu, UskudarStudyoSahne, KucukSahne.
 * Each of them has different capacity but the all system can deal with maximum 300 client at the same time.
 *
 * The desired and number of client given by user input file. The file also includes data of each client dor example
 * arrival time, waiting time, and requested seat.
 *
 * The client have to sleep in the amount of arrival time after it created
 * from main. The arrived clients are put on queue and tellers deals with them according to their priority order and availability
 * status. While teller making booking and prepare its ticket in the amount of waiting time, client shouldn't leave from
 * the system and wait until ticket is ready.
 *
 * Also, booking is made according to client's desired seat but if it is not available.
 * Teller is looking for empty seat with the smallest integer number. If no seat is found as available, client cannot take seat.
 * But still have to wait.
 *
 * How to compile: gcc main.cpp -o main.o -lpthread
 * How to run: ./main.o input_path output_path
 */


#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>
using namespace std;

/* Size of the buffer */
#define MAX_CLIENT 300


/* Define mutex and semaphores */
pthread_mutex_t client_queue; //critical section for when client wakes up, add it to the queue
pthread_mutex_t availability; //critical section for one client is assigned to a teller
pthread_mutex_t booking; //critical section for seat reservation
pthread_mutex_t writer; //critical section for writing to the output file
pthread_mutex_t ticket_ready;//critical section to sleep the client when teller prepares the ticket


sem_t writer_end; //Semaphore informs main that teller has arrived. Main can continue to produce next tellers.
sem_t empty; //Semaphore for buffer size
sem_t full; // Semaphore for when client is ready, unlock teller to take care of that client



/* Function definitions that will be used by threads*/
void *teller_thread(void *param);
void *client_thread(void *param);

/*Define theater struct
 * Struct holds name and capacity information of each theater*/
struct theater {
    string name;
    int capacity;

};
/*Define client struct
 * Struct holds client name, arrival time, waiting time and requested seat*/
struct client_data{
    char name[30];
    int arrival_time;
    int requested_seat;
    int waiting_time;

};
/*Define teller struct
 * Struct holds teller name and its availability status*/
struct teller_data{
    string name;
    bool availabiliy=true;
};
/*Number of teller given by question*/
const int tellerThreads=3;

/*Elements in the teller array includes information of each teller in the A,B,C order */
struct teller_data teller[tellerThreads];

/*Elements in the my_queue includes information of each client which is added to the array
 * after it wakes up from waiting time*/
struct client_data my_queue[MAX_CLIENT];

/*Elements in the my_finished_queue includes information of each client which is added to the array
 * after its ticket is prepared*/
struct client_data my_finished_queue[MAX_CLIENT];

/*Define global variables*/
int my_rank=0; //index of newly arrived client
int finished_rank=0; //index of client whose ticket has been just prepared
int teller_start_number=0; //index of next client for tellers
int current_capacity; //capacity of given theater
int clientThreads;    //number of client given by user
int* seat_status;     //size of seat status array, it changes depends on given theater
pthread_t tid[tellerThreads];  //teller thread id array

/*Define global variable of output file and input file path given by user*/
char const *output_path;
string input_path;




int main(int argc, char *argv[]){


    /*sem writer_start is initialized as 0*/
    sem_init(&writer_end, 0, 0);
    /* sem empty is initialized as BUFFER_SIZE */
    sem_init(&empty, 0, MAX_CLIENT);
    /* sem empty is initialized as 0 */
    sem_init(&full, 0, 0);

    /*mutexes initialization as NULL*/
    pthread_mutex_init(&client_queue, NULL);
    pthread_mutex_init(&availability, NULL);
    pthread_mutex_init(&booking, NULL);
    pthread_mutex_init(&writer,NULL);
    pthread_mutex_init(&ticket_ready,NULL);


    if(argc != 3){
        fprintf(stderr, "Number of argument is not appropriate\n");
        return -1;
    }
    /*Define features of each theater as name and capacity*/
    struct theater class_record[3];
    class_record[0].name ="OdaTiyatrosu";
    class_record[0].capacity = 60;
    class_record[1].name ="UskudarStudyoSahne";
    class_record[1].capacity = 80;
    class_record[2].name = "KucukSahne";
    class_record[2].capacity = 200;


    input_path = argv[1]; //input path given by user as an argument
    output_path=argv[2]; //ouput path given by user as an argument

    /*Remove already exist data in the given input file*/
    remove(output_path);

    /*Create the output file in the given path via command-line argument*/
    fstream w_out(output_path,ios::out | ios::app);

    /*Read the input file in the given path via command-line argument*/
    ifstream inst_file (input_path);

    /*Define string which are read from input file*/
    string inst, token1, token2, token3, token4, theater_name;

    /*Check input file is opened*/
    if (!inst_file.is_open()) return 0;

    /*Read theater name from the first line of document*/
    inst_file>>theater_name ;

    /* Define capacity of current theater according to given name considering previously recorded theater information*/
    for(int i = 0; i < 3; i++){
        if(theater_name.compare(class_record[i].name)==0){
            current_capacity = class_record[i].capacity;
        }
    }

    /*Define seat status array according to theater capacity
     * All elements in array is started with 0, it means all seats available at the beginning.*/
    seat_status=new int [current_capacity];

    /*Read number of client given by input file*/
    inst_file>>inst;
    clientThreads = stoi(inst);

    /*Define client array
     *Elements in the client array includes information of each client in the given order in the input file */
    struct client_data client[clientThreads];

    /*Read each line any save that information in the client array
     * Read remaining lines line by line
     * Separate them with ',' and store each client information according to index in client array*/
    for (int i=0;i<clientThreads;i++){

        inst_file>>inst; //Read next line
        stringstream splitline(inst);

        /*Client name*/
        getline(splitline, token1, ',');
        strcpy(client[i].name, token1.c_str());

        /*Client arrival time*/
        getline(splitline, token2, ',');
        client[i].arrival_time=stoi(token2);

        /*Client waiting time*/
        getline(splitline, token3, ',');
        client[i].waiting_time=stoi(token3);

        /*Client requested seat*/
        getline(splitline, token4, ',');
        client[i].requested_seat=stoi(token4);

    }
    /*Close input file*/
    inst_file.close();

    /*Define teller name*/
    teller[0].name='A';
    teller[1].name='B';
    teller[2].name='C';

    /*Define client thread id array*/
    pthread_t cid[clientThreads];

    /*Inform Sync-Ticket system has started.*/
    w_out<<"Welcome to the Sync-Ticket!"<<endl;

    /*Create teller threads*/
    for(int i = 0; i < tellerThreads; i++){
        pthread_create(&tid[i], NULL, &teller_thread, (void *)&i);
        /*Locked until newly created thread inform user about its availability*/
        sem_wait(&writer_end);
    }

    /*Create client threads*/
    for(int j = 0; j < clientThreads; j++){
        pthread_create(&cid[j], NULL, &client_thread, (void *)&client[j]);
    }



    /* Join threads to make sure waiting until threads are terminated*/
    for(int i = 0; i < clientThreads; i++) {
        pthread_join(cid[i], NULL);

    }
    for(int j = 0; j < tellerThreads; j++) {
        pthread_join(tid[j], NULL);
    }

    /*Write all clients are taken care of by tellers*/
    w_out<<"All clients received service."<<endl;

    /*Close the outputfile*/
    w_out.close();

    /*Destroy all semaphores*/
    pthread_mutex_destroy(&client_queue);
    pthread_mutex_destroy(&availability);
    pthread_mutex_destroy(&booking);
    pthread_mutex_destroy(&writer);
    pthread_mutex_destroy(&ticket_ready);
    /*Destroy all mutexes*/
    sem_destroy(&writer_end);
    sem_destroy(&empty);
    sem_destroy(&full);

    return 0;
}

void *teller_thread(void *param){
    /*Define output file to write on it*/
    fstream w_out(output_path,ios::out | ios::app);

    /*Take index of teller. 1 corresponds 'A', 2 corresponds 'B', 3 corresponds 'C'.*/
    int teller_index=*(int *)param;

    /*Write teller <name of teller> has arrived. */
    w_out<<"Teller "<<teller[teller_index].name<<" has arrived."<<endl;

    /*Allow main to produce next teller*/
    sem_post(&writer_end);
    //string current_teller_name;

    /*Define client struct here to store information of client, tellers currently dealing with it*/
    struct client_data client_current;

    while (1) {
        /*Define exit condition:
         * teller_start_number holds how many clients take service whether it is successful or not.
         * If one of the teller turn back to the loop with maximum teller_start_number,
         * It means when it equals to client number given by user,
         * It stopped other tellers who are currently available and doesn't take care of any client currently
         * by using pthread_cancel and available threads' id. Because we know ne client cannot come.
         * In the mean time, other unavailable threads, who are sleeping during this period,
         * will reach top of the while loop with maximum teller_start_number,
         * because it is global data, and exit. */

        if (teller_start_number==clientThreads) {
            for (int k=0;k<tellerThreads;k++){
                if(k!=teller_index && teller[k].availabiliy==1){
                    pthread_cancel(tid[k]);
                }
            }
            pthread_exit(NULL);
        }


        /*Define teller and client match:
         * When client is ready after arrival sleep sem full is increased one. And tellers can check their priorities.
         * In order to prevent client confuse between tellers availability mutex is used.
         * When one thread check if it is 'A' and it is available, or if it is 'B' and it is available and 'A' in unavailable.
         * or if it is 'C' and both 'B' and 'A' are unavailable. If all conditions are not satisfies teller turn back to
         * the top of if conditions after increasing sem full to allow other threads to try its chance.
         * */

        here:
        sem_wait(&full);
        pthread_mutex_lock(&availability);

        /*For teller A*/
        if (teller[0].availabiliy == 1 && teller[teller_index].name.compare(teller[0].name)==0 ) {
            teller[0].availabiliy = 0; //change its availability
            client_current = my_queue[teller_start_number]; //take next client in the queue
            teller_start_number=(teller_start_number+1)%MAX_CLIENT; //buffer size arrangement
            //cout<<"I am Teller "<<teller[teller_index].name<< ". I have to wait "<<client_current.waiting_time<<"ms. My client is "<<client_current.name<<endl;
            pthread_mutex_unlock(&availability); //unlock mutex
        }

        /*For teller B*/
        else if ( teller[0].availabiliy == 0 && teller[1].availabiliy == 1 && teller[teller_index].name.compare(teller[1].name)==0) {
            teller[1].availabiliy = 0; //change its availability
            client_current = my_queue[teller_start_number]; //take next client in the queue
            teller_start_number=(teller_start_number+1)%MAX_CLIENT; //buffer size arrangement
            //cout<<"I am Teller "<<teller[teller_index].name<< ". I have to wait "<<client_current.waiting_time<<"ms. My client is "<<client_current.name<<endl;
            pthread_mutex_unlock(&availability);//unlock mutex
        }

        /*For teller C*/
        else if (teller[0].availabiliy == 0 && teller[1].availabiliy == 0 && teller[2].availabiliy == 1 && teller[teller_index].name.compare(teller[2].name)==0) {
            teller[2].availabiliy = 0;//change its availability
            client_current = my_queue[teller_start_number]; //take next client in the queue
            teller_start_number=(teller_start_number+1)%MAX_CLIENT;//buffer size arrangement
            //cout<<"I am Teller "<<teller[teller_index].name<< ". I have to wait "<<client_current.waiting_time<<"ms. My client is "<<client_current.name<<endl;
            pthread_mutex_unlock(&availability); //unlock mutex

        }
        /*Turn back to if conditions to give a trial to other threads*/
        else{
            pthread_mutex_unlock(&availability);
            sem_post(&full);
            goto here;
        }



        /*Define reservation:
         * If client desired seat is empty, book it to the client.
         * Else find seat who has smallest number among the other empty seat.
         * This process made in mutex because we want to prevent any tellers cannot attempt to give same seat to the
         * different clients.
         * reserved_seat holds which seat is booked
         * seat_found holds indicates the client can find an empty seat whether or not it is desired seat.*/
        int reserved_seat;
        pthread_mutex_lock(&booking);
        bool seat_found=false;

        if (seat_status[(client_current.requested_seat) - 1] == 0) {
            seat_status[client_current.requested_seat - 1] = 1; //make not empty to reserved seat
            reserved_seat = client_current.requested_seat;
            seat_found=true;  //client find a seat
        }
        else {
            for (int i = 0; i < current_capacity; i++) {
                if (seat_status[i] == 0) {
                    seat_status[i] = 1;
                    reserved_seat = i + 1;
                    seat_found=true;   //client find a seat
                    break;
                }
            }
        }
        /*Unlock booking mutex. Booking process is done*/
        pthread_mutex_unlock(&booking);

        /*Sleep in the amount of waiting time*/
        usleep(client_current.waiting_time*1000);
        //cout<<teller[teller_index].name<< " finished to sleep."<<endl;

        /*Define writing operation:
         * It should be in mutex to prevent any teller to write the file at the same time
         * It booking operation is successful it means seat_found is equal to true,
         * It prints reservation sear number, client name and teller name.
         * In other case, it cannot find ant seat so it indicates that with writing "None".*/
        pthread_mutex_lock(&writer);
        if (seat_found==true)
            w_out<<client_current.name<<" requests seat "<< client_current.requested_seat<< ", reserves seat "<<reserved_seat<<". Signed by Teller "<<teller[teller_index].name<<"."<<endl;
        else
            w_out<<client_current.name<<" requests seat "<< client_current.requested_seat<< ", reserves None. Signed by Teller "<<teller[teller_index].name<<"."<<endl;

        /*Define client leaving:
         * Put client whose ticket is ready to my_finished_queue to allow them to leave from system
         * finished rank holds the number of client whose job is finished.
         * These process should be in mutex to allow client to leave in ticket preparetion order.*/
        pthread_mutex_lock(&ticket_ready);
        my_finished_queue[finished_rank]=client_current;
        finished_rank = (finished_rank + 1) % MAX_CLIENT;
        pthread_mutex_unlock(&ticket_ready);
        pthread_mutex_unlock(&writer);

        /*Because client can leave the system lets increase empty by one to allow more client can come*/
        sem_post(&empty);

        /*Because teller job is finished make them available*/
        /*For teller A*/
        if (teller[teller_index].name.compare(teller[0].name) == 0) {
            teller[0].availabiliy = 1;
            //cout<< "I am "<<teller[teller_index].name<<", available"<<endl;
        }
        /*For teller B*/
        else if (teller[teller_index].name.compare(teller[1].name) == 0) {
            teller[1].availabiliy = 1;
            //cout<< "I am "<<teller[teller_index].name<<", available"<<endl;

        }
        /*For teller C*/
        else if (teller[teller_index].name.compare(teller[2].name)==0) {
            teller[2].availabiliy = 1;
            //cout<< "I am "<<teller[teller_index].name<<", available"<<endl;
        }
    }
}
void *client_thread(void *param) {
    /*Take current client information */
    struct client_data *client;
    client = (struct client_data *) param;

    /*Sleep this client amount of arrival time */
    usleep(client->arrival_time * 1000);

    /*Check buffer is full or not*/
    sem_wait(&empty);

    /*Prevent any other client will be put to the queue of client who already woke up*/

    pthread_mutex_lock(&client_queue);
    //cout << client->name << " sleeps" << client->arrival_time << "ms" << endl; //Checking which client sleep how much

    /*Put client who has just woke up to the my queue to match with teller*/
    my_queue[my_rank] = *client;

    /*Increase index by one and take mode according to buffer size*/
    my_rank = (my_rank + 1) % MAX_CLIENT;

    /*Incerease full sem to inform tellers one client is readt to take ticket.*/
    sem_post(&full);

    /*Allow other client who woke up to the critical section */
    pthread_mutex_unlock(&client_queue);

    /*During ticket preparation time clients in infinite while loop
     * Until teller put the my_finished queue
     * It means teller slept amount of waiting time and woke up.
     * There is no reason to client to stay because its ticket is ready.
     * When client find itself in the my_finished_queue it leaves from system. */
    while(1) {
        for(int i=0; i<MAX_CLIENT;i++){
            if (strcmp(client->name, (my_finished_queue[i].name))==0){
                //cout<<client->name<< "is dying"<<endl;
                pthread_exit(NULL);
            }
        }
    }
}
