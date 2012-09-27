/**
 * gradebook.c
 * Matt Brennan, Robert Breznak
 * This program is an example simple gradebook system that uses a MySQL
 * database to store its information.
 */

#include <ctype.h>
#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#define MYSQL_HOST "127.0.0.1"     //Host for MySQL
#define MYSQL_USER "root"          //Username for MySQL
#define MYSQL_PASS "Dr. John A. Zoidberg"    //Password for MySQL
#define MYSQL_DB   "turnoutc"      //MySQL database to use
#define PASSWORD "Slurm"

typedef struct _auser AUser;

struct _auser {
  char *username;
  AUser *next;
};

AUser *readConfig(const char *);              // Reads the given config file
char  *getUsername(void);                     // Gets the current username
int   superUser(const char *, const AUser *); // Is a superuser?
char  *getGrade(const MYSQL *, const char *); // Returns the grade for user
void  showGrade(const MYSQL *, const char *); // Shows the grade for a user
int   showMenu(int superuser);                         // Show menu and get choice
void  studentList(const MYSQL *);             // Give student list w/grades
void  changeGrade(const MYSQL *);             // Change a grade
int superLogin();

char *config = "/usr/local/etc/gradebook.cfg";  // Config file (default)
char *misc = "cylons!";
int debug=0; // Are we running in debug mode?
int isSU;

int main(int argc, char **argv) {
  AUser *admins;
  MYSQL *connect;
  char *username=getUsername();
  int choice;

  if (argc > 0) {
    int i;
    for(i = 1; i < argc; i++) {
      if(!strcasecmp(argv[i], "-debug")) {
        debug = 1;
      }
      else if(!strcasecmp(argv[i], "-config")) {
        if(i < argc) {
          config = argv[i+1];
          i++;
	  printf("Using config file: %s\n\n", config);
        }
      }
      else if(!strcasecmp(argv[i], "--help")) {
        printf("Usage: turnout [OPTION]\n");
        return 0;
      } else {
          printf("turnout: invalid option -- '%s'\n",argv[i]);
          printf("Try `turnout --help for more information.\n");
          return 1;
      }
    }
  }

  if (debug)
    printf("DEBUG: Current username is %s\n\n", username);


  // Read Config
  admins=readConfig(config);

  // Initialize MySQL connection variable
  connect = mysql_init(NULL);

  // Connect to the MySQL database, altering the user and dieing if we fail
  if (mysql_real_connect(connect,MYSQL_HOST,MYSQL_USER,MYSQL_PASS,MYSQL_DB,3306,NULL,0)==NULL) {
    printf("FATAL: Unable to connect to database!\n\n");
    return -1;
  }

  // If they ain't a superuser .. 
  if (!superUser(username,admins)) {
    showGrade(connect,username); // Give them THEIR grade

    return 1;  // And bail
  }

  // If we get here, they are a superuser
  do {
    choice=showMenu(isSU);
    switch(choice) {
      case 1: // They want a list of students
        studentList(connect);
        break;
      case 2: // They want to change a grade
        changeGrade(connect);
        break;
      case 3: // They want their grade
        showGrade(connect,username);
        break;
      case 4:
        superLogin();
        break;
      case 9: // They want to exit
        printf("\nExiting...\n\n");
        break;
      default:
        printf("\nPlease select an option from the menu\n\n");
    }
  } while (choice != 9); // Continue until they want to exit

  mysql_close(connect); // Close the database

  return 0;
}

int superLogin() {
  char buff[10];
  int su=1;

  printf("Password: ");
  gets(buff);

  if(!strcmp(buff, PASSWORD)) {
    su = 1;
  }
  isSU = su;

  return 0;
}

