myShell
==========
A terminal interface for UNIX systems programmed in C
----------
Pair Programmed by **Andrew Walters** and **Matthew Thorp**
----------
The primary control structure of the shell is an infinite loop that
-reads input from STDIN
-checks for specific inputs such as "exit" which will break out of loop
-parses the input in to tokens, or space separated strings
-prepares an array of tokens to be passed into a function
-forks the process, creating a child process that can run in the background or foreground

The shell also handles signals
-CTRL+C (SIG_IGN) kills the currently executing child process, but not the shell
-CTRL+Z (SIGTSTP) suspends the currently executing child process but allows the shell to continue execution
