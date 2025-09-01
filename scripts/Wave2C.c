/*
 * @file
 * @author
 * @date
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include <limits.h>


// Generate extra output
int verbose = 0;


// @brief   Output Flags
// @{
enum {
    FLAGS_STATISTICS    = 1<<0,
    FLAGS_CCODE         = 1<<1,
    FLAGS_OUTPUTTOFILE  = 1<<2
};
// @}

// @brief   Output Options
// @{
enum {
    PROCESSED_FMT  = 1<<0,
    PROCESSED_DATA = 1<<1
};
// @}

typedef struct {
    FILE        *inputfile;
    FILE        *outputfile;
    unsigned    outputflags;
    unsigned    chunksprocessed;
    unsigned    format;
    unsigned    channelsn;
    unsigned    samplerate;
    unsigned    datarate;
    unsigned    blocksize;
    unsigned    bitssample;
    char        identifier[30];
} WaveInfo_t;

// Table for processing different chunk formats
typedef struct {
    char *ident;
    int (*func)(WaveInfo_t *waveinfo, size_t size);
} ChunkTable_t;


int readchunk_fmt(WaveInfo_t  *waveinfo, size_t chunksize);
int readchunk_data(WaveInfo_t *waveinfo, size_t chunksize);
int readchunk_skip(WaveInfo_t *waveinfo, size_t chunksize);

ChunkTable_t chunktab[] = {
        { "fmt ",      readchunk_fmt      },
        { "data",      readchunk_data     },
        { 0,           readchunk_skip     }
    };


/**
 *  @brief
 *
 *  @param      Pointer to 4-byte value
 *
 *  @returns    Concatenated value
 *
 */
int getint4(unsigned char *p) {

    return p[0]|(p[1]<<8)|(p[2]<<16)|(p[3]<<24);
}

/**
 *  @brief
 *
 *  @param      Pointer to 2-byte value
 *
 *  @returns    Concatenated value
 *
 */
int getint2(unsigned char *p) {

    return p[0]|(p[1]<<8);
}

/**
 *  @brief      Gets the basename of a file path
 *
 *  @param      s           Pointer to the path
 *  @param      basename    Output string
 *  @param      n           Maximal size of output string
 *
 *  @returns    Converted value
 *
 */
int getbasename(char *s, char basename[restrict], int n) {
char *pslash;
char *pdot;
char *p,*q;

    n--;
    p = s;
    pdot = NULL;
    pslash = NULL;
    while( *p ) {
        if( *p == '/' ) pslash = p;
        else if( *p == '.' ) pdot = p;
        p++;
    }
    if( !pslash ) {
        pslash = s;
    } else {
        pslash++;
    }
    if( !pdot ) pdot = p;
    p = pslash;
    q = basename;
    while ( (p < pdot) && (q-basename)<n ) {
        *q++ = *p++;
    }
    *q = 0;
    // Replace all invalid character by '_';
    q = basename;
    while(*q) {
        if( !isalpha(*q)&&!isdigit(*q)) *q = '_';
        q++;
    }


    if( verbose ) {
        fprintf(stderr,"Basename: %s\n",basename);
    }
    return 0;
}

/**
 *  @brief      Transforms a string into a a C identifier from a file name
 *
 *  @param      in   pointer the the string containing the filename
 *  @param      out  pointer to the output string
 *
 *  @returns    Concatenated value
 *
 */
int getidentifier(char *in, char *out, int n) {
char *p = in;
char *q = out;

    n--; // account for the null terminator
    while(*p && (n>0) ) {
        // Skip non valid characters
        if( isalpha(*p) || isdigit(*p) || *p != '_' ) {
            // Use only upper char
            if( islower(*p) ) {
                *q = toupper(*p);
            } else {
                *q = *p;
            }
            q++;
        }
        p++;
    }
    *q = 0;

    // First char must be an alphabetic character
    if( isdigit(out[0]) ) out[0] = 'A'+(out[0]);
}

/**
 *
 *  @brief      process_file
 *
 *  @parameter  *filename*   Name of file to be processed
 *  @parameter  *flags*      Output options
 *
 *  Flag options:
 *          FLAGS_STATISTICS
 *          FLAGS_OUTPUTTOFILE
 *          FLAGS_CCODE
 */