// Reads the given config file and imports the admins
// Pre:  config contains a valid config file location
// Post: returns a linked list of admin user names
AUser *readConfig(const char *config) {
  FILE *inputFile;
  char buff[50];
  AUser *admins=NULL, *head=NULL;

  // If it's NULL, better bail
  if (!config) {
    printf("FATAL: Error in configuration file!\n");
    exit(-1);
  }

  // Can't read the config? BAIL
  if ((inputFile=fopen(config,"r"))==NULL) {
    printf("FATAL: Unable to open config file %s\n", config);
    exit(-1);
  }
  while (fgets(&buff[0],50,inputFile)) {  // Step through the file
    if (buff[0]!='#') {
      if (!admins) { // Is this the first one?
        if ((admins=malloc(sizeof(AUser)))==NULL) { // Allocate memory
          printf("FATAL: Unable to allocate memory!\n"); // Die if we can't
          exit(-1);
        }
        head=admins; // Save the head of the list
      } else {
        if ((admins->next=malloc(sizeof(AUser)))==NULL) { // Allocate memory
          printf("FATAL: Unable to allocate memory!\n"); // Die if we can't
          exit(-1);
        }
        admins=admins->next; // We want to work with the new one
      }

      admins->username=strdup(&buff[0]); // Copy the user name into the list
      admins->next=NULL;
      if (admins->username[strlen(admins->username)-1]=='\n')
        admins->username[strlen(admins->username)-1]='\0';
    }
  }

  fclose(inputFile); // Close the file
  
  return head;
}

// Returns the user name of the user running this process
// Pre:  The program is NOT run setuid()
// Post: The username of the logged in user is returned
char *getUsername() {
  return getenv("USERNAME");
}

// Returns non-zero iff user is a member of the admins linked list
// Pre:  username contains a valid username; admins is a valid linked list
// Post: returns true iff user is in admins
int superUser(const char *username, const AUser *admins) {
  AUser *ptr;

  for (ptr=admins; ptr; ptr=ptr->next) {
    if (debug)
      printf("DEBUG: Processing admin %s against %s\n",ptr->username,username);
    if (ptr && ptr->username && !strcasecmp(ptr->username, username))
      return 1; // We have a match
  }

  return 0; // Nope, no match
}

// Get the grade for the given username from the MySQL database
// Pre:  conect is a valid, active MySQL connection
//       username contains a valid username
// Post: the grade is returned. Returns NULL if there is an error
char *getGrade(const MYSQL *connect, const char *username) {
  int rows;
  char buff[1024], *grade;
  MYSQL_RES *result;
  MYSQL_ROW row;

  if ((!connect) || (!username)) {
    // Somethings messed up... let's consider this FATAL
    // Tell the user and BAIL
    printf("FATAL: NULL values in parameters to getGrade()!\n");
    return NULL;
  }

  snprintf(&buff[0],1023,"SELECT grade FROM grades WHERE username='%s'",username);
  if (mysql_query(connect,&buff[0]) != 0) {
    // If it failed, tell the user
    printf("Error: %s!\n", mysql_error(connect));
    return NULL; // Return NULL tells the caller the data isn't valid
  }
  
  result = mysql_store_result(connect);
  if (result==NULL) {
    // If it failed, tell the user
    printf("Error: %s!\n", mysql_error(connect));
    return NULL; // Return NULL tells the caller the data isn't valid
  }

  // See how many rows we have
  rows=mysql_num_rows(result);
  if (rows==0) {
    return NULL; // Return NULL tells the caller the data isn't valid
  }

  // Get the data out of the result
  row=mysql_fetch_row(result);
  grade=strdup(row[0]);

  mysql_free_result(result); // Free the result
 
  return grade; // Send the grade back to the caller
}

// Show the grade for the given user from the MySQL database given
// Pre:  conect is a valid, active MySQL connection
//       username contains a valid username
// Post: the grade has been shown -- variables remain unchanged
void showGrade(const MYSQL *connect, const char *username) {
  if ((!connect) || (!username)) {
    // Something's messed up ... tell the user and BAIL
    printf("FATAL: NULL values in parameters to showGrade()!\n");
    exit(-1);
  }

  char *grade=getGrade(connect,username); // Get the grade

  if (grade) {  // If it exists show them
    printf("Grade for %s is: %s\n",username,grade);
    free(grade); // The free the memory used
  } else {
    // We don't have a grade for this user... tell them
    printf("Error: This user does not have a grade!\n");
  }

  return;
}

