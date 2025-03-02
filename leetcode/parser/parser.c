#include <math.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 1024

int main(int argc, const char *argv[])
{

    if (argc > 1)
    {
        FILE *stream = NULL;

        errno_t err = fopen_s(&stream, argv[1], "r");
        if (err != 0)
        {
            fprintf(stderr, "Couldn't open file: %s, error: %d\n", argv[1], err);
            return -1;
        }

        size_t num_read = 0;
        int count = 0;

        const char *sig = "void test";
        const int sig_len = strnlen(sig, 64);

        do
        {
            char buffer[BUFFER_SIZE];
            num_read = fread_s(buffer, BUFFER_SIZE, 1, BUFFER_SIZE - 1, stream);
            buffer[num_read] = '\0';

            for (int i = 0; i < num_read - sig_len + 1; i++)
            {
                if (buffer[i] == 'v')
                {
                    if (strncmp(buffer + i, sig, sig_len) == 0)
                    {
                        i += sig_len - 1;
                        count++;
                    }
                }
            }
        } while (num_read == BUFFER_SIZE - 1);

        err = fclose(stream);
        if (err != 0)
        {
            fprintf(stderr, "Couldn't close file: %s, error: %d\n", argv[1], err);
            return -1;
        }

        if (count > 0)
        {
            for (int i = 1; i < count; i++)
            {
                printf("%d;", i);
            }
            printf("%d", count);
        }
    }
    else
    {
        printf("USAGE: parse <FILENAME>, missing FILENAME\n");
    }

    return 0;
}
