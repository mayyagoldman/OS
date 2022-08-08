#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <sched.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstring>
#include <sys/mount.h>
#include <fstream>
#include <sys/stat.h>

#define STACK_SIZE 8192
#define VALID_ARGC 6
#define CLONE_FLAGS CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | SIGCHLD
#define FAIL -1
#define CONTAINER_PID 1
#define MKDIR_MODE 0755
#define FS_PATH "/sys/fs"
#define CGROUP_PATH "/sys/fs/cgroup"
#define PIDS_PATH "/sys/fs/cgroup/pids"
#define FILE_NOTIFY_ON_RELEASE "/notify_on_release"
#define FILE_PIDS "/pids.max"
#define FILE_CGROUP "/cgroup.procs"

typedef struct Container {
    char *new_hostname;
    char *new_filesystem_directory;
    int num_processes;
    char *path_to_program; //_to_run_within_the_container
    char *args_for_program;
} Container;

void *childStack;


/*////////////////////////////////////////////////////////////////////////////
 * Error handling
 ///////////////////////////////////////////////////////////////////////////*/

#define CONTAINER_ERR "container builder error: "
#define SYSTEM_CALL_ERR "system error: "

/**
 * shows an error message and exit/return failure code
 * @param error_type type of error to show
 * @return failure code
 */
int raise_error (int error_type)
{
  switch (error_type)
    {
      case 0:
        std::cerr << CONTAINER_ERR << "args count invalid\n";
      return EXIT_FAILURE;
      case 1:
        std::cerr << SYSTEM_CALL_ERR << "sethostname failed\n";
      break;
      case 2:
        std::cerr << SYSTEM_CALL_ERR << "chroot failed\n";
      break;
      case 3:
        std::cerr << SYSTEM_CALL_ERR << "chdir failed\n";
      break;
      case 4:
        std::cerr << SYSTEM_CALL_ERR << "execvp failed\n";
      break;
      case 5:
        std::cerr << CONTAINER_ERR << "clone failed\n";
      break;
      case 6:
        std::cerr << SYSTEM_CALL_ERR << "malloc failed\n";
      break;
      case 7:
        std::cerr << SYSTEM_CALL_ERR << "mount failed\n";
      break;
      case 8:
        std::cerr << SYSTEM_CALL_ERR << "creating cgroup.procs file failed\n";
      break;
      case 9:
        std::cerr << SYSTEM_CALL_ERR << "creating pids.max file failed\n";
      break;
      case 10:
        std::cerr << SYSTEM_CALL_ERR << "creating notify_on_release file failed\n";
      break;
      case 11:
        std::cerr << SYSTEM_CALL_ERR << "mkdir failed\n";
      break;
      case 12:
        std::cerr << SYSTEM_CALL_ERR << "umount failed\n";
      break;
      case 13:
        std::cerr << SYSTEM_CALL_ERR << "remove file failed\n";
      break;
      case 14:
        std::cerr << SYSTEM_CALL_ERR << "remove folders failed\n";
      break;
      default:
        break;
    }
  exit (1);
}

/*////////////////////////////////////////////////////////////////////////////
 * Implementation
 ///////////////////////////////////////////////////////////////////////////*/

/**
 * initializing the container
 * @param arg the Container struct with all info
 * @return success/fail state
 */
int childFunc (void *arg)
{
  auto *container = (Container *) arg;

  //host name
  if (sethostname (container->new_hostname, (int) strlen (container->new_hostname)) == FAIL)
    raise_error (1);

  //root directory
  if (chroot (container->new_filesystem_directory) == FAIL)
    raise_error (2);

  //working directory
  if (chdir ("/") == FAIL)
    raise_error (3);

  //mounting the new procfs
  if (mount ("proc", "/proc", "proc", 0, nullptr) == FAIL)
    raise_error (7);

  //creating directories
  if (mkdir (FS_PATH, MKDIR_MODE) == FAIL)
    raise_error (11);
  if (mkdir (CGROUP_PATH, MKDIR_MODE) == FAIL)
    raise_error (11);
  if (mkdir (PIDS_PATH, MKDIR_MODE) == FAIL)
    raise_error (11);

  //attaching the container process to new cgroup
  std::fstream cgroup_file;
  std::string cgroup_path = (std::string) PIDS_PATH + FILE_CGROUP;
  cgroup_file.open (cgroup_path, std::ios::out);
  if (!cgroup_file.good ())
    raise_error (8);
  cgroup_file << CONTAINER_PID;
  cgroup_file.close ();

  //limit num of processes
  std::fstream pids_file;
  std::string pids_path = (std::string) PIDS_PATH + FILE_PIDS;
  pids_file.open (pids_path, std::ios::out);
  if (!pids_file.good ())
    raise_error (9);
  pids_file << container->num_processes;
  pids_file.close ();

  //for releasing the resources when container is finished
  std::fstream release_file;
  std::string release_path = (std::string) PIDS_PATH + FILE_NOTIFY_ON_RELEASE;
  release_file.open (release_path, std::ios::out);
  if (!release_file.good ())
    raise_error (10);
  release_file << CONTAINER_PID;
  release_file.close ();

  //executing the program
  char *prog_args[] = {container->path_to_program, container->args_for_program, nullptr};
  if (execvp (container->path_to_program, prog_args) == FAIL)
    raise_error (4);

  return EXIT_SUCCESS;
}

/**
 * closing the program
 * @return success/fail state
 */
int closeContainer (char *new_filesystem_directory)
{

  std::string path_base = std::string (new_filesystem_directory);
  //delete files
  std::string folder_path = path_base + std::string (PIDS_PATH);
  if (remove ((folder_path + FILE_NOTIFY_ON_RELEASE).c_str ()) == FAIL ||
      remove ((folder_path + FILE_PIDS).c_str ()) == FAIL ||
      remove ((folder_path + FILE_CGROUP).c_str ()) == FAIL)
    raise_error (13);

  //delete folders
  if (rmdir (folder_path.c_str ()) == FAIL ||
      rmdir ((path_base + CGROUP_PATH).c_str ()) == FAIL ||
      rmdir ((path_base + FS_PATH).c_str ()) == FAIL)
    raise_error (14);

  //unmounting procfs
  if (umount ((path_base + "/" + "proc").c_str ()) == FAIL)
    raise_error (12);

  free (childStack);
  return EXIT_SUCCESS;
}

/**
 * main function for program
 * @param argc suppose to be 6
 * @param argv ./container <new_hostname> <new_filesystem_directory>
 * <num_processes> <path_to_program>
 * <args_for_program>
 * @return success/fail state
 */
int main (int argc, char *argv[])
{
  if (argc != VALID_ARGC)
    return raise_error (0);

  childStack = (void *) malloc (STACK_SIZE);
  if (childStack == nullptr)
    raise_error (6);

  Container container = {container.new_hostname = argv[1],
      container.new_filesystem_directory = argv[2],
      container.num_processes = atoi (argv[3]),
      container.path_to_program = argv[4],
      container.args_for_program = argv[5]};

  //creating the container
  int child_pid = clone (childFunc, (char *) childStack + STACK_SIZE, CLONE_FLAGS, &container);
  if (child_pid == FAIL)
    return raise_error (5);

  //waits for the container to be created before finishing
  wait (nullptr);
  return closeContainer (container.new_filesystem_directory);
}