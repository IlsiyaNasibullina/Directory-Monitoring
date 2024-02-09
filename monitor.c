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
#include <dirent.h>
#include <sys/inotify.h>


char *path_dir;

int is_directory(const char *path) {
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        return -1;
    }

    return S_ISDIR(path_stat.st_mode);
}

void print_all_info(char * path) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;

    dir = opendir(path);

    if (dir == NULL) {
        perror("Error while opening directory in monitor.c\n");
        exit(EXIT_FAILURE);
    }

    while ((entry = readdir(dir)) != NULL) {
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (stat(full_path, &file_stat) == -1) {
            perror("Unable to get file stat");
            exit(EXIT_FAILURE);
        }

        if (S_ISDIR(file_stat.st_mode)) {
            printf("Type: Directory\n");
        } else {
            printf("Type: File\n");
        }

        printf("Name: %s\n", entry->d_name);
        printf("Permissions: %o\n", file_stat.st_mode & 0777);
        printf("Directory: %s\n", path);
        printf("Size: %ld bytes\n", file_stat.st_size);
        printf("Blocks allocated: %ld\n", file_stat.st_blocks);
        printf("IO Block: %ld\n", file_stat.st_blksize);
        printf("Device: %ld\n", file_stat.st_dev);
        printf("Inode: %ld\n", file_stat.st_ino);
        printf("Links: %ld\n", file_stat.st_nlink);
        printf("Uid: %d\n", file_stat.st_uid);
        printf("Gid: %d\n", file_stat.st_gid);
        printf("Access: %s", ctime(&file_stat.st_atime));
        printf("Modify: %s", ctime(&file_stat.st_mtime));
        printf("Change: %s", ctime(&file_stat.st_ctime));

        printf("\n");
    }
    closedir(dir);

}


void print_info(char *path) {
    int result = is_directory(path);
    struct stat file_stat;
    if (stat(path, &file_stat) < 0) {
        if (errno == ENOENT) {
            printf("The File/Directory was deleted\n");
            return;
        }
        else {
            printf("The file/directory does not exist\n");
            return;
        }
    }

    if (S_ISDIR(file_stat.st_mode)) {
        printf("Type: Directory\n");
    }
    else {
        printf("Type: File\n");
    }

    printf("Directory: %s\n", path);
    printf("Size: %ld bytes\n", file_stat.st_size);
    printf("Blocks allocated: %ld\n", file_stat.st_blocks);
    printf("IO Block: %ld\n", file_stat.st_blksize);
    printf("Device: %ld\n", file_stat.st_dev);
    printf("Inode: %ld\n", file_stat.st_ino);
    printf("Links: %ld\n", file_stat.st_nlink);
    printf("Uid: %d\n", file_stat.st_uid);
    printf("Gid: %d\n", file_stat.st_gid);
    printf("Access: %s", ctime(&file_stat.st_atime));
    printf("Modify: %s", ctime(&file_stat.st_mtime));
    printf("Change: %s", ctime(&file_stat.st_ctime));
    printf("\n");
}

void handler(int signum) {
    if (signum == SIGTERM) {
        printf("Printing the stat info before terminating\n");
        print_all_info(path_dir);
        printf("The monitor.c is terminating\n");
        exit(EXIT_SUCCESS);
    }
}
void monitor(char * path) {
    int inotify_fd = inotify_init();
    if (inotify_fd == -1) {
        perror("Error in inotify_init in monitor.c\n");
        exit(EXIT_FAILURE);
    }
    int watch_desc = inotify_add_watch(inotify_fd, path, IN_ACCESS | IN_CREATE | IN_DELETE | IN_MODIFY | IN_OPEN | IN_ATTRIB);
    if (watch_desc == -1) {
        perror("Error in inotify_add_watch in monitor.c\n");
        exit(EXIT_FAILURE);
    }
    char buffer[sizeof(struct inotify_event) + NAME_MAX + 1];
    ssize_t numRead;

    while (1) {
        numRead = read(inotify_fd, buffer, sizeof(buffer));
        if (numRead == 0) {
            printf("Read from inotify fd returned 0\n");
            exit(EXIT_FAILURE);
        }
        if (numRead == -1) {
            perror("Error while reading from inotify\n");
            exit(EXIT_FAILURE);
        }

        struct inotify_event *event = (struct inotify_event *)buffer;
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, event->name);

        if (event->mask & IN_ACCESS) {
            printf("%s was accessed\n", event->name);
            print_info(full_path);
        }

        if (event->mask & IN_CREATE) {
            printf("%s created in watched directory\n", event->name);
            print_info(full_path);
        }

        if (event->mask & IN_DELETE) {
            printf("%s was deleted from watched directory\n", event->name);
            print_info(full_path);
        }

        if (event->mask & IN_MODIFY) {
            printf("%s was modified\n", event->name);
            print_info(full_path);
        }

        if (event->mask & IN_OPEN) {
            printf("%s was opened\n", event->name);
            print_info(full_path);
        }

        if (event->mask & IN_ATTRIB) {
            printf("%s metadata changed\n", event->name);
            print_info(full_path);
        }
    }
}



int main(int argc, char *argv[]) {
    if (argc != 2) {
        perror("Error in args in monitor.c\n");
        exit(EXIT_FAILURE);
    }
    path_dir = argv[1];
    printf("Printing the stat info on the start up\n");
    print_all_info(path_dir);

    signal(SIGTERM, handler);

    monitor(path_dir);

    return EXIT_SUCCESS;
}
