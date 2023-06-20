#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

int main() {
    char* dir = "diretorioDe/Teste";
    if (mkdir(dir, 0777)) {
        printf("Falha ao criar diretorio '%s'\n" , dir);
        if (errno == EEXIST) {
            printf("\tDiretorio ja existe\n");
        }else {
            printf("\tRazao desconhecida\n");
        }
    }

    return 0;
}
