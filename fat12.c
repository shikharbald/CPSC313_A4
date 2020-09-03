  /* TO BE COMPLETED BY THE STUDENT */
#include "fat12.h"

#include <fuse.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fsuid.h>

/* revise_filename(char * filename) helper function takes in a filename of a text 
(it could take in a directory too but would not change it) and moves the '.' 
character and its following extention to the left to rid of spaces in the name
at the end of the filename it adds a null terminating charachter
*/
 void revise_filename(char * filename){

 char filename_new[13];
 for (int i =0; i < 12; i ++){
   filename_new[i] = 0x20;
 }
 filename_new[12] = filename[12];
int i =0;
while(filename[i] !=0x20){
    i++;
    }
  
    for(int s =0; s < i; s++ ){
    filename_new[s] = filename[s];
    }
    if(i > 7)
       i = 8;
    filename_new[i] = filename[8];
    if(filename[9] == 0x20){filename_new[i+1] ='\0';}
    else
    filename_new[i+1] = filename[9];
    if(filename[10] == 0x20){filename_new[i+2] ='\0';}
    else
   filename_new[i+2] = filename[10];
   if(filename[11] == 0x20){filename_new[i+3] ='\0';}
   else{
   filename_new[i+3] = filename[11];
   filename_new[i+4] = '\0';}
for(int i =0; i< 13; i++){
    filename[i] = filename_new[i];
   
   
   }

 }
/*  mask for retrieving single bits (helps with retrieving date and time info)
*/
uint16_t create_mask(unsigned a, unsigned b)
{
   uint16_t r = 0;
   for (unsigned i=a; i<=b; i++)
       r |= 1 << i;

   return r;
}

/* checks given a byte, return 1 if it the file is a byte a dir 
(NOTE: Knowing that a specific flag of a byte in the directory 
entry represents subdirectory
*/

int is_dir(char * p){

 uint8_t num  =*((uint8_t *)(p+12));
 uint8_t mask = (uint8_t) create_mask(4,4);
  num &= mask;
   num = num >> 4;
   return num;
}

/* given a pointer at a position at a path string 
determines if the file is the last filename 
or other possible files follow, checks if there is a '/' 
ahead in the path, also increaments z, string length of the filename
*/
int last_filename(const char * temp, int * z){

while (*temp != '/' ){
  if(*temp == 0){

    return 1;
  }
 temp++;
 *z += 1;

}

if(*(++temp) == 0)
return 1;


else 
  return 0;

}
/* read_unsigned_le: Reads a little-endian unsigned integer number
   from buffer, starting at position.
   
   Parameters:
     buffer: memory position of the buffer that contains the number to
             be read.
     position: index of the initial position within the buffer where
               the number is to be found.
     num_bytes: number of bytes used by the integer number within the
                buffer. Cannot exceed the size of an int.
   Returns:
     The unsigned integer read from the buffer.
 */
unsigned int read_unsigned_le(const char *buffer, int position, int num_bytes) {
  long number = 0;
  while (num_bytes-- > 0) {
    number = (number << 8) | (buffer[num_bytes + position] & 0xff);
  }
  return number;
}

/* open_volume_file: Opens the specified file and reads the initial
   FAT12 data contained in the file, including the boot sector, file
   allocation table and root directory.
   
   Parameters:
     filename: Name of the file containing the volume data.
   Returns:
     A pointer to a newly allocated fat12volume data structure with
     all fields initialized according to the data in the volume file,
     or NULL if the file is invalid, data is missing, or the file is
     smaller than necessary.
 */
