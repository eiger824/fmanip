#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/ioctl.h>

#define NORM "\x1B[0m"
#define MARK "\x1B[31m"

typedef unsigned char u8_t;

int debug = 0;

void help()
{
   printf("USAGE: ./process-file [args]\n");
   printf("[args]\n");
   printf("-d\t\tTurn on debug mode\n");
   printf("-f file\t\tRead from file\n");
   printf("-g nbyte\tObtain byte value at position \"nbyte\" ( >0 )\n");
   printf("-h\t\tShow help\n");
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
                 "Tried to access out-of-bounds byte position (file size is %lu)\n", size);
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
                 "Tried to access out-of-bounds byte position (file size is %lu)\n", size);
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
   FILE *fp = fopen(fname, "rb");
   long size;
   int i;
   u8_t *buffer;
   if (fp != NULL)
   {
      fseek(fp, 0, SEEK_END);
      size = ftell(fp);
      rewind(fp);
      if (debug)
         printf("(File contains %lu bytes)\n", size);
      buffer = (u8_t*)malloc(sizeof(u8_t) * size);
      size_t result = fread(buffer, 1, size, fp);
      if (size == result)
      {
         for (i=0; i<size; ++i)
         {
            if (i > 0 && i % (width/3) == 0) 
            {
               printf("\n");
            }
            printf("%s%s%x%s "
                   ,(i==mark)?MARK:""
                   ,(buffer[i]<16)?"0":""
                   ,buffer[i]
                   ,(i==mark)?NORM:"");
            
         }
         printf("\n");
      }
      else
      {
         fprintf(stderr, "Expected and obtained sizes don't match(%d != %d)\n"
                 , size, result);
         return -1;
      }
   }
   else
   {
      return -1;
   }
}

int main(int argc, char* argv[])
{
   char *file = (char*)malloc(200);
   char *buffer, *arg;
   int c, bn = -1, pos=-1, val=-1;

   while ((c = getopt(argc, argv, "df:g:s:hv")) != -1)
   {
      switch(c)
      {
         case 'd':
            debug=1;
            break;
         case 'f':
            file = optarg;
            break;
         case 'g':
            if (pos >= 0)
            {
               fprintf(stderr, "You either set or get stuff. Don't do both\n");
               help();
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
         case 's':
            if (bn >= 0)
            {
               fprintf(stderr, "You either set or get stuff. Don't do both\n");
               help();
               return -1;
            }
            arg = optarg;
            char *cl = strstr(arg, ":");
            char *p,*v;
            p=malloc(100);
            v=malloc(100);
            if (cl != NULL)
            {
               //last check
               if (cl-arg == 0 || arg[strlen(arg)-1] == ':')
               {
                  fprintf(stderr, "Wrong format. Format must be position:value\n");
                  help();
                  return -1;
               }
               memcpy(p, arg, cl-arg);
               memcpy(v, arg+(cl-arg+1),strlen(arg)-(cl-arg)-1);
               pos = atoi(p);
               val = atoi(v);
               if (pos < 0)
               {
                  fprintf(stderr, "Invalid byte number: %d\n", pos);
                  help();
                  return -1;
               }
               free(p);
               free(v);
            }
            else
            {
               fprintf(stderr, "Wrong format. Format must be position:value\n");
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

   if (strlen(file) == 0)
   {
      memcpy(file, "testfile.txt", 13);
      file[12] = '\0';
   }
   
   if (debug)
      printf("Attempting to read from \"%s\"\n", file);

   if (bn >= 0)
   {
      dump(file, bn);
      printf("The byte value at position %d is: %x\n", bn, get_byte_value(file, bn));
      return 0;
   }
   else if (pos >= 0)
   {
      if (set_byte_value(file, pos, val) == 0)
      {
         dump(file, pos);
         printf("Success!\n");
         return 0;
      }
      else
      {
         fprintf(stderr, "Error setting byt value\n");
         return -1;
      }
   }
   else
   {
      //Otherwise just dump file
      dump(file, -1);
   }
   

   return 0;
}
