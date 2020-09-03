#include "fat12.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>

/* The FUSE version has to be defined before any call to relevant
   includes related to FUSE. */
#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26
#endif
#include <fuse.h>

#ifndef FATDEBUG
#define FATDEBUG 1
#endif

#if FATDEBUG
#define debug_print(...) fprintf(stderr, __VA_ARGS__)
#else
#define debug_print(...) ((void) 0)
#endif

#define VOLUME ((fat12volume *) fuse_get_context()->private_data)

static void *fat12_init(struct fuse_conn_info *conn);
static void fat12_destroy(void *private_data);
static int fat12_getattr(const char *path, struct stat *stbuf);
static int fat12_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi);
static int fat12_open(const char *path, struct fuse_file_info *fi);
static int fat12_release(const char *path, struct fuse_file_info *fi);
static int fat12_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi);

static const struct fuse_operations fat12_operations = {
  .init = fat12_init,
  .destroy = fat12_destroy,
  .open = fat12_open,
  .read = fat12_read,
  .release = fat12_release,
  .getattr = fat12_getattr,
  .readdir = fat12_readdir,
};

int main(int argc, char *argv[]) {
  char *volumefile = argv[--argc];
  fat12volume *volume = open_volume_file(volumefile);
  argv[argc] = NULL;
  
  if (!volume) {
    fprintf(stderr, "Invalid volume file: '%s'.\n", volumefile);
    exit(1);
  }
  
  fuse_main(argc, argv, &fat12_operations, volume);
  return 0;
}

/* fat12_init: Function called when the FUSE file system is mounted.
 */
static void *fat12_init(struct fuse_conn_info *conn) {

  debug_print("init()\n");
  
  return VOLUME;
}

/* fat12_destroy: Function called before the FUSE file system is
   unmounted.
 */
static void fat12_destroy(void *private_data) {


  debug_print("destroy()\n");
  
  close_volume_file((fat12volume *) private_data);
  
}

/* fat12_getattr: Function called when a process requests the metadata
   of a file. Metadata includes the file type, size, and
   creation/modification dates, among others (check man 2 fstat).
   
   Parameters:
     path: Path of the file whose metadata is requested.
     stbuf: Pointer to a struct stat where metadata must be stored.
   Returns:
     In case of success, returns 0, and fills the data in stbuf with
     information about the file. In case of error, it will return one
     of these error codes:
       -ENOENT: If the file does not exist;
       -ENOTDIR: If one of the path components is not a directory.
 */
static int fat12_getattr(const char *path, struct stat *stbuf) {
 
  dir_entry entry;
  debug_print("getattr(path=%s)\n", path);
  if(!find_directory_entry(VOLUME, path, &entry)){

    if(entry.is_directory){

   stbuf->st_mode = S_IFDIR | 0555; 

}

   else{
   stbuf->st_mode = S_IFREG | 0555;

}
   stbuf->st_size = entry.size;
   stbuf->st_ctime = 0;
   stbuf->st_gid = getgid();
   stbuf->st_uid = getuid(); 
   stbuf->st_blksize = VOLUME->cluster_size*VOLUME->sector_size;
   stbuf->st_blocks = stbuf->st_size/stbuf->st_blksize;
   return 0;

}
 





    
  /* TO BE COMPLETED BY THE STUDENT */
  /* Some comments about members in the struct stat definition:
     -- st_dev, st_ino, st_rdev: FUSE updates them automatically, no
        need to set them to any value;
     -- st_nlink: set it to 1;
     -- st_ctime, st_mtime, st_atime: all can be set with same value;
     -- st_gid, st_uid: use the result of getgid() and getuid(),
        respectively;
     -- st_mode: use 0555 (read/execute permission, but no write);
     -- st_blksize: the size of a cluster;
     -- st_blocks: the number of clusters used in this file.
     -- other members should be updated based on their description in
        the man page for stat.
   */
  return -ENOENT;
}

/* fat12_readdir: Function called when a process requests the listing
   of a directory.
   
   Parameters:
     path: Path of the directory whose listing is requested.
     buf: Pointer that must be passed as first parameter to filler
          function.
     filler: Pointer to a function that must be called for every entry
             in the directory.  Will accept four parameters, in this
             order: buf (previous parameter), the filename for the
             entry, a pointer to a struct stat containing the metadata
             of the file (optional, may be passed NULL), and an offset
             (see observation below, you can use 0).
     offset: Not used in this implementation of readdir.
     fi: Not used in this implementation of readdir.

   Returns:
     In case of success, returns 0, and calls the filler function for
     each entry in the provided directory. In case of error, it will
     return one of these error codes:
       -ENOENT: If the directory does not exist;
       -ENOTDIR: If the provided path (or one of the components of
                 the path) is not a directory;
       -EIO: If there was an I/O error trying to obtain the data.
 */
