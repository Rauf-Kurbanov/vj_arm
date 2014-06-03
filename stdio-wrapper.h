#ifndef STDIO_WRAPPER_INCLUDED
#define STDIO_WRAPPER_INCLUDED

#define VERBOSE

#ifdef microblaze
# include <sysace_stdio.h>

# define  FILE	 SYSACE_FILE
# define  fopen  sysace_fopen
# define  fclose sysace_fclose
# define  fread  sysace_fread
# define  fwrite sysace_fwrite
# define  ftell  sysace_ftell
# define  fseek  sysace_fseek
# define  fgetc  sysace_fgetc
# define  putc   sysace_putc
# define  fputc   sysace_putc
# define  fgets  sysace_fgets
# define  fputs	 sysace_fputs
# define  feof	 sysace_feof
# define  printf  xil_printf
# define  fprintf  xil_printf

int sysace_fgetc(SYSACE_FILE *stream);
int sysace_putc(int c, SYSACE_FILE *stream);
char * sysace_fgets(char *buf, int bsize, SYSACE_FILE *fp);
int sysace_fputs(const char *s, SYSACE_FILE *iop);
int sysace_feof(SYSACE_FILE *stream);
long sysace_ftell(SYSACE_FILE *stream );
int sysace_fseek(SYSACE_FILE *stream, long offset, int whence );

#else
#include <stdio.h>
# define SYSACE_FILE   FILE
# define sysace_fopen  fopen
# define sysace_fclose fclose
# define sysace_fread  fread
# define sysace_fwrite fwrite
# define sysace_ftell  ftell
# define sysace_fseek  fseek
# define sysace_fgetc  fgetc
# define sysace_putc   putc
# define sysace_fgets  fgets
# define sysace_fputs  fputs
# define sysace_feof   feof
# define xil_printf    printf
#endif

#endif /* STDIO_WRAPPER_INCLUDED */

