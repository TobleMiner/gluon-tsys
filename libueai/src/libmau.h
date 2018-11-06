#pragma once

ssize_t get_mirrorlist(char*** retval, char* branch);
void free_mirrorlist(char** mirrors, size_t len);
