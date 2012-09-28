Andrew Hurle
CS 4401
Turnout C Patch

1.  Changed default value of su in superLogin() to be 0, so the password check actually does something.  This prevents any random user from being able to log in as a superuser.

2.  Used getpass in superLogin to prevent buffer overflows which could allow a user to log in with an incorrect password, or potentially execute arbitrary code.

3.  Escape SQL input with mysql_real_escape_string.  This prevents SQL injection attacks through the username input when changing grades, and through the USERNAME environment variable.

