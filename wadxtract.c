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

int usage(char *progname)
{
	printf("\
WAD file decompiler.\n\
    Usage: %s <wadfile> <directory>\n\
        wadfile  - filename of wadfile;\n\
        directory - where to place resulting lumps (created if necessary).\n\n\
    Example: %s mywad.wad ./output\n", progname, progname);
	return 1;
}

int main(int argc, char *argv[])
{
	if (argc != 3) return usage(argv[0]);

	int     i;
	size_t  l;
	char   *wadfile  = argv[1];
	char   *path     = argv[2];

	printf("Analyzing %s:\n", wadfile);
	FILE *wadfile_f = fopen(wadfile, "r");

	struct header_t     header;
	l = fread(&header, sizeof(struct header_t), 1, wadfile_f);
	if (feof(wadfile_f) || l < 1) goto file_error;
	switch(*(uint32_t*)header.identifier)
	{
		case 0x44415749: /* IWAD */
			printf("IWAD, "); break;
		case 0x44415750: /* PWAD */
			printf("PWAD, "); break;
		default:
			printf("Unknown header magic: %lX. Probably it's not a file of DOOM WAD format.\n", *(uint32_t*)header.identifier);
			fclose(wadfile_f);
			return 3;
	}
	printf("%d entries, directory at 0x%08lX.\n", header.items, header.directory);

	// Non-portable, but easiest way.
	if (path[strlen(path) - 1] == '/') path[strlen(path) - 1] = '\0';
	char *mkdir_cmd = malloc(10 + strlen(path));
	sprintf(mkdir_cmd, "mkdir -p %s", path);
	system(mkdir_cmd);
	free(mkdir_cmd);

	struct directory_t *directory = malloc (header.items * sizeof(struct directory_t));
	fseek(wadfile_f, header.directory, SEEK_SET);
	l = fread(directory, sizeof(struct directory_t), header.items, wadfile_f);
	if (feof(wadfile_f) || l < header.items) goto file_error;

	char *filename = malloc(strlen(path) + 19);
	for (i = 0; i < header.items; ++i)
	{
		char sane_lumpname[9], lumpname[9] = "\0\0\0\0\0\0\0\0";
		snprintf(lumpname, 9, "%s", directory[i].lumpname);
		for (l = 0; l < 9; ++l)
		{
			char c = lumpname[l];
			switch(c)
			{
				case '\\':
				case '[':
				case ']':
					c = '_';
				default:
					sane_lumpname[l] = c;
			}
		}
		sprintf(filename, "%s/%04u_%s.lmp", path, i + 1, sane_lumpname);
		printf("    Extracting %-8s [offset 0x%08X, size %10lu] -> %s ... ", lumpname, directory[i].start, directory[i].size, filename);
		FILE  *lumpfile_f = fopen(filename,"w");
		if(lumpfile_f == NULL) {fprintf(stderr, "Error opening %s.\n", filename); return 1;}
		if (directory[i].size > 0)
		{
			void  *lumpdata = malloc(directory[i].size);
			fseek(wadfile_f, directory[i].start, SEEK_SET);
			l = fread (lumpdata, 1, directory[i].size, wadfile_f);
			if (feof(wadfile_f) || l < directory[i].size) { fclose(lumpfile_f); goto file_error; }
			l = fwrite(lumpdata, 1, directory[i].size, lumpfile_f);
			printf ((l < directory[i].size) ? "Failure!\n" : "OK\n");
			free(lumpdata);
		}
		else printf ("Empty file\n");
		fclose(lumpfile_f);
	}
	fclose(wadfile_f);
	free(filename);
	free(directory);
	printf("Finished.\n");
	return 0;
file_error:
	printf("Unexpected end of WAD file. Terminating.\n");
	fclose(wadfile_f);
	return 2;
}