fat12volume *open_volume_file(const char *filename) {
  /* TO BE COMPLETED BY THE STUDENT */
  fat12volume * volume = (fat12volume *) malloc(sizeof(fat12volume));
  volume->volume_file = fopen(filename, "r");
  fseek(volume->volume_file, 0L, SEEK_END);
  int size = ftell(volume->volume_file);
  if (size < BOOT_SECTOR_SIZE)
    return NULL;
   rewind(volume->volume_file);
   char  boot_sector[BOOT_SECTOR_SIZE];
  fread(boot_sector, 1, BOOT_SECTOR_SIZE, volume->volume_file);
  /* 
   for (int i = 0; i < BOOT_SECTOR_SIZE; i++)
        printf("%d :: %x \n", i, boot_sector[i]);*/
   volume->sector_size = read_unsigned_le(boot_sector,11,2);
   volume->cluster_size = read_unsigned_le(boot_sector,13,1);  
   volume->reserved_sectors = read_unsigned_le(boot_sector,14,2);
   volume->fat_copies = read_unsigned_le(boot_sector,16,1);
   volume->rootdir_entries = read_unsigned_le(boot_sector,17,2);
   volume->hidden_sectors = read_unsigned_le(boot_sector,28,2);
   volume->fat_num_sectors = read_unsigned_le(boot_sector,22,2);
   volume->fat_offset = volume->reserved_sectors;
   volume->rootdir_offset = volume->reserved_sectors+(volume->fat_copies)*(volume->fat_num_sectors);
   volume->rootdir_num_sectors = volume->rootdir_entries/16;
   volume->cluster_offset = volume->rootdir_offset + volume->rootdir_num_sectors - 2 * volume->cluster_size;
//volume->rootdir_offset + volume->rootdir_num_sectors;
// + volume->cluster_size;
   int size_fat_array =volume->fat_copies*volume->fat_num_sectors*volume->sector_size;
   int size_rootdir_array = volume->rootdir_num_sectors*volume->sector_size;
   volume->fat_array = (char *) malloc((sizeof(char))*size_fat_array);
   volume->rootdir_array =(char *) malloc((sizeof(char))*size_rootdir_array);
    
    fread(volume->fat_array,1 , size_fat_array, volume->volume_file);
    fseek(volume->volume_file, volume->rootdir_offset*512,SEEK_SET );
    fread(volume->rootdir_array, 1 ,size_rootdir_array , volume->volume_file);
 
   return volume;
}

/* close_volume_file: Frees and closes all resources used by a FAT12 volume.
   
   Parameters:
     volume: pointer to volume to be freed.
 */
void close_volume_file(fat12volume *volume) {
free(volume->fat_array);
free(volume->rootdir_array);
fclose(volume->volume_file);
free(volume);

}

/* read_sectors: Reads one or more contiguous sectors from the volume
   file, saving the data in a newly allocated memory space. The caller
   is responsible for freeing the buffer space returned by this
   function.
   
   Parameters:
     volume: pointer to FAT12 volume data structure.
     first_sector: number of the first sector to be read.
     num_sectors: number of sectors to read.
     buffer: address of a pointer variable that will store the
             allocated memory space.
   Returns:
     In case of success, it returns the number of bytes that were read
     from the set of sectors. In that case *buffer will point to a
     malloc'ed space containing the actual data read. If there is no
     data to read (e.g., num_sectors is zero, or the sector is at the
     end of the volume file, or read failed), it returns zero, and
     *buffer will be undefined.
 */
int read_sectors(fat12volume *volume, unsigned int first_sector,
		 unsigned int num_sectors, char **buffer) {
  
  int result = num_sectors* volume->sector_size;

  fseek(volume->volume_file, 0L, SEEK_END);
  int size = ftell(volume->volume_file);
  int total_secs = size /volume->sector_size;
  if (first_sector > total_secs || num_sectors == 0)
    return 0;
  if (total_secs - first_sector < num_sectors)
       result = (total_secs - first_sector) * volume->sector_size;
   
  *buffer = (char *) malloc(sizeof(char)*num_sectors*volume->sector_size);
   rewind(volume->volume_file); 
  
   fread(*buffer, 1, num_sectors*volume->sector_size, volume->volume_file);
return result;
}

/* PARAMATERS : cluster:  starting cluster 
               count : which is always zero
               volume: ----

               using the FAT TABLE returns the number of
               clusters to be read 
*/

int number_clusters(fat12volume *volume, unsigned int cluster, int count){
int next_cluster = get_next_cluster(volume, cluster);
if(next_cluster == 0xfff || next_cluster == 0x0ff || next_cluster == 0xff8 || next_cluster ==0xff6){
count++;
return count;

}
       
     count++; 
    return number_clusters(volume, next_cluster, count);


}

/*
Helper function for read cluster 
PARAMETERS: cluster: starting cluster
            count : number of clusters to be read, calculated from func above
            volume : ____
            buffer: a double pointer to a buffer where data will be stored 

            returns: void 
            saves the data to buffer ( first malloc based on count and size of a cluster)
            then with while loop writes the data to **buffer*/
