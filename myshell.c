#include "dirent.h"
#include "errno.h"
#include "fcntl.h"
#include "myshell.h"
#include "pwd.h"
#include "signal.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/types.h"
#include "termios.h"
#include "time.h"
#include "unistd.h"
#include "wait.h"

extern char** environ;
char cwd[MAX_DIRECTORY_PATH];

char* input_file_name;
char* output_file_name;

bool input_redirection;
bool output_redirection;
bool append_output_file;
bool is_background_task;

tasks shell_tasks =
{
    .foreground =
    {
        .pid = -1,
        .finished = true,
    },
    .background = NULL,
    .iterator = 0,
    .size = 0,
};

void init_environ()
{
    if (getcwd(cwd, MAX_DIRECTORY_PATH) != NULL) 
    {
        memcpy(cwd + strlen(cwd), "/myshell.exe", strlen("/myshell.exe"));

        char* SHELL = malloc(strlen("SHELL=") + strlen(cwd));
        memcpy(SHELL, "SHELL=", strlen("SHELL="));
        memcpy(SHELL + strlen("SHELL="), cwd, strlen(cwd) + 1);

        char* PARENT = malloc(strlen("PARENT=") + strlen(cwd));
        memcpy(PARENT, "PARENT=", strlen("PARENT="));
        memcpy(PARENT + strlen("PARENT="), cwd, strlen(cwd) + 1);

        putenv(SHELL);
        putenv(PARENT);
    }
}

void display() 
{
    uid_t uid = geteuid();
    struct passwd *pw = getpwuid(uid);

    if (pw != NULL)
        printf("%s%s%s:", PRIMARY_COLOR, pw->pw_name, RESET_COLOR);

    if (getcwd(cwd, MAX_DIRECTORY_PATH) != NULL)
        printf("%s%s%s", SECONDARY_COLOR, cwd, RESET_COLOR);

    printf(": ");
}

char* readline() 
{
    char* line = NULL;
    size_t size = 0;
    ssize_t line_length;

    if ((line_length = getline(&line, &size, stdin)) == -1) 
    {
        if (errno != 0) 
            printf("[ERROR] Couldn't read from stdin!\n");

        free(line);
        printf("\n");
        
        return NULL;
    }

    if (line[line_length - 1] == '\n')
        line[line_length - 1] = '\0';

    return line;
}

char** split(char* line) 
{
    size_t position  = 0;
    size_t buffer_size = DEFAULT_BUFF_SIZE;

    char* token;
    char** tokens = (char**)malloc(sizeof(char*) * buffer_size);

    input_redirection = false;
    output_redirection = false;
    append_output_file = false;
    is_background_task = false;

    if (tokens == NULL) 
    {
        printf("[ERROR] Couldn't allocate buffer for splitting!\n");
        return NULL;
    }

    token = strtok(line, TOKENS_DELIMITERS);

    while (token != NULL) 
    {
        if (token[0] == '<')
        {
            input_redirection = true;
            input_file_name = strtok(NULL, TOKENS_DELIMITERS);
        }
        else if (token[0] == '>')
        {
            if (&token[1] != NULL && token[1] == '>')
                append_output_file = true;
            output_redirection = true;

            output_file_name = strtok(NULL, TOKENS_DELIMITERS);
        }
        else if (token[0] == '&')
        {
            is_background_task = true;
        }
        else
        {
            tokens[position++] = token;
        }

        if (position >= buffer_size) 
        {
            buffer_size *= 2;
            tokens = (char**)realloc(tokens, buffer_size * sizeof(char*));

            if (tokens == NULL) 
            {
                printf("[ERROR] Couldn't reallocate buffer for tokens!\n");
                return NULL;
            }
        }

        token = strtok(NULL, TOKENS_DELIMITERS);
    }

    tokens[position] = NULL;
    return tokens;
}

int execute(char** args) 
{
    if (args[0] == NULL) {
        return CONTINUE;
    } else if (strcmp(args[0], "cd") == 0) {
        return cd(args);
    } else if (strcmp(args[0], "clr") == 0) {
        return clear();
    } else if (strcmp(args[0], "dir") == 0) {
        return dir(args);
    } else if (strcmp(args[0], "environ") == 0) {
        return env();
    } else if (strcmp(args[0], "echo") == 0) {
        return echo(args);
    } else if (strcmp(args[0], "help") == 0) {
        return help();
    } else if (strcmp(args[0], "pause") == 0) {
        return pause();
    } else if (strcmp(args[0], "quit") == 0) {
        return quit();
    } else if (strcmp(args[0], "jobs") == 0) {
        return jobs();
    } else {
       return launch(args);
    }
}

