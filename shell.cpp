/*
The limitations of this shell compared to a regular Unix shell are the built in
commands. Regular Unix shells like Bash have many built in commands that are not
implemented in this shell such as "help" and "logout". Also this shell does not
have any I/O redirection functionality or special characters besides "&". Use of
the arrow keys does not work either.
*/

#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <time.h>
#include <sys/time.h>
#include <sstream>
#include <algorithm>

using namespace std;

// constant for the maximum number of arguments in one line
const int ARGV_SIZE = 33;
// global vector for storing background task pid's and process names
vector<int> bgPid;
vector<char*> bgName;
vector<int> bgParent;

void repl();
void runCmd(char*[]);
void runCmd(char*[], int);
void runBGTask(char*[]);
void execute(char*[]);
int input(char*[]);
int parseLine(string, char*[]);
void displayStats(char*, double);
void displayBGTasks();
char** getNewArgs(int, char*[]);
double getWallTime();
double getCPUTime();
double convertTimeval(struct timeval);
vector<string> split(const string &s, char);
void fillArgs(vector<string>, char*[]);
void cd(int, char*[]);
void cleanBGTasks();

/**
 * Run the program.
 *
 * @param  int  argc
 * @param  char* argv[]
 * @return int
 */
int main(int argc, char* argv[]) {
    // run the repl if no arguments were passed in
    if (argc == 1) {
        repl();
    } else {
        // get the arguments to execute
        char** argvNew = argv + 1;
        // run the command from the arguments with stats displayed
        runCmd(argvNew);
    }
}

/**
 * Run the repl.
 */
void repl() {
    // array for arguments from user input
    char* argv[ARGV_SIZE];
    // store the number of arguments
    int argc;
    // loop until the exit command is called
    while (true) {
        // check for completed background tasks
        cleanBGTasks();
        // get the arguments from the user
        argc = input(argv);
        // check for an empty line
        if (!argc) {
            continue;
        // check for built in commands
        } else if (!strcmp(argv[0], "exit")) {
            // check if any background tasks are still running
            if (bgPid.size()) {
                cerr << "Cannot exit while background tasks are still running\n";
                continue;
            } else {
                // exit the shell by breaking the loop
                break;
            }
        } else if (!strcmp(argv[0], "cd")) {
            // run the change directory command
            cd(argc, argv);
        } else if (!strcmp(argv[0], "jobs")) {
            // display background tasks that are running
            displayBGTasks();
        // check for background task indicator
        } else if (!strcmp(argv[argc - 1], "&")) {
            // remove the & character
            argv[argc - 1] = NULL;
            // run the command in the background
            runBGTask(argv);
        } else {
            // run the command from the arguments with stats displayed
            runCmd(argv);
        }
    }
}

/**
 * Run the command specified by the arguments while recording stats.
 *
 * @param  char* argv[]
 */
void runCmd(char* argv[]) {
    int pid;
    double wallTimeStart;
    // start timer
    wallTimeStart = getWallTime();
    // fork this process and remember pid
    if ((pid = fork()) < 0) {
        cerr << "Fork error\n";
        exit(1);
    } else if (pid == 0) {
        /* child process */
        // execute the command from the arguments and stop
        execute(argv);
    } else {
        /* parent process */
        int status;
        // wait for the child to finish and get the exit status
        wait(&status);
        // check that the child did not exit with an error
        if (WIFEXITED(status) && WEXITSTATUS(status) != 1) {
            // display usage stats about the child process
            displayStats(argv[0], wallTimeStart);
        }
    }
}

/**
 *  Run the command specified by the arguments while recording stats. Write
 *    the child pid to the given file descriptor.
 *
 * @param  char* argv[]
 * @param  int fd
 */
void runCmd(char* argv[], int fd) {
    int pid;
    double wallTimeStart;
    // start timer
    wallTimeStart = getWallTime();
    // fork this process and remember pid
    if ((pid = fork()) < 0) {
        cerr << "Fork error\n";
        exit(1);
    } else if (pid == 0) {
        /* child process */
        // write the child pid to the file descriptor
        pid = getpid();
        write(fd, &pid, sizeof(int));
        // execute the command from the arguments and stop
        execute(argv);
    } else {
        /* parent process */
        int status;
        // wait for the child to finish and get the exit status
        wait(&status);
        // check that the child did not exit with an error
        if (WIFEXITED(status) && WEXITSTATUS(status) != 1) {
            // display usage stats about the child process
            displayStats(argv[0], wallTimeStart);
        }
    }
}

/**
 * Run the command in the background.
 *
 * @param  char* argv[]
 */
void runBGTask(char* argv[]) {
    // create a pipe for transfering the grandchild pid to this process
    int fd[2];
    pipe(fd);
    int pid;
    // fork this process and remember pid
    if ((pid = fork()) < 0) {
        cerr << "Fork error\n";
        exit(1);
    } else if (pid == 0) {
        /* child process */
        // write the child pid to the file descriptor
        close(fd[0]);
        pid = getpid();
        write(fd[1], &pid, sizeof(int));
        // run the command from the arguments passing the file descriptor for
        //   the grandchild pid
        runCmd(argv, fd[1]);
        // exit the process
        exit(0);
    } else {
        /* parent process */
        // read child and grandchild pid from pipe
        close(fd[1]);
        // child pid
        read(fd[0], &pid, sizeof(int));
        bgParent.push_back(pid);
        // grandchild pid
        read(fd[0], &pid, sizeof(int));
        // indicate background task started
        cout << "[" << bgPid.size() + 1 << "] " << pid << "\n";
        bgPid.push_back(pid);
        bgName.push_back(strdup(argv[0]));
    }
}

