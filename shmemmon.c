// JvW 21-05-2002  initial version, derived from SHMEM.C
// JvW 21-05-2002  Updated to cycle between two 500Kb logs

#include <sys\types.h>
#include <sys\stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define  INCL_BASE
#include <os2.h>

// Definitions to create 256Kb blocks, upto a maximum of 4Gb
#define BLOCKS_PER_MB    4
#define MAX_BLOCKS     ((ULONG) 0x4000)
#define ONE_BLOCK      ((ULONG) 0x40000)

#define ONE_MINUTE     ((ULONG) 60000)

#define MAX_LOG_SIZE   512

// Show usage of the SHMEMMON program
void shmemUsage
(
   char               *program                  // IN    program name
);

// Handle output to screen and logfile, cycled
void shmemOut
(
   char               *program,                 // In    program name
   char               *logname,                 // IN    basename for log
   ULONG               maxsize,                 // IN    maximum logsize
   char               *text                     // IN    output
);

int main (int argc, char *argv[]);

/*****************************************************************************/
// Main function, entry point
/*****************************************************************************/
int main (int argc, char *argv[])
{
   APIRET              rc;
   ULONG               size = 0;
   ULONG               ivt;                     // interval time in minutes
   char                line[ 256];

   if ((argc > 1) &&                            // at least one argument
       (argv[1][0] != '?') &&                   // and no explicit
       (argv[1][1] != '?') &&                   // help request
       ((ivt = atol( argv[1])) != 0) )          // and valid interval
   {
      PBYTE           *blocks = NULL;              // array of pointers
      char            *lname  = NULL;
      ULONG            lsize  = MAX_LOG_SIZE;

      if (argc > 2)
      {
         lname = argv[2];
         if (argc > 3)
         {
            lsize = (ULONG) atol(argv[3]);
         }
      }

      if ((blocks = calloc( MAX_BLOCKS, sizeof(PBYTE))) != NULL)
      {
         while (blocks != NULL)                 // forever :-)
         {
            ULONG      i;
            ULONG      allocated = 0;
            char       tstamp[80];
            time_t     tt = time( &tt);         // current date/time

            strftime( tstamp, 80, "%A %d-%m-%Y %H:%M:%S", localtime( &tt));

            for (i = 0; i < MAX_BLOCKS; i++)
            {
               if (DosAllocSharedMem(
                     (PVOID *) &(blocks[i]),    // block i
                     (PSZ) NULL,                // unnamed
                      ONE_BLOCK,                // amount of memory
                      fALLOCSHR) == 0)          // shareable memory
               {
                  allocated++;
               }
            }
            for (i = 0; i < MAX_BLOCKS; i++)
            {
               if (blocks[i] != NULL)
               {
                  DosFreeMem( blocks[i]);       // Free each block
               }
            }
            sprintf( line, "%s - Available: % 5lu 256Kb blocks = %6.1lf Mb\n",
                     tstamp, allocated, ((double)(allocated)) / BLOCKS_PER_MB);
            shmemOut( argv[0], lname, lsize, line);

            DosSleep( ivt * ONE_MINUTE);
         }
      }
      else
      {
         printf( "\nError allocating normal memory\n");
      }
   }
   else
   {
      shmemUsage( argv[0]);
   }
   return(0);
}                                               // end 'main'
/*---------------------------------------------------------------------------*/

/*****************************************************************************/
// Show usage of the SHMEM program
/*****************************************************************************/
void shmemUsage
(
   char               *program                  // IN    program name
)
{
   printf( "\nUsage: %s  interval  [logfile  [maxsize]]\n", program);

   printf( "\nParameters:  interval    Time between samples in minutes\n"
             "             logfile     Path and filename for logfile\n"
             "             maxsize     Maximum size of logfile in Kb\n");

   printf( "\nWhen specified, output will be APPENDED to the logfile.\n");
   printf( "The size will be limited to the specified maximum with a\n"
           "default of 512 Kb. At this point the log will be renamed\n"
             "to '*.old' and a new one will be started\n");

}                                               // end 'shmemUsage'
/*---------------------------------------------------------------------------*/

/*****************************************************************************/
// Handle output to screen and logfile, cycled
/*****************************************************************************/
void shmemOut
(
   char               *program,                 // In    program name
   char               *logname,                 // IN    basename for log
   ULONG               maxsize,                 // IN    maximum logsize Kb
   char               *text                     // IN    output
)
{
   if (logname && strlen(logname))
   {
      struct stat      ls;
      BOOL             newlog = TRUE;
      FILE            *log;
      char             base[ 256];

      if (stat( logname, &ls) == 0)
      {
         if ((ls.st_size + strlen(text)) > (maxsize * 1024))
         {
            char         *s;

            strcpy( base, logname);
            if ((s = strrchr( base, '.')) != NULL)
            {
               *s = '\0';
            }
            strcat( base, ".old");
            remove( base);
            rename( logname, base);
         }
         else
         {
            newlog = FALSE;
         }
      }
      if ((log = fopen( logname, "a")) != NULL)
      {
         if (newlog)                            // not existing or too large
         {
            sprintf( base, "\n%s 1.1 Monitor shared memory.  "
                    "(c) 2002  Fsys Software; Jan van Wijk\n"
                    "Starting new logfile, maximum size: %lu Kb\n\n",
                     program, maxsize);
            printf(       "%s", base);
            fprintf( log, "%s", base);
         }
         fprintf( log, "%s", text);
         fclose(  log);
      }
      else
      {
         printf( "\nCannot append to file '%s', logging disabled\n",
                    logname);
      }
   }
   printf( "%s", text);
}                                               // end 'shmemOut'
/*---------------------------------------------------------------------------*/

