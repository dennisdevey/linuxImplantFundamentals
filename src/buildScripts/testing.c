#include <stdio.h>




int main(int argc, char const *argv[]){


#ifdef HELLO
    printf("Hello world\n");
#endif


#ifdef DEBUG
    printf("DEBUG IS ON");
#endif

    return 0;

}