void read_cluster_helper(fat12volume *volume, unsigned int cluster, char **buffer, int count){

int cluster_size = volume->cluster_size * volume->sector_size;
int i = 0;
int next_cluster = cluster;

*buffer = (char *) malloc(sizeof(char)*cluster_size*count);
do{
fseek(volume->volume_file,  (volume->cluster_offset+next_cluster*volume->cluster_size)*volume->sector_size, SEEK_SET);
fread(*buffer,1 ,cluster_size, volume->volume_file);
next_cluster = get_next_cluster(volume, next_cluster);
*buffer += cluster_size; 
i++;

}while(next_cluster != 0xfff);
//printf("ending &buffer: %d \n", *buffer );
*buffer -= cluster_size*i;

   // printf("ending &buffer: %d \n", *buffer );
    


}



/* read_cluster: Reads a specific data cluster from the volume file,
   saving the data in a newly allocated memory space. The caller is
   responsible for freeing the buffer space returned by this
   function. Note that, in most cases, the implementation of this
   function involves a single call to read_sectors with appropriate
   arguments.
   
   Parameters:
     volume: pointer to FAT12 volume data structure.
     cluster: number of the cluster to be read (the first data cluster
              is numbered two).
     buffer: address of a pointer variable that will store the
             allocated memory space.
   Returns:
     In case of success, it returns the number of bytes that were read
     from the cluster. In that case *buffer will point to a malloc'ed
     space containing the actual data read. If there is no data to
     read (e.g., the cluster is at the end of the volume file), it
     returns zero, and *buffer will be undefined.
 */


int read_cluster(fat12volume *volume, unsigned int cluster, char **buffer) {
   int cnt = number_clusters( volume,  cluster,0);
  //printf("count: %d \n",cnt); 
   int cluster_size = volume->cluster_size * volume->sector_size;
   
    //printf("initial &&buffer: %s \n", buffer );

   if(cluster != 0){
     if (cluster == volume->rootdir_offset/volume->cluster_size){
      
       *buffer = volume->rootdir_array;
       return volume->rootdir_num_sectors/volume->cluster_size;
     }
     else
     read_cluster_helper(volume, cluster, buffer, cnt);
    //printf("ENDING &buffer: %llu \n", *buffer );
    
   } else {
     return 0; 
    }
   
   cnt *=  cluster_size;
   
    return cnt;

}


 
  





/* get_next_cluster: Finds, in the file allocation table, the number
   of the cluster that follows the given cluster.
   
   Parameters:
     volume: pointer to FAT12 volume data structure.
     cluster: number of the cluster to seek.
   Returns:
     Number of the cluster that follows the given cluster (i.e., whose
     data is the sequence to the data of the current cluster). Returns
     0 if the given cluster is not in use, or a number larger than or
     equal to 0xff8 if the given cluster is the last cluster in a
     file.
 */
unsigned int get_next_cluster(fat12volume *volume, unsigned int cluster) {
int  i;  
int result;
if (cluster % 2  == 1){
     cluster--;
     i = (cluster/2)*3;
     int num = read_unsigned_le(volume -> fat_array, i, 3);
     result = num / 4096;
 }
else{
  i = (cluster/2)*3;
   int num = read_unsigned_le(volume -> fat_array, i, 3);
   result =  num % 4096;


}

   return result;

}

/* fill_rootdir_entry: Reads the root directory entry from a
   FAT12-formatted directory and assigns its attributes to a dir_entry
   data structure.
   
   Parameters:
     data: pointer to the beginning of the directory entry in FAT12
           format. This function assumes that this pointer is at least
           DIR_ENTRY_SIZE long.
     entry: pointer to a dir_entry structure where the data will be
            stored.
 */





void fill_rootdir_entry(fat12volume *volume,dir_entry *entry){

entry->ctime.tm_isdst = -1;
entry->ctime.tm_sec = 00;
entry->ctime.tm_min = 00;
entry->ctime.tm_hour = 00;
entry->ctime.tm_year =  1970;
entry->ctime.tm_mon = 01;
entry->ctime.tm_mday = 01;

entry->filename[0] = '/';
entry->filename[1] = '\0';
for(int i = 2; i<13 ; i++){
  entry->filename[i] = 0x20;

}
entry->is_directory =  1;

entry->first_cluster = volume->rootdir_offset/volume->cluster_size;
entry->size = volume->rootdir_num_sectors*volume->sector_size; 
  

}

