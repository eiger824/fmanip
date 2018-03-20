#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <getopt.h>
#include <sys/ioctl.h>

#include "strops.h"
#include "defs.h"
#include "files.h"

int debug = 0;

void help()
{
    printf("USAGE: fmanip [dh[G|g|s]v] f [i]\n");
    printf("[args]\n");
    printf("-d\t\t\tTurn on debug mode\n");
    printf("-g nbyte\t\tObtain byte value at position \"nbyte\" ( >0 )\n");
    printf("-G value\t\tPrint all bytes matching \"value\"\n");
    printf("        \t\t(For \\0, just input 00 as argument)\n");
    printf("-h\t\t\tShow help\n");
    printf("-i\t\t\tIf -f was selected, insert a sentence at a given line\n");
    printf("\t\t\t(interactively)\n");
    printf("-s nbyte[-nbyte2]:value Set \"value\" in position \"nbyte\" or\n");
    printf("\t\t\tbetween range \"nbyte\" - \"nbyte2\" (\"nbyte*\" >0)\n");
    printf("-S byteval1:byteval2    Substitute byte byteval1 with byteval2\n");
    printf("-v\t\t\tShow version\n");
}

void version()
{
    printf("Version 0.2, by Santiago Pagola\n");
}

int get_win_width()
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    if (w.ws_col) return w.ws_col;
    else return -1;
}

void show_progress(int current, int total)
{
    int i,j,c_limit,width;
    width = get_win_width() - 9;
    printf("\r");
    fflush(stdout);
    printf("[");
    c_limit = current * width / total;
    for (i=0; i<c_limit; ++i)
    {
        printf("#");
    }
    for (j=c_limit; j<width; ++j)
    {
        printf("-");
    }
    printf("] %.2f%%", (float)current/total * 100);
}

