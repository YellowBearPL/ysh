#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int isBuiltin(const char *cmd)
{
    const char *builtins[] = {"exit", "echo", "type", NULL};
    for (int i = 0; builtins[i] != NULL; i++)
    {
        if (strcmp(cmd, builtins[i]) == 0)
        {
            return 1;
        }
    }

    return 0;
}

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

        if (strcmp(args[0], "type") == 0)
        {
            if (args[1] == NULL)
            {
                fprintf(stderr, "type: missing argument\n");
                continue;
            }

            if (isBuiltin(args[1]))
            {
                printf("%s is a shell builtin\n", args[1]);
            }
            else
            {
                printf("%s: not found\n", args[1]);
            }

            continue;
        }

        printf("%s: not found\n", args[0]);
    }

    return 0;
}