/**
 * Execute the command specified by the arguments.
 *
 * @param  char* argv[]
 */
void execute(char* argv[]) {
    // execute the command
    if (execvp(argv[0], argv) < 0) {
        cerr << "Execvp error\n";
        exit(1);
    }
}

/**
 * Return user input as an array of strings split by spaces.
 *
 * @param  char* argv[]
 * @return  bool
 */
int input(char* argv[]) {
    // string for holding input
    string line;
    // prompt user and get input
    cout << "==>";
    getline(cin, line);
    // parse the input and return the command identifier
    return parseLine(line, argv);
}

/**
 * Convert the given string to an array of arguments. Return the number of
 *   arguments.
 *
 * @param  char line[]
 * @param  char* argv[]
 * @return  bool
 */
int parseLine(string line, char* argv[]) {
    // split the input line by spaces and store the strings in a vector
    vector<string> argvStr = split(line, ' ');
    // fill the argument array with the vector of split strings
    fillArgs(argvStr, argv);
    // return the number of arguments
    return argvStr.size();
}

/**
 * Print out statistics about the child process.
 *
 * @param  char* name
 * @param  double wallTimeStart
 */
void displayStats(char* name, double wallTimeStart) {
    // end wall timer
    double wallTimeEnd = getWallTime();
    // calculate elapsed time
    double wallTime = wallTimeEnd - wallTimeStart;
    // get usage from child process
    struct rusage usage;
    getrusage(RUSAGE_CHILDREN, &usage);
    // get user and system cpu time
    double userCPUTime   = convertTimeval(usage.ru_utime);
    double systemCPUTime = convertTimeval(usage.ru_stime);
    // print statistics about the execution
    cout << "\nStatistics for process: " << name << "\n";
    cout << "------------------\n";
    cout << "Wall time elapsed: "            << wallTime        << " ms\n";
    cout << "User CPU time used: "           << userCPUTime     << " ms\n";
    cout << "System CPU time used: "         << systemCPUTime   << " ms\n";
    cout << "Major page faults: "            << usage.ru_majflt << "\n";
    cout << "Minor page faults: "            << usage.ru_minflt << "\n";
    cout << "Voluntary context switches: "   << usage.ru_nvcsw  << "\n";
    cout << "Involuntary context switches: " << usage.ru_nivcsw << "\n";
    cout << "Maximum resident set size: "    << usage.ru_maxrss << "\n";
    cout << "------------------\n";
}

/**
 * Print the process id and name of each background task.
 */
void displayBGTasks() {
    for (int i = 0; i < bgPid.size(); i++) {
        cout << "[" << i + 1 << "] " << bgPid[i] << " " << bgName[i] << "\n";
    }
}

/**
 * Return the wall time.
 *
 * @return double
 */
double getWallTime() {
    struct timeval time;
    // store time of day in timeval struct
    if (gettimeofday(&time, NULL)) {
        cerr << "Gettimeofday error\n";
        exit(1);
    }
    // add the seconds and microseconds together
    return convertTimeval(time);
}

/**
 * Convert the timeval structure to milliseconds.
 *
 * @param  timeval  time
 * @return double
 */
double convertTimeval(struct timeval time) {
    // add the seconds and microseconds together
    return (double) time.tv_sec + (double) time.tv_usec * 0.000001;
}

/**
 * Split the string by the delimiter.
 *
 * @param  const string &s
 * @param  char delim
 */
vector<string> split(const string &s, char delim) {
    // store the strings in a vector
    vector<string> elems;
    // create a stream of the string to read from
    stringstream ss(s);
    // token string
    string item;
    // read until the delimiter or end of file
    while (getline(ss, item, delim)) {
        // append the non empty token string to the vector
        if (!item.empty()) {
            elems.push_back(item);
        }
    }
    // return the vector of strings
    return elems;
}

/**
 * Convert the vector of strings to an array of c strings.
 *
 * @param  vector<string> vec
 * @param  char* arr[]
 */
void fillArgs(vector<string> vec, char* arr[]) {
    // convert vector of strings to array of c strings
    for (int i = 0; i < vec.size(); i++) {
        arr[i] = (char*) vec[i].c_str();
    }
    // end the argument array with null
    arr[vec.size()] = NULL;
}

/**
 * Run the change directory command with the given arguments
 *
 * @param  int argc
 * @param  char* argv[]
 */
void cd(int argc, char* argv[]) {
    if (argc > 2) {
        cerr << "cd: wrong number of arguments\n";
    } else if (argc == 1) {
        chdir("/");
    } else {
        chdir(argv[1]);
    }
}

/**
 * Iterate over all background tasks, remove any that have exited, and print out
 *   a message.
 */
void cleanBGTasks() {
    int status;
    // loop over all the background tasks
    for (int i = 0; i < bgPid.size(); i++) {
        // get the status of the task
        status = waitpid(bgParent[i], NULL, WNOHANG);
        // check if the task has exited
        if (status) {
            cout << "[" << i + 1 << "] " << bgPid[i] << " " << bgName[i] << " done\n";
            bgPid.erase(bgPid.begin() + i);
            free(bgName[i]);
            bgName.erase(bgName.begin() + i);
            bgParent.erase(bgParent.begin() + i);
            i--;
        }
    }
}
