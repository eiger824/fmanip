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

void help()
{
   printf("USAGE: fmanip [dh[G|g|s]v] f [i]\n");
   printf("[args]\n");
   printf("-d\t\t\tTurn on debug mode\n");
   printf("-f file\t\t\tRead from file\n");
   printf("-g nbyte\t\tObtain byte value at position \"nbyte\" ( >0 )\n");
   printf("-G value\t\tPrint all bytes matching \"value\"\n");
   printf("        \t\t(For \\0, just input 00 as argument)\n");
   printf("-h\t\t\tShow help\n");
   printf("-i\t\t\tIf -f was selected, insert a sentence at a given line\n");
   printf("\t\t\t(interactively)\n");
   printf("-s nbyte[-nbyte2]:value Set \"value\" in position \"nbyte\" or\n");
   printf("\t\t\tbetween range \"nbyte\" - \"nbyte2\" (\"nbyte*\" >0)\n");
   printf("-v\t\t\tShow version\n");
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

int set_byte_value(const char* fname, int pos, int to, u8_t val)
{
   if (pos < 0) return -1;
   if (val > 255) return -2;
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
   long size;
   int i, start = 0, end = 0;
   u8_t *buffer;
   char color[10];
   if (fp != NULL)
   {
      fseek(fp, 0, SEEK_END);
      size = ftell(fp);
      rewind(fp);
      
      //last check: mark is bigger than file size
      if (mark > size)
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

int main(int argc, char* argv[])
{
   char file[100];
   char *arg;
   int c, bn = -1, pos= -1, to = -1, val = -1, byteval = -1, err = -1;
   int sentence = -1, from_file = -1, with_range = -1;

   while ((c = getopt(argc, argv, "df:g:G:s:hiv")) != -1)
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
      case 'v':
         version();
         return 0;
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

   if (bn != -1)
   {
      err = dump(file, bn, with_range, -1);
      if (debug)
         printf("The byte value at position %d is: %x\n"
                , bn, get_byte_value(file, bn));
      return err;
   }
   else if (pos >= 0)
   {
      if (set_byte_value(file, pos, to, val) == 0)
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
   

   return err;
}
