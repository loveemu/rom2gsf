
#define NOMINMAX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <string>
#include <map>
#include <vector>
#include <iterator>
#include <limits>
#include <algorithm>

#include "rom2gsf.h"
#include "PSFFile.h"
#include "cpath.h"

#ifdef WIN32
#include <direct.h>
#include <float.h>
#define getcwd _getcwd
#define chdir _chdir
#define isnan _isnan
#define strcasecmp _stricmp
#else
#include <unistd.h>
#endif

#define APP_NAME    "rom2gsf"
#define APP_VER     "[2019-02-03]"
#define APP_URL     "http://github.com/loveemu/rom2gsf"

#define GSF_PSF_VERSION        0x22
#define GSF_EXE_HEADER_SIZE    12

#define GBA_ROM_MAX_SIZE       0x02000000

static void writeInt(uint8_t * buf, uint32_t value)
{
	buf[0] = value & 0xff;
	buf[1] = (value >> 8) & 0xff;
	buf[2] = (value >> 16) & 0xff;
	buf[3] = (value >> 24) & 0xff;
}

bool rom2gsf(const char * rom_path, const char * gsf_path, uint32_t entrypoint, uint32_t load_offset, const std::map<std::string, std::string> & tags)
{
	off_t off_rom_size = path_getfilesize(rom_path);
	if (off_rom_size == -1) {
		fprintf(stderr, "Error: File not found \"%s\"\n", rom_path);
		return false;
	}

	size_t rom_size = off_rom_size;
	if (rom_size > GBA_ROM_MAX_SIZE) {
		fprintf(stderr, "Error: File too large \"%s\"\n", rom_path);
		return false;
	}

	uint8_t * exe = (uint8_t *)malloc(GSF_EXE_HEADER_SIZE + rom_size);
	if (exe == NULL) {
		fprintf(stderr, "Error: Memory allocation error\n");
		return false;
	}
	writeInt(&exe[0], entrypoint);
	writeInt(&exe[4], load_offset);
	writeInt(&exe[8], (uint32_t)rom_size);

	FILE * rom_file = fopen(rom_path, "rb");
	if (rom_file == NULL) {
		fprintf(stderr, "Error: File open error \"%s\"\n", rom_path);
		free(exe);
		return false;
	}

	if (fread(&exe[12], 1, rom_size, rom_file) != rom_size) {
		fprintf(stderr, "Error: File read error \"%s\"\n", rom_path);
		fclose(rom_file);
		free(exe);
		return false;
	}

	fclose(rom_file);

	ZlibWriter zlib_exe(Z_BEST_COMPRESSION);
	zlib_exe.write(exe, GSF_EXE_HEADER_SIZE + rom_size);

	if (!PSFFile::save(gsf_path, GSF_PSF_VERSION, NULL, 0, zlib_exe, tags)) {
		fprintf(stderr, "Error: File write error \"%s\"\n", gsf_path);
		free(exe);
		return false;
	}

	free(exe);

	return true;
}

static void usage(const char * progname)
{
	printf("%s %s\n", APP_NAME, APP_VER);
	printf("<%s>\n", APP_URL);
	printf("\n");
	printf("Usage\n");
	printf("-----\n");
	printf("\n");
	printf("Syntax: `%s (options) [GBA ROM files]`\n", progname);
	printf("\n");

	printf("### Options\n");
	printf("\n");
	printf("`--help`\n");
	printf("  : Show help\n");
	printf("\n");
	printf("`-m`\n");
	printf("  : Multiboot ROM (set entrypoint to 0x2000000)\n");
	printf("\n");
	printf("`-o [output.gsf]`\n");
	printf("  : Specify output filename\n");
	printf("\n");
	printf("`--load [offset]`\n");
	printf("  : Load offset of GBA executable\n");
	printf("\n");
	printf("`--lib [libname.gsflib]`\n");
	printf("  : Specify gsflib library name\n");
	printf("\n");
	printf("`--psfby [name]` (aka. `--gsfby`)\n");
	printf("  : Set creator name of GSF\n");
	printf("\n");
}

