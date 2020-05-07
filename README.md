# smallsh
Basic shell written in C that emulates some of the functions found natively in Linux's bash. This is mostly an exercise in better understanding forking, executing and killing multiple processes.

## Using smallsh
1) Clone this repo
2) Compile smallsh.c using `make`
3) Run smallsh with `./smallsh`
4) Available commands are listed below

### Commands
The commands available in smallsh are created to mirror the same commands used in bash.
#### exit
Kills all child processes spawned from smallsh before closing smallsh
#### cd
Moves the working directory to the specified directory. If no directory is specified, moves working directory to HOME. Both relative and absolute paths are supported.
#### status
Prints terminating signal or exit status of the last foreground process. Exit status of 0 is printed if no foreground process has been run yet.

### More Commands
Smallsh can also accept other commands used in bash by passing the command through smallsh and returning the result. Here are some examples to try out:
- `ls`
- `pwd`
- `date`
- `sleep [seconds]`
- `echo [text]`

### Signals
#### SIGINT
CTRL-C sends a SIGINT signal and will terminate a foreground process if one is running. This signal will not terminate the smallsh process. The exit command can instead be used to kill smallsh.
#### SIGTSTP
CTRL-Z sends a SIGTSTP signal and will toggle foreground-only mode ON or OFF. In foreground-only mode, processes will no longer be able to be run in the background with '&'. If '&' is included with a command it will simply be ignored.
