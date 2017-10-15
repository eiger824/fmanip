#ifndef STROPS_H_
#define STROPS_H_

#include <stdlib.h>
#include <string.h>

unsigned strnrchr(char *p, int c)
{
   char *q = p;
   if (strlen(p) == 0) return 0;
   int n ;
   for (n = 1; p[n]; p[n] == c ? ++n : *p++);
   //reset the pointer p
   p = q;
   return n;
}

int strinsrt(char *dst, char *src, size_t len, char* line, int line_nr)
{
   unsigned offset, i, eols, eol_cnt = 1;
   char *q = NULL;

   // find out number of endlines
   eols = strnrchr(src, '\n');

   // copy the input buffer to q
   q = (char*) malloc ((sizeof *q) * (len + strlen(line) + 1));
   memcpy(q, src, len + 1);
   q[len] = '\0';

   // special cases, just prepend/append
   if (line_nr == 1)
   {
      // preprend to existing buffer
      memmove (q + strlen(line), q, len + 1); // +1 accounts for the \0
      memcpy (q, line, strlen(line));
     
      memcpy(dst, q, strlen(line) + len + 1);
      dst[strlen(line) + len] = '\0';
      free(q);
      return (int) strlen(dst);
   }
   else if (line_nr == eols + 1)
   {
      memcpy (q + len + 1, line, strlen(line) - 1);
      // add last \n
      memset (q + len, '\n', 1);
      q[len + strlen(line)] = '\0';
      offset = 1;
     
      memcpy(dst, q,  strlen(line) + len + offset);
      dst[len + strlen(line) + offset - 1] = '\0';
      free(q);
      return (int) strlen(dst);
   }
   else
   {
      // loop through the string and identify where to start inserting
      for (i=0; i<len; ++i)
      {
         if (q[i] == '\n')
         {
            if (eol_cnt == line_nr - 1)
            {
               // move remaining characters further away
               memmove (q + i + 1 + strlen(line),
                        q + i + 1,
                        len - i - 1);

               // insert our line
               memcpy (q + i + 1, line, strlen(line));

               memcpy(dst, q, strlen(line) + len + 1);
               dst[strlen(line) + len] = '\0';
               free(q);
               return (int) strlen(dst);
            }
            ++eol_cnt;
         }
      }
   }
   return -1;
}

#endif /* STROPS_H_ */
