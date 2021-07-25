# smallSH

An excerpt from an academic assignment of a small shell program written in C that has 3 built in commands and relies on bash for the rest of the commands.

The full program supports

* a prompt for running commands
* blank lines and comments
* expansion for the variable $$
* 3 'built-in' commands 
  * exit
  * cd
  * status
* other commands from the exec family of functions
* input and output redirection
* running commands in foreground and background processes
* custom handlers for 2 signals
  * SIGINT
  * SIGTSTP