int cd(char** args) 
{
    if (args[1] == NULL) 
    {
        printf("[ERROR] Expected argument for \"cd\" command!\n");
    } else if (chdir(args[1]) != 0) 
    {
        printf("[ERROR] Couldn't change directory to \"%s\"!\n", args[1]);
    }

    if (getcwd(cwd, MAX_DIRECTORY_PATH) != NULL) 
    {   
        char* PWD = malloc(strlen("PWD=") + strlen(cwd) + 1);

        memcpy(PWD, "PWD=", strlen("PWD="));
        memcpy(PWD + strlen("PWD="), cwd, strlen(cwd) + 1);

        putenv(PWD);
    }

    return CONTINUE;
}

int clear()
{
    system("clear");
    return CONTINUE;
}

int dir(char** args) 
{
    if (output_redirection)
    {
        pid_t pid;
        pid = fork();

        if (pid < 0) 
        {
            printf("[ERROR] Couldn't create child process!\n");
        }
        else if (pid == 0) 
        {
            open_file_descriptor(1);
            dir_print(args);
            exit(1);
        }
    }
    else
    {
        dir_print(args);
    }

    return CONTINUE;
}

void dir_print(char** args) 
{
    DIR *directory;
    struct dirent *directory_entity;

    if (args[1] != NULL)
    {
        directory = opendir(args[1]);
    }
    else
    {
        directory = opendir(".");
    }

    while ((directory_entity = readdir(directory)))
        printf("%s ", directory_entity->d_name);
            
    printf("\n");

    if (directory_entity != NULL)
    {
        if (strcmp(".", directory_entity->d_name) && strcmp("..", directory_entity->d_name))
            closedir(directory);
    }
}

int env()
{
    if (output_redirection)
    {
        pid_t pid;
        pid = fork();

        if (pid < 0) 
        {
            printf("[ERROR] Couldn't create child process!\n");
        }
        else if (pid == 0) 
        {
            open_file_descriptor(1);
            
            int i = 0;

            while (environ[i] != NULL)
            {
                printf("%s\n", environ[i]);
                ++i;
            }

            exit(1);
        }
    }
    else
    {
        int i = 0;

        while (environ[i] != NULL)
        {
            printf("%s\n", environ[i]);
            ++i;
        }
    }
    
    return CONTINUE;
}

int echo(char** args) 
{
    if (args[1] == NULL)
    {
        printf("[ERROR] Nothing to print (missing second string argument)!\n");
        return CONTINUE;
    }

    if (output_redirection)
    {
        pid_t pid;
        pid = fork();

        if (pid < 0) 
        {
            printf("[ERROR] Couldn't create child process!\n");
        }
        else if (pid == 0) 
        {
            open_file_descriptor(1);
            
            int i = 1;

            while (args[i] != NULL)
            {
                printf("%s ", args[i]);
                ++i;
            }

            printf("\n");

            exit(1);
        }
    }
    else
    {
        int i = 1;

        while (args[i] != NULL)
        {
            printf("%s ", args[i]);
            ++i;
        }

        printf("\n");
    }
    
    return CONTINUE;
}

