#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <netdb.h>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>

#define VALID_ARGC_SERVER 3
#define VALID_ARGC_CLIENT 4
#define BUFFER_SIZE 256
#define MAX_CLIENTS 5
#define MAXHOSTNAME 256
#define FAILED -1
/*////////////////////////////////////////////////////////////////////////////
 * Error handling
 ///////////////////////////////////////////////////////////////////////////*/
#define SYSTEM_CALL_ERR "system error: "

/**s
 * shows an error message and exit/return failure code
 * @param error_type type of error to show
 * @return failure code
 */
int raise_error (int error_type)
{
  switch (error_type)
    {
      case 0:
        std::cerr << SYSTEM_CALL_ERR << "gethostname" << std::endl;
      break;
      case 1:
        std::cerr << SYSTEM_CALL_ERR << "gethostbyname" << std::endl;
      break;
      case 2:
        std::cerr << SYSTEM_CALL_ERR << "create socket" << std::endl;
      break;
      case 3:
        std::cerr << SYSTEM_CALL_ERR << "bind" << std::endl;
      break;
      case 4:
        std::cerr << SYSTEM_CALL_ERR << "connect" << std::endl;
      break;
      case 5:
        std::cerr << SYSTEM_CALL_ERR << "accept" << std::endl;
      break;
      case 6:
        std::cerr << SYSTEM_CALL_ERR << "read" << std::endl;
      break;
      case 7:
        std::cerr << SYSTEM_CALL_ERR << "sprintf" << std::endl;
      break;
      case 8:
        std::cerr << SYSTEM_CALL_ERR << "write" << std::endl;
      break;
      case 9:
        std::cerr << SYSTEM_CALL_ERR << "arguments count invalid" << std::endl;
      break;
      case 10:
        std::cerr << SYSTEM_CALL_ERR << "not server" << std::endl;
      break;
      case 11:
        std::cerr << SYSTEM_CALL_ERR << "not client" << std::endl;
      break;
      case 12:
        std::cerr << SYSTEM_CALL_ERR << "terminal_command_to_run too short" << std::endl;
      break;
      case 13:
        std::cerr << SYSTEM_CALL_ERR << "establish connection failed" << std::endl;
      break;
      case 14:
        std::cerr << SYSTEM_CALL_ERR << "get_connection failed" << std::endl;
      break;
      default:
        break;
    }
  exit (1);
}

/*////////////////////////////////////////////////////////////////////////////
 * Implementation
 ///////////////////////////////////////////////////////////////////////////*/
int establish_connection (char *hostname, unsigned short portnum, bool server)
{
  int s;
  struct sockaddr_in sa{};
  struct hostent *hp;
  if (gethostname (hostname, MAXHOSTNAME) < 0)
    { raise_error (0); }
  if ((hp = gethostbyname (hostname)) == nullptr)
    { raise_error (1); }

  if (server)
    {
      bzero (&sa, sizeof (sa));
    }

  memset (&sa, 0, sizeof (sa));
  memcpy ((char *) &sa.sin_addr, hp->h_addr, hp->h_length);
  sa.sin_family = hp->h_addrtype;
  sa.sin_port = htons((u_short) portnum);

  if (server)
    {
      if ((s = socket (AF_INET, SOCK_STREAM, 0)) < 0)
        { raise_error (2); }
      if (bind (s, (struct sockaddr *) &sa, sizeof (struct sockaddr_in)) < 0)
        {
          close (s);
          raise_error (3);
        }
      listen (s, MAX_CLIENTS);
    }
  else
    {
      if ((s = socket (hp->h_addrtype, SOCK_STREAM, 0)) < 0)
        { raise_error (2); }
      if (connect (s, (struct sockaddr *) &sa, sizeof (sa)) < 0)
        {
          close (s);
          raise_error (4);
        }
    }

  return s;
}

int get_connection (int s)
{
  int t;
  if ((t = accept (s, nullptr, nullptr)) < 0)
    { raise_error (5); }
  return t;
}

void read_data (int s, char *buf, int n)
{
  ssize_t count = 0, br = 0;
  while (count < n)
    {
      br = read (s, buf, n - count);
      if (br > 0)
        {
          count += br;
          buf += br;
        } // make sure char array ends with back slesh 0
      else if (br == FAILED)
        raise_error (6);
      else return;
    }
}

void write_data (int s, char *buf, char *cmd)
{
  if (sprintf (buf, "%s", cmd) < 0)
    { raise_error (7); }
  int buf_len = (int) strlen (buf);
  if (write (s, buf, buf_len) != buf_len)
    { raise_error (8); }
}

/**s
 * main function for program
 * @param argc 3 for server, 4  for client
 * @param argv ./sockets client <port> <terminal_command_to_run>
 *.             ./sockets server <port>
 * @return success/fail state
 */
int main (int argc, char *argv[])
{
  int server_socket, client_socket, new_socket;
  char server_command_buffer[BUFFER_SIZE]; //recieve
  char client_command_buffer[BUFFER_SIZE]; //send
  char server_hostname[MAXHOSTNAME + 1];
  char client_hostname[MAXHOSTNAME + 1];

  if (argc < VALID_ARGC_SERVER)
    { raise_error (9); }
  unsigned long int port = strtoul (argv[2], nullptr, 10);
  if (argc == VALID_ARGC_SERVER)
    {
      if (strcmp (argv[1], "server") != 0)
        { raise_error (10); }
      server_socket = establish_connection (server_hostname, port, true);
      if (server_socket == FAILED)
        { raise_error (13); }
      while (true)
        {
          new_socket = get_connection (server_socket);
          if (new_socket == FAILED)
            { raise_error (14); }
          read_data (new_socket, server_command_buffer, BUFFER_SIZE);
          system (server_command_buffer);
          memset (server_command_buffer, 0, BUFFER_SIZE);
          close (new_socket);
          bzero (server_command_buffer, strlen (server_command_buffer));
        }
    }
  else if (argc >= VALID_ARGC_CLIENT)
    {
      if (strcmp (argv[1], "client") != 0)
        { raise_error (11); }
      client_socket = establish_connection (client_hostname, port, false);
      if (client_socket == FAILED)
        { raise_error (13); }

      int args_ptr = 3; // ptr on the start of command args
      std::string str;
      while (args_ptr < argc)
        {
          str += argv[args_ptr];
          args_ptr++;
          if (args_ptr != argc)
            {
              str += " "; //add space between each arg
            }
        }
      char *command = const_cast<char *>(str.c_str ());
      write_data (client_socket, client_command_buffer, command);
      close (client_socket);
    }
  else
    { raise_error (9); }
}
