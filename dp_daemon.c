#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "list.h"

#define	DP_DEBUG	(0)
#define	dp_log(format, ...)	printf("[isp_dp]: "format "\n" , ##__VA_ARGS__)
#define	dp_err(format, ...)	printf("[isp_dp]: line %d error! "format "\n" , __LINE__, ##__VA_ARGS__)
#if DP_DEBUG
#define	dp_dbg(format, ...)	printf("[isp_dp]: "format "\n" , ##__VA_ARGS__)
#else
#define	dp_dbg(format, ...)
#endif

/*----------------------------------------------*/
/* 		dump to file 			*/
/*----------------------------------------------*/
#define STI_PER_LINE	(32)
#define	DUMP_PATH	"./dump"

struct dp_yuv_t {
	uint8_t *yaddr;
	uint8_t *caddr;
	uint32_t ysize;
	uint32_t csize;
	uint16_t xres;
	uint16_t yres;
};

void dp_yuv_dump(struct dp_yuv_t *dy, int index)
{
	int fd, ylen = 0, clen = 0, wlen;
	char dump_fname[64] = { 0 };

        if (access(DUMP_PATH, F_OK | W_OK))
                mkdir(DUMP_PATH, 0766);

	sprintf(dump_fname, "%s/%d-%dx%d.nv12", DUMP_PATH, index, dy->xres, dy->yres);

	dp_log("dump to %s", dump_fname);

	fd = open(dump_fname, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if (fd < 0) {
		dp_err("yuv dump file open error");
		return;
	}

	while (ylen < dy->ysize) {
		wlen = write(fd, dy->yaddr + ylen, dy->ysize - ylen);
		if (wlen < 0) {
			dp_err("yuv dump write y error");
			break;
		}
		ylen += wlen;
	}

	while (clen < dy->csize) {
		wlen = write(fd, dy->caddr + clen, dy->csize - clen);
		if (wlen < 0) {
			dp_err("yuv dump write uv error");
			break;
		}
		clen += wlen;
	}

	close(fd);
}

void dp_statistic_dump(struct statistics_t *st, int index)
{
	FILE *fp;
	char *dump_buf;
	int dump_len = 0;
	int write_len = 0;
	int wlen = 0;
	int i, j, k, l;
	float fv;
	int *parray;
	char dump_file[64] = { 0 };
	struct rgb_statistic_t *pgrid;

	#define dump_printf(fmt, ...) 		\
		dump_len += sprintf(dump_buf + dump_len, fmt, ##__VA_ARGS__);
	#define dump_write() do {			\
		write_len = 0;					\
		while(write_len < dump_len) { 	\
			wlen = fwrite(dump_buf+write_len, 1, dump_len - write_len, fp); \
			if (0 > wlen) {				\
				dp_err("sti dump file write"); \
				break;					\
			}							\
			write_len += wlen;			\
		} dump_len = 0;					\
		} while(0)

	dump_buf = calloc(sizeof(*st), 8);
	if (NULL == dump_buf) {
		dp_err("sti buf calloc failed");
		return;
	}

	sprintf(dump_file, "%s/%d-statistic-data.txt", DUMP_PATH, index);
	dp_log("statistics dump to %s", dump_file);

	fp = fopen(dump_file, "w+");
	if (NULL == fp) {
		dp_err("file open error");
		return;
	}

	/* GRID */
#if 0
	dump_printf("\n[GRID-R/G: %dx%d]\n", GRID_V_BLOCK_NUM, GRID_H_BLOCK_NUM);
	for (i = 0; i < GRID_V_BLOCK_NUM; i++) {
		for (j = 0; j < GRID_H_BLOCK_NUM; j++) {
			pgrid = &st->rgb_statistic[i][j];
			fv = ((float)pgrid->Rsum_avg) / pgrid->Gsum_avg;
			dump_printf("%4.2f ", fv);
		}
		dump_printf("\n", fv);
	}

	dump_printf("\n[GRID-B/G: %dx%d]\n", GRID_V_BLOCK_NUM, GRID_H_BLOCK_NUM);
	for (i = 0; i < GRID_V_BLOCK_NUM; i++) {
		for (j = 0; j < GRID_H_BLOCK_NUM; j++) {
			pgrid = &st->rgb_statistic[i][j];
			fv = ((float)pgrid->Bsum_avg) / pgrid->Gsum_avg;
			dump_printf("%4.2f ", fv);
		}
		dump_printf("\n", fv);
	}
#else
	dump_printf("\n[GRID-R: %dx%d]\n", GRID_V_BLOCK_NUM, GRID_H_BLOCK_NUM);
	for (i = 0; i < GRID_V_BLOCK_NUM; i++) {
		for (j = 0; j < GRID_H_BLOCK_NUM; j++) {
			pgrid = &st->rgb_statistic[i][j];
			dump_printf("%d ", pgrid->Rsum_avg);
		}
		dump_printf("\n");
	}

	dump_printf("\n[GRID-G: %dx%d]\n", GRID_V_BLOCK_NUM, GRID_H_BLOCK_NUM);
	for (i = 0; i < GRID_V_BLOCK_NUM; i++) {
		for (j = 0; j < GRID_H_BLOCK_NUM; j++) {
			pgrid = &st->rgb_statistic[i][j];
			dump_printf("%d ", pgrid->Gsum_avg);
		}
		dump_printf("\n");
	}

	dump_printf("\n[GRID-B: %dx%d]\n", GRID_V_BLOCK_NUM, GRID_H_BLOCK_NUM);
	for (i = 0; i < GRID_V_BLOCK_NUM; i++) {
		for (j = 0; j < GRID_H_BLOCK_NUM; j++) {
			pgrid = &st->rgb_statistic[i][j];
			dump_printf("%d ", pgrid->Bsum_avg);
		}
		dump_printf("\n");
	}
#endif

	dump_printf("\r\n[GRID-H: %dx%d]\r\n", GRID_V_BLOCK_NUM, GRID_H_BLOCK_NUM);
	for (i = 0; i < GRID_V_BLOCK_NUM; i++) {
		for (j = 0; j < GRID_H_BLOCK_NUM; j++) {
			pgrid = &st->rgb_statistic[i][j];
			dump_printf("%-4d ", pgrid->dark_num);
		}
		dump_printf("\r\n", fv);
	}

	dump_printf("\r\n[GRID-L: %dx%d]\r\n", GRID_V_BLOCK_NUM, GRID_H_BLOCK_NUM);
	for (i = 0; i < GRID_V_BLOCK_NUM; i++) {
		for (j = 0; j < GRID_H_BLOCK_NUM; j++) {
			pgrid = &st->rgb_statistic[i][j];
			dump_printf("%-4d ", pgrid->sat_num);
		}
		dump_printf("\r\n");
	}

	/* RSUM */
#if 1
	dump_printf("\n[RSUM: %d]\n", RSUM_ROW_NUM);
	parray = st->rsum_statistic_info;
	for (i = 0; i < (RSUM_ROW_NUM / STI_PER_LINE); i++) {
		for (j = 0; j < STI_PER_LINE; j++) {
			dump_printf("%-4d ", parray[i * STI_PER_LINE + j]);
		dump_printf("\n");
		}
	}
#else
	dump_printf("\n[RSUM: %d]\n", RSUM_ROW_NUM);
	parray = st->rsum_statistic_info;
	for (i = 0; i < RSUM_ROW_NUM - 1; i++)
		dump_printf("%-4d\n", parray[i] - parray[i + 1]);
#endif

	/* HIST */
	for (k = 0; k < 3; k++) {
		int total = 0;
		dump_printf("\n[HIST: %d-%d]\n", k, HIST_BIN_NUM);
		parray = st->hist[k];
		for (i = 0; i < (HIST_BIN_NUM / STI_PER_LINE); i++) {
			for (j = 0; j < STI_PER_LINE; j++) {
				total += parray[i * STI_PER_LINE + j];
				dump_printf("%-4d ", parray[i * STI_PER_LINE + j]);
			}
			dump_printf("\n");
		}
		dump_printf("total: %d\n", total);
		dump_printf("\n");
	}

	/* TILE */
	for (k = 0; k < CDR_V_NUM; k++) {
		for (l = 0; l < CDR_H_NUM; l++) {
			int total = 0;
			dump_printf("\r\n[TILE: %d:%d-%d]\r\n", k, l, CDR_BIN_NUM);
			parray = st->tile[k][l];
			for (i = 0; i < (CDR_BIN_NUM / STI_PER_LINE); i++) {
				for (j = 0; j < STI_PER_LINE; j++) {
					//total += parray[i * STI_PER_LINE + j];
					dump_printf("%-4d ", parray[i * STI_PER_LINE + j]);
				}
				dump_printf("\r\n", fv);
			}
			//dump_printf("total: %d\n", total);
			//dump_printf("\n");
		}
	}

	dump_write();

	fclose(fp);
	free(dump_buf);

	return;
}

/*----------------------------------------------*/
/* 		daemon main logic 		*/
/*----------------------------------------------*/

#define MAX(a, b) (a > b ? a : b)
#define MIX(a, b) (a > b ? b : a)

#define	DATA_CNT	(5)
#define	STATISTICS_SIZE	(sizeof(struct statistics_t))
#define	Y_DATA_SIZE	(3840*2160)
#define	UV_DATA_SIZE	(3840*2160)

#define	PORT	(6000)
#define	IP_ADDR	"127.0.0.1"
#define LISTEN_BACKLOG	(50)

#define	THREAD_CNT	(2)
#define	THREAD_ALL_EXIT	THREAD_CNT	

struct dp_list_t {
	struct list_head dp_pending;
	struct list_head dp_work;
	struct list_head dp_done;
	pthread_cond_t cond;
	pthread_mutex_t mtx;
};

struct dp_flag_t {
	char sti:1;
	char yuv:1;
	char grid:1;
	char hist:1;
	char tile:1;
	char rsum:1;
	char exp:1;
	char gain:1;
};

struct dp_data_t {
	void *ptr;				//statistics
	struct dp_yuv_t dy;
	struct list_head node;
};

struct dp_list_t *dp;
struct dp_data_t *d[DATA_CNT];
struct dp_flag_t df;
short frame_cnt;
short run = 1;

void dp_list_view(char *str);

void dp_cmd_parser(int argc, char *argv[])
{
	int opt, s = 0, y = 0, v = 0;

	dp_dbg("cmd argc %d", argc);

        if (argc == 1) {
                s = 1;
                y = 1;
                df.yuv = 1;
                df.sti = 1;
                goto out;
        }

	optind = 1;
	while ((opt = getopt(argc, argv, "s:y:w:g:")) != -1) {
		switch (opt) {
		case 's':
			df.sti = 1;
			sscanf(optarg, "%d", &s);
			dp_dbg("case s %d", s);
			break;
		case 'y':
			df.yuv = 1;
			sscanf(optarg, "%d", &y);
			dp_dbg("case y %d", y);
			break;
		case 'w':
			sscanf(optarg, "%d", &v);
			if (v > AWB_MODE_A) {
				printf("mode %d exceed valid range\n", v);
				break;
			}
			set_param(SET_AWB_MODE, &v);
			break;
		case 'e':
			sscanf(optarg, "%d", &v);
			if (v > AWB_MODE_A) {
				printf("mode %d exceed valid range\n", v);
				break;
			}
			set_param(SET_AE_MODE, &v);
			break;
		case '?':
			dp_err("unrecognized option: %c, argv: %s", optopt, argv[optind - 1]);
			break;
		case ':':
			dp_err("option: %c missing argument\n", optopt);
			break;
		default:
			dp_err("option: %c\n", opt);
			break;
		}
	}

out:
	frame_cnt = MIX(MAX(s, y), DATA_CNT);
}

/*
 * dp_cmd_thread
 * list pending --> list work
 */
void *dp_cmd_thread(void *arg)
{
#define	STR_MAX	32
	int i = 0, n = 0;
	char *cmds[STR_MAX];
	struct dp_data_t *d;
	int listenfd, connfd;
	socklen_t clilen;
	struct sockaddr_in servaddr, cliaddr;

	dp_log("%s start\n", __func__);

	pthread_detach(pthread_self());

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		dp_err("socket error\n");
		goto out1;
	}

	bzero(&servaddr, sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);

	if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
		dp_err("bind error\n");
		goto out;
	}

	if (listen(listenfd, LISTEN_BACKLOG) < 0) {
		dp_err("listen error\n");
		goto out;
	}

	for (i = 0; i < STR_MAX; i++)
		cmds[i] = calloc(sizeof(char), STR_MAX);

	while (run) {

		for (i = 0; i < STR_MAX; i++)
			memset(cmds[i], 0, STR_MAX);

		clilen = sizeof(cliaddr);
		if((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0) {
			dp_err("accept error\n");
			goto out;
		}

		if (list_empty(&dp->dp_pending))
			continue;

		i = 0;
		while ((n = read(connfd, cmds[i++], STR_MAX)) > 0);

		dp_cmd_parser(--i, cmds);

		close(connfd);

		dp_list_view("cmd_thread ");
		for (i = 0; i < frame_cnt; i++) {
			d = list_entry(dp->dp_pending.next, struct dp_data_t, node);
			list_move_tail(&d->node, &dp->dp_work);
		}
		dp_list_view("cmd_thread ");
	}

	for (i = 0; i < STR_MAX; i++) {
		free(cmds[i]);
		cmds[i] = NULL;
	}

out:
	close(listenfd);

out1:
	dp_log("%s exit", __func__);
	pthread_exit(NULL);
}