int help() 
{
    char* help_text =
    "Simple C-shell.\n\n"

    "Built-in commands:\n"
    "\tcd <directory> - change the current working directory to a <directory> argument;\n"
    "\tclr - clear all command shell output;\n"
    "\tdir <directory> - output the file contents of the current working directory, or the <directory> argument;\n"
    "\tenviron - display a list of environment variables;\n"
    "\techo <comment> - display a <comment> argument;\n"
    "\thelp - display information about shell and the commands available in it with more filter (shell crashing bug);\n"
    "\tpause - pause the execution of operations in the command shell until the [Enter] key is pressed;\n"
    "\tquit - terminate all active processes and close the command shell;\n"
    "\tjobs - display a list of all shell processes (both active and terminated).\n\n"

    "Run shell with the batchfile argument to get commands from it.\n"
    "Use '<', '>' and \">>\" symbols for I/O redirection.\n"
    "Run tasks in background using '&' symbol in the end of command.";

    if (output_redirection)
    {
        pid_t pid;
        pid = fork();

        if (pid < 0) 
        {
            printf("[ERROR] Couldn't create child process!\n");
        }
        else if (pid == 0) 
        {
            open_file_descriptor(1);
            printf("%s", help_text);
            exit(1);
        }
    }
    else
    {
        char* more_execvp[1][2] = 
        {
            { "more", NULL }, // command, arguments
        };

        int p[2]; // 0 - чтение, 1 - запись
        int pipe_descriptor = STDIN_FILENO; // Сохраняем дескриптор стандартного ввода stdin

        // Создаём канал данных, который будет использоваться для взаимодействия между процессами
        if (pipe(p) == -1)
            printf("[ERROR] Failed to create a pipe!\n");

        pid_t pid;
        pid = fork();

        if (pid < 0)
        {
            printf("[ERROR] Couldn't create child process!\n");
        }
        else if (pid == 0)
        {
            if (pipe_descriptor != STDIN_FILENO)
            {
                // Перенаправляем предыдущий канал на стандартный ввод
                dup2(pipe_descriptor, STDIN_FILENO);
                close(pipe_descriptor);
            }

            // Перенаправляем стандартный вывод stdout на текущий канал
            dup2(p[1], STDOUT_FILENO);
            close(p[1]);

            printf("%s", help_text); // Выполняем команду "help" с перенаправленным выводом

            exit(1);
        }
        else
        {
            // Закрываем для чтения предыдущий канал, так как его больше не существует (дочерний процесс завершил свою работу)
            close(pipe_descriptor);
            // Закрываем для записи текущий канал
            close(p[1]);

            // Сохраняем то, что мы прочитали (результат выполнения команды "help")
            pipe_descriptor = p[0];

            if (pipe_descriptor != STDIN_FILENO)
            {
                // Перенаправляем это на стандартный ввод
                dup2(pipe_descriptor, STDIN_FILENO);
                close(pipe_descriptor);
            }

            // Выполняем команду "more" с перенаправленным вводом
            if (execvp(more_execvp[0][0], more_execvp[0]) == -1)
            {
                printf("[ERROR] Couldn't execute unknown command!\n");
            }
            else
            {
                printf("\nThe end of the help command has been reached!\n");
            }
        }
    }

    return CONTINUE;
}

int getch()
{
   struct termios oldt, newt;
   int c;
   
   tcgetattr( STDIN_FILENO, &oldt );
   newt = oldt;
   newt.c_lflag &= ~( ICANON | ECHO );
   
   tcsetattr( STDIN_FILENO, TCSANOW, &newt );
   c = getchar();
   tcsetattr( STDIN_FILENO, TCSANOW, &oldt );
   
   return c;
}

int pause()
{
	int pressed_key = -1;
    
    while (pressed_key != 10) // 10 - [Enter] key code
    {
    	printf("Press [Enter] key to resume...\n");
    	pressed_key = getch();
    }
    
    return CONTINUE;
}

int quit() 
{
    bg_task* background_task;

    // Игнорируем (SIG_IGN) сигналы, посылаемые дочерними (SIGCHLD) процессами при изменении их статуса (завершён, приостановлен или возобновлён)
    signal(SIGCHLD, SIG_IGN);

    if (!shell_tasks.foreground.finished)
        kill_foreground();

    for (size_t i = 0; i < shell_tasks.iterator; ++i)
    {
        background_task = &shell_tasks.background[i];

        if (!background_task->finished)
            kill(background_task->pid, SIGTERM); // SIGTERM - сигнал для запроса завершения процесса по его идентификатору (PID)
    }

    return EXIT;
}

int jobs()
{
    bg_task* background_task;

    for (size_t i = 0; i < shell_tasks.iterator; ++i)
    {
        background_task = &shell_tasks.background[i];

        printf(
            "[%zu] pid: %d; command: %s; state: %s\n",
            i,
            background_task->pid,
            background_task->command,
            background_task->finished ? "finished" : "active"
        );
    }

    return CONTINUE;
}

