Andrew Hurle
CS 4401
Turnout C Patch

1.  Changed default value of su in superLogin() to be 0, so the password check actually does something.  This prevents any random user from being able to log in as a superuser.

2.  Used getpass in superLogin to prevent buffer overflows which could allow a user to log in with an incorrect password, or potentially execute arbitrary code.

3.  Escape SQL input with mysql_real_escape_string.  This prevents SQL injection attacks through the username input when changing grades, and through the USERNAME environment variable.

4.  Get the current username with getpwuid(getuid)), instead of reading from the environment variable.  This prevents the user from seeing other people's grades or even attempting logging in as superuser by changing the environment variable.

5.  Changed the makefile so it builds with -fstack-protector-all, except on debug builds.  This makes it harder for an attacker to take advantage of buffer overflow vulnerabilities.

6.  Restricted menu input to choices 1, 2, and 9 to prevent being able to change and see grades without knowing the superuser password.

7.  Used DEBUG_MODE flag to only add in -debug and -config arguments at compile time on debug builds.  This means that you can only use these arguments if you have access to the source code and can build it in debug mode.  This makes it harder for an attacker to get useful information from debug printfs, and it's harder to get superuser access.



Regarding the database passwords stored in the executable:  There isn't really a simple way to solve this.  Anything that leaves the passwords in the executable is just obfuscation.  At some point, the user needs to be able to read the credentials to access the database.  Furthermore, any internal password checking (like the superuser password) can certainly be bypassed by a determined person by flipping some bits in memory.  However, we can at least only store a hash of the password so an attacker has to put forth some efort to find it, though it's especially vulnerable to brute force attacks because the attacker has easy access to the hash.

The user really shouldn't be touching the database directly.  The proper way to do this is to have a separate server application running, which has individual accounts for each user.  The user then has to enter their credentials, the server verifies their identity and sets up a secure session, and then the user can perform very specific actions that don't involve providing SQL to run on the database.  However, I think that's well out of the scope of this assignment.  Here's what I'll do without getting ridiculous and rewriting the entire program:

8.  Changed the cs440x password to something different than the database password, so that getting superuser access on the program and sudo access on the machine isn't trivial.

9.  Stored the md5 hash of the superuser password in the executable, rather than the plaintext password.  This makes it harder to see what the password is with the strings command.  md5 isn't at all secure, but you get the idea of how it works - it should be done with a hashing algorithm that takes significantly longer to compute in order to stave off brute force attacks.  Ideally, the password would only be checked on a server, making storing the password locally unnecessary.


