#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    char command[1024];
    char *args[64];
    while (1)
    {
        printf("¥ ");
        fflush(stdout);
        if (fgets(command, sizeof(command), stdin) == NULL)
        {
            printf("\n");
            break;
        }

        command[strcspn(command, "\n")] = '\0';
        int i = 0;
        args[i] = strtok(command, " ");
        while (args[i] != NULL && i < 63)
        {
            args[++i] = strtok(NULL, " ");
        }

        args[i] = NULL;
        if (args[0] == NULL)
        {
            continue;
        }

        if (strcmp(args[0], "exit") == 0)
        {
            int status = 0;
            if (args[1])
            {
                status = (int)strtol(args[1], NULL, 0);
            }

            exit(status);
        }

        if (strcmp(args[0], "echo") == 0)
        {
            for (int j = 1; args[j] != NULL; j++)
            {
                printf("%s", args[j]);
                if (args[j + 1] != NULL)
                {
                    printf(" ");
                }
            }

            printf("\n");
            continue;
        }

        pid_t pid = fork();
        if (pid == 0)
        {
            execvp(args[0], args);
            perror("command not found");
            exit(1);
        }
        else if (pid > 0)
        {
            wait(NULL);
        }
        else
        {
            perror("fork failed");
        }
    }

    return 0;
}