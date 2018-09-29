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

char** getInput(char** history, int* storedCommands, int* ampersand);
char** parseInput(char* args, int* ampersand);
void dispOutput(char** args);
void dispHistory(char** args, int* storedCommands);
void executeCommand(char** args, int* ampersand);
void executeCommandFromHistory(char** args, char** history, int* storedCommands);
int wc(char* inp);
void dispInp(char* inp);
int getStringArrayLength(char** strings);


int main(int argc, char* argv[])
{
    char **args;  // an array of c-strings; holds the current entered command
    int *ampersand = 0; // 1 or 0 to indicate whether the input contained an ampersand
    char **history = (char**)malloc(10*sizeof(char**)); // keep historical commands as c-strings
    int *storedCommands = 0; // tracks historical commands stored. Used as index to history.  
    
    while (TRUE)
    {
        printf("myShell> ");
        fflush(stdout);
        args = getInput(history, storedCommands, ampersand);
        dispOutput(args);
        //if the user has entered "quit" break out of the loop.
        if (strcmp(args[0], "quit") == 0) 
        { 
            free(args);
            free(history);
            return 0;  
        }
        else if (strcmp(args[0], "history") == 0) 
        {
            dispHistory(history, storedCommands);
        }
        else if (strcmp(args[0], "!") == 0) 
        {
            executeCommandFromHistory(args, history, storedCommands);
        }
        else 
        {
            executeCommand(args, ampersand);
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
char** getInput(char** history, int* storedCommands, int* ampersand)
{  
    char* inp = (char*) malloc(MAX_LINE);
    char* start = inp; // keep the start of the string as we take in input
    char c;
    while ((c = getchar()) != '\n')
        *inp++ = c; 
    *inp = '\0'; 
    // if there are &, they are stored in history with the command. Using historical
    // commands, displaying history, and quit are not saved.
    if (start[0] != '!' && strcmp(start, "history") != 0 && strcmp(start, "quit") != 0) 
    {
        if (*storedCommands >= 10) // reached max commands
        {
            printf("There are 10 commands in history. Commands are no longer being stored.\n");
        } 
        else  // store in history normally.
        {
            if (*storedCommands == 9)
                printf("There are now 10 commands in history. Further commands will not be stored.\n");
            history[*storedCommands] = (char*)malloc(strlen(start));
            strcpy(history[*storedCommands], start);
            *storedCommands++;
        }
    }
    return parseInput(start, ampersand);
}

/*
pre: inp is a cstring holding the keyboard input
post: returns an array of cstrings, each holding one of the arguments entered at
the keyboard. The structure is the same as that of argv
Here the user has entered three arguements:
myShell>cp x y
parse int will return args[0] = cp, args[1] = x, args[2] = y
*/
 
char** parseInput(char* inp, int* ampersand)
{
    int length = wc(inp) + 1; 
    if(inp[0] == '!')
        length++;
    char** args = (char**)malloc(length*sizeof(char**)); // just use MAX_LINE? 
    char buff[MAX_LINE]; // buffer to track each word
    int j = 0; // count for buff
    int i = 0; // count for inp
    int count = 0; // count for args
    
    if(inp[0] == '!') // signals that we are referencing commands stored in history
    {
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
        count++;
    }
    else
    {
	    while(i <= strlen(inp)) // go through inp including final null
	    {
	        if(inp[i] == ' ' || inp[i] == '\0') // when we encounter a space or null
	        {
	            buff[j] = '\0'; // end c string in buff with null
		        args[count] = (char*)malloc(strlen(buff)*sizeof(char*)); // allocate space to args
		        j = 0;
		        strcpy(args[count], buff); // copy buff(the buffer containing the current word) to args
		        // printf("%d", count);
		        count++;
	        } 
	        else 
	        {
                if (inp[i] == '&')
                    *ampersand = 1;  // remember that there is an ampersand
                else 
                {
                    buff[j] = inp[i]; // inp[i] is normal store it in buff
                    j++; // increment buff's index counter
                }
            }
            i++; // increment inp's input counter
        }
    }
    args[count] = NULL;
    return args;
} 

 
/*
pre: takes an array of c strings and a number of how many strings in args
Displays the arguments entered by the user and parsed by getInput
*/
void dispOutput(char** args)
{
    int i;
    int length = getStringArrayLength(args);
    for (i = 0; i < length; i++)
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
void dispHistory(char** history, int* storedCommands)
{
    
    int i;
    for (i = 0; i < *storedCommands; i++) 
    {
        dispInp(history[i]);
    }
}

/*
pre: args is an array where structure is the same as that of argv
post: executes the command given by array args in a child process
*/
void executeCommand(char** args, int* ampersand)
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
             if (*ampersand == 1)
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
void executeCommandFromHistory(char** args, char** history, int* storedCommands) 
{
    int *ampersand = 0;
    if (strcmp(args[1], "!") == 0) 
    {
        // printf("Executing the most recent command\n");
        executeCommand(parseInput(history[*storedCommands - 1], ampersand), ampersand);
    }
    else 
    {
        int whichCommand = atoi(args[1]) - 1; // 1st command is slot 0, 1 indexed
        if (whichCommand < 0 || whichCommand > 9) // filter bad input
            printf("Invalid Command. Please enter a number between 1 and 10. \n");
        else 
        {
            // printf("Executing command %d\n", whichCommand);
            if (whichCommand >= *storedCommands) // filter bad input
                printf("There are not sufficient commands in history");
            else
                executeCommand(parseInput(history[whichCommand], ampersand), ampersand);    
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

int getStringArrayLength(char** strings) {
    int i = 0;
    while (strings[i] != NULL) 
    {
        i++;
    }
    return i;
}
