/*                                                         02.Feb.2010 v.1.1
   =========================================================================

   bs-stats.c
   ~~~~~~~~~~

   Program Description:
   ~~~~~~~~~~~~~~~~~~~~

   This example program reports in ASCII format the frame sizes
   encountered in an encoded bitstream file and some statistics.

   The file containing an encoded speech bitstream can be in the G.192
   serial bitstream format (which uses 16-bit softbits), or in the
   byte-oriented G.192 format. For this type of verification, the file
   cannot be in the compact binary format, nor headerless.

   Files in the G.192 format will contain 16-bit, right-aligned
   'short' words. The start of a frame is signalled by the synchronism
   header, which consists of two shorts: the first word is the frame
   synchronism word, 0x6B2z, z=0..F, and the second word contains the
   number N of softbits in the frame (i.e., the frame size). After the
   synchronism header, comes the frame "payload" that comprise N
   shorts equal to 0x007F [an 8-bit softbit representation of the
   hardbit '0'], to 0x0081 [an 8-bit softbit representation of the
   hardbit '1'], or to 0x0000 [total uncertaintly, in the case of a
   frame erasure]. Hence, a frame will occupy N+2 shorts, or 2N+4
   bytes. G.192 bitstreams need to be byte-swapped when used across
   different platforms that differ in byte-order organization (little
   and big endian systems).

   The byte-oriented G.192 format is similar to the G.192 format,
   except that data is organized in bytes, whereby only the least
   significat byte information is preserved. Hence, the frame
   syncronism word is represented by the char 0x2z, z=0..F; the
   payload will contain bytes 0x7F, 0x81, or 0x00. Byte-oriented G.192
   bitstreams do not need byteswapping when using them across
   platforms that have different byte-order architectires (i.e. big
   and little endian systems). However, the byte-oriented G.192 format
   has some important limitations: frame sizes are limited to 255
   (soft)bits, and true softbit usage is not recommended since the
   syncronism word is no longer unique.

   Limitations:
   ~~~~~~~~~~~~

   This program will not distinguish among different channels that
   might be multiplexed in the bitstream file (identified by different
   header synchronism values), nor will separate statistics for erased
   frames. All data will be consolidated irrespective of different
   channel assignment and/or frame erasures.

   Usage:
   ~~~~~
   bs-stats [Options] in_bs bs_info
   Where:
   in_bs ...... input encoded speech bitstream file
   bs_info .... ASCII file where the frame length values found in the
                file are reported, sequencially. Additional stats
                printed on the screen can be saved to a file via
                redirection of stdin.

   Options:
   -bs mode ... Mode for bitstream (g192, byte, or bit)
   -q ......... Quiet operation
   -qq ........ VERY Quiet operation: no ASCII file generated
   -? ......... Displays this message
   -help ...... Displays a complete help message

   Original Author:
   ~~~~~~~~~~~~~~~~
   Simao Ferraz de Campos Neto
   Comsat Laboratories              Tel:    +1-301-428-4516
   22300 Comsat Drive               Fax:    +1-301-428-9287
   Clarksburg MD 20871 - USA        E-mail: simao.campos@labs.comsat.com

   History:
   ~~~~~~~~
   02.Feb.2000 v.1.0 Created based on eid-xor.c <simao>
   02.Feb.2010 v.1.1 Modified maximum string length for filenames to
                     avoid buffer overruns (y.hiwasaki)

   ========================================================================= */

/* ..... Generic include files ..... */
#include "ugstdemo.h"           /* general UGST definitions */
#include <stdio.h>              /* Standard I/O Definitions */
#include <math.h>
#include <stdlib.h>
#include <string.h>             /* memset */
#include <ctype.h>              /* toupper */

/* ..... OS-specific include files ..... */
#if defined (unix) && !defined(MSDOS)
/*                 ^^^^^^^^^^^^^^^^^^ This strange construction is necessary
                                      for DJGPP, because "unix" is defined,
				      even it being MSDOS! */
#if defined(__ALPHA)
#include <unistd.h>             /* for SEEK_... definitions used by fseek() */
#else
#include <sys/unistd.h>         /* for SEEK_... definitions used by fseek() */
#endif
#endif




