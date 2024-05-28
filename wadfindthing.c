#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// If one may tell me the way to portable identify endianness on preprocessing stage, i'll be greatly appreciating it.
#if !defined _LITTLE_ENDIAN && !defined __LITTLE_ENDIAN && (!defined __BYTE_ORDER__ || !defined __ORDER_LITTLE_ENDIAN__ || (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__))
	#error This source may be built only on little-endian architecture.
#endif

enum
{
	VER_DOOM,
	VER_HEXEN
} ver;

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

struct doomthings_t
{
	int16_t x;
	int16_t y;
	int16_t angle;
	int16_t type;
	int16_t flags;
} __attribute__((packed));

struct hexenthings_t
{
	int16_t tid;
	int16_t x;
	int16_t y;
	int16_t height;
	int16_t angle;
	int16_t type;
	int16_t flags;
	uint8_t special;
	uint8_t arg1;
	uint8_t arg2;
	uint8_t arg3;
	uint8_t arg4;
	uint8_t arg5;
} __attribute__((packed));

int usage(char *progname)
{
	printf("\
Thing finder.\n\
    Usage: %s <wadfile> <version> <thing type>\n\
        wadfile  - filename of wadfile;\n\
        version - expected version of THINGS lump: d for DOOM, h for Hexen;\n\
        thing type - decimal number of thing type.\n\n\
    Example: %s mywad.wad h 9058\n", progname, progname);
	return 1;
}

int main(int argc, char *argv[])
{
	if (argc != 4) return usage(argv[0]);
	if (!strcmp(argv[2],"h")) ver = VER_HEXEN; else if (!strcmp(argv[2],"d")) ver = VER_DOOM; else return usage(argv[0]);

	char   *e;
	int     type = strtol(argv[3], &e, 0); if (*e) return usage(argv[0]);
	int     i;
	int     enabled = 0;
	size_t  l;
	char   *wadfile  = argv[1];
	char    current_map[9] = "UNKNOWN\0";

	printf("Analyzing %s:\n", wadfile);
	FILE *wadfile_f = fopen(wadfile, "r");

	struct header_t header;
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

	struct directory_t *directory = malloc (header.items * sizeof(struct directory_t));
	fseek(wadfile_f, header.directory, SEEK_SET);
	l = fread(directory, sizeof(struct directory_t), header.items, wadfile_f);
	if (feof(wadfile_f) || l < header.items) goto file_error;

	for (i = 0; i < header.items; ++i)
	{
		char lumpname[9] = "\0\0\0\0\0\0\0\0";
		snprintf(lumpname, 9, "%s", directory[i].lumpname);

		// We suppose map marker to be zero sized, and no zero sized lumps before its THINGS.
		if(!directory[i].size) strcpy(current_map, lumpname);

		if(!strcmp(lumpname, "THINGS"))
		{
			//printf("THINGS for map %s at 0x%08X\n", current_map, directory[i].start);
			void  *lumpdata = malloc(directory[i].size);
			fseek(wadfile_f, directory[i].start, SEEK_SET);
			l = fread (lumpdata, 1, directory[i].size, wadfile_f);
			if ((l < directory[i].size)) goto file_error;

			switch(ver)
			{
				case VER_DOOM:
					for (struct doomthings_t *t = lumpdata; (void*)&(t[1]) <= lumpdata + directory[i].size; ++t)
						if (t->type == type) printf("Found thing of type %d at map %s (%d, %d), angle %d, flags 0x%04x\n", t->type, current_map, t->x, t->y, t->angle, t->flags);
					break;
				case VER_HEXEN:
					for (struct hexenthings_t *t = lumpdata; (void*)&(t[1]) <= lumpdata + directory[i].size; ++t)
						if (t->type == type) printf("Found thing of type %6d (id %6d) at map %8s (%5d, %5d, %5d), angle %5d, flags 0x%04x, special %5d (%5d, %5d, %5d, %5d, %5d)\n", t->type, t->tid, current_map, t->x, t->y, t->height, t->angle, t->flags, t->special, t->arg1, t->arg2, t->arg3, t->arg4, t->arg5);
					break;
			}

			free(lumpdata);
		}
	}
	fclose(wadfile_f);
	free(directory);
	printf("Finished.\n");
	return 0;
file_error:
	printf("Unexpected end of WAD file. Terminating.\n");
	fclose(wadfile_f);
	return 2;

}
