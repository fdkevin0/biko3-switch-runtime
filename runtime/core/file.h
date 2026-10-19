#ifndef BK_FILE_H
#define BK_FILE_H
/* The application installs its platform's transaction service before opening
 * save adapters, and clears it after releasing their owners and platform. */
typedef int (*BkFileCommit)(const char *path, char error[256]);
void bk_file_set_commit(BkFileCommit commit);
int bk_file_commit(const char *path, char error[256]);
/* A completed temporary file is renamed atomically where supported. On
 * no-overwrite filesystems the old file stays in .bak until installation.
 * Recovery restores a missing primary, never erases an existing one; callers
 * still validate the recovered contents before overwriting their files. */
int bk_file_recover(const char *path, char error[256]);
int bk_file_replace(const char *temporary, const char *path, char error[256]);
#endif