int launch(char** args) 
{
    pid_t pid;
    pid = fork();

    if (pid < 0) 
    {
        printf("[ERROR] Couldn't create child process!\n");
    }
    else if (pid == 0) 
    {
        if (is_background_task)
            // Помещаем фоновый (дочерний) процесс в новую группу процессов. Главный (родительский) процесс при этом просто продолжает свою работу
            setpgid(0, 0);

        if (input_redirection)
            open_file_descriptor(0);

        if (output_redirection)
            open_file_descriptor(1);

        if (execvp(args[0], args) == -1)
            printf("[ERROR] Couldn't execute unknown command!\n");

        exit(1);
    }
    else 
    {
        // Если текущий процесс - фоновый, то добавляем его в список фоновых процессов shell_tasks.background и не ждём (waitpid) его завершения в главном процессе
        if (is_background_task)
        {
            if (add_background(pid, args[0]) == 0)
                quit();
        }
        else
        {
            set_foreground(pid);

            if (waitpid(pid, NULL, 0) == -1) 
            {
                if (errno != EINTR)
                    printf("[ERROR] Couldn't track the completion of the process!\n");
            }
        }
    }

    return CONTINUE;
}

void set_foreground(pid_t pid) 
{
    shell_tasks.foreground.pid = pid;
    shell_tasks.foreground.finished = 0;
}

void kill_foreground() 
{
    if (shell_tasks.foreground.pid != -1) 
    {
        kill(shell_tasks.foreground.pid, SIGTERM); // SIGTERM - сигнал для запроса завершения процесса по его идентификатору (PID)
        shell_tasks.foreground.finished = true;
        printf("\n");
    }
}

int add_background(pid_t pid, char* command)
{
    bg_task* background_task;

    // Если закончилось место в списке фоновых процессов shell_tasks.background
    if (shell_tasks.iterator >= shell_tasks.size)
    {
        // Выделяем дополнительную память под новый процесс
        shell_tasks.size = shell_tasks.size * 2 + 1;
        shell_tasks.background = (bg_task*)realloc(shell_tasks.background, sizeof(bg_task) * shell_tasks.size);

        if (shell_tasks.background == NULL)
        {
            printf("[ERROR] Couldn't reallocate buffer for background tasks!\n");
            return EXIT;
        }
    }

    // Сохраняем информацию о процессе в списке
    background_task = &shell_tasks.background[shell_tasks.iterator];
    background_task->pid = pid;
    background_task->finished = false;
    strcpy(background_task->command, command);

    printf("[%zu] %d\n", shell_tasks.iterator, background_task->pid);

    shell_tasks.iterator += 1; // Берём следующий фоновый процесс

    return CONTINUE;
}

void catch_finished_task()
{
    bg_task* background_task;
    pid_t pid = waitpid(-1, NULL, 0); // Ловим завершение любого (главного или фонового) из запущенных процессов

    // В зависимости от типа процесса, помечаем его как завершённый
    if (pid == shell_tasks.foreground.pid)
    {
        shell_tasks.foreground.finished = true;
    }
    else
    {
        for (size_t i = 0; i < shell_tasks.iterator; ++i)
        {
            background_task = &shell_tasks.background[i];

            if (background_task->pid == pid)
            {
                printf("\n[%zu]+ Done\n", i);
                background_task->finished = true;
                break;
            }
        }
    }
}

void open_file_descriptor(int descriptor_type)
{
    int descriptor = -1;

    switch (descriptor_type)
    {
        case 0:
            if ((descriptor = open(input_file_name, O_RDONLY, 0)) < 0)
                printf("[ERROR] Failed to open the input file!\n");

            dup2(descriptor, STDIN_FILENO);
            close(descriptor);
            break;

        case 1:
            if (append_output_file)
            {
                if ((descriptor = open(output_file_name, O_CREAT | O_RDWR | O_APPEND, 0644)) < 0)
                    printf("[ERROR] Failed to open the output file!\n");
            }
            else if ((descriptor = creat(output_file_name, 0644)) < 0)
                printf("[ERROR] Failed to open the output file!\n");

            dup2(descriptor, STDOUT_FILENO);
            close(descriptor);
            break;
        
        default:
            printf("[ERROR] An unsupported file descriptor type was specified!\n");
            break;
    }
}
