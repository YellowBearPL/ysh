#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

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

char *findInPath(const char *cmd)
{
    char *pathEnv = getenv("PATH");
    if (!pathEnv)
    {
        return NULL;
    }

    char *pathCopy = strdup(pathEnv);
    if (!pathCopy)
    {
        return NULL;
    }

    char *dir = strtok(pathCopy, ":");
    static char fullpath[1024];
    while (dir != NULL)
    {
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, cmd);
        if (access(fullpath, X_OK) == 0)
        {
            free(pathCopy);
            return fullpath;
        }

        dir = strtok(NULL, ":");
    }

    free(pathCopy);
    return NULL;
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

            const char *target = args[1];
            if (isBuiltin(target))
            {
                printf("%s is a shell builtin\n", target);
                continue;
            }

            char *path = findInPath(target);
            if (path)
            {
                printf("%s is %s\n", target, path);
            }
            else
            {
                printf("%s: not found\n", target);
            }

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