/*
 * dp_dump_thread
 * list done --> list pending
 */
void *dp_dump_thread(void *arg)
{
	int c1 = 0, c2 = 0;
	struct dp_data_t *d;

	dp_log("%s start\n", __func__);

	pthread_detach(pthread_self());

	while (run) {
		pthread_mutex_lock(&dp->mtx);

		if (list_empty(&dp->dp_done)) {
			df.sti = 0;
			df.yuv = 0;
			c1 = c2 = 0;
			pthread_cond_wait(&dp->cond, &dp->mtx);
		}

		d = list_entry(dp->dp_done.next, struct dp_data_t, node);

		if (df.sti)
			dp_statistic_dump(d->ptr, c1++);

		if (df.yuv)
			dp_yuv_dump(&d->dy, c2++);

		memset(d->ptr, 0, STATISTICS_SIZE);
		memset(d->dy.yaddr, 0, Y_DATA_SIZE);
		memset(d->dy.caddr, 0, UV_DATA_SIZE);

		dp_list_view("dump_thread ");
		list_move_tail(&d->node, &dp->dp_pending);
		dp_list_view("dump_thread ");
	
out:
		pthread_mutex_unlock(&dp->mtx);
	}

	dp_log("%s exit", __func__);
	pthread_exit(NULL);
}

