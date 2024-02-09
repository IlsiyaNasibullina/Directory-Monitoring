#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <limits.h>
#include <math.h>
#include <sys/types.h>
#include <libgen.h>
#include <dirent.h>

char *path_dir;

void find_all_hlinks(const char *source) {
    DIR *dir;
    struct dirent *entry;

    char path_copy[strlen(source) + 1];
    strcpy(path_copy, source);
    char *pre_folder_path = dirname(path_copy);

    struct stat source_stat;
    if (stat(source, &source_stat) < 0) {
        perror("Error while getting source file information\n");
        closedir(dir);
        return;
    }

    if ((dir = opendir(pre_folder_path)) == NULL) {
        perror("Error while opening directory\n");
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        char full_path[PATH_MAX];
        snprintf(full_path, PATH_MAX, "%s/%s", pre_folder_path, entry->d_name);
        struct stat current_stat;

        if (stat(full_path, &current_stat) < 0) {
            continue;
        }

        if (current_stat.st_ino == source_stat.st_ino) {
            printf("Hardlink found:\n");
            printf("Inode: %lu, Path: %s\n", (unsigned long)current_stat.st_ino, full_path);
        }
    }
    closedir(dir);
}



void unlink_all(const char *source) {
    DIR *dir;
    struct dirent *entry;
    char path_copy[strlen(source) + 1];
    strcpy(path_copy, source);
    char *pre_folder_path = dirname(path_copy);
    if ((dir = opendir(pre_folder_path)) == NULL) {
        perror("Error while opening directory");
        return;
    }

    struct stat source_stat;
    if (lstat(source, &source_stat) == -1) {
        perror("Error in lstat in unlinking");
        closedir(dir);
        return;
    }

    ino_t source_inode = source_stat.st_ino;
    char last_hard_link_path[PATH_MAX];
    strcpy(last_hard_link_path, source);

    while ((entry = readdir(dir)) != NULL) {
        char full_path[PATH_MAX];
        snprintf(full_path, PATH_MAX, "%s/%s", pre_folder_path, entry->d_name);
        struct stat st;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue; // Skip . and .. entries
        }

        if (lstat(full_path, &st) == -1) {
            perror("lstat");
            continue;
        }

        if (st.st_ino == source_inode) {
            if (strcmp(last_hard_link_path, full_path) != 0) {
                if (unlink(full_path) == 0) {
                    printf("Removed duplicate hard link: %s\n", full_path);
                } else {
                    perror("unlink");
                }
            }
        }
    }

    closedir(dir);

    if (strcmp(last_hard_link_path, "") != 0) {
        printf("Path of the last hard link: %s\n", last_hard_link_path);
    } else {
        printf("No duplicates found for hard link: %s\n", source);
    }
}

void create_sym_link(const char* source, const char* link) {
    if (symlink(source, link) == 0) {
        printf("Symbolic link created: %s -> %s\n", link, source);
    } else {
        perror("symlink");
    }
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        perror("Error in args in monitor.c\n");
        exit(EXIT_FAILURE);
    }
    path_dir = argv[1];

    char full_path[PATH_MAX];
    snprintf(full_path, PATH_MAX, "%s/%s", path_dir, "myfile1.txt");
    FILE *file1 = fopen(full_path, "w");
    if (file1 == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fprintf(file1, "Hello world.\n");
    fclose(file1);

    char link_path[PATH_MAX];
    snprintf(link_path, PATH_MAX, "%s/%s", path_dir, "myfile11.txt");
    link(full_path, link_path);
    char link_path2[PATH_MAX];
    snprintf(link_path2, PATH_MAX, "%s/%s", path_dir, "myfile12.txt");
    link(full_path, link_path2);

    find_all_hlinks(full_path);

    if (rename(full_path, "/tmp/myfile1.txt") == 0) {
        printf("File myfile1.txt moved to /tmp/myfile1.txt\n");
    } else {
        perror("Error while moving file\n");
        exit(EXIT_FAILURE);
    }

    char new_path[PATH_MAX];
    snprintf(new_path, PATH_MAX, "%s/%s", path_dir, "myfile11.txt");
    FILE *file11 = fopen(new_path, "a");
    if (file11 == NULL) {
        perror("Error while fopen myfile to rewrite");
        exit(EXIT_FAILURE);
    }
    fprintf(file11, "Modified content in myfile11.txt\n");
    fclose(file11);

    char sym[PATH_MAX];
    snprintf(sym, PATH_MAX, "%s/%s", path_dir, "myfile13.txt");
    create_sym_link("/tmp/myfile1.txt", sym);

    FILE *tmp_file = fopen("/tmp/myfile1.txt", "a");
    if (tmp_file == NULL) {
        perror("Error while opening /tmp/myfile1\n");
        exit(EXIT_FAILURE);
    }
    fprintf(tmp_file, "Modified content in /tmp/myfile1.txt\n");
    fclose(tmp_file);
  
    unlink_all(link_path);

    printf("Stat info of the kept hard link:\n");
    find_all_hlinks(link_path);

    return EXIT_SUCCESS;
}
