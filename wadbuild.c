#include <stdio.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// If one may tell me the way to portable identify endianness on preprocessing stage, i'll be greatly appreciating it.
#if !defined _LITTLE_ENDIAN && !defined __LITTLE_ENDIAN && (!defined __BYTE_ORDER__ || !defined __ORDER_LITTLE_ENDIAN__ || (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__))
	#error This source may be built only on little-endian architecture.
#endif

struct header_t
{
	char     identifier[4];
	uint32_t items;
	uint32_t directory;
};

struct directory_t
{
	uint32_t start;
	uint32_t size;
	char     lumpname[8];
};

struct archive_t
{
	char   *filename;
	char   *lumpname;
	size_t  filesize;
};

int usage(char *progname)
{
	printf("\
Simple WAD file compiler.\n\
    Usage: %s <wadfile> filename lumpname [filename lumpname...]\n\
        wadfile  - filename of newly created wadfile (will be overwritten if exist);\n\
        filename - filename of lump to add (specify '-' if you want a marker);\n\
        lumpname - name of lump as which this file would reside in WAD.\n\n\
    Example: %s mywad.wad decorate1.txt DECORATE decorate2.txt DECORATE - A_START acslib_compiled.o ACSLIB - A_END acslib.acs ACSLIB\n", progname, progname);
	return 1;
}

int main(int argc, char *argv[])
{
	if ((argc < 4) || (argc % 1)) return usage(argv[0]);

	int                 i;
	char               *wadfile   = argv[1];
	int                 items     = argc / 2 - 1;
	struct archive_t   *archive   = malloc (items * sizeof(struct archive_t));
	struct directory_t *directory = malloc (items * sizeof(struct directory_t));
	struct header_t     header;
	int                 wadpos    = sizeof(struct header_t) + items * sizeof(struct directory_t);

	// Fill corresponding structures
	memcpy(header.identifier, "PWAD", 4);
	header.items     = items;
	header.directory = sizeof(struct header_t);
	for (i = 0; i < items; ++i)
	{
		archive[i].filename = argv[2 + i * 2];
		archive[i].lumpname = argv[3 + i * 2];

		// Looks like it's not portable, but...
		if (strcmp(archive[i].filename, "-"))
		{
			struct stat st;
			stat(archive[i].filename, &st);
			archive[i].filesize = st.st_size;
		}
		else archive[i].filesize = 0;

		directory[i].start = wadpos;
		directory[i].size  = archive[i].filesize;
		memset (directory[i].lumpname, 0, 8);
		strncpy(directory[i].lumpname, archive[i].lumpname, 8);
		wadpos += directory[i].size;
	}

	// Wadfile will be overwritten!
	printf("Creating %s:\n", wadfile);
	FILE *wadfile_f = fopen(wadfile, "w");
	fwrite(&header,    sizeof(struct header_t),    1,     wadfile_f);
	fwrite( directory, sizeof(struct directory_t), items, wadfile_f);
	for (i = 0; i < items; ++i)
	{
		printf("    Adding %s...\n", archive[i].filename);
		if (archive[i].filesize > 0)
		{
			FILE  *lumpfile_f = fopen(archive[i].filename,"r");
			void  *lumpdata   = malloc(archive[i].filesize);
			fread (lumpdata, 1, archive[i].filesize, lumpfile_f);
			fwrite(lumpdata, 1, archive[i].filesize, wadfile_f);
			free  (lumpdata);
		}
	}
	fclose(wadfile_f);
	free(archive);
	free(directory);
	printf("Finished.\n");
	return 0;
}