/*
 * dp_data_process
 * list work --> list done
 */
void dp_data_process(struct dp_yuv_t * const in)
{
	struct dp_data_t *d;

	if (list_empty(&dp->dp_work))
		return;

	d = list_entry(dp->dp_work.next, struct dp_data_t, node);

	isp_statistics_read(d->ptr);

	d->dy.ysize = in->ysize;
	d->dy.csize = in->csize;
	d->dy.xres = in->xres;
	d->dy.yres = in->yres;
	memcpy(d->dy.yaddr, in->yaddr, in->ysize);
	memcpy(d->dy.caddr, in->caddr, in->csize);

	dp_list_view("data_process ");
	list_move_tail(&d->node, &dp->dp_done);
	dp_list_view("data_process ");

	/* all of node in work list are processed, it's time to wake up dump thread */
	if (list_empty(&dp->dp_work))
		pthread_cond_signal(&dp->cond);
}

void dp_list_view(char *str)
{
	int p = 0, w = 0, d = 0;
	struct list_head *this, *next;

	list_for_each_safe(this, next, &dp->dp_pending)
		p++;

	list_for_each_safe(this, next, &dp->dp_work)
		w++;

	list_for_each_safe(this, next, &dp->dp_done)
		d++;

	dp_dbg("(%s) pending list count %d", str, p);
	dp_dbg("(%s) work list count %d", str, w);
	dp_dbg("(%s) done list count %d", str, d);
}