/* fill_directory_entry: Reads the directory entry from a
   FAT12-formatted directory and assigns its attributes to a dir_entry
   data structure.
   
   Parameters:
     data: pointer to the beginning of the directory entry in FAT12
           format. This function assumes that this pointer is at least
           DIR_ENTRY_SIZE long.
     entry: pointer to a dir_entry structure where the data will be
            stored.
 */



void fill_directory_entry(const char *data, dir_entry *entry) {

uint16_t value1 = read_unsigned_le(data,22,2);
uint16_t value2 = read_unsigned_le(data, 24,2);
uint16_t hour_mask = create_mask(11,15);
uint16_t minutes_mask = create_mask(5,10);
uint16_t seconds_day_mask = create_mask(0,4);
uint16_t year_mask = create_mask(9,15);
uint16_t month_mask = create_mask(5,8);


int hour = (value1 & hour_mask) >> 11;
int minutes = (value1 & minutes_mask) >> 5;
int seconds =  (value1 & seconds_day_mask);
int year = (value2 & year_mask) >> 9;
int month = (value2 & month_mask) >> 5;
int day = (value2 & seconds_day_mask);
entry->ctime.tm_isdst = -1;
entry->ctime.tm_sec = 2*seconds;
entry->ctime.tm_min = minutes;
entry->ctime.tm_hour = hour;
entry->ctime.tm_year =  year+ 80;
entry->ctime.tm_mon = month;
entry->ctime.tm_mday = day;



for(int i = 0; i < 8; i++){
  entry->filename[i] = *(data+i); 
}

entry->filename[8] = 0x20;
for(int i = 9; i < 13; i++){
entry->filename[i] = *(data+i-1);

} 
entry->is_directory =  is_dir(entry->filename);
if (!entry->is_directory){
entry->filename[8] = 0x2e;
revise_filename(entry->filename);
} else{
  int i = 0;
  while(entry->filename[i] != 0x20){
    i++;
  }
  entry->filename[i] = '\0';
  
}
entry->first_cluster = read_unsigned_le(data,26,2);
entry->size = read_unsigned_le(data,28,4); 
   


//entry->tm->tm_sec = 
  /* TO BE COMPLETED BY THE STUDENT */
  /* OBS: Note that the way that FAT12 represents a year is different
     than the way used by mktime and 'struct tm' to represent a
     year. In particular, both represent it as a number of years from
     a starting year, but the starting year is different between
     them. Make sure to take this into account when saving data into
     the entry. */
}

/* find_directory_entry: finds the directory entry associated to a
   specific path.
   
   Parameters:
     volume: Pointer to FAT12 volume data structure.
     path: Path of the file to be found. Will always start with a
           forward slash (/). Path components (e.g., subdirectories)
           will be delimited with "/". A path containing only "/"
           refers to the root directory of the FAT12 volume.
     entry: pointer to a dir_entry structure where the data associated
            to the path will be stored.
   Returns:
     In case of success (the provided path corresponds to a valid
     file/directory in the volume), the function will fill the data
     structure entry with the data associated to the path and return
     0. If the path is not a valid file/directory in the volume, the
     function will return -ENOENT, and the data in entry will be
     undefined. If the path contains a component (except the last one)
     that is not a directory, it will return -ENOTDIR, and the data in
     entry will be undefined.
 */
int is_same_file(char * filename, char * filename2){
//printfilename(filename,filename2 );
for(int i =0; i < 11; i++){

   if (filename[i] != filename2[i])
       return 0;

   }
    return 1;
}



/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
void printfilename(char * filename, char * filename2){
printf("filename: ");
for(int i =0; i < 11; i++){
printf("%x ",filename[i]);
}
printf("\n");
printf("filename2: ");
for(int i =0; i < 11; i++){
printf("%x ",filename2[i]);
}


printf("\n");
}

