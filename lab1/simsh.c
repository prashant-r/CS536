// simple shell example using fork() and execlp()

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
pid_t k;
char buf[100];
int status;
int len;

while(1) {

        // print prompt
        fprintf(stdout,"[%d]$ ",getpid());

        // read command from stdin into the buf character array
        fgets(buf, 100, stdin);
        // if only a single character was read then its the return key being pressed
        len = strlen(buf);
        //since no characters were populated in the buffer and we by default have first character to be the null
        //string termination character so the size would be 1 for an empty string.
        if(len == 1)                            // only return key pressed
          continue;
        // set the second to last character to be the null termination sequence for string
        buf[len-1] = '\0';
        // starts a new child and set k to be pid of newly started child process
        k = fork();
        if (k==0) {
        // child code if k == 0
        // go execute the command on child
          if(execlp(buf,buf,NULL) == -1)        // if execution failed, terminate child
                exit(1);
        }
        else {
        // parent code if k > 0
          waitpid(k, &status, 0);
         // wait until child has terminated and then remove from proctab the zombie process.
         // if k == -1 then the parent may wait indefinitely on a nonexistent child process {BUG 2}.
        }
  }
}
