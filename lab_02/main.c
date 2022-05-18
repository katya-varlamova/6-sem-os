#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>

#define LOCKFILE "/var/run/mydaemon.pid"
#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

#define SLEEP_T1 2
#define SLEEP_T2 5

bool flag = true;
int sleep_time = SLEEP_T1;

sigset_t mask;

int lockfile(int fd) {
  struct flock fl;

  fl.l_type = F_WRLCK;
  fl.l_start = 0;
  fl.l_whence = SEEK_SET;
  fl.l_len = 0;

  return fcntl(fd, F_SETLK, &fl);
}

int already_running(void) {
  int fd;
  char buf[16];

  fd = open(LOCKFILE, O_RDWR | O_CREAT, LOCKMODE);
  if (fd < 0) {
    syslog(LOG_ERR, "Can't open %s: %s\n", LOCKFILE, strerror(errno));
    return 1;
  }
  if (lockfile(fd) < 0) {
    if (errno == EACCES || errno == EAGAIN) {
      close(fd);
      return 1;
    }

    syslog(LOG_ERR, "Can't set block %s: %s\n", LOCKFILE, strerror(errno));
    return 1;
  }

  ftruncate(fd, 0);
  sprintf(buf, "%d", getpid());
  write(fd, buf, strlen(buf) + 1);
  syslog(LOG_INFO, "First daemon, pid: %d", getpid());

  return 0;
}

void update_freq(void) {
  sleep_time = flag ? SLEEP_T2 : SLEEP_T1;
  flag = !flag;
}

void *sig_handler(void *arg) {
  (void)arg;

  int signo;

  while (true) {
    if (sigwait(&mask, &signo) != 0) {
      syslog(LOG_ERR, "Sigwait function call error: %s.\n", strerror(errno));
      exit(-1);
    }

    switch (signo) {
    case SIGHUP:
      update_freq();
      syslog(LOG_INFO, "Recieved signal: SIGHUP.\n");
      syslog(LOG_INFO, "Log frequency updated.\n");
      break;

    case SIGTERM:
      syslog(LOG_INFO, "Recieved signal: SIGTERM; exit.\n");

      if (remove(LOCKFILE) < 0)
        syslog(LOG_ERR, "Cant't remove .pid file: %s.\n", strerror(errno));

      exit(0);

    default:
      syslog(LOG_INFO, "Unexpected signal: %d\n", signo);
      break;
    }
  }

  return 0;
}

int daemonize() {
  int fd0, fd1, fd2;
  pid_t pid;
  struct rlimit rl;
  struct sigaction sa;

  

  if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
    printf("Can't get limit of number of file descriptors: %s.\n",
           strerror(errno));
    return 1;
  }

  if ((pid = fork()) < 0) {
    printf("Can't fork: %s.\n", strerror(errno));
    return 1;
  } else if (pid != 0)
    exit(0);
   else {}
    
  umask(0);

  setsid();

  sa.sa_handler = SIG_IGN;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  if (sigaction(SIGHUP, &sa, NULL) < 0) {
    printf("Can't ignore SIGHUP: %s.\n", strerror(errno));
    return 1;
  }

  // Демон не будет являться лидером сеанса
  // if ((pid = fork()) < 0) {
  //   printf("Can't fork: %s.\n", strerror(errno));
  //   return 1;
  // } elsef if (pid != 0)
  //   exit(0);

  if (chdir("/") < 0) {
    printf("Can't change diretctory: %s.\n", strerror(errno));
    return 1;
  }

  if (rl.rlim_max == RLIM_INFINITY)
    rl.rlim_max = 1024;

  for (rlim_t i = 0; i < rl.rlim_max; i++) {
    close(i);
  }

  fd0 = open("/dev/null", O_RDWR);
  fd1 = dup(0);
  fd2 = dup(0);

  openlog("mydaemonlog", LOG_CONS, LOG_DAEMON);
  if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
    syslog(LOG_ERR, "Wrong file decsriptors: %d %d %d\n", fd0, fd1, fd2);
    return 1;
  }

  return 0;
}

int process() {
  struct tm *info;
  time_t t;
  while (true) {
    sleep(sleep_time);

    t = time(NULL);
    info = localtime(&t);

    syslog(LOG_INFO, "Time: %d:%d:%d\n", info->tm_hour, info->tm_min,
           info->tm_sec);
  }

  return 0;
}

int set_sig_handler() {
  struct sigaction sa;
  sa.sa_handler = SIG_DFL;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  if (sigaction(SIGHUP, &sa, NULL) < 0) {
    printf("Can't repair SIG_DFL for SIGHUP: %s.\n", strerror(errno));
    return 1;
  }

  sigfillset(&mask);

  pthread_t tid;
  if (pthread_sigmask(SIG_BLOCK, &mask, NULL) != 0) {
    printf("SIG_BLOCK error: %s.\n", strerror(errno));
    return 1;
  }

  if (pthread_create(&tid, NULL, sig_handler, 0) != 0) {
    printf("Can't create thread: %s.\n", strerror(errno));
    return 1;
  }

  return 0;
}

int main(void) {
  int rc = daemonize();
  if (rc)
    return rc;

  if (already_running()) {
    syslog(LOG_ERR, "Daemon already exists.\n");
    return 1;
  }

  syslog(LOG_INFO, "Daemon created.\n");

  rc = set_sig_handler();
  if (rc)
    return rc;

  return process();
}