/* find_directory_entry_recur: finds the directory entry associated to a
   specific path.
   
   It  recursively looks for the entry with matching filenames
   every function looks for one filename, once its
    foundcalls the function again to find the next name


   Parameters:
     volume: Pointer to FAT12 volume data structure.
     path: Path of the file to be found. Will always start with a
           forward slash (/). Path components (e.g., subdirectories)
           will be delimited with "/". A path containing only "/"
           refers to the root directory of the FAT12 volume.
     entry: pointer to a dir_entry structure where the data associated
            to the path will be stored.
   Returns:
     In case of success (the provided path corresponds to a valid
     file/directory in the volume), the function will fill the data
     structure entry with the data associated to the path and return
     0. If the path is not a valid file/directory in the volume, the
     function will return -ENOENT, and the data in entry will be
     undefined. If the path contains a component (except the last one)
     that is not a directory, it will return -ENOTDIR, and the data in
     entry will be undefined.
 */


int find_directory_entry_recur(fat12volume *volume, const char *path, dir_entry *entry, int cluster_debut) {
  

  
  if(*path != '/'){
       return -ENOTDIR;
}
     int i = 0;
     int s = 0;
     int c =0;
     char * filename = (char *) calloc(11, sizeof(char));
     char * filename2 = (char *) calloc(11,sizeof(char));
     char nextcluster[2];  
     char * temp2 = (char *) path;
     char * temp1 = (char *) path;
     char * temp3 = (char *) path;
     temp1++;
     temp2++;
     temp3++;
/*

Comparing the filenames, the name in the path has to the same format as is fouund 
in the volume->volume_file, entries , the for loops below make that happen    
*/
     for(int i =0 ; i < 11; i++){
     filename[i] = 0x20;
      filename2[i] = 0x20;
}
     while( *temp2 != '/' && *temp2 !=0){
     temp2++;
     i++;}
 if(*temp1 == '.'){
  if(*(temp1+1) == '.'){
      filename2[0] = '.';
      filename2[1] = '.';
}
  else
   filename2[0] = '.';

 }
 else{    
 for( int h = 0 ; h < i; h++){
 if(*(temp1+h) == '.'){
//if the it has a dot then it is a regfile and if it not last in path
// return -ENOTDIR
  if(!last_filename(temp3, &c)){
     return -ENOTDIR;
   }
   
   filename2[8] = *(temp1+h+1);
   if(*(temp1+h+2) == 0){
       filename2[9] = 0x20;
       filename2[10] = 0x20;
       break;
       }
    else 
   filename2[9] = *(temp1+h+2);
   if( *(temp1+h+3) == 0)
     filename2[10] = 0x20;
     else{
    
   filename2[10] = *(temp1+h+3);
   
   }
  break;}
else{
  filename2[h] = *(temp1+h);
   }
}}
fseek(volume->volume_file,  (volume->cluster_offset+cluster_debut*volume->cluster_size)*volume->sector_size+32*s, SEEK_SET);

fread(filename,1 ,11, volume->volume_file);

// checking if the filenames are the same to find the file
while(!is_same_file(filename, filename2)){
s++;
// If S >= 64 that means it is the end of cluster so we find 
// the next cluster and put s = 0
if(s >= volume->cluster_size*volume->sector_size/32){
 cluster_debut = get_next_cluster(volume, cluster_debut);
if (cluster_debut == 0xFFF || cluster_debut == 0x000 || cluster_debut == 0xFF8 ||
    cluster_debut == 0xFF6)
      break;

s = 0;
  fseek(volume->volume_file, (volume->cluster_offset+cluster_debut*volume->cluster_size)*volume->sector_size+32*s, SEEK_SET);

} 

else{
fseek(volume->volume_file, (volume->cluster_offset+cluster_debut*volume->cluster_size)*volume->sector_size+32*s, SEEK_SET);
}
fread(filename,1, 11, volume->volume_file);

}    
char  entry_array[32];
if(i > 0){
if(!is_same_file(filename, filename2)){
 

    return -ENOENT;
} }

  

rewind(volume->volume_file);
fseek(volume->volume_file,  (volume->cluster_offset+cluster_debut*volume->cluster_size)*volume->sector_size+32*s, SEEK_SET);
fseek(volume->volume_file,26, SEEK_CUR); 
  
fread(nextcluster,1 ,2, volume->volume_file);
fseek(volume->volume_file,-2, SEEK_CUR);
  

int cluster_start = read_unsigned_le(nextcluster, 0, 2);

 fseek(volume->volume_file,-26, SEEK_CUR);
if(*temp2 == 0){
     
     fread(entry_array,1 ,32, volume->volume_file);
    fill_directory_entry(entry_array, entry);
     return 0; 
    }

  return find_directory_entry_recur(volume,temp2,entry, cluster_start);
}

