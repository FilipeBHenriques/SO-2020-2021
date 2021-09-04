#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define VSIZE 167

int main()
{
    char op;
    int numero;
    int i, e;
    int len;

    char texto[VSIZE];
    char temp[VSIZE];
    char *letra;
    while (strcmp(&op, "p") != 1)
    {
        scanf("\n%c %d %[^\n]", &op, &numero, texto);
        len = strlen(texto);

        switch (op)
        {

        case 'q':
            printf("Exiting -->");
            return 0;
            break;
        case 'r':
            strcpy(temp, " ");
            if (numero < 0)
            {
                e = 0;
                numero *= -1;
                letra = texto;

                for (i = numero - 1; i < len; i++)
                {
                    temp[e++] = letra[i];
                }
                for (i = 0; i < numero - 1; i++)
                {
                    temp[e++] = letra[i];
                }
            }
            if (numero > 0)
            {
                e = 0;
                for (i = len - numero; i < len; i++)
                {
                    letra = texto;
                    temp[e++] = letra[i];
                }
                for (i = 0; i < len - numero; i++)
                {
                    letra = texto;

                    temp[e++] = letra[i];
                }
                printf("%s\n", temp);
            }
            strcpy(texto, temp);
            break;
        case 'h':
            printf("%s\n", texto);
            break;

        default:
            printf("Error");
            break;
        }
    }

    return 0;
}