int
process_file(char *filename,unsigned flags) {
FILE *fin;
int rc;
unsigned char area[44];
size_t filesize;
int err;
FILE *fout = stdout;
char basename[30];
char identifier[30];
char outputfilename[50];

    err = 0;
    if( verbose ) {
        fprintf(stderr,"Processing \"%s\" file\n",filename);
    }
    if( !filename || !filename[0] ) {
        err = -1;
        return err;
    }

    fin = fopen(filename,"rb");
    if( !fin ) {
        err = -2;
        goto error;
    }

    if( (flags&FLAGS_OUTPUTTOFILE) || (flags&FLAGS_CCODE)) {
        getbasename(filename,basename,30);
        if( verbose ) {
            fprintf(stderr,"Basename:%s\n",basename);
        }
        strcpy(outputfilename,basename);
        char *suffix = ".txt";
        if( flags&FLAGS_CCODE )
            suffix = ".h";
        strcat(outputfilename,suffix);
        fout = fopen(outputfilename,"w");
        if( !fout ) {
            err = -3;
            goto error;
        }
    }

    identifier[0] = 0;
    if( flags&FLAGS_CCODE) {
        getidentifier(basename,identifier,30);
        if( verbose ) {
            fprintf(stderr,"Identifier:%s\n",identifier);
        }
    }


    if( verbose ) {
        fprintf(stderr,"Starting processing\n");
    }
    // read RIFF header
    rc = fread(area,12,1,fin);
    if( rc < 0 || rc != 1 ) {
        err = -3;
        goto error;
    }

    // Check ID
    if( strncmp(area,"RIFF",4) != 0 ) {
        err = -4;
        goto error;
    }
    // Filesize
    filesize = getint4(&area[4]);
    if( filesize <= 0 ) {
        err = -5;
        goto error;
    }
    filesize+=8;
    if( verbose ) {
        fprintf(stderr,"File size          = %zd\n",filesize);
    }
    // File format Must be WAVE for wave files
    if( strncmp(&area[8],"WAVE",4) != 0 ) {
        err = -6;
        goto error;
    }

    WaveInfo_t  waveinfo;
    memset(&waveinfo,0,sizeof(waveinfo));

    waveinfo.inputfile = fin;
    waveinfo.outputfile = fout;
    waveinfo.outputflags = flags;
    strncpy(waveinfo.identifier,identifier,30-1);

    while( !feof(fin) ) {
        // read chunk header
        rc = fread(area,8,1,fin);
        if( rc < 0 || rc != 1 ) {
            err = -7;
            goto error;
        }

        if( feof(fin) )
            break;

        int index=0;
        for(index=0;chunktab[index].ident;index++) {
            if( strncmp(&area[0],chunktab[index].ident,4) == 0 )
                break;
        }

        size_t chunksize = getint4(&area[4]);

        // Process chunk
        int rc = chunktab[index].func(&waveinfo,chunksize);
        if( rc < 0 ) {
            err = -rc;
            goto error;
        }

    }
error:
    if( verbose && err )
        fprintf(stderr,"Error %d\n",err);

    if( err < -2 )
        fclose(fin);

    if( verbose ) {
        fprintf(stderr,"Ending processing\n");
    }
    return err;
}

/**
 *  @brief          readchunk_fmt
 *
 *  @param
 *
 *  @returns
 *
 */

int readchunk_fmt(WaveInfo_t  *waveinfo, size_t chunksize) {
unsigned char area[44];

    FILE *fin = waveinfo->inputfile;

    if( (chunksize!=16)&&(chunksize!=18)&&(chunksize!=40) ) {
        return -11;
    }

    // Get chunk data
    int rc = fread(area,16,1,fin);
    if( rc < 0 || rc != 1 ) {
        return -12;
    }

    waveinfo->format    = getint2(&area[0]);
    waveinfo->channelsn = getint2(&area[2]);
    waveinfo->samplerate= getint4(&area[4]);
    waveinfo->datarate  = getint4(&area[8]);
    waveinfo->blocksize = getint2(&area[12]);
    waveinfo->bitssample= getint2(&area[14]);

    if( verbose ) {
        printf("Format             = %d\n",waveinfo->format);
        printf("Number of channels = %d\n",waveinfo->channelsn);
        printf("Sample rate        = %d\n",waveinfo->samplerate);
        printf("Data rate          = %d\n",waveinfo->datarate);
        printf("Block size         = %d\n",waveinfo->blocksize);
        printf("Bits per sample    = %d\n",waveinfo->bitssample);
    }

    int extensionsize = 0;
    // Check if there is extension data
    if( chunksize > 16 ) {
        rc = fread(area,2,1,fin);
        if( rc < 0 || rc != 1 ) {
            return -13;
        }
        extensionsize = getint2(&area[0]);
        if( (extensionsize != 0)&&(extensionsize!=22) ) {
            return -14;
        }

        if( extensionsize == 22 ) {
            rc = fread(area,22,1,fin);
            if( rc < 0 || rc != 1 ) {
                return -15;
            }
            // TBD: Process extension data. For now, ignored

        }

    }
    waveinfo->chunksprocessed |= PROCESSED_FMT;
    return 0;
}

/**
 *  @brief          readchunk_data
 *
 *  @param
 *
 *  @returns
 *
 */

