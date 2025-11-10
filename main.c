#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_ARGS 64
#define MAX_CMD_LEN 1024
#define MAX_HISTORY 1000

char *history[MAX_HISTORY];
int historyCount = 0;

int isBuiltin(const char *cmd)
{
    const char *builtins[] = {"exit", "echo", "type", "pwd", "cd", "history", NULL};
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

void parseCommand(const char *input, char *args[], char **outFile, int *outAppend, char **errFile, int *errAppend)
{
    int i = 0, j = 0;
    int inSingle = 0, inDouble = 0;
    char *arg = malloc(MAX_CMD_LEN);
    int argPos = 0;
    *outFile = NULL;
    *errFile = NULL;
    *outAppend = 0;
    *errAppend = 0;
    while (input[i] != '\0')
    {
        char c = input[i];
        if (c == '\\')
        {
            if (!inSingle && !inDouble && input[i + 1] != '\0')
            {
                arg[argPos++] = input[++i];
            }
            else if (inDouble)
            {
                char next = input[i + 1];
                if (next == '"' || next == '\\')
                {
                    arg[argPos++] = next;
                    i++;
                }
                else
                {
                    arg[argPos++] = '\\';
                }
            }
            else
            {
                arg[argPos++] = '\\';
            }
        }
        else if (c == '\'' && !inDouble)
        {
            inSingle = !inSingle;
        }
        else if (c == '"' && !inSingle)
        {
            inDouble = !inDouble;
        }
        else if (!inSingle && !inDouble && (c == ' ' || c == '\t'))
        {
            if (argPos > 0)
            {
                arg[argPos] = '\0';
                args[j++] = strdup(arg);
                argPos = 0;
            }
        }
        else if (!inSingle && !inDouble && c == '>')
        {
            if (argPos > 0)
            {
                arg[argPos] = '\0';
                args[j++] = strdup(arg);
                argPos = 0;
            }

            int isErr;
            if (j > 0 && (strcmp(args[j - 1], "1") == 0 || (isErr = strcmp(args[j - 1], "2") == 0)))
            {
                j--;
            }

            if (input[i + 1] == '>')
            {
                if (isErr)
                {
                    *errAppend = 1;
                }
                else
                {
                    *outAppend = 1;
                }

                i++;
            }
            else
            {
                if (isErr)
                {
                    *errAppend = 0;
                }
                else
                {
                    *outAppend = 0;
                }
            }

            i++;
            while (input[i] == ' ' || input[i] == '\t')
            {
                i++;
            }

            char filename[MAX_CMD_LEN];
            int k = 0;
            while (input[i] != '\0' && input[i] != ' ' && input[i] != '\t')
            {
                filename[k++] = input[i++];
            }

            filename[k] = '\0';
            *outFile = strdup(filename);

            continue;
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

void addToHistory(const char *cmd)
{
    if (historyCount < MAX_HISTORY)
    {
        history[historyCount++] = strdup(cmd);
    }
    else
    {
        free(history[0]);
        memmove(history, history + 1, sizeof(char *) * (MAX_HISTORY - 1));
        history[MAX_HISTORY - 1] = strdup(cmd);
    }
}

void printHistory()
{
    for (int i = 0; i < historyCount; i++)
    {
        printf("%d %s\n", i + 1, history[i]);
    }
}

int main(void)
{
    char input[MAX_CMD_LEN];
    char *args[MAX_ARGS];
    char *outFile, *errFile;
    int outAppend, errAppend;
    while (1)
    {
        printf("¥ ");
        fflush(stdout);
        if (fgets(input, sizeof(input), stdin) == NULL)
        {
            printf("\n");
            break;
        }

        input[strcspn(input, "\n")] = '\0';
        int i = 0;
        args[i] = NULL;
        parseCommand(input, args, &outFile, &outAppend, &errFile, &errAppend);
        if (strlen(input) == 0)
        {
            continue;
        }

        addToHistory(input);
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

        if (strcmp(args[0], "history") == 0)
        {
            printHistory();
            continue;
        }

        if (strcmp(args[0], "echo") == 0)
        {
            int savedStdout = -1;
            int fd;
            if (outFile) {
                fd = open(outFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd == -1)
                {
                    perror(outFile);
                    continue;
                }

                savedStdout = dup(STDOUT_FILENO);
                dup2(fd, STDOUT_FILENO);
                close(fd);
            }

            for (int k = 1; args[k]; k++)
            {
                printf("%s", args[k]);
                if (args[k + 1])
                {
                    printf(" ");
                }
            }

            printf("\n");
            if (outFile)
            {
                fflush(stdout);
                dup2(savedStdout, STDOUT_FILENO);
                close(savedStdout);
                free(outFile);
                outFile = NULL;
            }

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
            if (outFile)
            {
                int flags = O_WRONLY | O_CREAT | (outAppend ? O_APPEND : O_TRUNC);
                int fdOut = open(outFile, flags, 0644);
                if (fdOut == -1)
                {
                    perror(outFile);
                    exit(1);
                }

                dup2(fdOut, STDOUT_FILENO);
                close(fdOut);
            }

            if (errFile)
            {
                int flags = O_WRONLY | O_CREAT | (errAppend ? O_APPEND : O_TRUNC);
                int fdErr = open(errFile, flags, 0644);
                if (fdErr == -1)
                {
                    perror(errFile);
                    exit(1);
                }

                dup2(fdErr, STDERR_FILENO);
                close(fdErr);
            }

            execvp(path, args);
            perror("exec failed");
            exit(1);
        }
        else if (pid > 0)
        {
            wait(NULL);
            if (outFile)
            {
                free(outFile);
            }

            if (errFile)
            {
                free(errFile);
            }
        }
        else
        {
            perror("fork failed");
        }
    }

    for (int i = 0; i < historyCount; i++)
    {
        free(history[i]);
    }

    return 0;
}