// Show the user a menu and get their answer
// Pre:  None
// Post: Returns the number of the menu option the user selected
int showMenu(int superuser) {
  char myChar;
  int choice;

  if (!superuser) {
    printf("1. Get My Grade\n");
    printf("2. Login As Superuser\n");
  } else {
    printf("1. List Students & Grades\n");
    printf("2. Change A Grade\n");
  }
  printf("9. Exit\n");

  do {
    myChar=getchar();
  } while (myChar=='\n');

  getchar();
  choice = (myChar-48);

  if (!superuser && (choice != 9))
    return 2+choice;
  else
    return choice;
}

// Lists the sudents in the database, alphabetical by username, with grades
// Pre:  Database is connected
// Post: List is shown
void studentList(const MYSQL *connect) {
  int rows, i;
  char buff[1024];
  MYSQL_RES *result;
  MYSQL_ROW row;

  if (!connect) {
    // Somethings messed up... let's consider this FATAL
    // Tell the user and BAIL
    printf("FATAL: NULL values in parameters to studentList()!\n");
    return;
  }

  snprintf(&buff[0],1023,"SELECT * FROM grades ORDER BY username");
  if (mysql_query(connect,&buff[0]) != 0) {
    // If it failed, tell the user
    printf("Error: %s!\n", mysql_error(connect));
    return;
  }

  result = mysql_store_result(connect);
  if (result==NULL) {
    // If it failed, tell the user
    printf("Error: %s!\n", mysql_error(connect));
    return; 
  }

  // See how many rows we have
  rows=mysql_num_rows(result);
  if (rows==0) {
    // We don't have any grades... tell them
    printf("Error: There are no users in the system!\n");
    return;
  }

  // Get the data out of the result
 
  printf("\nUsername\tGrade\n"); 
  for (i=0; i<rows; i++) {
    row=mysql_fetch_row(result);
    printf("%s    \t%s\n",row[0],row[1]);
  }
  printf("End of list.\n\n");

  mysql_free_result(result); // Free used memory

  return;
}

// Changes the grade for a student in the database
// Pre:  Database is connected
// Post: Grade is updated, or no change
void changeGrade(const MYSQL *connect) {
  char buff[1024], choice;
  char uname[30];

  if (!connect) {
    // Somethings messed up... let's consider this FATAL
    // Tell the user and BAIL
    printf("FATAL: NULL values in parameters to changeGrade()!\n");
    return;
  }

  // Get the username
  printf("Enter the username: ");
  fgets(&uname[0],30,stdin);
  uname[strlen(&uname[0])-1]='\0';

  if (debug)
    printf("DEBUG: Calling getGrade() for %s\n",&uname[0]);

  if (!getGrade(connect,&uname[0])) { // Check if they exist
    printf("Error: The username you entered is not in the system!\n\n");
    return;
  }

  // If they exist, get the new grade
  printf("New Grade (A=A,B=B,C=C,N=NR): ");
  choice=toupper(getchar());

  // Make sure its valid
  if (((choice<'A') || (choice>'C')) && (choice != 'N')) {
    printf("Error: Please enter A,B,C or N for grade\n\n");
    return;
  }

  // Build our query
  if (choice=='N')
    snprintf(&buff[0],1023,"UPDATE grades SET grade='NR' WHERE username='%s'",&uname[0]);
  else
    snprintf(&buff[0],1023,"UPDATE grades SET grade='%c' WHERE username='%s'",choice,&uname[0]);

  if (debug)
    printf("DEBUG: Query is %s\n",&buff[0]);

  // And run it
  if (mysql_query(connect,&buff[0]) != 0) {
    // If it failed, tell the user
    printf("Error: %s!\n", mysql_error(connect));
    return;
  }

  return;
}
