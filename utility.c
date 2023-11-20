#include "myshell.h"
#include "signal.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define MAX_FILE_CAPACITY 8192

int main(int argc, char** argv)
{
    FILE *batchfile;

    char* line; // Строка пользовательского ввода
    char fileline[MAX_FILE_CAPACITY];
    char** args; // Слова (токены) из строки пользовательского ввода line
    int status; // Статус выполнения главного цикла do while командной оболочки

    // При любом изменении статуса дочерних (SIGCHLD) процессов (завершён, приостановлен или возобновлён), вызываем соответствующую функцию
    signal(SIGCHLD, catch_finished_task);

    if (argc == 2)
    {
        if ((batchfile = fopen(argv[1], "r")) == NULL)
        {
            printf("[ERROR] Failed to open the file!\n");
            exit(2); // 2 - серьёзная ошибка
        }
    }

    init_environ();

    do 
    {
        if (argc != 2)
        {
            display();

            line = readline();

            if (line == NULL)
                exit(1); // 1 - незначительная проблема
            args = split(line);

            if (args == NULL) 
            {
                free(line);
                exit(2);
            }
        }
        else
        {
            if (fgets(fileline, MAX_FILE_CAPACITY, batchfile) == NULL)
            {
                exit(0); // 0 - ошибок нет
            }
            else
            {
                fileline[strcspn(fileline, "\n")] = 0;
                args = split(fileline);
            }

            if (args == NULL)
                exit(2);
        }

        status = execute(args); // Пытаемся выполнить введённую пользователем команду и получаем статус о её выполнении

        free(line);
        free(args);

    } while (status); // Командная оболочка запущена, пока status == CONTINUE и прерывается, когда status == EXIT

    return 0;
}
