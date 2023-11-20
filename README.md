Simple C-shell.

## Built-in commands:
1. `cd <directory>` - change the current working directory to a `<directory>` argument;
2. `clr` - clear all command shell output;
3. `dir <directory>` - output the file contents of the current working directory, or the `<directory>` argument;
4. `environ` - display a list of environment variables;
5. `echo <comment>` - display a `<comment>` argument;
6. `help` - display information about shell and the commands available in it with `more` filter (shell crashing bug);
7. `pause` - pause the execution of operations in the command shell until the `[Enter]` key is pressed;
8. `quit` - terminate all active processes and close the command shell;
9. `jobs` - display a list of all shell processes (both active and terminated).

Run shell with the `batchfile` argument to get commands from it.<br/>
Use `<`, `>` and `>>` symbols for I/O redirection.<br/>
Run tasks in background using `&` symbol in the end of command.

## Getting started
```bash
git clone https://github.com/ghub-ayrtom/simple-c-shell.git
cd simple-c-shell

make
./myshell

help
```
