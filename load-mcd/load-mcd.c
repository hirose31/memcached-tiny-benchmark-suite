/* -*- mode: c; coding: utf-8 -*-
 * 衝撃的なほどテキトーだからびっくりしないで寝！
 *
 * If you use Linux,
 * before run this program, do following command to reuse TIME_WAIT sockets.
 *
 * # echo 1 > /proc/sys/net/ipv4/tcp_tw_reuse
 * # echo 1 > /proc/sys/net/ipv4/tcp_tw_recycle
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <getopt.h>

#include "libmemcached/memcached.h"

#define DEBUG 0

#if 1
#define TRACE(fmt, ...) \
  {\
    struct tm tm;            \
    struct timeval tv;       \
    gettimeofday(&tv, NULL); \
    localtime_r(&(tv.tv_sec), &tm); \
    printf("%02d:%02d:%02d.%06lu %.12s.%4d: (trace ) "fmt"\n", \
         tm.tm_hour,tm.tm_min,tm.tm_sec,         \
         tv.tv_usec,                             \
         __FILE__, __LINE__, __VA_ARGS__);       \
  }
#else
#define TRACE(fmt, ...)
#endif

#if DEBUG
#define TRACE2(fmt, ...) \
  {\
    struct tm tm;            \
    struct timeval tv;       \
    gettimeofday(&tv, NULL); \
    localtime_r(&(tv.tv_sec), &tm); \
    printf("%02d:%02d:%02d.%06lu %.12s.%4d: (trace2) "fmt"\n", \
         tm.tm_hour,tm.tm_min,tm.tm_sec,         \
         tv.tv_usec,                             \
         __FILE__, __LINE__, __VA_ARGS__);       \
  }
#else
#define TRACE2(fmt, ...)
#endif

#define DATA_MAX 65535

int  pid;
char data[DATA_MAX][8];
int  do_one_connection = 0;

static void die(const char* msg)
{
  perror(msg);
  exit(EXIT_FAILURE);
}

void usage(void) {
  printf("bench-mcd -m host[:port] [-k key_num] [-c loop_num]\n");
  printf("  -m host[:port]  host and port which memcached is running.\n");
  printf("  -k key_num      number of data. default is 50,000.\n");
  printf("  -c loop_num     number of benchmark loop. default is 1.\n");
  printf("  -1              do all set in 1 connection.\n");
  printf("  -b              use binary protocol.\n");
  printf("  \n");
  exit(EXIT_FAILURE);
}

void do_set(memcached_st *mc, int key_num)
{
  int i;
  char key[12];
  for (i=0; i<key_num; i++) {
    sprintf(key, "%d_%d", i, pid);
    TRACE2("%s : %s", key, data[i]);
    memcached_set(mc, key, strlen(key), data[i], strlen(data[i]), 0, 0);

    if (! do_one_connection)
      memcached_quit(mc);
  }
  printf("last set: key:%s val:%s\n", key, data[i-1]);
}

void do_get(memcached_st *mc, int key_num)
{
  int       i;
  char      key[12];
  char     *val = '\0';
  size_t    value_length;
  uint32_t  flags;
  memcached_return rc;

  for (i=0; i<key_num; i++) {
    sprintf(key, "%d_%d", i, pid);
    val = memcached_get(mc, key, strlen(key), &value_length, &flags, &rc);
    TRACE2("%s : %s", key, val);
    if (! do_one_connection)
      memcached_quit(mc);
#if DEBUG
    if (strcmp(val, data[i])) {
      printf("[ERROR] mismatch: [%d] <%s>vs<%s>\n", i, data[i], val);
    }
#endif
  }
  printf("last get: key:%s val:%s\n", key, val);
}

int main(int argc, char ** argv)
{
  int           key_num     = 50000;
  int           bench_count = 1;
  int           binary_prot = 0;
  char          server[255] = "";
  char          port[8]     = "11211";
  int           i;
  memcached_st *mc;
  int           c;

  pid = getpid();

  if (argc == 1) usage();
  while ((c = getopt(argc, argv, "m:k:c:h1b")) >= 0) {

    switch (c) {
    case 'm':
      memcpy(server, optarg, strlen(optarg)+1);
      for (i=0; server[i]!='\0'; i++) {
        if (server[i] == ':') {
          server[i]='\0';
          break;
        }
      }
      i++;
      memcpy(port, optarg+i, strlen(optarg)-i+1);
      break;
    case 'k':
      key_num = atoi(optarg);
      break;
    case 'c':
      bench_count = atoi(optarg);
      break;
    case '1':
      do_one_connection = 1;
      break;
    case 'b':
      binary_prot = 1;
      break;
    case 'h':
    case '?':
      usage();
      break;
    default:
      usage();
    }
  }

  TRACE("server:port=%s:%d", server, atoi(port));
  if (strcmp(server, "") == 0) usage();
  printf("bench_count:%d key_num:%d\n", bench_count, key_num);
  if (key_num >= DATA_MAX) die("key_num is too big.");

  srand(time(NULL));
  for (i=0; i<key_num; i++) {
    sprintf(data[i], "%d",
            abs(10000*(rand()/(RAND_MAX+1.0)))
            );
    TRACE2("[%d]%s", i, data[i]);
  }

  mc = memcached_create(NULL);
  if (mc == NULL) die("memcached_create");
  memcached_server_add(mc, server, atoi(port));

  memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, binary_prot);

  TRACE("%s","begin set");
  for (i=0; i<bench_count; i++) {
    do_set(mc, key_num);
  }
  TRACE("%s","end   set");

  TRACE("%s","begin get");
  for (i=0; i<bench_count; i++) {
    do_get(mc, key_num);
  }
  TRACE("%s","end   get");

  return 0;
}

