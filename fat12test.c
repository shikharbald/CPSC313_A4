#include <stdio.h>
#include "fat12.h"

int main(int argc, char *argv[]) {
 
 
  fat12volume *volume;
  int cluster, num_entries_fat;
  dir_entry entry;
  
  if (argc < 2) {
    fprintf(stderr, "Usage: %s volume_file\n", argv[0]);
    return 1;
  }
  char * str = argv[2];
  volume = open_volume_file(argv[1]);
  if (!volume) {
    fprintf(stderr, "Provided volume file is invalid or incomplete: %s.\n", argv[1]);
    return 1;
  }
  
  printf("Sector size (in bytes): %u\n", volume->sector_size);
  printf("Sectors per cluster   : %u\n", volume->cluster_size);
  printf("Reserved sectors      : %u\n", volume->reserved_sectors);
  printf("Hidden sectors        : %u\n", volume->hidden_sectors);
  printf("First sector in FAT   : %u\n", volume->fat_offset);
  printf("Sectors per FAT copy  : %u\n", volume->fat_num_sectors);
  printf("Number of FAT copies  : %u\n", volume->fat_copies);
  printf("1st sector of root dir: %u\n", volume->rootdir_offset);
  printf("Entries in root dir   : %u\n", volume->rootdir_entries);
  printf("Sectors in root dir   : %u\n", volume->rootdir_num_sectors);
  printf("Sector of cluster zero: %u\n", volume->cluster_offset);
  
  printf("\n=== FILE ALLOCATION TABLE ===");
  num_entries_fat = volume->sector_size * volume->fat_num_sectors / 3;
  for (cluster = 0; cluster < num_entries_fat; cluster++) {
    if (cluster % 16 == 0) printf("\n%04x: ", cluster);
    printf(" %03x", get_next_cluster(volume, cluster));
  }
  
  printf("\n\nFirst entry in root directory:\n");
  fill_directory_entry(volume->rootdir_array, &entry);
  mktime(&entry.ctime); // Update weekday and day of the year
  printf("  File name    : %s\n", entry.filename);
  printf("  Creation time: %s\n", asctime(&entry.ctime));
  printf("  File size    : %u\n", entry.size);
  printf("  File type    : %s\n", entry.is_directory ? "DIRECTORY" : "REGULAR FILE");
  printf("  First cluster: %03x\n", entry.first_cluster);
  
  printf("\nFinding file %s \n", str);
  if (!find_directory_entry(volume,  str, &entry)) {
    mktime(&entry.ctime); // Update weekday and day of the year
    printf("  File name    : %s\n", entry.filename);
    printf("  Creation time: %s\n", asctime(&entry.ctime));
    printf("  File size    : %u\n", entry.size);
    printf("  File type    : %s\n", entry.is_directory ? "DIRECTORY" : "REGULAR FILE");
    printf("  First cluster: %03x\n", entry.first_cluster);
   
     char * content;
     int rv = read_cluster(volume, entry.first_cluster, &content);
  
  
  if (rv > entry.size) rv = entry.size;
       printf("===== CONTENT =====\n%.*s\n===================\n", rv, content);

  } else
    printf("  FILE NOT FOUND!\n");
  
  printf("\nFinding file /LARGEDIR/FILE066.TXT:\n");
  if (!find_directory_entry(volume, "/LARGEDIR/FILE066.TXT", &entry)) {
    mktime(&entry.ctime); // Update weekday and day of the year
    printf("  File name    : %s\n", entry.filename);
    
    printf("  Creation time: %s\n", asctime(&entry.ctime));
    printf("  File size    : %u\n", entry.size);
    printf("  File type    : %s\n", entry.is_directory ? "DIRECTORY" : "REGULAR FILE");
    printf("  First cluster: %03x\n", entry.first_cluster);

    char *content;
    int rv = read_cluster(volume, entry.first_cluster, &content);
    if (rv > entry.size) rv = entry.size;
       printf("===== CONTENT =====\n%.*s\n===================\n", rv, content);
  } else
    printf("  FILE NOT FOUND!\n");
  
  return 0;
}
