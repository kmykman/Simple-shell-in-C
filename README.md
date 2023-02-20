# Simple-shell-in-C
An interactive shell in C

## Author
Man Yuet Ki Kimmy

## Features
#### Interactive command
e.g.
```
ls -la
```
#### Excute valid program with absolute path or relative path
```
/home/word/a.out
```
```
./a.out
```
#### Interrupt signal depends on executed program
If a program is executed in this shell, the reaction after `Ctrl-C` depends on the feature set in this executed program.

#### Pipe command
```
cat a.txt | grep t | wc -l
```
#### Exit shell
This shell program ignore `Ctrl-C`.  You can only terminate with
```
exit
```
It release all its resources and then terminate.

#### Showing executing details
Use `timeX` before the command to show the terminated process's ID, program name (without the path), user cpu time and system cpu time.
e.g.
Input
```
timeX cat a.txt | grep t | wc -l
```
Output
```
10
(PID)89  (CMD)cat  (user)0.001 s  (sys)0.000 s
(PID)90  (CMD)grep  (user)0.001 s  (sys)0.000 s
(PID)91  (CMD)wc  (user)0.001 s  (sys)0.000 s
```
#### Execute program in the background
Add `&` behind the command needed to be executed in the background.
```
vi &
```

#### Invalid input
If invalid input is inputted, error message is given.

## Usage
```
gcc 3230shell.c -o 3230shell
./3230shell
```

Copyright 2022 Man Yuet Ki Kimmy