void dp_data_construct()
{
	int i;

	for (i = 0; i < DATA_CNT; i++) {
		d[i] = calloc(sizeof(struct dp_data_t), 1);
		d[i]->ptr = calloc(STATISTICS_SIZE, 1);
		d[i]->dy.yaddr = calloc(Y_DATA_SIZE, 1);
		d[i]->dy.caddr = calloc(UV_DATA_SIZE, 1);
		list_add_tail(&d[i]->node, &dp->dp_pending);
	}
}

void dp_data_deconstruct()
{
	int i;

	for (i = 0; i < DATA_CNT; i++) {
		free(d[i]->ptr);
		free(d[i]->dy.yaddr);
		free(d[i]->dy.caddr);
		free(d[i]);
	}
}

void dp_init()
{
	int ret;
	pthread_t pid_s;
	pthread_t pid_d;

	dp = calloc(sizeof(struct dp_list_t), 1);

	pthread_mutex_init(&dp->mtx, NULL);
	pthread_cond_init(&dp->cond, NULL);

	INIT_LIST_HEAD(&dp->dp_pending);
	INIT_LIST_HEAD(&dp->dp_work);
	INIT_LIST_HEAD(&dp->dp_done);

	dp_data_construct();
	dp_list_view("dp_init ");

        ret = pthread_create(&pid_s, NULL, (void *)dp_cmd_thread, NULL);
        if (ret) {
                dp_err("dp_cmd_thread thread create failed\n");
                return;
        }

        ret = pthread_create(&pid_d, NULL, (void *)dp_dump_thread, NULL);
        if (ret) {
                dp_err("dp_dump_thread thread create failed\n");
                return;
        }

	dp_log("%s done", __func__);
}

void dp_deinit()
{
	run = 0;

	pthread_cond_signal(&dp->cond);
	usleep(100*1000);
	pthread_mutex_destroy(&dp->mtx);
	pthread_cond_destroy(&dp->cond);
	dp_data_deconstruct();
	free(dp);

	dp_log("%s done", __func__);
}
