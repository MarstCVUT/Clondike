#ifndef DIRECTOR_CLI_H
#define DIRECTOR_CLI_H

int initialize(int serverPort);
int check_permission(char* key, char* permission_type, char* operation, char* object);
void finalize();

#endif
