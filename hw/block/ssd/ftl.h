// File: ftl.h
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#ifndef _FTL_H_
#define _FTL_H_

#include "common.h"
#include "vssim_config_manager.h"
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

struct ssdstate;
//extern int64_t blocking_to;

void FTL_INIT(struct ssdstate *ssd);
void FTL_TERM(struct ssdstate *ssd);

int64_t FTL_READ(struct ssdstate *ssd, int64_t sector_nb, unsigned int length);
int64_t FTL_WRITE(struct ssdstate *ssd, int64_t sector_nb, unsigned int length);

int64_t _FTL_READ(struct ssdstate *ssd, int64_t sector_nb, unsigned int length);
int64_t _FTL_WRITE(struct ssdstate *ssd, int64_t sector_nb, unsigned int length);



/***********************************/
/* multi-tenant */

struct USER_INFO {
    int32_t channel_num;
    int32_t started_channel;
    int32_t ended_channel;
    int64_t minLPN;
    int64_t maxLPN;

    int32_t started_plane;
    int32_t ended_plane;

    int32_t next_write_plane;

    /* Garbage Collection */
    int gc_count;
    int TOTAL_EMPTY_BLOCK_NB;
    int GC_THRESHOLD_BLOCK_NB;
    
    
    /* Physical resources */
    int32_t free_block_num;
    int32_t used_block_num;

    int32_t free_page_num;
    int32_t valid_page_num;
    int32_t invalid_page_num;
    
    /* Deduplication Statistic */   
    int64_t self_page_read;
    int64_t other_page_read;
    int64_t self_page_write;
    int64_t other_page_write;
    int64_t gc_page_read;
    int64_t gc_page_write;

    /* Average IO Time */
    double avg_write_delay;
    double total_write_count;
    double total_write_delay;

    double avg_read_delay;
    double total_read_count;
    double total_read_delay;

    double avg_gc_write_delay;
    double total_gc_write_count;
    double total_gc_write_delay;

    double avg_gc_read_delay;
    double total_gc_read_count;
    double total_gc_read_delay;

    double avg_seq_write_delay;
    double total_seq_write_count;
    double total_seq_write_delay;

    double avg_ran_write_delay;
    double total_ran_write_count;
    double total_ran_write_delay;

    double avg_ran_cold_write_delay;
    double total_ran_cold_write_count;
    double total_ran_cold_write_delay;

    double avg_ran_hot_write_delay;
    double total_ran_hot_write_count;
    double total_ran_hot_write_delay;

    double avg_seq_merge_read_delay;
    double total_seq_merge_read_count;
    double total_seq_merge_read_delay;

    double avg_ran_merge_read_delay;
    double total_ran_merge_read_count;
    double total_ran_merge_read_delay;

    double avg_seq_merge_write_delay;
    double total_seq_merge_write_count;
    double total_seq_merge_write_delay;

    double avg_ran_merge_write_delay;
    double total_ran_merge_write_count;
    double total_ran_merge_write_delay;

    double avg_ran_cold_merge_write_delay;
    double total_ran_cold_merge_write_count;
    double total_ran_cold_merge_write_delay;

    double avg_ran_hot_merge_write_delay;
    double total_ran_hot_merge_write_count;
    double total_ran_hot_merge_write_delay;

    /* IO Latency */
    unsigned int io_request_nb;
    unsigned int io_request_seq_nb;

     /* Calculate IO Latency */
    double read_latency_count;
    double write_latency_count;

    double avg_read_latency;
    double avg_write_latency;
};

/* Assitent Functions */
void myPanic(const char func[], const char msg[]);
int32_t sum(const int array[]);


/* Multi-tenant */
void INIT_MULTITENANT_CONFIG(struct ssdstate *ssd);
int CAL_USER_BY_CHANNEL(struct ssdstate *ssd, int channel);
int CAL_USER_BY_LPN(struct ssdstate *ssd, int64_t lpn);

/* Deduplication */
void INIT_zipf_AND_fingerprint(struct ssdstate *ssd);
int64_t FP_GENERATOR(struct ssdstate *ssd, int64_t lpn);

/* debug */
#define DEBUG
#ifdef DEBUG
void PRINT_USER_CONFIG(struct ssdstate *ssd);
void PRINT_USER_PLANE_STAT(struct ssdstate *ssd, int user);
void PRINT_USER_STAT(struct ssdstate *ssd);

void PRINT_PLANE_STAT(struct ssdstate *ssd);

bool CHECK_MULTITENANT_LEGAL(struct ssdstate *ssd); /* Check if the configuration is legal */

#endif //DEBUG

#endif
