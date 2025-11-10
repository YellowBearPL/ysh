#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_ARGS 64
#define MAX_CMD_LEN 1024

int isBuiltin(const char *cmd)
{
    const char *builtins[] = {"exit", "echo", "type", "pwd", "cd", NULL};
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

    static char fullpath[1024];
    char *dir = strtok(pathCopy, ":");
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

void parseCommand(const char *input, char *args[])
{
    int i = 0, j = 0;
    int inSingle = 0, inDouble = 0;
    char *arg = malloc(MAX_CMD_LEN);
    int argPos = 0;
    while (input[i] != '\0')
    {
        char c = input[i];
        if (c == '\'' && !inDouble)
        {
            inSingle = !inSingle;
        }
        else if (c == '"' && !inSingle)
        {
            inDouble = !inDouble;
        }
        else if (!inSingle && !inDouble  && (c == ' ' || c == '\t'))
        {
            if (argPos > 0)
            {
                arg[argPos] = '\0';
                args[j++] = strdup(arg);
                argPos = 0;
            }
        }
        else
        {
            arg[argPos++] = c;
        }

        i++;
    }

    if (argPos > 0)
    {
        arg[argPos] = '\0';
        args[j++] = strdup(arg);
    }

    args[j] = NULL;
    free(arg);
}

int main(void)
{
    char command[MAX_CMD_LEN];
    char *args[MAX_ARGS];
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
        args[i] = NULL;
        parseCommand(command, args);
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
            for (int k = 1; args[k] != NULL; k++)
            {
                printf("%s", args[k]);
                if (args[k + 1] != NULL)
                {
                    printf(" ");
                }
            }

            printf("\n");
            continue;
        }

        if (strcmp(args[0], "pwd") == 0)
        {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL)
            {
                printf("%s\n", cwd);
            }
            else
            {
                perror("pwd");
            }

            continue;
        }

        if (strcmp(args[0], "cd") == 0)
        {
            char *target = args[1];
            if (target == NULL || strcmp(target, "~") == 0)
            {
                target = getenv("HOME");
                if (target == NULL)
                {
                    fprintf(stderr, "cd: HOME not set\n");
                    continue;
                }
            }
            else if (target[0] == '~')
            {
                char *home = getenv("HOME");
                if (home == NULL)
                {
                    fprintf(stderr, "cd: HOME not set\n");
                    continue;
                }

                static char expanded[1024];
                snprintf(expanded, sizeof(expanded), "%s%s", home, target + 1);
                target = expanded;
            }

            if (chdir(target) != 0)
            {
                perror("cd");
            }

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

        char *path = findInPath(args[0]);
        if (!path)
        {
            printf("%s: not found\n", args[0]);
            continue;
        }

        pid_t pid = fork();
        if (pid == 0)
        {
            execvp(path, args);
            perror("exec failed");
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