static int fat12_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi) {

  dir_entry new_entry;
  debug_print("readdir(path=%s, offset=%ld)\n", path, (long) offset);
  dir_entry entry; 
  if(!find_directory_entry(VOLUME, path, &entry)){
    if(entry.is_directory){
   (void) fi;
   (void) offset; 
   char * content;
    char * str = (char *) malloc(sizeof(char)*12);
    //strcpy(str, new_entry.filename);
   

  int cnt = read_cluster(VOLUME, entry.first_cluster, &content);
  
  int cluster_size_entry = VOLUME->cluster_size*VOLUME->sector_size/32;
for(int i = 0; i< cluster_size_entry*cnt;i++){
    
    
    if(*(content+i*32) == 0) return 0;
   if(*(content+i*32) != 0xffffffe5){
   fill_directory_entry(content+i*32, &new_entry);
    strcpy(str, new_entry.filename);
    // printf("%x \n",*(content+i*32) );
     //printf("filename: %s \n", str);

    filler(buf, str, NULL, 0);}
 }
  return 0;

}
   else
     return -ENOTDIR;
}

      else 
        return -ENOENT;  



  /* TO BE COMPLETED BY THE STUDENT */
  /* There are two ways to implement this function. We highly
     recommend the first one, where you return all entries in the
     directory at once. In this case, you will call filler for each
     entry with the last parameter (offset) equal to 0.
     
     An alternative implementation involves returning the entries in
     the directory in batches (e.g., one call for each cluster). In
     this case you will use the parameter offset and use a non-zero
     value in the fourth parameter of filler. You are not required to
     use this type of implementation, and there will be no bonus or
     extra help for it.
  */
}

/* fat12_open: Function called when a process opens a file in the file
   system.
   
   Parameters:
     path: Path of the file being opened.
     fi: Data structure containing information about the file being
         opened. Some useful fields include:
	 flags: Flags for opening the file. Check 'man 2 open' for
                information about which flags are available.
	 fh: File handle. The value you set to fi->fh will be
             passed unmodified to all other file operations involving
             the same file.
   Returns:
     In case of success, returns 0. In case of error, it will return
     one of these error codes:
       -ENOENT: If the file does not exist;
       -ENOTDIR: If one of the components of the path is not a
                 directory;
       -EISDIR: If the path corresponds to a directory;
       -EACCES: If the open operation was for writing, and the file is
                read-only.
 */
static int fat12_open(const char *path, struct fuse_file_info *fi) {
  debug_print("open(path=%s, flags=0%o)\n", path, fi->flags);
  dir_entry entry;
  
  // If opening for writing, returns error
  if (fi->flags & O_WRONLY || fi->flags & O_RDWR)
    return -EACCES;
  int res = find_directory_entry(VOLUME, path, &entry); 
  if (!res){

   if (!entry.is_directory){
   char * content;
  read_cluster(VOLUME, entry.first_cluster, &content);

   fi->fh = (uint64_t) content;
      return 0;
}

else return -EISDIR;

}
  else  return res; 
  

  
  /* TO BE COMPLETED BY THE STUDENT */
}

/* fat12_release: Function called when a process closes a file in the
   file system. If the open file is shared between processes, this
   function is called when the file has been closed by all processes
   that share it.
   
   Parameters:
     path: Path of the file being closed.
     fi: Data structure containing information about the file being
         opened. This is the same structure used in fat12_open.
   Returns:
     In case of success, returns 0. There is no expected error case.
 */
static int fat12_release(const char *path, struct fuse_file_info *fi) {

  debug_print("release(path=%s)\n", path);
  
  /* TO BE COMPLETED BY THE STUDENT */
  return 0;
}

/* fat12_read: Function called when a process reads data from a file
   in the file system.
   
   Parameters:
     path: Path of the open file.
     buf: Pointer where data is expected to be stored.
     size: Maximum number of bytes to be read from the file.
     offset: Byte offset of the first byte to be read from the file.
     fi: Data structure containing information about the file being
         opened. This is the same structure used in fat12_open.
   Returns:
     In case of success, returns the number of bytes actually read
     from the file--which may be smaller than size, or even zero, if
     (and only if) offset+size is beyond the end of the file. In case
     of error, may return one of these error codes (not all of them
     are required):
       -ENOENT: If the file does not exist;
       -EISDIR: If the path corresponds to a directory;
       -EIO: If there was an I/O error trying to obtain the data.
 */
static int fat12_read(const char *path, char *buf, size_t size, off_t offset,
		      struct fuse_file_info *fi) {

 debug_print("read(path=%s, size=%zu, offset=%zu)\n", path, size, offset);
if(offset >= size) return 0;

 char * content = (char *) fi->fh;
  for(int i =offset; i < size; i++){
   
  *(buf+i) = *(content+i);
   }

 

return size - offset;



 
  /* TO BE COMPLETED BY THE STUDENT */
}

