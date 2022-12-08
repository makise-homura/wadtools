#define _GNU_SOURCE
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>
#include <glob.h>

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
	char    lumpname[9];
	size_t  filesize;
};

int usage(char *progname)
{
	printf("\
Decompiled WAD file builder.\n\
    Usage: %s <wadfile> <directory>\n\
        wadfile  - filename of wadfile;\n\
        directory - where to read extracted lumps.\n\n\
    Example: %s mywad.wad ./output\n", progname, progname);
	return 1;
}

int main(int argc, char *argv[])
{
	if (argc != 3) return usage(argv[0]);

	int                 i;
	int                 items;
	char               *wadfile      = argv[1];
	const char         *globpattern  = "/[0-9][0-9][0-9][0-9]_*.lmp";

	struct header_t     header;
	char               *pattern      = malloc(strlen(argv[2]) + strlen(globpattern) + 1);
	glob_t              gs;

	strcpy(pattern, argv[2]);
	strcat(pattern, globpattern);
	if(glob(pattern, GLOB_TILDE_CHECK | GLOB_ERR, NULL, &gs)) {fprintf(stderr, "Error checking for %s: check if files exist.\n", pattern); return 1;}
        items = gs.gl_pathc;

	struct archive_t   *archive      = malloc (items * sizeof(struct archive_t));
	struct directory_t *directory    = malloc (items * sizeof(struct directory_t));
	int                 wadpos       = sizeof(struct header_t) + items * sizeof(struct directory_t);

	for (i = 0; i < items; ++i)
	{
		archive[i].filename = strdup(gs.gl_pathv[i]);
		memset (archive[i].lumpname, 0, 9);
		strncpy(archive[i].lumpname, basename(strdup(archive[i].filename)) + 5, 9);
		*strchrnul(archive[i].lumpname, '.') = '\0';

		struct stat st;
		stat(archive[i].filename, &st);
		archive[i].filesize = st.st_size;

		directory[i].start = wadpos;
		directory[i].size  = archive[i].filesize;
		memset (directory[i].lumpname, 0, 8);
		strncpy(directory[i].lumpname, archive[i].lumpname, 8);
		wadpos += directory[i].size;
		printf("    Creating %-8s [offset 0x%08X, size %10lu] <- %s ...\n", directory[i].lumpname, directory[i].start, directory[i].size, archive[i].filename);
	}
	globfree(&gs);

	memcpy(header.identifier, "PWAD", 4);
	header.directory = sizeof(struct header_t);
	header.items     = items;

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
			if(lumpfile_f == NULL) {fprintf(stderr, "Error opening %s.\n", archive[i].filename); return 1;}
			void  *lumpdata   = malloc(archive[i].filesize);
			fread (lumpdata, 1, archive[i].filesize, lumpfile_f);
			fwrite(lumpdata, 1, archive[i].filesize, wadfile_f);
			free  (lumpdata);
			fclose(lumpfile_f);
		}
	}
	fclose(wadfile_f);
	free(archive);
	free(directory);
	printf("Finished.\n");
	return 0;
}
