#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/ioctl.h>

#include "strops.h"

#define NORM "\x1B[0m"
#define MARK "\x1B[31m"

typedef unsigned char u8_t;

int debug = 0;

void print(char *p, char *line)
{
   if (line == NULL)
   {
      printf("Text (length=%zu)\n---------------\n", strlen(p));
      printf("%s\n---------------\n", p);
   }
   else
   {
      char *substr = (char*) malloc((sizeof *substr) * 1000);
      memcpy(substr, line, strlen(line));
      substr[strlen(line) - 1] = '\0';
      
      char *s = strstr(p, substr);
      if (s != NULL)
      {
         int where = s - p;
         printf("New text, length:%zu (%zu + %zu)\n",
                strlen(p), strlen(p) - strlen(line), strlen(line));
         printf("------------------------------\n");
         for (unsigned i=0; i<strlen(p); ++i)
         {
            if (i == where) printf("%s", MARK);
            if (i == where + strlen(line)) printf("%s", NORM);
            printf("%c", p[i]);
         }
         //ensure to get back to normal color
         printf("%s", NORM);
         printf("\n------------------------------\n");
      }
      else
      {
         printf("New text, length=%zu (%zu + %zu)\n",
                strlen(p), strlen(p) - strlen(line), strlen(line));
         printf("------------------------------\n%s\n", p);
         printf("------------------------------\n");
      }

      free(substr);
   }
}

void help()
{
   printf("USAGE: ./process-file [dh[g|s]v] f [i]\n");
   printf("[args]\n");
   printf("-d\t\tTurn on debug mode\n");
   printf("-f file\t\tRead from file\n");
   printf("-g nbyte\tObtain byte value at position \"nbyte\" ( >0 )\n");
   printf("-h\t\tShow help\n");
   printf("-i\t\tIf -f was selected, insert a sentence at a given line (interactively)\n");
   printf("-s nbyte:value  Set \"value\" in position \"nbyte\" (\"nbyte\" >0)\n");
   printf("-v\t\tShow version\n");
}

void version()
{
   printf("Version 0.1, by Santiago Pagola\n");
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
   width = get_win_width()-9;
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

u8_t get_byte_value(const char* fname, int pos)
{
   if (pos < 0) return -1;
   FILE *fp = fopen(fname, "rb");
   if (fp != NULL)
   {
      fseek(fp, 0, SEEK_END);
      long size = ftell(fp);
      if (debug)
         printf("(File contains %lu bytes)\n", size);
      rewind (fp);

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

int set_byte_value(const char* fname, int pos, u8_t val)
{
   if (pos < 1) return -1;
   if (val > 255) return -2;
   printf("Setting %x to pos %d\n", val, pos);
   FILE *fp = fopen(fname, "r+b");
   if (fp != NULL)
   {
      fseek(fp, 0, SEEK_END);
      long size = ftell(fp);
      if (debug)
         printf("(File contains %lu bytes)\n", size);
      rewind (fp);

      if (pos > size)
      {
         fprintf(stderr,
                 "Tried to access out-of-bounds byte position (file size is %lu)\n"
                 , size);
         return -1;
      }
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
   else
   {
      return -1;
   }
}

int dump(const char* fname, int mark)
{
   int width = get_win_width();
   if (width > 100) width = 100;
   FILE *fp = fopen(fname, "rb");
   long size;
   int i, start = 0, end = 0;
   u8_t *buffer, *buffer_cpy;
   if (fp != NULL)
   {
      fseek(fp, 0, SEEK_END);
      size = ftell(fp);
      rewind(fp);
      buffer = (u8_t*) malloc((sizeof *buffer) * size);
      buffer_cpy = buffer;

      size_t result = fread(buffer, 1, size, fp);
      if (size == result)
      {
         for (i=0; i<size; ++i)
         {
            if (i > 0 && i % (width/3 - 7) == 0)
            {
               end = i;
               printf(" (bytes %d - %d)\n", start, end);
               start = end+1;
            }
            printf("%s%s%x%s "
                   ,(i==mark)?MARK:""
                   ,(buffer[i]<16)?"0":""
                   ,buffer[i]
                   ,(i==mark)?NORM:"");

         }
         if (i % (width/3-7) != 0)
         {
            for (unsigned k=(i%(width/3-7))*3; k<width - 3*7-1; k++)
               printf(" ");
            printf(" (bytes %d - %lu)\n", start, size);
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

int main(int argc, char* argv[])
{
   char file[100];
   char *arg;
   int c, bn = -1, pos=-1, val=-1;
   int sentence = -1, from_file = -1;

   while ((c = getopt(argc, argv, "df:g:s:hiv")) != -1)
   {
      switch(c)
      {
         case 'd':
            debug=1;
            break;
         case 'f':
            from_file = 1;
            strcpy(file, optarg);
            break;
         case 'g':
            if (pos >= 0)
            {
               fprintf(stderr
                       , "You either set or get stuff. Don't do both\n");
               help();;
               return -1;
            }
            bn = atoi(optarg);
            if (bn < 0)
            {
               fprintf(stderr, "Invalid byte number (<0)\n");
               help();
               return -1;
            }
            break;
         case 'h':
            help();
            return 0;
            break;
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
            char *cl = strstr(arg, ":");
            char *p,*v;
            p = (char*) malloc((sizeof *p) * 100);
            v = (char*) malloc((sizeof *v) * 100);
            if (cl != NULL)
            {
               //last check
               if (cl-arg == 0 || arg[strlen(arg)-1] == ':')
               {
                  fprintf(stderr
                          , "Wrong format. Format must be position:value\n");
                  help();
                  return -1;
               }
               memcpy(p, arg, cl-arg);
               memcpy(v, arg+(cl-arg+1),strlen(arg)-(cl-arg)-1);
               pos = atoi(p);
               val = atoi(v);
               if (pos < 0)
               {
                  fprintf(stderr
                          , "Invalid byte number: %d\n", pos);
                  help();
                  return -1;
               }
               free(p);
               free(v);
            }
            else
            {
               fprintf(stderr
                       , "Wrong format. Format must be position:value\n");
               help();
               return -1;
            }
            break;
         case 'v':
            version();
            return 0;
            break;
         default:
            help();
            return -1;
      }
   }
   
   if (from_file == -1)
   {
		fputs("Provide at least one file name\n", stderr);
		return 1;
   }

   if (debug)
      printf("Attempting to read from \"%s\"\n", file);

   if (bn >= 0)
   {
      dump(file, bn);
      if (debug)
         printf("The byte value at position %d is: %x\n"
                , bn, get_byte_value(file, bn));
      return 0;
   }
   else if (pos >= 0)
   {
      if (set_byte_value(file, pos, val) == 0)
      {
         dump(file, pos);
         return 0;
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
   }
   else
   {
      //Otherwise just dump file
      dump(file, -1);
   }

   return 0;
}
