#include <stdio.h>
#include <string.h>

int main(void)
{
    char command[1024];
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
        if (strcmp(command, "exit") == 0)
        {
            break;
        }

        printf("You entered: %s\n", command);
    }

    return 0;
}