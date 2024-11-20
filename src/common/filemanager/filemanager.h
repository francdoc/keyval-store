#include "common/common.h"

typedef struct {
    char* cmd_set;
    char* cmd_get;
    char* cmd_del;
    char* cmd_separator;
    isize read_buffsize;
} filemanager_t;

extern filemanager_t new_filemanager(char* cmd_set, char* cmd_get, char* cmd_del, char* cmd_separator, isize read_buffsize);
extern error filemanager_process_cmd(filemanager_t flm, byte* cmd, isize len_cmd, char* read_val);