/*
since the rootdir has different size and rootdir_array which is different
implemented a seperate find_in_rootdirectory which must be called everytime 
at first

*/
int find_in_rootdirectory(fat12volume *volume, const char * path, dir_entry *entry){
const char * temp2 = (char *) path;
const char * temp1 = (char *) path;
const char * temp3 = (char *) path;
temp1++;
temp2++;
temp3++;
char * filename = (char *) calloc(11, sizeof(char));
char * filename2 = (char *) calloc(11,sizeof(char));
char  * entry_ptr =volume->rootdir_array;
int s = 0;
int i = 0;
int c =0;

for(int i =0 ; i < 11; i++){
     filename[i] = 0x20;
    filename2[i] = 0x20;
}

while( *temp2 != '/' && *temp2 !=0){
     temp2++;
     i++;}

if(*(temp2) ==0 && *(temp2-1) == '/'){
 
  fill_rootdir_entry(volume, entry);
  return 0;
}

 
for( int h = 0 ; h < i; h++){
 if(*(temp1+h) == '.'){
  if(!last_filename(temp3, &c)){
     return -ENOTDIR;
   }
   
   filename2[8] = *(temp1+h+1);
   if(*(temp1+h+2) == 0x0){
      filename2[9] = 0x20;
       filename2[10] = 0x20;
       break;}
    else {
      filename2[9] = *(temp1+h+2);}
    if( *(temp1+h+3) == 0x0){
      filename2[10] = 0x20;}
     else{
   filename2[10] = *(temp1+h+3); }
  break;}
else{
  filename2[h] = *(temp1+h);
   }
}

fseek(volume->volume_file, volume->rootdir_offset*volume->sector_size+32*s, SEEK_SET);

fread(filename,1 ,11, volume->volume_file);


while(!is_same_file(filename, filename2) && s < volume->rootdir_entries){
    // printfilename(filename, filename2);
      s++;
      fseek(volume->volume_file, volume->rootdir_offset*volume->sector_size+32*s, SEEK_SET);
      fread(filename, 1 ,11, volume->volume_file);

}

temp2 = path;
temp2++;




if(!is_same_file(filename, filename2)){
    return -ENOTDIR;
} 
if(!last_filename(temp2, &i)){
  return s;
}
else{
 
  fill_directory_entry(entry_ptr + 32*s, entry);
  return 0;
}



}



/* find_directory_entry_recur: finds the directory entry associated to a
   specific path.
  
   Parameters:
     volume: Pointer to FAT12 volume data structure.
     path: Path of the file to be found. Will always start with a
           forward slash (/). Path components (e.g., subdirectories)
           will be delimited with "/". A path containing only "/"
           refers to the root directory of the FAT12 volume.
     entry: pointer to a dir_entry structure where the data associated
            to the path will be stored.
   Returns:
     In case of success (the provided path corresponds to a valid
     file/directory in the volume), the function will fill the data
     structure entry with the data associated to the path and return
     0. If the path is not a valid file/directory in the volume, the
     function will return -ENOENT, and the data in entry will be
     undefined. If the path contains a component (except the last one)
     that is not a directory, it will return -ENOTDIR, and the data in
     entry will be undefined.
 */




int find_directory_entry(fat12volume *volume, const char *path, dir_entry *entry) {
   
   char * temp2 = (char *) path;
   temp2++;
   int s = 0;
   int i = 0;
   
   
   
   if(*path != '/')
     return -ENOTDIR;

  
  
  
  if(last_filename(temp2, &i))
   {  
   return find_in_rootdirectory(volume, path, entry);}

  else   
    {
    s = find_in_rootdirectory(volume, path, entry);}
int cluster_start = read_unsigned_le(volume->rootdir_array, 32*s+26, 2);
temp2 += i;

    return find_directory_entry_recur(volume,  temp2, entry, cluster_start);


  /* TO BE COMPLETED BY THE STUDENT */
  /* OBS: In the specific case where the path corresponds to the root
     directory ("/"), this function should fill the entry with
     information for the root directory, but this entry will not be
     based on a proper entry in the volume, since the root directory
     is not obtained from such an entry. In particular, the date/time
     for the root directory can be set to Unix time 0 (1970-01-01 0:00
     GMT). */
}
