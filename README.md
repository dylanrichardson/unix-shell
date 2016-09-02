This program implements a basic command shell for Unix. It supports command execution, interactive prompts, and background tasks. Limitations of this shell are the lack of built in commands, I/O redirection, and arrow key functionality. It was built for WPI CS 3013 Operating Systems.

#### Examples

Command execution
```
$ ./shell ls -la
total 751
drwxrws--x 2 drich14 drich14    183 Sep  2 09:12 .
drwxrws--x 3 drich14 drich14     26 Aug 27 21:40 ..
-rwxrwx--x 1 drich14 drich14  11639 Sep  2 09:11 shell.cpp
-rwxrwx--x 1 drich14 drich14      0 Sep  2 09:18 example.txt
-rwxrwx--x 1 drich14 drich14    106 Aug 30 17:33 makefile
-rwxrwx--x 1 drich14 drich14    110 Sep  2 08:11 README.md
-rwxrwx--x 1 drich14 drich14     66 Sep  2 09:13 sleep10.sh

Statistics for process: ls
------------------
Wall time elapsed: 0.00569201 ms
User CPU time used: 0 ms
System CPU time used: 0.004 ms
Major page faults: 0
Minor page faults: 328
Voluntary context switches: 12
Involuntary context switches: 9
Maximum resident set size: 1008
------------------
```

Interactive prompts and background tasks
```
$ ./shell
==>cd ..
==>ls
shell

Statistics for process: ls
------------------
Wall time elapsed: 0.00460696 ms
User CPU time used: 0.004 ms
System CPU time used: 0 ms
Major page faults: 0
Minor page faults: 290
Voluntary context switches: 2
Involuntary context switches: 1
Maximum resident set size: 856
------------------
==>cd shell
==>jobs
==>sleep 20 &
[1] 8367
==>echo now
now

Statistics for process: echo
------------------
Wall time elapsed: 0.0023129 ms
User CPU time used: 0.004 ms
System CPU time used: 0 ms
Major page faults: 0
Minor page faults: 477
Voluntary context switches: 3
Involuntary context switches: 4
Maximum resident set size: 856
------------------
==>./sleep10.sh &
[2] 15409
==>sleeping for 10 seconds

==>jobs
[1] 8367 sleep
[2] 15409 ./sleep10.sh
==>exit
Cannot exit while background tasks are still running

Statistics for process: ./sleep10.sh
------------------
Wall time elapsed: 10.0119 ms
User CPU time used: 0 ms
System CPU time used: 0 ms
Major page faults: 0
Minor page faults: 573
Voluntary context switches: 15
Involuntary context switches: 11
Maximum resident set size: 1144
------------------

Statistics for process: sleep
------------------
Wall time elapsed: 20.0052 ms
User CPU time used: 0 ms
System CPU time used: 0 ms
Major page faults: 0
Minor page faults: 194
Voluntary context switches: 2
Involuntary context switches: 1
Maximum resident set size: 532
------------------

[1] 8367 sleep done
[1] 15409 ./sleep10.sh done
==>jobs
==>exit
```