/* ..... Module definition files ..... */
#include "softbit.h"            /* Soft bit definitions and prototypes */

/* ..... Definitions used by the program ..... */

/* Generic definitions */
#define EID_BUFFER_LENGTH 256
#define OUT_RECORD_LENGTH 512
#define MAX_FRAME 32767

/* ************************* AUXILIARY FUNCTIONS ************************* */

/*
  --------------------------------------------------------------------------
  display_usage()

  Shows program usage.

  History:
  ~~~~~~~~
  8/Feb/2001  v1.0 Created <simao>
  --------------------------------------------------------------------------
*/
void display_usage (int level) {
  printf ("bs-stats.c - Version 1.1 of 02.Feb.2010\n");

  if (level) {
    printf ("\nThis example program reports in ASCII format the frame sizes\n");
    printf ("encountered in an encoded bitstream file and some statistics.\n");
    printf ("\n");
    printf ("The file containing an encoded speech bitstream can be in the G.192\n");
    printf ("serial bitstream format (which uses 16-bit softbits), or in the\n");
    printf ("byte-oriented G.192 format. For this type of verification, the file\n");
    printf ("cannot be in the compact binary format, nor headerless.\n");
    printf ("\n");
    printf ("Files in the G.192 format will contain 16-bit, right-aligned\n");
    printf ("'short' words. The start of a frame is signalled by the synchronism\n");
    printf ("header, which consists of two shorts: the first word is the frame\n");
    printf ("synchronism word, 0x6B2z, z=0..F, and the second word contains the\n");
    printf ("number N of softbits in the frame (i.e., the frame size). After the\n");
    printf ("synchronism header, comes the frame \"payload\" that comprise N\n");
    printf ("shorts equal to 0x007F [an 8-bit softbit representation of the\n");
    printf ("hardbit '0'], to 0x0081 [an 8-bit softbit representation of the\n");
    printf ("hardbit '1'], or to 0x0000 [total uncertaintly, in the case of a\n");
    printf ("frame erasure]. Hence, a frame will occupy N+2 shorts, or 2N+4\n");
    printf ("bytes. G.192 bitstreams need to be byte-swapped when used across\n");
    printf ("different platforms that differ in byte-order organization (little\n");
    printf ("and big endian systems).\n");
    printf ("\n");
    printf ("The byte-oriented G.192 format is similar to the G.192 format,\n");
    printf ("except that data is organized in bytes, whereby only the least\n");
    printf ("significat byte information is preserved. Hence, the frame\n");
    printf ("syncronism word is represented by the char 0x2z, z=0..F; the\n");
    printf ("payload will contain bytes 0x7F, 0x81, or 0x00. Byte-oriented G.192\n");
    printf ("bitstreams do not need byteswapping when using them across\n");
    printf ("platforms that have different byte-order architectires (i.e. big\n");
    printf ("and little endian systems). However, the byte-oriented G.192 format\n");
    printf ("has some important limitations: frame sizes are limited to 255\n");
    printf ("(soft)bits, and true softbit usage is not recommended since the\n");
    printf ("syncronism word is no longer unique.\n");
    printf ("\n");
    printf ("Limitations:\n");
    printf ("~~~~~~~~~~~~\n");
    printf ("\n");
    printf ("This program will not distinguish among different channels that\n");
    printf ("might be multiplexed in the bitstream file (identified by different\n");
    printf ("header synchronism values), nor will separate statistics for erased\n");
    printf ("frames. All data will be consolidated irrespective of different\n");
    printf ("channel assignment and/or frame erasures.\n");
    printf ("\n");
  } else {
    printf (" Reports in ASCII format the frame sizes found in an encoded\n");
    printf (" bitstream file and some related statistics. The encoded bitstream\n");
    printf (" can be in word- or byte-oriented G.192 format, but cannot be in\n");
    printf (" compact or headerless format. This program will report only the\n");
    printf (" aggregate frame size information, irrespective of any (possible)\n");
    printf (" different channel multiplexing or frame erasures present in the\n");
    printf (" bitstream. Try option -help for full help text.\n");
  }
  printf ("Usage:\n");
  printf (" bs-stats [Options] in_bs bs_info\n");
  printf ("Where:\n");
  printf (" in_bs ...... input encoded speech bitstream file\n");
  printf (" bs_info .... ASCII file where the frame length values found in the\n");
  printf ("              file are reported, sequencially. Additional stats\n");
  printf ("              printed on the screen can be saved to a file via\n");
  printf ("              redirection of stdin.\n");
  printf ("Options:\n");
  printf (" -bs mode ... Mode for bitstream (g192, byte, or bit)\n");
  printf (" -q ......... Quiet operation\n");
  printf (" -qq ........ VERY Quiet operation: no ASCII file generated\n");
  printf (" -? ......... Displays this message\n");
  printf (" -help ...... Displays a complete help message\n");

  /* Quit program */
  exit (-128);
}