int main(int argc, char* argv[])
{
    char *arg;
    int c, bn = -1, pos= -1, to = -1, val = -1, byteval = -1, err = -1;
    int sentence = -1, with_range = -1;

    char **files = (char ** ) malloc (sizeof *files * MAX_FNAMES); // 10 ptrs to MAX_FNAMES fnames

    while ((c = getopt(argc, argv, "dg:G:s:S:hiv")) != -1)
    {
        switch(c)
        {
            case 'd':
                debug=1;
                break;
            case 'g':
                if (pos >= 0)
                {
                    fprintf(stderr
                            , "You either set or get stuff. Don't do both\n");
                    help();
                    return -1;
                }
                bn = atoi(optarg);
                if (bn == 0 && strlen(optarg) > 1)
                {
                    fprintf(stderr, "Invalid byte number \"%s\"\n", optarg);
                    help();
                    return -1;
                }
                break;
            case 'G':
                if (pos >= 0)
                {
                    fprintf(stderr
                            , "You either set of get stuff. Don't do both\n");
                    help();
                    return -1;
                }
                //check if input was given as a simple char or an integer
                if (strlen(optarg) == 1 && (optarg[0] > 32 && optarg[0] < 127))
                {
                    byteval = optarg[0];
                }
                else
                {
                    byteval = atoi(optarg);
                }
                //check validity
                if (byteval < 0 || byteval > 255)
                {
                    fprintf(stderr, "Invalid byte value. Must be between 0 and 255\n");
                    help();
                    return -1;
                }

                if (byteval > 32)
                {
                    printf("Looking for occurrences of '%c'\n", byteval);
                }
                else
                {
                    printf("Looking for occurrences of '\\%d'\n", byteval);
                }

                break;
            case 'h':
                help();
                return 0;
            case 'i':
                sentence = 1;
                break;
            case 's':
                if (bn >= 0)
                {
                    fprintf(stderr
                            , "You either set or get stuff. Don't do both\n");
                    help();
                    return -1;
                }
                arg = optarg;
                char *p,*p2,*v;
                char *cl = strchr(arg, ':');

                if (cl != NULL)
                {
                    //check for ':'
                    if (cl-arg == 0 || arg[strlen(arg)-1] == ':')
                    {
                        fprintf(stderr
                                , "Wrong format. Format must be position[-range]:value\n");
                        help();
                        return -1;
                    }

                    //check for '-' (opt)
                    char *r = strchr(arg, '-');
                    if (r != NULL)
                    {
                        if (r-arg == 0)
                        {
                            fprintf(stderr
                                    , "Wrong format. Format must be position[-range]:value\n");
                            help();
                            return -1; 
                        }
                        else if (cl - r == 1)
                        {
                            fprintf(stderr
                                    , "Wrong format. Format must be position[-range]:value\n");
                            help();
                            return -1;
                        }
                        p = (char*) malloc((sizeof *p) * 100);
                        p2 = (char*) malloc((sizeof *p2) * 100);
                        v = (char*) malloc((sizeof *v) * 100);

                        memcpy(p, arg, r-arg);
                        p[r-arg] = '\0';
                        memcpy(p2, arg+(r-arg)+1, (cl-r)-1);
                        p2[(cl-r)-1] = '\0';
                        memcpy(v, cl+1, strlen(arg)-(cl-arg)-1);
                        v[strlen(arg)-(cl-arg)-1] = '\0';

                        pos = atoi(p);
                        to = atoi(p2);
                        val = atoi(v);

                        //last checks
                        if (pos < 0)
                        {
                            fprintf(stderr
                                    , "Invalid byte number: %d\n", pos);
                            help();
                            free(p);
                            free(p2);
                            free(v);
                            return -1;
                        }
                        else if (to < pos)
                        {
                            fprintf(stderr
                                    , "Error: value of position 1 must be smaller or equal\n");
                            fprintf(stderr, "than value of position 2\n");
                            help();
                            free(p);
                            free(p2);
                            free(v);
                            return -1;
                        }
                        if (val < 0 || val > 255)
                        {
                            fprintf(stderr
                                    , "Invalid byte value: %d\n", val);
                            help();
                            free(p);
                            free(p2);
                            free(v);
                            return -1;
                        }
                        with_range = to - pos;

                        free(p);
                        free(p2);
                        free(v);
                    }
                    else
                    {
                        //no range was specified, single position
                        p = (char*) malloc((sizeof *p) * 100);
                        v = (char*) malloc((sizeof *v) * 100);

                        memcpy(p, arg, cl-arg);
                        memcpy(v, arg+(cl-arg+1),strlen(arg)-(cl-arg)-1);

                        pos = atoi(p);
                        val = atoi(v);
                        if (pos < 0)
                        {
                            fprintf(stderr
                                    , "Invalid byte number: %d\n", pos);
                            help();
                            free(p);
                            free(v);
                            return -1;
                        }
                        free(p);
                        free(v);
                    }
                }
                else
                {
                    fprintf(stderr
                            , "Wrong format. Format must be position[-range]:value\n");
                    help();
                    return -1;
                }
                break;
            case 'S':
                if (bn >= 0)
                {
                    fprintf(stderr
                            , "You either set or get stuff. Don't do both\n");
                    help();
                    return -1;
                }
                arg = optarg;
                char byte[3];
                char * byteval1 = arg;
                char * byteval2 = strchr(byteval1, ':');
                /* TODO: Check input format ...*/
                size_t diff = byteval2 - byteval1;
                memcpy(byte, byteval1, diff);
                byte[diff] = '\0';
                int val1 = atoi(byte);
                byteval2++;
                diff = byteval1 + strlen(optarg) - byteval2;
                memcpy(byte, byteval2, diff);
                byte[diff] = '\0';
                int val2 = atoi(byte);
                subst_bytes("foo", val1, val2);
                return 0;
            case 'v':
                version();
                return 0;
            default:
                help();
                return -1;
        }
    }

    if (optind == argc)
    {
        fputs("Provide at least one file name\n", stderr);
        return 1;
    }

    if (argc - optind > MAX_FNAMES)
    {
        fprintf(stderr, "Only a maximum of %d files allowed\n", MAX_FNAMES);
        return 1;
    }
    int j = 0;
    int index;
    for (index = optind; index < argc; ++index)
    {
        *(files + j++) = argv[index];
    }
    *(files + j) = NULL;

    char * file;
    // Loop through the files and process them!
    for (unsigned i = 0; *(files + i) != NULL; ++i)
    {
        file = *(files + i);
        if (debug)
            printf("Will open file:\t%s (%p)\n", *(files + i),(void*) *(files + i));

        if (debug)
            printf("Attempting to read from \"%s\"\n", file);

        if (bn != -1)
        {
            err = dump(file, bn, with_range, -1);
            if (debug)
                printf("The byte value at position %d is: %x\n"
                        , bn, get_byte_value(file, bn, debug));
            return err;
        }
        else if (pos >= 0)
        {
            if (set_byte_value(file, pos, to, val, debug) == 0)
            {
                err = dump(file, pos, with_range, -1);
                return err;
            }
            else
            {
                fprintf(stderr, "Error setting byte value\n");
                return -1;
            }
        }
        else if (sentence == 1)
        {
            char *line = NULL;
            size_t size;

            printf("Enter a sentence (hit Enter to finish): ");
            if (getline (&line, &size, stdin) == -1)
            {
                fprintf(stderr, "Error parsing line\n");
                return 1;
            }

            FILE *fp = fopen(file, "r");
            if (fp == NULL)
            {
                perror("Error opening");
                free(line);
                return 1;
            }
            fseek(fp, 0 , SEEK_END);
            long lsize = ftell(fp);
            rewind(fp);

            //read
            char *buffer = (char*) malloc ((sizeof *buffer) * (lsize + 1));
            long n = fread(buffer, 1, lsize, fp);
            buffer[lsize] = '\0';
            if (n != lsize)
            {
                perror ("Reading error");
                free(buffer);
                free(line);
                return 1;
            }
            printf("Got %zu bytes from file\n", n);

            //reopen file with write permissions
            FILE *fpn = freopen(file, "w", fp);
            if (fpn == NULL)
            {
                perror("Error reopening");
                free(buffer);
                free(line);
                return 1;
            }

            unsigned nolines = strnrchr(buffer, '\n');
            int linr = -10;
            // ask position to insert
            //do
            while ( (linr != -1 && linr <= 0) || (linr > nolines + 1) )
            {
                printf("Provide a line where your sentence should be inserted\n");
                printf("(Accepted values between %d and %d): ", 1, nolines + 1);
                scanf(" %d", &linr);

                if (linr < 0 || linr > nolines + 1)
                {
                    fprintf(stderr, "Wrong index '%d'\n", linr);
                }
            }
            // Allocate big buffer
            char *s = (char*) malloc( (sizeof *s) * ( n + strlen(line) + 1 ) );

            // Insert our line into the buffer
            int res = strinsrt (s, buffer, strlen(buffer), line, linr);

            size_t nb = fwrite (s, 1, res, fpn);
            printf("Wrote %zu bytes to \"%s\"\n", nb, file);

            // free resources
            fclose(fpn);
            free(buffer);
            free(line);
            free(s);

            // update error code
            err = (nb > 0) ? 0 : 1;
        }
        else if (byteval != -1)
        {
            err = dump(file, -1, with_range, byteval);
        }

    }
    return err;
}
