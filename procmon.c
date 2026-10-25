#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <ncurses.h>
#define MAX 32768
typedef struct{int pid;char name[128];unsigned long long ticks,start;long rss;double cpu;int sustained;}Process;
static Process old[MAX],now_[MAX];static size_t old_n,n;static int by_memory;
static int read_process(const char*root,int pid,Process*p){
 char file[256],line[4096];snprintf(file,sizeof file,"%s/%d/stat",root,pid);FILE*f=fopen(file,"r");if(!f)return 0;
 if(!fgets(line,sizeof line,f)){fclose(f);return 0;}fclose(f);char*left=strchr(line,'('),*right=strrchr(line,')');if(!left||!right)return 0;
 memset(p,0,sizeof(*p));p->pid=pid;size_t len=(size_t)(right-left-1);if(len>=sizeof p->name)len=sizeof p->name-1;memcpy(p->name,left+1,len);
 char*save;char*token=strtok_r(right+2," ",&save);unsigned long long utime=0,stime=0;
 for(int field=3;token;field++,token=strtok_r(NULL," ",&save)){
  if(field==14)utime=strtoull(token,NULL,10);if(field==15)stime=strtoull(token,NULL,10);if(field==22)p->start=strtoull(token,NULL,10);if(field==24)p->rss=strtol(token,NULL,10)*sysconf(_SC_PAGESIZE);
 }p->ticks=utime+stime;return 1;
}
static int cmp(const void*a,const void*b){const Process*x=a,*y=b;double delta=by_memory?(double)y->rss-x->rss:y->cpu-x->cpu;return delta>0?1:delta<0?-1:0;}
static void sample(double seconds){
 DIR*d=opendir("/proc");if(!d){perror("/proc");exit(1);}n=0;struct dirent*entry;
 while((entry=readdir(d))&&n<MAX){int pid=atoi(entry->d_name);if(pid<1)continue;Process p;if(!read_process("/proc",pid,&p))continue;
 for(size_t i=0;i<old_n;i++)if(old[i].pid==pid&&old[i].start==p.start){if(p.ticks>=old[i].ticks)p.cpu=100.0*(p.ticks-old[i].ticks)/sysconf(_SC_CLK_TCK)/seconds;p.sustained=(p.cpu>80||p.rss>512L*1024*1024)?old[i].sustained+1:0;break;}
 now_[n++]=p;
 }closedir(d);qsort(now_,n,sizeof(Process),cmp);memcpy(old,now_,n*sizeof(Process));old_n=n;
}
int main(int argc,char**argv){
 int daemon=argc>1&&!strcmp(argv[1],"--daemon");FILE*log=NULL;int selected=0;
 if(daemon){log=fopen(argc>2?argv[2]:"procmon.log","a");if(!log){perror("log");return 1;}}
 else{initscr();cbreak();noecho();keypad(stdscr,1);timeout(1000);}
 struct timespec previous,current;clock_gettime(CLOCK_MONOTONIC,&previous);
 for(;;){
  if(daemon){struct timespec delay={1,0};nanosleep(&delay,NULL);}
  clock_gettime(CLOCK_MONOTONIC,&current);double elapsed=(current.tv_sec-previous.tv_sec)+(current.tv_nsec-previous.tv_nsec)/1e9;if(elapsed<.001)elapsed=1;previous=current;
  sample(elapsed);
  if(daemon){for(size_t i=0;i<n;i++)if(now_[i].sustained==10){fprintf(log,"%ld pid=%d name=%s cpu=%.1f rss=%ld\n",(long)time(NULL),now_[i].pid,now_[i].name,now_[i].cpu,now_[i].rss);fflush(log);}continue;}
  if(selected>=(int)n)selected=(int)n-1;if(selected<0)selected=0;erase();mvprintw(0,0,"PID      CPU%%    RSS MiB  NAME | c/m sort, arrows select, k terminate, q quit");
  for(int i=0;i<(int)n&&i<LINES-2;i++){if(i==selected)attron(A_REVERSE);mvprintw(i+1,0,"%-8d %6.1f %9.1f  %.80s",now_[i].pid,now_[i].cpu,now_[i].rss/1048576.0,now_[i].name);if(i==selected)attroff(A_REVERSE);}refresh();
  int key=getch();if(key=='q')break;if(key=='m')by_memory=1;if(key=='c')by_memory=0;if(key==KEY_DOWN&&selected<(int)n-1)selected++;if(key==KEY_UP&&selected>0)selected--;
  if(key=='k'&&n){int pid=now_[selected].pid;mvprintw(LINES-1,0,"Send SIGTERM to %d? Press y to confirm",pid);refresh();if(getch()=='y'&&pid>1){if(kill(pid,SIGTERM))beep();}}
 }endwin();return 0;
}
