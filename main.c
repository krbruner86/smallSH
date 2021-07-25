/*
* runs the non built in commands
* parts of code adapted from exploration creating and terminating proccesses, montorring child processes, executing a new program, and process I/O
*/
void otherComArgs(char** parsed, int* argC)
{
	// Forking a child 
	pid_t newProcess;
	int childStatus;
	int background = 0;
	int inRedirect = 0;
	int inRedirectLoc = 0;
	int outRedirect = 0;
	int outRedirectLoc = 0;
	char* str[512];

	for (int i = 0; i < *argC - 1; i++) {
		if (strcmp(parsed[i], ">") == 0) {
			outRedirectLoc = i + 1;
			outRedirect++;
		}
		else if ((strcmp(parsed[i], "<") == 0)) {
			inRedirectLoc = i + 1;
			inRedirect++;
		}
	}

	if (argC > 0 && strcmp(parsed[*argC - 1], "&") == 0) {

		if (sigStpFlag) {
			parsed[*argC - 1] = '\0';
		}
		else {
			background = 1;
			for (int i = 0; i < *argC - 1; i++) {
				str[i] = parsed[i];
			}
			parsed = str;
		}
	}

	parsed[*argC] = NULL;

	newProcess = fork();
	if (newProcess == -1) {
		printf("\nFailed forking child..");
		return;
	}
	else if (newProcess == 0){

		if (outRedirect && inRedirect) {
			// Open source file
			int sourceFD = open(parsed[inRedirectLoc], O_RDONLY);
			if (sourceFD == -1) {
				perror("source open()");
				exit(1);
			}

			// Redirect stdin to source file
			int result = dup2(sourceFD, 0);
			if (result == -1) {
				perror("source dup2()");
				exit(2);
			}

			// Open target file
			int targetFD = open(parsed[outRedirectLoc], O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (targetFD == -1) {
				perror("target open()");
				exit(1);
			}

			// Redirect stdout to target file
			result = dup2(targetFD, 1);
			if (result == -1) {
				perror("target dup2()");
				exit(2);
			}
			parsed[1] = NULL;
			if (execvp(parsed[0], parsed) < 0) {
				printf("Could not execute command..\n");
			}
			exit(0);

		}
		else if (outRedirect) {
			// Open target file
			int targetFD = open(parsed[outRedirectLoc], O_WRONLY | O_CREAT | O_TRUNC, 0644);
			if (targetFD == -1) {
				perror("target open()");
				exit(1);
			}

			// Redirect stdout to target file
			int result = dup2(targetFD, 1);
			if (result == -1) {
				perror("target dup2()");
				exit(2);
			}

			parsed[1] = NULL;
			if (execvp(parsed[0], parsed) < 0) {
				printf("Could not execute command..\n");
				fflush(stdout);
			}
			exit(0);

		}
		else if (inRedirect) {
			// Open source file
			int sourceFD = open(parsed[inRedirectLoc], O_RDONLY);
			if (sourceFD == -1) {
				perror("source open()");
				exit(1);
			}

			// Redirect stdin to source file
			int result = dup2(sourceFD, 0);
			if (result == -1) {
				perror("source dup2()");
				exit(2);
			}

			parsed[1] = NULL;
			if (execvp(parsed[0], parsed) < 0) {
				printf("Could not execute command..\n");
			}
			exit(0);

		}
		else {
			if (execvp(parsed[0], parsed) < 0) {
				printf("Could not execute command..\n");
			}
		}
	}
	else {
		// waiting for child to terminate
		if (background) {
			printf("background pid is %d\n", getpid());
			fflush(stdout);
			pid_t cpid = waitpid(newProcess, &childStatus, WNOHANG);
		}
		else {
			signal(SIGINT, handle_SIGINT);
			waitpid(newProcess, &childStatus, 0);
		}

		if (WIFEXITED(childStatus)) {

			exitStatus = WEXITSTATUS(childStatus);

			if (background) {
				printf("background pid %d is done: exit value %d\n", getpid(), WEXITSTATUS(childStatus));
				fflush(stdout);
			}

		}
		else{
			exitStatus = WTERMSIG(childStatus);
		}

		if (background) {
			printf("background pid %d is done: exit value %d\n", getpid(), WEXITSTATUS(childStatus));
			fflush(stdout);
		}
	}
	return;
}

/*
* runs the built in commands
*/
int builtInCom(char** parsed)
{
	if (strcmp(parsed[0], "exit") == 0) {
		exit(0);
	}
	else if (strcmp(parsed[0], "cd") == 0) {
		chdir(parsed[1]);
		return 1;
	}
	else if (strcmp(parsed[0], "status") == 0) {

		//print exit status or terminating signal of last foreground process
		//not including the built-ins
		if (exitStatus != -99) {
			printf("exit status %d\n", exitStatus);
			exitStatus = -99;
		}

		return 1;
	}
	else {
		return 0;
	}

}

/*
* parse the args of the commands
*/
void parseArgs(char* str, char** parsed, int *numOfArgs)
{
	char* saveptr;
	char* currLine = str;
	char* token;
	int p = 0;
	int count = 0;
	int found = 0;

	//https://stackoverflow.com/questions/8257714/how-to-convert-an-int-to-string-in-c
	//convert pid to string
	int intPid = getpid();
	int length = snprintf(NULL, 0, "%d", intPid);
	char* strPid = malloc(length + 1);
	snprintf(strPid, length + 1, "%d", intPid);

	// get command
	parsed[0] = strtok_r(currLine, " ", &saveptr);

	// loop through and get arguments
	for (int i = 1; i < 512; i++) {
		token = strtok_r(NULL, " ", &saveptr);

		if (token != NULL) {

			// look for variable to expand
			for (int j = 0; j < strlen(token); j++) {
				if ((token[j] == '$') && (token[j + 1] == '$')) {
					found++;
					break;
				}
			}

			// expand variable
			if (found) {

				char tempToken[strlen(token) + strlen(strPid) - 1];
				tempToken[-1] = '\0';

				for (int j = 0; j < strlen(token); j++) {
					if ((token[j] == '$') && (token[j + 1] == '$')) {
						p = j;
						for (int k = 0; k < strlen(strPid); k++) {
							tempToken[p] = strPid[k];
							count++;
							if (k != (strlen(strPid) - 1)) {
								p++;
							}
						}
						j++;
					}
					else {
						tempToken[count] = token[j];
						count++;
					}
				}

				//allocate memory and copy into array
				parsed[i] = strdup(tempToken);
				found = 0;
			}
			else {
				parsed[i] = token;
			}
		} else {
			parsed[i] = NULL;
		}

		if (parsed[i] == NULL) {
			*numOfArgs = i;
			break;
		}
	}

	free(strPid);
}

/*
* split the user input and run the built in command if necessary
*/
int processInput(char* str, char** parsed, int* argNum)
{
    parseArgs(str, parsed, argNum);

    if (builtInCom(parsed))
        return 0;
    else
        return 1;
}

/*
* check for exit status of process and print them out
*/
void checkEndProc(void) {

	if (exitStatus != -99) {
		printf("exit status %d\n", exitStatus);
		exitStatus = -99;
	}

	return;
}