int readchunk_data(WaveInfo_t  *waveinfo, size_t chunksize) {
FILE *fin = waveinfo->inputfile;
FILE *fout = waveinfo->outputfile;
unsigned char area[44];
int rc;

    if( (waveinfo->chunksprocessed&PROCESSED_FMT) == 0 ) {
        return -21;
    }
    if( verbose ) {
        printf("Data size          = %ld\n",chunksize);
    }

    if( waveinfo->format != 1 ) {
        return -22;
    }

    unsigned datasize = chunksize;

    unsigned blocksize = waveinfo->blocksize;
    unsigned bitssample = waveinfo->bitssample;
    unsigned samplerate = waveinfo->samplerate;
    unsigned channelsn  = waveinfo->channelsn;

    // Reading Wav data
    unsigned wav,wavmax,wavmin;
    unsigned long wavsum;
    int iwav,iwavmax,iwavmin;
    long iwavsum;
    unsigned cnt;

    cnt = 0;
    iwavsum = 0;
    wavsum  = 0;

    wavmax = 0;
    wavmin = UINT_MAX;
    iwavmax = -INT_MAX;
    iwavmin = INT_MAX;

    if( waveinfo->outputflags&FLAGS_CCODE ) {
        fprintf(fout, "#ifndef %s_H\n", waveinfo->identifier);
        fprintf(fout, "#define %s_H\n", waveinfo->identifier);
        fprintf(fout, "\n");
        fprintf(fout,"#include <stdint.h>\n");

        if( waveinfo->channelsn ) {
            fprintf(fout,"/* Even: channel 1 | Odd: channel 2 */\n\n");
        }
        
        int size = (datasize-blocksize-1)*(bitssample/8)/blocksize;
        fprintf(fout,"const int%1d_t %s[] = {\n",bitssample,waveinfo->identifier);
    }
    for(long pos=0; pos<datasize; pos+=blocksize) {
        rc = fread(area,blocksize,1,fin);
        if( rc < 0 || rc != 1 ) {
            return -23;
        }
        for(int i=0;i<channelsn;i++) {
            if( bitssample == 16 ) {
                wav = getint2(&area[i*2]);
                iwav = (short) wav;
                if( waveinfo->outputflags&FLAGS_CCODE ) {
                    fprintf(fout,"%6d, ",iwav);
                } else {
                    fprintf(fout,"%6d",iwav);
                }
                cnt++;

                wavsum += wav;
                iwavsum += iwav;
                if( wav > wavmax )   wavmax = wav;
                if( wav < wavmin )   wavmin = wav;
                if( iwav > iwavmax ) iwavmax = iwav;
                if( iwav < iwavmin ) iwavmin = iwav;
            } else if ( bitssample == 8 ) {
                wav = (short ) area[i];
                iwav = (short) wav;
                fprintf(fout,"%6d",iwav);
                cnt++;
                wavsum += wav;
                iwavsum += iwav;
                if( wav > wavmax )   wavmax = wav;
                if( wav < wavmin )   wavmin = wav;
                if( iwav > iwavmax ) iwavmax = iwav;
                if( iwav < iwavmin ) iwavmin = iwav;
            } else {
                return -24;
            }
        }
        fputc('\n',fout);
    }
    if( waveinfo->outputflags&FLAGS_CCODE ) {
        fprintf(fout,"}; // %s\n",waveinfo->identifier);
        fprintf(fout, "#endif // %s_H\n", waveinfo->identifier);
    }
    if( (waveinfo->outputflags&FLAGS_STATISTICS) && (cnt>0)) {
        printf("samples:  %6d = %8.3f s\n",cnt,((double) cnt)/samplerate);
        printf("unsigned: minimal=%6u maximal=%6u average=%6.1f\n",
            wavmin,wavmax,(double) wavsum/cnt);
        printf("signed:   minimal=%6d maximal=%6d average=%7.1f\n",
            iwavmin,iwavmax,(double) iwavsum/cnt);
    }

    return 0;
}


/**
 *  @brief          readchunk_data
 *
 *  @note           Just skips
 *
 *  @returns
 *
 */
int readchunk_skip(WaveInfo_t  *waveinfo, size_t chunksize) {
FILE *fin = waveinfo->inputfile;

   if( verbose ) {
        //  Print extra chunks
        int c;
        do {
            c = fgetc(fin);
            if( isprint(c) ) {
                fputc(c,stderr);
            } else {
                fprintf(stderr,"\\x%02X",c&0xFF);
            }
        } while( c != EOF );
        fputc('\n',stderr);
    }
}

/**
 *  @brief          main routine
 *
 */

int
main (int argc, char *argv[]) {
int index,opt;
unsigned flags = 0;

    while ((opt = getopt(argc, argv, "vsfc")) != -1) {
       switch (opt) {
       case 'v':
           verbose = 1;
           break;
       case 's':
           flags |= FLAGS_STATISTICS;
           break;
       case 'f':
           flags |= FLAGS_OUTPUTTOFILE;
           break;
       case 'c':
           flags |= FLAGS_CCODE;
           break;
       default: /* '?' */
           fprintf(stderr, "Usage: %s [-vcfs] files...\n",
                   argv[0]);
           exit(EXIT_FAILURE);
       }
    }

    if( optind >= argc ) {
       fprintf(stderr, "Usage: %s [-vsfc] files...\n",
               argv[0]);
       exit(EXIT_FAILURE);
    }
    for (index = optind; index < argc; index++) {
        if ( verbose ) {
            fprintf (stderr,"File %s\n", argv[index]);
        }
        process_file(argv[index],flags);
    }
    return 0;

}
