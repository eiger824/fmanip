#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "files.h"

void subst_bytes(const char * fname, int val1, int val2, int debug)
{
    FILE * fp = fopen(fname, "rb");
    size_t size;
    char * buffer;
    if (fp)
    {
        size = file_size(fp); 
        if (debug)
            printf("(File contains %lu bytes)\n", size);
        buffer = (char * ) malloc (sizeof *buffer * size);
        fread(buffer, 1, size, fp);

        for (size_t i = 0; i < size; ++i)
        {
            if (buffer[i] == (char) val1)
                buffer[i] = (char) val2;
        }
        // Write back
        fwrite(buffer, 1, size, fp);
        fclose(fp);
    }
}

u8_t get_byte_value(const char* fname, int pos, int debug)
{
    size_t size;
    if (pos < 0) return -1;
    FILE *fp = fopen(fname, "rb");
    if (fp != NULL)
    {
        size = file_size(fp);
        if (debug)
            printf("(File contains %lu bytes)\n", size);

        if (pos > size)
        {
            fprintf(stderr,
                    "Tried to access out-of-bounds byte position (file size is %lu)\n"
                    , size);
            return -1;
        }
        fseek(fp, pos, SEEK_SET);
        u8_t byte;
        fread(&byte, 1, 1, fp);
        fclose(fp);
        return byte;
    }
    else
    {
        return -1;
    }
}

int set_byte_value(const char* fname, int pos, int to, u8_t val, int debug)
{
    size_t size;
    if (pos < 0) return -1;
    if (val > 255) return -2;
    FILE *fp = fopen(fname, "r+b");
    if (fp != NULL)
    {
        size = file_size(fp);
        if (debug)
            printf("(File contains %lu bytes)\n", size);

        if (pos > size)
        {
            fprintf(stderr,
                    "Tried to access out-of-bounds byte position (file size is %lu)\n"
                    , size);
            return -1;
        }
        if (to != -1)
        {
            int performance = 0;
            for (int n = pos; n<to+1; ++n)
            {
                fseek(fp, n, SEEK_SET);

                int r;
                if ((r = fwrite(&val, 1, 1, fp)) > 0)
                {
                    ++performance;               
                }
            }
            //last check
            if (performance == to-pos+1)
            {
                printf("Success! %.2f%% of the specified positions were changed!\n"
                        , (float)((performance / (to-pos+1) * 100)));
                fclose(fp);
                return 0;
            }
            else
            {
                fprintf(stderr,
                        "Error: %.2f%% of the specified positions were changed\n"
                        , (float)((performance / (to-pos+1) * 100)));
                fclose(fp);
                return -1;
            }
        }
        else
        {
            fseek(fp, pos, SEEK_SET);
            int r;
            if ((r = fwrite(&val, 1, 1, fp)) > 0)
            {
                printf("Success! Set the value %d at position %d\n", val, pos);
                fclose(fp);
                return 0;
            }
            else
            {
                fprintf(stderr,"Error writing\n");
                fclose(fp);
                return -1;
            } 
        }
    }
    else
    {
        return -1;
    }
}

int dump(const char* fname, int mark, int range, int byteval)
{
    int width = get_win_width();
    if (width > 100) width = 100;
    FILE *fp = fopen(fname, "rb");
    size_t size = 0;
    int i, start = 0, end = 0;
    u8_t *buffer;
    char color[10];
    if (fp != NULL)
    {
        size = file_size(fp);
        //last check: mark is bigger than file size
        if (mark != -1 && mark > size)
        {
            fprintf(stderr
                    , "Error: input byte position (%d) is greater than size of file (%zu)\n"
                    , mark, size);
            fclose(fp);
            return -1;
        }
        buffer = (u8_t*) malloc((sizeof *buffer) * size);

        size_t result = fread(buffer, 1, size, fp);
        if (size == result)
        {
            for (i=0; i<size; ++i)
            {
                if (i > 0 && i % (width/3 - 7) == 0)
                {
                    end = i-1;
                    printf("%s", NORM);
                    printf(" (bytes %d - %d)\n", start, end);
                    printf("%s", color);
                    start = end+1;
                }
                if (range != -1)
                {
                    printf("%s%s%x%s "
                            ,(i==mark || buffer[i] == byteval)?MARK:""
                            ,(buffer[i]<16)?"0":""
                            ,buffer[i]
                            ,(i==mark+range || buffer[i] == byteval)?NORM:"");
                    if (i==mark) strcpy(color, MARK);
                    if (i==mark+range) strcpy(color, NORM);
                }
                else
                {
                    printf("%s%s%x%s "
                            ,(i==mark || buffer[i] == byteval)?MARK:""
                            ,(buffer[i]<16)?"0":""
                            ,buffer[i]
                            ,(i==mark || buffer[i] == byteval)?NORM:"");
                }
            }
            if (i % (width/3-7) != 0)
            {
                printf("%s", NORM);
                for (unsigned k=(i%(width/3-7))*3; k<width - 3*7-1; k++)
                    printf(" ");
                printf(" (bytes %d - %lu)\n", start, size);
                printf("%s", color);
            }
            else
                printf("\n");

        }
        else
        {
            fprintf(stderr
                    , "Expected and obtained sizes don't match(%lu != %lu)\n"
                    , size, result);

            //free(buffer);
            fclose(fp);
            return -1;
        }
    }
    else
    {
        return -1;
    }

    //free allocated resources
    free(buffer);
    fclose(fp);
    return 0;
}

size_t file_size (FILE * fp)
{
    size_t s;
    if (!fp) return -1;
    fseek(fp, 0L, SEEK_END);
    s = ftell(fp);
    rewind(fp);
    return s;
}
