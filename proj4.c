/*
Class: CPSC 346-01
Team Member 1: Alexa Andrews
Team Member 2: N/A 
GU Username of project lead: aandrews4
Pgm Name: proj4.c 
Pgm Desc: basic shell with history functionality 
*/

#include <stdio.h>
#include <unistd.h> 
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

#define MAX_LINE 80
#define TRUE 1

char** getInput();
char** parseInput(char*);
void dispOutput(char**, int num_strings);
void dispHistory();
void executeCommand(char** args);
void executeCommandFromHistory(char** args);
int wc(char* inp);
void dispInp(char* out);

int num_inputs; // track how many words in the input for dispOutput
int ampersand; // 1 or 0 to indicate whether the input contained an ampersand
char **history; // keep historical commands as c-strings
int stored_commands; // tracks historical commands stored. Used as index to history.

int main(int argc, char* argv[])
{
    //A pointer to an array of pointers to char.  In essence, an array of 
    //arrays.  Each of the second level arrays is a c-string. These hold
    //the command line arguments entered by the user.
    //as exactly the same structure as char* argv[]
    char **args;  
    ampersand = 0;
    history = (char**)malloc(10*sizeof(char**));
    stored_commands = 0; 

    while (TRUE)
    {
        // printf("Stored commands: %d\n", stored_commands); 
        printf("myShell> ");
        fflush(stdout);
        args = getInput();
        dispOutput(args, num_inputs);
        //if the user has entered "quit" break out of the loop.
        if (strcmp(args[0], "quit") == 0) 
        { // this was originally args[1]??
            free(args);
            free(history);
            return 0;  // break didn't work
        }
        else if (strcmp(args[0], "history") == 0) 
        {
            dispHistory();
        }
        else if (strcmp(args[0], "!") == 0) 
        {
            executeCommandFromHistory(args);
        }
        else 
        {
            executeCommand(args);
        }
    }
    free(args);
    free(history);
    return 0;
}


/*
pre: none
post: Reads input string from the key board.   
invokes parseInput and returns the parsed input to main
input string is stored in history if it is neither a call to 
a previous command in history, nor a call to display the history, 
nor a quit command
*/
char** getInput()
{  
    char* inp = (char*) malloc(MAX_LINE);
    char* start = inp; // keep the start of the string as we take in input
    char c;
    while ((c = getchar()) != '\n')
        *inp++ = c; 
    *inp = '\0'; 
    num_inputs = wc(start); // this will be needed for parseInput and dispOutput
    if (start[0] != '!' && strcmp(start, "history") != 0 && strcmp(start, "quit") != 0) 
    {
        if (stored_commands >= 10) 
        {
            printf("There are 10 commands in history. Further commands are not stored.\n");
        } 
        else  // store in history
        {
            history[stored_commands] = (char*)malloc(strlen(start));
            strcpy(history[stored_commands], start);
            stored_commands++;
        }
    }
    return parseInput(start);
}

/*
pre: inp is a cstring holding the keyboard input
post: returns an array of cstrings, each holding one of the arguments entered at
the keyboard. The structure is the same as that of argv
Here the user has entered three arguements:
myShell>cp x y
parse int will return args[0] = cp, args[1] = x, args[2] = y
*/
 
char** parseInput(char* inp)
{
    char** args = (char**)malloc(num_inputs*sizeof(char**)); // just use MAX_LINE? 
    char buff[MAX_LINE]; // buffer to track each word
    int j = 0; // count for buff
    int i = 0; // count for inp
    int count = 0; // count for args
    
    if(inp[0] == '!') // signals that we are referencing commands stored in history
    {
        num_inputs++;
        buff[0] = '!'; // ! is index 0 to easily check this is a historical command
        buff[1] = '\0';
        args[count] = (char*)malloc(2*sizeof(char*));
        strcpy(args[count], buff);
        count++;
        buff[0] = inp[1]; // either another ! or the numbered command
        if (strlen(inp) > 2)  
        { // if executing the 10th command
            buff[1] = inp[2];
            buff[2] = '\0';
        }
        else
            buff[1] = '\0';    
        args[count] = (char*)malloc(strlen(buff)*sizeof(char*));
        strcpy(args[count], buff);
    }
    else
    {
	    while(i <= strlen(inp)) 
	    {
	        if(inp[i] == ' ' || inp[i] == '\0') 
	        {
	            buff[j] = '\0';
		        args[count] = (char*)malloc(strlen(buff)*sizeof(char*));
		        j = 0;
		        strcpy(args[count], buff);
		        count++;
	        } 
	        else 
	        {
                if (inp[i] == '&')
                    ampersand = 1;
                else 
                {
                    buff[j] = inp[i];
                    j++;
                }
            }
            i++;
        }
    }
    
    return args;
} 

 
/*
pre: takes an array of c strings and a number of how many strings 
Displays the arguments entered by the user and parsed by getInput
*/
void dispOutput(char** args, int num_strings)
{
    int i;
    for (i = 0; i < num_strings; i++)
    {
        printf("%s ", args[i]);
    }
    printf("\n");
}

/*
pre: none
post: displays the history of commands the user has entered. If the user has not previously
entered a command, nothing will be displayed
*/
void dispHistory()
{
    
    int i;
    for (i = 0; i < stored_commands; i++) 
    {
        dispInp(history[i]);
    }
}

/*
pre: args is an array where structure is the same as that of argv
post: executes the command given by array args in a child process
*/
void executeCommand(char** args)
{
    pid_t pid;
   
    pid = fork();
   
    if (pid < 0)  //fork failed
    {
        fprintf(stderr, "Fork Error");
    } 
    else
    {
         if (pid == 0)  //child process
             execvp(args[0], args);  //load args[0] into the child's address space 
         else    //parent process
         {        
             if (ampersand == 0)
                 wait(NULL);
         }
     }
}


/*
pre: args is an array of c strings with args[0] = ! and 
args[1] = ! OR args[1] = ## where ## is which command in history
the user wishes to run, 1 indexed for human usability 
post: the requested command from history is run
If the number entered is not within the range of commands
stored in history, an appropriate message is displayed
*/
void executeCommandFromHistory(char** args) 
{
    if (strcmp(args[1], "!") == 0) 
    {
        executeCommand(parseInput(history[stored_commands - 1]));
    }
    else 
    {
        int whichCommand = atoi(args[1]) - 1; // 1st command is slot 0, 1 indexed
        if (whichCommand < 0 || whichCommand > 9) // filter bad input
            printf("Invalid Command. Please enter a number between 1 and 10. \n");
        else 
        {
            if (whichCommand >= stored_commands) // filter bad input
                printf("There are not sufficient commands in history");
            else
                executeCommand(parseInput(history[whichCommand]));    
        }
    }
}

/*
pre: inp is a c string 
post: returns the word count of inp
*/
int wc(char* inp)
{
	int count = 0;
	while(*inp == ' ')  // ignore leading spaces or new lines
		inp++;
	while(*inp != '\0') 
	{
		if(*inp == ' ') 
		{
			count ++;
			while(*inp == ' ')   // ignore further spaces or new lines
				inp++;
		} 
		else inp++;
	}
	inp--;
	if (*inp != ' ')  // if the last char isn't a space, it is a word, so increment count
		count++;
	return count;
}

/*
pre: out is a c string 
post: prints the contents of out to the terminal followed by a newline
*/
void dispInp(char* out)
{
 while(*out)  //continue until the null character is encountered
  putchar(*out++);
 putchar('\n');
}
  