int main(int argc, char *argv[])
{
	if (argc == 1) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	long longval;
	char *endptr = NULL;

	bool multiboot = false;
	uint32_t load_offset = 0;
	bool default_load_offset = true;
	char gsf_path[PATH_MAX] = { '\0' };
	char libname[PATH_MAX] = { '\0' };

	char *psfby = NULL;

	int argi = 1;
	while (argi < argc && argv[argi][0] == '-') {
		if (strcmp(argv[argi], "--help") == 0) {
			usage(argv[0]);
			return EXIT_FAILURE;
		}
		else if (strcmp(argv[argi], "-m") == 0) {
			multiboot = true;
		}
		else if (strcmp(argv[argi], "-o") == 0) {
			if (argi + 1 >= argc) {
				fprintf(stderr, "Error: Too few arguments for \"%s\"\n", argv[argi]);
				return EXIT_FAILURE;
			}

			strcpy(gsf_path, argv[argi + 1]);

			argi++;
		}
		else if (strcmp(argv[argi], "--load") == 0) {
			if (argi + 1 >= argc) {
				fprintf(stderr, "Error: Too few arguments for \"%s\"\n", argv[argi]);
				return EXIT_FAILURE;
			}

			longval = strtol(argv[argi + 1], &endptr, 16);
			if (*endptr != '\0' || errno == ERANGE || longval < 0)
			{
				fprintf(stderr, "Error: Number format error \"%s\"\n", argv[argi + 1]);
				return EXIT_FAILURE;
			}
			load_offset = longval;
			default_load_offset = false;

			if (!((load_offset >= 0x8000000 && load_offset <= 0x9ffffff) || (load_offset >= 0x2000000 && load_offset <= 0x203ffff))) {
				fprintf(stderr, "Error: Load offset 0x%08X is out of range (0x80XXXXX is preferred for most cases)\n", load_offset);
				return EXIT_FAILURE;
			}

			argi++;
		}
		else if (strcmp(argv[argi], "--lib") == 0) {
			if (argi + 1 >= argc) {
				fprintf(stderr, "Error: Too few arguments for \"%s\"\n", argv[argi]);
				return EXIT_FAILURE;
			}

			strcpy(libname, argv[argi + 1]);

			argi++;
		}
		else if (strcmp(argv[argi], "--psfby") == 0 || strcmp(argv[argi], "--gsfby") == 0) {
			if (argi + 1 >= argc) {
				fprintf(stderr, "Error: Too few arguments for \"%s\"\n", argv[argi]);
				return EXIT_FAILURE;
			}

			psfby = argv[argi + 1];

			argi++;
		}
		else {
			fprintf(stderr, "Error: Unknown option \"%s\"\n", argv[argi]);
			return EXIT_FAILURE;
		}
		argi++;
	}

	uint32_t gsf_entrypoint = multiboot ? 0x02000000 : 0x08000000;
	if (default_load_offset) {
		load_offset = gsf_entrypoint;
	}

	int argnum = argc - argi;
	if (argnum == 0) {
		fprintf(stderr, "Error: No input files\n");
		return EXIT_FAILURE;
	}
	if (argnum > 1 && strcmp(gsf_path, "") != 0) {
		fprintf(stderr, "Error: Unable to specify output filename for multiple inputs\n");
		return EXIT_FAILURE;
	}

	int num_error = 0;
	for (; argi < argc; argi++) {
		const char * rom_path = argv[argi];
		const char * rom_ext = path_findext(rom_path);

		if (strcmp(gsf_path, "") == 0) {
			strcpy(gsf_path, rom_path);
			path_stripext(gsf_path);
			if (strcmp(libname, "") != 0) {
				strcat(gsf_path, ".minigsf");
			}
			else {
				strcat(gsf_path, ".gsf");
			}
		}

		std::map<std::string, std::string> tags;
		if (strcmp(libname, "") != 0) {
			tags["_lib"] = libname;
		}

		if (psfby != NULL && strcmp(psfby, "") != 0) {
			tags["gsfby"] = psfby;
		}

		if (!rom2gsf(rom_path, gsf_path, gsf_entrypoint, load_offset, tags)) {
			num_error++;
		}

		puts(gsf_path);
	}

	if (num_error != 0) {
		fprintf(stderr, "%d error(s)\n", num_error);
	}

	return (num_error == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
