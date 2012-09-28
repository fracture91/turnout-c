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