/* ....................... End of display_usage() ....................... */


/* ************************************************************************* */
/* ************************** MAIN_PROGRAM ********************************* */
/* ************************************************************************* */
int main (int argc, char *argv[]) {
  /* Command line parameters */
  char bs_format = g192;        /* Generic Speech bitstream format */
  char ibs_file[MAX_STRLEN];    /* Input bitstream file */
  char out_file[MAX_STRLEN];    /* Output ASCII file */
  char log = 1;                 /* Flag for en/dis-abling output file */
  long fr_len = 0;              /* Frame length in bits */
  long bs_len;                  /* BS frame length, with headers */
  long ori_bs_len, ori_fr_len;  /* Frame/BS legth memory */
  long start_frame = 1;         /* Start inserting error from 1st one */
  char sync_header = 1;         /* Flag for input BS */

  /* File I/O parameter */
  FILE *Fibs;                   /* Pointer to input encoded bitstream file */
  FILE *Fout = 0;               /* Pointer to ASCII file with frame sizes */
#ifdef DEBUG
  FILE *F;
#endif

  /* Data arrays */
  short *bs;                    /* Encoded speech bitstream */
  short *payload;               /* Point to payload in bitstream */

  /* Aux. variables */
  long no_sizes = -1;           /* No. of diff. frame sizes found in BS */
  long distr[MAX_FRAME];        /* Array with distrib. of frame sizes */
  short offset = 0;             /* Where is next frame leng.val. in the BS */
  long max_fr = 0;              /* Max. frame length found in bitstream */
  long min_fr = 100000;         /* Min. frame length found in bitstream */
  double frame_no = 0;          /* Total # of frames in BS */
  char vbr = 1;                 /* Flag for variable bit rate mode */
  long ibs_sample_len;          /* Size (bytes) of samples in the BS */
  char tmp_type;
  long i;
  long items;                   /* Number of output elements */
#if defined(VMS)
  char mrs[15] = "mrs=512";
#endif
  char quiet = 0;

  /* Pointer to a function */
  long (*read_data) () = read_g192;     /* To read input bitstream */

  /* ......... GET PARAMETERS ......... */

  /* Check options */
  if (argc < 2)
    display_usage (0);
  else {
    while (argc > 1 && argv[1][0] == '-')
      if (strcmp (argv[1], "-start") == 0) {
        /* Define starting frame */
        start_frame = atol (argv[2]);

        /* Move arg{c,v} over the option to the next argument */
        argc -= 2;
        argv += 2;
      } else if (strcmp (argv[1], "-bs") == 0) {
        /* Define input & output encoded speech bitstream format */
        for (i = 0; i < nil; i++) {
          if (strstr (argv[2], format_str (i)))
            break;
        }
        if (i == nil) {
          error_terminate ("Invalid BS format type. Aborted\n", 5);
        } else
          bs_format = i;

        /* Move arg{c,v} over the option to the next argument */
        argc -= 2;
        argv += 2;
      } else if (strcmp (argv[1], "-qq") == 0) {
        /* Disables creation of output ASCII file */
        log = 0;

        /* Move arg{c,v} over the option to the next argument */
        argc--;
        argv++;
      } else if (strcmp (argv[1], "-q") == 0) {
        /* Set quiet mode */
        quiet = 1;

        /* Move arg{c,v} over the option to the next argument */
        argc--;
        argv++;
      } else if (strcmp (argv[1], "-?") == 0) {
        display_usage (0);
      } else if (strstr (argv[1], "-help")) {
        display_usage (1);
      } else {
        fprintf (stderr, "ERROR! Invalid option \"%s\" in command line\n\n", argv[1]);
        display_usage (0);
      }
  }

  /* Get command line parameters */
  GET_PAR_S (1, "_Input bit stream file ..................: ", ibs_file);
  if (log) {
    GET_PAR_S (2, "_Output ASCII file ......................: ", out_file);
  }

  /* Initializations */
  memset (distr, 0, (int) MAX_FRAME * sizeof (long));

  /* Starting frame is from 0 to number_of_frames-1 */
  start_frame--;

  /* Open files */
  if ((Fibs = fopen (ibs_file, RB)) == NULL)
    error_terminate ("Could not open input bitstream file\n", 1);
  if (strcmp (out_file, "-") == 0)
    Fout = stdout;
  else if (log) {
    if ((Fout = fopen (out_file, WT)) == NULL)
      error_terminate ("Could not open output ASCII file\n", 1);
  }
#ifdef DEBUG
  F = fopen ("ep.g192", WB);    /* File to save the EP in G.192 format */
#endif


  /* *** CHECK CONSISTENCY *** */

  /* Do preliminary inspection in the INPUT BITSTREAM FILE to check its format (byte, bit, g192) */
  i = check_eid_format (Fibs, ibs_file, &tmp_type);

  /* Check whether the specified BS format matches with the one in the file */
  if (i != bs_format) {
    /* The input bitstream format is not the same as specified */
    fprintf (stderr, "*** Switching bitstream format from %s to %s ***\n", format_str ((int) bs_format), format_str (i));
    bs_format = i;
  }

  /* Check whether the BS has a sync header */
  if (tmp_type == FER) {
    /* The input BS may have a G.192 synchronism header - verify */
    if (bs_format == g192) {
      short tmp[2];

      /* Get presumed first G.192 sync header */
      fread (tmp, sizeof (short), 2, Fibs);
      /* tmp[1] should have the frame length */
      i = tmp[1];
      /* advance file to the (presumed) next G.192 sync header */
      fseek (Fibs, (long) (tmp[1]) * sizeof (short), SEEK_CUR);
      /* get (presumed) next G.192 sync header */
      fread (tmp, sizeof (short), 2, Fibs);
      /* Verify */
      /* if (((tmp[0] & 0xFFF0) == 0x6B20) && (i == tmp[1])) */
      if ((tmp[0] & 0xFFF0) == 0x6B20) {
        fr_len = i;
        sync_header = 1;
        if (i != tmp[1])
          vbr = 1;
      } else
        sync_header = 0;
    } else if (bs_format == byte) {
      char tmp[2];

      /* Get presumed first byte-wise G.192 sync header */
      fread (tmp, sizeof (char), 2, Fibs);
      /* tmp[1] should have the frame length */
      i = tmp[1];
      /* advance file to the (presumed) next byte-wise G.192 sync header */
      fseek (Fibs, (long) tmp[1], SEEK_CUR);
      /* get (presumed) next G.192 sync header */
      fread (tmp, sizeof (char), 2, Fibs);
      /* Verify */
      /* if (((tmp[0] & 0xF0) == 0x20) && (i == tmp[1])) */
      if ((tmp[0] & 0xF0) == 0x20) {
        fr_len = i;
        sync_header = 1;
        if (i != tmp[1])
          vbr = 1;
      } else
        sync_header = 0;
    } else
      sync_header = 0;

    /* Rewind file */
    fseek (Fibs, 0l, SEEK_SET);
  }

  /* Can't work with compact or headerless bitstreams: abort */
  if (bs_format == compact)
    error_terminate ("Compact bitstreams are not supported by this program.\n", 6);
  if (!sync_header || fr_len == 0)
    error_terminate ("Headerless bitstreams are not supported by this program.\n", 6);


  /* *** FINAL INITIALIZATIONS *** */

  /* Use the proper data I/O functions */
  read_data = bs_format == byte ? read_byte : (bs_format == g192 ? read_g192 : read_bit_ber);

  /* Define BS sample size, in bytes */
  ibs_sample_len = bs_format == byte ? 1 : (bs_format == g192 ? 2 : 0);

  /* Inspect the bitstream file for variable frame sizes */
  while (!feof (Fibs)) {
    /* Move to position where next frame length value is expected */
    fseek (Fibs, (long) (ibs_sample_len + offset), SEEK_CUR);

    /* Get (presumed) next G.192 sync header */
    if ((items = read_data (&offset, 1l, Fibs)) != 1)
      break;

    /* Increment conters in histogram */
    distr[offset]++;

    /* Do we have a different frame length here? */
    if (offset > max_fr) {
      max_fr = offset;
      no_sizes++;
    }
    if (offset < min_fr) {
      min_fr = offset;
      no_sizes++;
    }

    /* Write frame length to file, if enabled (default) */
    if (log) {
      if (fprintf (Fout, "%d\n", offset) <= 0)
        error_terminate ("Error writing to output ASCII file\n", 5);
    }

    /* Increment frame counter */
    frame_no++;

    /* Convert offset number read to no.of bytes */
    offset *= ibs_sample_len;
  }

  /* Rewind file */
  fseek (Fibs, 0l, SEEK_SET);

  /* Set the frame length to the maximum possible value */
  fr_len = max_fr;


  /* Define how many samples are read for each frame */
  /* Bitstream may have sync headers, which are 2 samples-long */
  bs_len = sync_header ? fr_len + 2 : fr_len;

  /* Save frame lengths in memory */
  ori_bs_len = bs_len;
  ori_fr_len = fr_len;

  /* Allocate memory for data buffers */
  if ((bs = (short *) calloc (bs_len, sizeof (short))) == NULL)
    error_terminate ("Can't allocate memory for bitstream. Aborted.\n", 6);

  /* Initializes to the start of the payload in input bitstream */
  payload = sync_header ? bs + 2 : bs;




  /* *** PRINT SUMMARY OF OPTIONS & RESULTS ON SCREEN *** */

  /* Restore frame lengths from memory (just in case they were changed) */
  bs_len = ori_bs_len;
  fr_len = ori_fr_len;

  /* Print summary */
  printf ("# -----------------------------------------------------\n");
  printf ("# Bitstream file: ........... %s\n", ibs_file);
  if (log)
    printf ("# Frame lengths saved in: ... %s\n", strcmp (out_file, "-") == 0 ? "stdout" : out_file);
  else
    printf ("# Frame lengths NOT saved to a file\n");
  printf ("# Bitstream format %s...... : %s\n", sync_header ? "(G.192 header) " : "(headerless) ..", format_str ((int) bs_format));
  printf ("# Frame size count summary (total %ld frame sizes found):\n", no_sizes);
  for (i = 0; i < MAX_FRAME; i++) {
    if (distr[i])
      printf ("# -Frame length %3ld count is %5ld\n", i, distr[i]);
  }
  printf ("# Total number of frames: %.0f\n", frame_no);
#ifdef DEBUG
  printf ("# (MMR) Ratio between longest and shortest frame count is %7.3f\n", (double) distr[max_fr] / (double) distr[min_fr]);
#endif
  printf ("# (Act) Ratio between longest and total frame count is    %7.3f\n", (double) distr[max_fr] / frame_no);
  printf ("# (Efc) Ratio between shortest and total frame count is   %7.3f\n", no_sizes == 1 ? 0.0 : (double) distr[min_fr] / frame_no);

  /* *** FINALIZATIONS *** */

  /* Free memory allocated */
  free (bs);

  /* Close the output file and quit *** */
  fclose (Fibs);
  if (log)
    fclose (Fout);
#ifdef DEBUG
  fclose (F);
#endif

#ifndef VMS                     /* return value to OS if not VMS */
  return 0;
#endif
}
