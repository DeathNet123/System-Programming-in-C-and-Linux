//Have to Run$ source setting.sh in order to export the lines and columns
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include <sys/types.h>
#include<dirent.h>
#include <sys/stat.h>
#include <stdbool.h>
#include<string.h>
#include <time.h>
#include <limits.h>
#include <pwd.h>
#include<math.h>
#include <grp.h>
#include <errno.h>
#include <sys/sysmacros.h>

static int LINES;
static int COLS;
extern int errno;

static int MASK = 0;
static int real_count = 0;
#define LONG_LIST 1
#define INODE 2
#define RECURSIVE 4
#define HIDDEN 8
void  populate_data(char *dir_name);
void free_mem(int mem_size, struct stat *info, struct dirent **namelist);
void smart_show(struct stat *info, struct dirent **namelist, int count_entries);
void long_list(struct stat *info, struct dirent **namelist, int count_entries);
void mode_decoder(struct stat buf, char *mode);
void user_name_decode(int uid, char *user_name);
void group_name_decode(int gid, char *group_name);
void set_size(struct dirent * entry, struct stat info, char *size);
void set_time(long seconds, char *times);
void handle_options(int argc, char *argv[]);
int main(int argc, char *argv[])
{
    real_count  = argc;
    LINES = atoi(getenv("LINES"));
    COLS = atoi(getenv("COLUMNS"));
    handle_options(argc, argv);
    if(system("test -t 0")) //if stdin was redirected
    {
        char filename[PATH_MAX];
        char proclink[PATH_MAX];
        int real_id = getpid();//get the pid 
        sprintf(proclink, "/proc/%d/fd/0", real_id);//get the link of the opened file from fd table..
        int size = readlink(proclink, filename, PATH_MAX);//get the file name using the readlink since they are soft link
        filename[size] = '\0';
        populate_data(filename);
    }
    else if(real_count > 1)
    {
        for(int idx = 1; idx < argc; idx++)
        {
            if(argv[idx] == NULL)
                continue;
            if(real_count > 2)
                printf("%s:\n", argv[idx]);
            populate_data(argv[idx]);
            if(real_count > 2)
                printf("\n\n");
        }
    }
    else
    {
        populate_data(".");
    }
    return 0;
}
void populate_data(char *dir_name)
{
    struct stat *info;//will hold the information of the file...
    struct dirent **namelist;//will hold the entries of the directory..
    //reading the entries
    int count_entries = 0; // number of entries in the directory..
    count_entries = scandir(dir_name, &namelist, NULL, alphasort); // it will scandir and place the read structure in my namelist
    /*
    int scandir(const char *dirp, struct dirent **namelist,
            int (*filter)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **));
    here the comparison function is alphasort and Filter is pointed towards NULL
    scandir allocate memory by itself but have to deallocate it
    */
   char path_name[PATH_MAX]; //for storing the path
    if(count_entries < 0)
    {
        perror("scandir");
        exit(errno);
    }
    else
    {
        info = malloc(count_entries * sizeof(struct stat));
        for(int idx = 0; idx < count_entries; idx++)
        {
            sprintf(path_name, "%s/%s", dir_name, namelist[idx]->d_name);
            int status = lstat(path_name, &info[idx]);
            if(status == -1)
            {
                perror("lstat");
            }
        }
        if(MASK & LONG_LIST)
            long_list(info, namelist, count_entries);
        else
            smart_show(info, namelist, count_entries);
        
        free_mem(count_entries, info, namelist);
    }
}
void free_mem(int mem_size, struct stat *info, struct dirent **namelist)
{
    for(int idx = 0; idx < mem_size; idx++)
    {
        free(namelist[idx]);
    }
        free(namelist);
        free(info);
}
void smart_show(struct stat *info, struct dirent **namelist, int count_entries)
{
    int max = 0, temp = 0, total = 0;
    for(int idx = 0; idx < count_entries; idx++)
    {
        temp = strlen(namelist[idx]->d_name)  + 1;
        if(temp > max)
        {
            max = temp;
        }
    }
    int cols = (float) COLS / (max + 1);
    int lines = (float) ceil(count_entries / cols);
    printf("%d, %d", cols, (int)ceil(count_entries / cols));
    for(int idx = 0; idx < count_entries; idx++)
    {
        if((namelist[idx]->d_name[0] == '.') && !(MASK & HIDDEN))
            continue;
        printf(" %s ", namelist[idx]->d_name);
    }
}
void long_list(struct stat *info, struct dirent **namelist, int count_entries)
{
    int total = 0;
    for(int idx = 0; idx < count_entries; idx++)
    {
        total += info[idx].st_blocks;
    }
    printf("total : %d\n", total/2);
     
    char per_type[11];
    char user_name[32]; //it is set to 32 since it is max
    char group_name[32]; // it is also 32
    char size[50]; //it is being taken as string because we will test the block and char file so different answer
    char times[17];
    for(int idx = 0; idx < count_entries; idx++)
    {
        if((namelist[idx]->d_name[0] == '.') && !(MASK & HIDDEN))
            continue;
        if(MASK & INODE)
            printf("%ld ", namelist[idx]->d_ino);
        mode_decoder(info[idx], per_type);
        user_name_decode(info[idx].st_uid, user_name);
        group_name_decode(info[idx].st_gid, group_name);
        set_size(namelist[idx], info[idx], size);
        set_time(info[idx].st_mtime, times);
        printf("%s %ld %s %s %s %s %s", per_type, info[idx].st_nlink, user_name, group_name, size, times, namelist[idx]->d_name);
        if(namelist[idx]->d_type == DT_LNK)
        {
            char filename[PATH_MAX];
            int n = readlink(namelist[idx]->d_name, filename, PATH_MAX);
            filename[n] = '\0';
            printf(" -> %s", filename);
        }
        printf("\n");

    }
} 
void mode_decoder(struct stat buf, char *str) //this code is courtessy of Sir Arif butt :) 
{
    int mode = buf.st_mode; 
    strcpy(str, "----------");
   //owner  permissions
   if((mode & 0000400) == 0000400) str[1] = 'r';
   if((mode & 0000200) == 0000200) str[2] = 'w';
   if((mode & 0000100) == 0000100) str[3] = 'x';
    //group permissions
   if((mode & 0000040) == 0000040) str[4] = 'r';
   if((mode & 0000020) == 0000020) str[5] = 'w';
   if((mode & 0000010) == 0000010) str[6] = 'x';
    //others  permissions
   if((mode & 0000004) == 0000004) str[7] = 'r';
   if((mode & 0000002) == 0000002) str[8] = 'w';
   if((mode & 0000001) == 0000001) str[9] = 'x';
    //special  permissions
   if((mode & 0004000) == 0004000) str[3] = 's';
   if((mode & 0002000) == 0002000) str[6] = 's';
   if((mode & 0001000) == 0001000) str[9] = 't';
   //setting the file type
   if ((buf.st_mode &  0170000) == 0010000) 
		str[0]='p';
   else if ((buf.st_mode &  0170000) == 0020000) 
		str[0]='c';
   else if ((buf.st_mode &  0170000) == 0040000) 
		str[0]='d';
   else if ((buf.st_mode &  0170000) == 0060000) 
		str[0]='b';
   else if ((buf.st_mode &  0170000) == 0100000) 
		str[0]='-';
   else if ((buf.st_mode &  0170000) == 0120000) 
		str[0]='l';
   else if ((buf.st_mode &  0170000) == 0140000) 
		str[0]='s';
   else 
		str[0]='u';
}
void user_name_decode(int uid, char *user_name)
{
    struct passwd * pwd = getpwuid(uid);
    memset(user_name, '\0', 32);
    strcpy(user_name,pwd->pw_name);
}     
void group_name_decode(int gid, char *group_name)
{
    struct group * grp = getgrgid(gid);
    memset(group_name, '\0', 32);
    strcpy(group_name, grp->gr_name);
}
void set_size(struct dirent * entry, struct stat info, char *size)
{
    memset(size, '\0', 50);
    if(entry-> d_type == DT_BLK || entry-> d_type == DT_CHR) 
    {
        unsigned int maj = major(info.st_rdev);
        unsigned int min = minor(info.st_rdev);
        sprintf(size, "%d, %d",maj, min);
    }
    else
    sprintf(size, "%ld", info.st_size);
}
void set_time(long seconds, char *times)
{
    char timer[30];
    memset(times, '0', sizeof(char) * 16);
    times[17] - '\0';
    sprintf(timer, "%s", ctime(&seconds));
    memcpy(times, timer, 16);
}
void handle_options(int argc, char *argv[])
{
    for(int idx = 1; idx < argc; ++idx)
    {
        if(argv[idx][0] == '-')
        {
            for(int kdx = 1; kdx < strlen(argv[idx]); kdx++)
            {
                switch(argv[idx][kdx])
                {
                    case 'l':
                        MASK |= LONG_LIST;
                        break;
                    case 'i':
                        MASK |= INODE;
                        break;
                    case 'R':
                        MASK |= RECURSIVE;
                        break;
                    case 'a':
                        MASK |= HIDDEN;
                        break;
                    default:
                        printf("ls: invalid option -- %c", argv[idx][kdx]);
                        exit(-1);
                        break;
                }
            }
            real_count--;
            argv[idx] = NULL;
        }
    }
}