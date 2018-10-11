#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void parent(int time_crit_sect, int time_non_crit_sect, char* turn, char* parent, char* child);
void child(int time_crit_sect, int time_non_crit_sect, char* turn, char* parent, char* child);
void cs(char process, int time_crit_sect);
void non_cs(int time_non_crit_sect);

void main(int argc, char* argv[])
{
    int time_parent, time_child, time_parent_non_cs, time_child_non_cs;
    int shmid_turn, shmid_parent, shmid_child;
    char* turn;
    char* parent_address;
    char* child_address;
    int pid;
    
    //check for proper arguments
    if (argc == 1) 
    {
        printf("No arguments were entered. Proceeding with default times\n");
        time_parent = 3;
        time_child = 1;
        time_parent_non_cs = 5; 
        time_child_non_cs = 2;
    }
    else if (argc != 5) 
    {
        fprintf(stderr, "Error: There should be 4 numbers entered. Aborting...\n");
        fprintf(stderr, "Usage: proj5 time_parent time_child time_parent_non_cs time_child_non_cs\n\n"); 
        exit(EXIT_FAILURE);
    }
    else
    {
        time_parent = atoi(argv[1]);
        time_child = atoi(argv[2]);
        time_parent_non_cs = atoi(argv[3]);
        time_child_non_cs = atoi(argv[4]);
    }

    shmid_turn = shmget(0,1,0777 | IPC_CREAT);
    turn = (char*)shmat(shmid_turn,0,0); 
    
    shmid_parent = shmget(0,1,0777 | IPC_CREAT);
    parent_address = (char*)shmat(shmid_parent,0,0);  
    
    shmid_child = shmget(0,1,0777 | IPC_CREAT);
    child_address = (char*)shmat(shmid_child,0,0); 
    
    *parent_address = '0';
    *child_address = '0';

    pid = fork();
    if (pid < 0)  //fork failed
    {
        fprintf(stderr, "Fork Error");
    } 
    else
    if (pid == 0)  //child process
        child(time_child, time_child_non_cs, turn, parent_address, child_address);
    else           //parent process
    {
        parent(time_parent, time_parent_non_cs, turn, parent_address, child_address);
        wait(NULL);
        shmctl(shmid_turn,IPC_RMID,0);
        shmctl(shmid_parent,IPC_RMID,0);
        shmctl(shmid_child,IPC_RMID,0);
    }
}

void parent(int time_crit_sect, int time_non_crit_sect, char* turn, char* parent, char* child)
{
    for (int i = 0; i < 10; i++)
    {
        *parent = '1';
        *turn = 'c';
        while(*child == '1' && *turn == 'c');
        cs('p', time_crit_sect);
        *parent = '0';
        non_cs(time_non_crit_sect); 
    }
    shmdt(turn);
    shmdt(parent);
    shmdt(child);
}

void child(int time_crit_sect, int time_non_crit_sect, char* turn, char* parent, char* child)
{
    for (int i = 0; i < 10; i++)
    {
        *child = '1';
        *turn = 'p';
        while(*parent == '1' && *turn == 'p');
        cs('c', time_crit_sect);
        *child = '0';
        non_cs(time_non_crit_sect); 
    }
    shmdt(turn);
    shmdt(parent);
    shmdt(child);
}

void cs(char process, int time_crit_sect)
{
    if (process == 'p')
    {
        printf("parent in critical section\n");
        sleep(time_crit_sect);
        printf("parent leaving critical section\n");
    }
    else
    {
        printf("child in critical section\n");
        sleep(time_crit_sect);
        printf("child leaving critical section\n");
    }
}

void non_cs(int time_non_crit_sect)
{
    sleep(time_non_crit_sect);
}
