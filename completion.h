#ifndef GOONSH_COMPLETION_H
#define GOONSH_COMPLETION_H

#include <readline/rltypedefs.h>

char* completion_generator(const char* text, int state);
char** goonsh_completion(const char* text, int start, int end);
void goonsh_redisplay();
int accept_suggestion(int, int);
#ifdef __cplusplus
extern "C" {
#endif
void clear_last_suggestion();
#ifdef __cplusplus
}
#endif

#endif // GOONSH_COMPLETION_H
