#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>
#include <fnmatch.h>

/*
	Easy recursive crawler
*/

void searchRec(const char*, const char*, int, int, char, char);


int main (int argc, char *argv[]) {
	int alreadyOptions = 0;
	char **paths = NULL;
	int numPaths = 0;
	int maxdepth = -1;
	int size = -1;
	char sizeRel = 'e';
	char *pattern = NULL;
	char type = 'n';

	for (int i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			if (alreadyOptions == 1) {
				printf("Usage: ./crawl path... [-maxdepth n]  [-name pattern] [-type {d,f}] [-size [+-]n]\n");
				return EXIT_FAILURE;
			}
			numPaths++;
			paths = realloc(paths, sizeof(char *)*numPaths);
			if (paths == NULL) {
				perror("realloc");
				exit(EXIT_FAILURE);
			}
			paths[numPaths - 1] = argv[i];
		} else {
			if (strcmp(argv[i], "-maxdepth") == 0) {
				maxdepth = atoi(argv[++i]);
				alreadyOptions = 1;
			} else if (strcmp(argv[i], "-name") == 0) {
				pattern = argv[++i];
				alreadyOptions = 1;
			} else if (strcmp(argv[i], "-type") == 0) {
				type = argv[++i][0];
				alreadyOptions = 1;
			} else if (strcmp(argv[i], "-size") == 0) {
				size = atoi(argv[++i]);
				if (size < 0) {
					size = -size;
				}
				if (argv[i][0] == '+') {
					sizeRel = 'g';
				} else if (argv[i][0] == '-') {
					sizeRel = 'l';
				}
				alreadyOptions = 1;
			} else {
				printf("Usage: ./crawl path... [-maxdepth n] [-name pattern] [-type {d,f}] [-size [+-]n]\n");
				return EXIT_FAILURE;
			}
		}
	}
	
	if (numPaths == 0) {
		searchRec(".", pattern, maxdepth, size, sizeRel, type);
	}
	for (int i = 0; i < numPaths; i++) {
		searchRec(paths[i], pattern, maxdepth, size, sizeRel, type);
	}
	free(paths);
	return EXIT_SUCCESS;
}

void searchRec(const char *dirname, const char *pattern, int depth, int size, char sizeRel, char type) {
	struct stat buf;
	if (lstat(dirname, &buf) == -1) {
		perror(dirname);
		return;
	}
	
	if (type == 'n' || (type == 'd' && S_ISDIR(buf.st_mode)) || (type == 'f' && S_ISREG(buf.st_mode))) {
		if (size == -1 || (sizeRel == 'e' && (int)(buf.st_size) == size)
		|| (sizeRel == 'g' && (int)(buf.st_size) > size) || (sizeRel == 'l' && (int)(buf.st_size) < size)) {
			if (pattern == NULL) {
				printf("%s\n", dirname);
			} else {
				char *dirnameCopy = strdup(dirname);
				if (dirnameCopy == NULL) {
					perror("strdup");
					exit(EXIT_FAILURE);
				}
				if (fnmatch(pattern, basename(dirnameCopy), FNM_PERIOD) == 0) {
					printf("%s\n", dirname);
				}
				free(dirnameCopy);
			}
		}
	}

	if (depth != 0 && S_ISDIR(buf.st_mode)) {
		DIR *curDir = opendir(dirname);
		if (curDir == NULL) {
			char *dirnameCopy = strdup(dirname);
			if (dirnameCopy == NULL) {
				perror("strdup");
				exit(EXIT_FAILURE);
			}
			perror(dirnameCopy);
			return;
		}
		struct dirent *entry = NULL;
		while (errno = 0, (entry = readdir(curDir)) != NULL) {
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
				continue;
			}
			char *dirnameNew = (char *) malloc(sizeof(char)*(strlen(dirname) + strlen(entry->d_name)) + 2);
			if (dirnameNew == NULL) {
				perror("malloc");
				exit(EXIT_FAILURE);
			}
			dirnameNew[0] = '\0';
			strcat(dirnameNew, dirname);
			strcat(dirnameNew, "/");
			strcat(dirnameNew, entry->d_name);
			if (lstat(dirnameNew, &buf) == -1) {
				perror(dirnameNew);
				return;
			}
			if (S_ISREG(buf.st_mode) || S_ISDIR(buf.st_mode)) {
				searchRec(dirnameNew, pattern, depth - 1, size, sizeRel, type);
			}
			free(dirnameNew);
		}
		if (errno != 0) {
			perror("readdir");
			return;
		}
		if (closedir(curDir) == -1) {
			perror("closedir");
			return;
		}
	}
}
