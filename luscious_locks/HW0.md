# Welcome to Homework 0!

For these questions you'll need the mini course and  "Linux-In-TheBrowser" virtual machine (yes it really does run in a web page using javascript) at -

http://cs-education.github.io/sys/

Let's take a look at some C code (with apologies to a well known song)-
```C
// An array to hold the following bytes. "q" will hold the address of where those bytes are.
// The [] mean set aside some space and copy these bytes into teh array array
char q[] = "Do you wanna build a C99 program?";

// This will be fun if our code has the word 'or' in later...
#define or "go debugging with gdb?"

// sizeof is not the same as strlen. You need to know how to use these correctly, including why you probably want strlen+1

static unsigned int i = sizeof(or) != strlen(or);

// Reading backwards, ptr is a pointer to a character. (It holds the address of the first byte of that string constant)
char* ptr = "lathe"; 

// Print something out
size_t come = fprintf(stdout,"%s door", ptr+2);

// Challenge: Why is the value of away equal to 1?
int away = ! (int) * "";


// Some system programming - ask for some virtual memory

int* shared = mmap(NULL, sizeof(int*), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
munmap(shared,sizeof(int*));

// Now clone our process and run other programs
if(!fork()) { execlp("man","man","-3","ftell", (char*)0); perror("failed"); }
if(!fork()) { execlp("make","make", "snowman", (char*)0); execlp("make","make", (char*)0)); }

// Let's get out of it?
exit(0);
```

## So you want to master System Programming? And get a better grade than B?
```C
int main(int argc, char** argv) {
	puts("Great! We have plenty of useful resources for you, but it's up to you to");
	puts(" be an active learner and learn how to solve problems and debug code.");
	puts("Bring your near-completed answers the problems below");
	puts(" to the first lab to show that you've been working on this.");
	printf("A few \"don't knows\" or \"unsure\" is fine for lab 1.\n"); 
	puts("Warning: you and your peers will work hard in this class.");
	puts("This is not CS225; you will be pushed much harder to");
	puts(" work things out on your own.");
	fprintf(stdout,"This homework is a stepping stone to all future assignments.\n");
	char p[] = "So, you will want to clear up any confusions or misconceptions.\n";
	write(1, p, strlen(p) );
	char buffer[1024];
	sprintf(buffer,"For grading purposes, this homework 0 will be graded as part of your lab %d work.\n", 1);
	write(1, buffer, strlen(buffer));
	printf("Press Return to continue\n");
	read(0, buffer, sizeof(buffer));
	return 0;
}
```
## Watch the videos and write up your answers to the following questions

**Important!**

The virtual machine-in-your-browser and the videos you need for HW0 are here:

http://cs-education.github.io/sys/

The coursebook:

http://cs341.cs.illinois.edu/coursebook/index.html

Questions? Comments? Use Ed: (you'll need to accept the sign up link I sent you)
https://edstem.org/

The in-browser virtual machine runs entirely in Javascript and is fastest in Chrome. Note the VM and any code you write is reset when you reload the page, *so copy your code to a separate document.* The post-video challenges (e.g. Haiku poem) are not part of homework 0 but you learn the most by doing (rather than just passively watching) - so we suggest you have some fun with each end-of-video challenge.

HW0 questions are below. Copy your answers into a text document (which the course staff will grade later) because you'll need to submit them later in the course. More information will be in the first lab.

## Chapter 1

In which our intrepid hero battles standard out, standard error, file descriptors and writing to files.

### Hello, World! (system call style)
1. Write a program that uses `write()` to print out "Hi! My name is `<Your Name>`".
	#### Answer
	```C
	#include <stdio.h>
	int main() {
		write(1, "Hi! My name is Jenny", 20);
	 	return 0;
	}
	```

### Hello, Standard Error Stream!
2. Write a function to print out a triangle of height `n` to standard error.
   - Your function should have the signature `void write_triangle(int n)` and should use `write()`.
   - The triangle should look like this, for n = 3:
   ```C
   *
   **
   ***
   ```
   #### Answer
   ```C
	void write_triangle(int n){
		int i;
		for(i=1;i<=n;i++){
			int j;
			for(j=0;j<i;j++){
				write(2, "*", 1);
			}
			write(2, "\n", 1);
		}
	}
   ```
### Writing to files
3. Take your program from "Hello, World!" modify it write to a file called `hello_world.txt`.
   - Make sure to to use correct flags and a correct mode for `open()` (`man 2 open` is your friend).
   	#### Answer
   	```C
	#include <stdio.h>
	int main(){
		mode_t mode = S_IRUSR | S_IWUSR;
		int fildes = open("hello_world.txt", O_CREAT | O_TRUNC | O_RDWR, mode);
		write(fildes, "Hi! My name is Jenny ", 20);
		return 0;
	}
   	```
### Not everything is a system call
4. Take your program from "Writing to files" and replace `write()` with `printf()`.
   - Make sure to print to the file instead of standard out!
   #### Answer
	```C
	#include <stdio.h>
	int main() {
		close(1);
		int fildes = open("hello_world.txt", O_CREAT | O_RDWR, S_IWUSR | S_IRUSR);
		printf("Hi! My name is Jenny");
		close(fildes);
		return 0;
	}
   	```
5. What are some differences between `write()` and `printf()`?
	#### Answer
	- write() is a system call, who do not have buffer and only write a sequence of bytes.
	- printf() is a function in C library, who owns the buffer and let’s you print the data in different format. 
	- printf() only print if the buffer is full or start a new line.

## Chapter 2

Sizing up C types and their limits, `int` and `char` arrays, and incrementing pointers

### Not all bytes are 8 bits?
1. How many bits are there in a byte?
	#### Answer
	- at least 8 bits but not actually should be 8 bits, it depends on the hardware and system.
2. How many bytes are there in a `char`?
	#### Answer
	- 1 byte
3. How many bytes the following are on your machine?
   - `int`, `double`, `float`, `long`, and `long long`
   	#### Answer
	- int: 4 bytes
	- double: 8 bytes
	- float: 4 bytes
	- long: 4 bytes
	- long long: 8 bytes

### Follow the int pointer
4. On a machine with 8 byte integers:
```C
int main(){
    int data[8];
} 
```
If the address of data is `0x7fbd9d40`, then what is the address of `data+2`?   

	#### Answer
	- Address of data+2 = 0x7fbd9d48
	- Because each int is 4 bytes, so the address of data+2 = data’s address+8bytes

5. What is `data[3]` equivalent to in C?
   - Hint: what does C convert `data[3]` to before dereferencing the address?
   	#### Answer
	- *(data+3)

### `sizeof` character arrays, incrementing pointers
  
Remember, the type of a string constant `"abc"` is an array.

6. Why does this segfault?
```C
char *ptr = "hello";
*ptr = 'J';
```
#### Answer
- Because the “hello” is constant that can only be read, and the hardware do know which memory can be read, write or both. Therefore, it causes segmentation fault.
7. What does `sizeof("Hello\0World")` return?
	#### Answer
	- 12
	- because the sizeof() function is calculate the size of given expression. In this case, it is 11(Hello\0World)+1(\0)
8. What does `strlen("Hello\0World")` return?
	#### Answer
	- 5
	- because the strlen() function is calculate the length until find \0, that is why is 5(Hello)
9. Give an example of X such that `sizeof(X)` is 3.
	#### Answer
	- X=xy
10. Give an example of Y such that `sizeof(Y)` might be 4 or 8 depending on the machine.
	#### Answer
	- *Y=xy
	- sizeof(&Y) might be 4 or 8 byte, because the size of pointer type is depends on the machine architecture and compiler.


## Chapter 3

Program arguments, environment variables, and working with character arrays (strings)

### Program arguments, `argc`, `argv`
1. What are two ways to find the length of `argv`?
	#### Answer
	- The length of argv represent the number of arguments. The two ways to find the length of argv is
	- (1)Iterate the argv[] until find the argv[i]==NULL, which is represent the end of argv[]. The number of iteration equal to the number of arguments, that is, the length of argv[]
	- (2)argc is the number of arguments, so it is equal to the length of argv[]

2. What does `argv[0]` represent?
	#### Answer
	- argv[0] represent the name of the program. For example, argv[0]=./program
### Environment Variables
3. Where are the pointers to environment variables stored (on the stack, the heap, somewhere else)?
	#### Answer
	- stored in stack
### String searching (strings are just char arrays)
4. On a machine where pointers are 8 bytes, and with the following code:
```C
char *ptr = "Hello";
char array[] = "Hello";
```
What are the values of `sizeof(ptr)` and `sizeof(array)`? Why?
#### Answer
- sizeof(ptr) = 4 , because the pointer is points to the address of first letter ‘H’. And the size of address is 4bytes.
- sizeof(array) = 6, because the array is actually indicating the string “Hello” plus one byte of end character ‘\0’.


### Lifetime of automatic variables

5. What data structure manages the lifetime of automatic variables?
	#### Answer
	- stack
## Chapter 4

Heap and stack memory, and working with structs

### Memory allocation using `malloc`, the heap, and time
1. If I want to use data after the lifetime of the function it was created in ends, where should I put it? How do I put it there?
	#### Answer
	- malloc, calloc, realloc, functions that can allocate the memory on the heap. 
	- The allocated memory function should be put insider the scope of function and always remember to free the memory after use.
2. What are the differences between heap and stack memory?
	#### Answer
	- First, heap memory doesn’t have de-allocation feature, we should free the memory by ourselves when we finish using that memory. However, stack memory has de-allocation feature, we no need to de-allocated the memory, it will be done automatically.
	- second, for heap memory, the memory exists as long as the whole application runs. For stack memory, the memory allocated for the variable last from once a function is called until the function calls over.
	- Third, stack memory is safer, because the data stored in stack memory can only be accessed by the owner thread. The data stored in heap memory is accessible or visible to all thread.

3. Are there other kinds of memory in a process?
	#### Answer
	- Yes, for example, static data is allocated and have a lifetime spans the duration of the program
4. Fill in the blank: "In a good C program, for every malloc, there is a ___".
	#### Answer
	- In a good C program, for every malloc, there is a free.
### Heap allocation gotchas
5. What is one reason `malloc` can fail?
	#### Answer
	- When there is not enough heap memory to fulfill the request of allocation.
6. What are some differences between `time()` and `ctime()`?
	#### Answer
	- time() is use to get the current time in numeric representation format. 
	- ctime() is use to convert the numeric representation format in to human-readable format. 
	- ctime() returns a string includes a newline character at the end.
7. What is wrong with this code snippet?
```C
free(ptr);
free(ptr);
```
#### Answer
- Once you tell the heap the memory is free, heap memory may mark the memory as available. 
- If you free the memory twice, the heap memory may confuse about free the memory already marks as available and cause memory corruption.
8. What is wrong with this code snippet?
```C
free(ptr);
printf("%s\n", ptr);
```
#### Answer
- The ptr pointer already been free, so you are invalid to access it.
9. How can one avoid the previous two mistakes?
	#### Answer 
	- After we free the memory, remember to set the pointer to NULL. By doing so, we can avoid the dangling pointer situation.
### `struct`, `typedef`s, and a linked list
10. Create a `struct` that represents a `Person`. Then make a `typedef`, so that `struct Person` can be replaced with a single word. A person should contain the following information: their name (a string), their age (an integer), and a list of their friends (stored as a pointer to an array of pointers to `Person`s).
	#### Answer
	```C
	#include <stdio.h>
	struct Person{
		char* name;
		int age;
		struct Person* friends;
	};
	typedef struct Person person_t;
	int main() {
		person_t* Smith = (person_t*) malloc(sizeof(person_t));
		person_t* Moore = (person_t*) malloc(sizeof(person_t));
		Smith->name = "Agent_Smith";
		Smith->age = 128;
		Moore->name = "Sonny_Moore";
		Moore->age = 256;
		return 0;
	}
	```
11. Now, make two persons on the heap, "Agent Smith" and "Sonny Moore", who are 128 and 256 years old respectively and are friends with each other.
	#### Answer
	```C
	#include <stdio.h>
	struct Person{
		char* name;
		int age;
		struct Person* friends;
	};
	typedef struct Person person_t;
	int main() {
		person_t* Smith = (person_t*) malloc(sizeof(person_t));
		person_t* Moore = (person_t*) malloc(sizeof(person_t));
		Smith->name = "Agent_Smith";
		Smith->age = 128;
		Smith->friends = Moore;
		Moore->name = "Sonny_Moore";
		Moore->age = 256;
		Moore->friends = Smith;
		return 0;
	}
	```
### Duplicating strings, memory allocation and deallocation of structures
Create functions to create and destroy a Person (Person's and their names should live on the heap).
12. `create()` should take a name and age. The name should be copied onto the heap. Use malloc to reserve sufficient memory for everyone having up to ten friends. Be sure initialize all fields (why?).
	#### Answer
	```C
	person_t* create(char* aname, int aage){
		printf("create person link %s, age = %d\n", aname, aage);
		person_t* result = (person_t*) malloc(sizeof(person_t));
		result->name = strdup(aname);
		result->age = aage;
		return result;
	}
	```
13. `destroy()` should free up not only the memory of the person struct, but also free all of its attributes that are stored on the heap. Destroying one person should not destroy any others.
	#### Answer
	```C
	void destroy(person_t* person){
		free(person->name);
		// free(person->age);
		memset(person, 0, sizeof(person_t));
		free(person->friends);
		free(person);
	}
	```

## Chapter 5 

Text input and output and parsing using `getchar`, `gets`, and `getline`.

### Reading characters, trouble with gets
1. What functions can be used for getting characters from `stdin` and writing them to `stdout`?
	#### Answer
	- stdin: gets, getchar
	- stdout: puts, putchar
2. Name one issue with `gets()`.
	#### Answer
	- gets() do not tell you if the input is too long and will cause buffer overflow
### Introducing `sscanf` and friends
3. Write code that parses the string "Hello 5 World" and initializes 3 variables to "Hello", 5, and "World".
	#### Answer
	```C
	#include <stdio.h>
	int main(){
		char* str="Hello 5 World";
		int num;
		char hello[10], world[10];
		sscanf(str, "%s %d %s", hello, &num, world);
		printf("%s %d %s", hello, num, world);
		return 0;
	}
	```
### `getline` is useful
4. What does one need to define before including `getline()`?
	#### Answer
	- pointer to buffer
	- capacity variable
5. Write a C program to print out the content of a file line-by-line using `getline()`.
	#### Answer
	```C
	#include <stdio.h>
	int main() {
		FILE* file = fopen("message.txt","r");
		char* buffer=NULL;
		size_t capacity=0;
		ssize_t result;
		while((result = getline(&buffer, &capacity, file))!=EOF){
			printf("%s", buffer);
			buffer = NULL;
			capacity =0;
		}
		return 0;
	}
	```

## C Development

These are general tips for compiling and developing using a compiler and git. Some web searches will be useful here

1. What compiler flag is used to generate a debug build?
	#### Answer
	- -g, use debugging tool like GDB
2. You modify the Makefile to generate debug builds and type `make` again. Explain why this is insufficient to generate a new build.
	#### Answer
	- because 'make' instruction only default to rebuild the file has been change since their last timestamp.
	- However, if we only modify Makefile but not source file
	- 'make' instruction might not detect the changes and won't trigger any new build
3. Are tabs or spaces used to indent the commands after the rule in a Makefile?
	#### Answer
	- tabs is used to indent the commands, but space not
4. What does `git commit` do? What's a `sha` in the context of git?
	#### Answer
	- save a snapshot of changes made to the codebase
	- sha means 'Secure Hash Algorithm', it is a kind of identifier for a commit or an object within the Git repository
5. What does `git log` show you?
	#### Answer
	- a chronological history of commits in Git reppository
6. What does `git status` tell you and how would the contents of `.gitignore` change its output?
	#### Answer
	- 'git status' give the information of current status and status of local repository
	- 'gitignore will change the 'git status output by excluding specific files' status information. 
7. What does `git push` do? Why is it not just sufficient to commit with `git commit -m 'fixed all bugs' `?
	#### Answer
	- 'git push' is send your local commits to a remote repository
	- 'git commit' is store commits in your local repository
	- using 'git commit -m 'fixed all bugs'' is insufficient because if commits do not be pushed in remote repository, others cannot see the content
	- In addition, remote repository serves as a backup of your codebase.
8. What does a non-fast-forward error `git push` reject mean? What is the most common way of dealing with this?
	#### Answer
	- means when you attempt to push commits from local repository to a remote repository, but the local one is diverged from the remote one. 
	- In this situation, remote branch's history has advanced beyond your local branch. That is why it is unable to perform simple, fast merge
	- To deal with this, performing merge or rebase can bring your local branch up to date with remote branch history.

## Optional (Just for fun)
- Convert your a song lyrics into System Programming and C code and share on Ed.
- Find, in your opinion, the best and worst C code on the web and post the link to Ed.
- Write a short C program with a deliberate subtle C bug and post it on Ed to see if others can spot your bug.
- Do you have any cool/disastrous system programming bugs you've heard about? Feel free to share with your peers and the course staff on Ed.
