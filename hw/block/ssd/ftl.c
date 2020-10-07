// File: ftl.c
// Date: 2014. 12. 03.
// Author: Jinsoo Yoo (jedisty@hanyang.ac.kr)
// Copyright(c)2014
// Hanyang University, Seoul, Korea
// Embedded Software Systems Laboratory. All right reserved

#include "qemu/osdep.h"
#include "block/block_int.h"
#include "block/qapi.h"
#include "exec/memory.h"
#include "hw/block/block.h"
#include "hw/hw.h"
#include "hw/pci/msix.h"
#include "hw/pci/msi.h"
#include "hw/pci/pci.h"
#include "qapi/visitor.h"
#include "qapi/error.h"
#include "qemu/error-report.h"
#include "qemu/bitops.h"
#include "qemu/bitmap.h"
#include "qom/object.h"
#include "sysemu/sysemu.h"
#include "sysemu/block-backend.h"
#include <qemu/main-loop.h>
#include "block/block_int.h"
#include "god.h"
#include "common.h"
#include "ssd_io_manager.h"
#include "ftl_gc_manager.h"
#include "ssd.h"
#include "vssim_config_manager.h"
#ifndef VSSIM_BENCH
//#include "qemu-kvm.h"
#endif

#ifdef FTL_GET_WRITE_WORKLOAD
FILE* fp_write_workload;
#endif
#ifdef FTL_IO_LATENCY
FILE* fp_ftl_w;
FILE* fp_ftl_r;
#endif

extern int64_t blocking_to;

//extern double ssd_util;
//extern int64_t mygc_cnt, last_gc_cnt;


//extern int64_t mycopy_page_nb;
//extern FILE *statfp;


#if 0
int64_t get_total_free_pages()
{
    int64_t nb_total_free_pages = 0;
    victim_block_root *vr = (victim_block_root *)victim_block_list;
    victim_block_entry *ve = NULL;
    block_state_entry *bse = NULL;
    int i;
    for (i = 0; i < VICTIM_TABLE_ENTRY_NB; i++) {
        ve = (victim_block_entry *)vr->head;
        while (ve != NULL) {
            bse = GET_BLOCK_STATE_ENTRY(ve->phy_flash_nb, ve->phy_block_nb);
            for (i = 0; i < PAGE_NB; i++) {
                if (bse->valid_array[i] == '0') {
                    nb_total_free_pages++;
                }
            }

            ve = ve->next;
        }

        vr += 1;
    }

    // traverse through empty block list 
    empty_block_root *ebr = (empty_block_root *)empty_block_list; 
    nb_total_free_pages += total_empty_block_nb * PAGE_NB;

    return nb_total_free_pages;
}
#endif

void FTL_INIT(struct ssdstate *ssd)
{
    int g_init = ssd->g_init;

	if(g_init == 0){
        	//printf("[%s] start\n", __FUNCTION__);

		INIT_SSD_CONFIG(ssd);
		INIT_MULTITENANT_CONFIG(ssd);
		INIT_zipf_AND_fingerprint(ssd);
#ifdef DEBUG
		//PRINT_USER_CONFIG(ssd);
#endif

		INIT_MAPPING_TABLE(ssd);
		INIT_INVERSE_MAPPING_TABLE(ssd);
		INIT_BLOCK_STATE_TABLE(ssd);
		INIT_VALID_ARRAY(ssd);
		INIT_EMPTY_BLOCK_LIST(ssd);
		INIT_VICTIM_BLOCK_LIST(ssd);
		INIT_PERF_CHECKER(ssd);
		
#ifdef FTL_MAP_CACHE
		INIT_CACHE();
#endif
#ifdef FIRM_IO_BUFFER
		INIT_IO_BUFFER();
#endif
#ifdef MONITOR_ON
		INIT_LOG_MANAGER();
#endif
		g_init = 1;
#ifdef FTL_GET_WRITE_WORKLOAD
		fp_write_workload = fopen("./data/p_write_workload.txt","a");
#endif
#ifdef FTL_IO_LATENCY
		fp_ftl_w = fopen("./data/p_ftl_w.txt","a");
		fp_ftl_r = fopen("./data/p_ftl_r.txt","a");
#endif
		SSD_IO_INIT(ssd);
	
		//printf("[%s] complete\n", __FUNCTION__);
	}
}

void FTL_TERM(struct ssdstate *ssd)
{
	printf("[%s] start\n", __FUNCTION__);

#if 0
#ifdef FIRM_IO_BUFFER
	TERM_IO_BUFFER();
#endif

	TERM_MAPPING_TABLE(ssd);
	TERM_INVERSE_MAPPING_TABLE(ssd);
	TERM_VALID_ARRAY(ssd);
	TERM_BLOCK_STATE_TABLE(ssd);
	TERM_EMPTY_BLOCK_LIST(ssd);
	TERM_VICTIM_BLOCK_LIST(ssd);
	TERM_PERF_CHECKER(ssd);

#ifdef MONITOR_ON
	TERM_LOG_MANAGER();
#endif

#ifdef FTL_IO_LATENCY
	fclose(fp_ftl_w);
	fclose(fp_ftl_r);
#endif
#endif
	printf("[%s] complete\n", __FUNCTION__);
}

int64_t FTL_READ(struct ssdstate *ssd, int64_t sector_nb, unsigned int length)
{
	int ret;

#ifdef GET_FTL_WORKLOAD
	FILE* fp_workload = fopen("./data/workload_ftl.txt","a");
	struct timeval tv;
	struct tm *lt;
	double curr_time;
	gettimeofday(&tv, 0);
	lt = localtime(&(tv.tv_sec));
	curr_time = lt->tm_hour*3600 + lt->tm_min*60 + lt->tm_sec + (double)tv.tv_usec/(double)1000000;
	//fprintf(fp_workload,"%lf %d %ld %u %x\n",curr_time, 0, sector_nb, length, 1);
	fprintf(fp_workload,"%lf %d %u %x\n",curr_time, sector_nb, length, 1);
	fclose(fp_workload);
#endif
#ifdef FTL_IO_LATENCY
	int64_t start_ftl_r, end_ftl_r;
	start_ftl_r = get_usec();
#endif
	return _FTL_READ(ssd, sector_nb, length);
#ifdef FTL_IO_LATENCY
	end_ftl_r = get_usec();
	if(length >= 128)
		fprintf(fp_ftl_r,"%ld\t%u\n", end_ftl_r - start_ftl_r, length);
#endif
}

int64_t FTL_WRITE(struct ssdstate *ssd, int64_t sector_nb, unsigned int length)
{
	int ret;

#ifdef GET_FTL_WORKLOAD
	FILE* fp_workload = fopen("./data/workload_ftl.txt","a");
	struct timeval tv;
	struct tm *lt;
	double curr_time;
	gettimeofday(&tv, 0);
	lt = localtime(&(tv.tv_sec));
	curr_time = lt->tm_hour*3600 + lt->tm_min*60 + lt->tm_sec + (double)tv.tv_usec/(double)1000000;
//	fprintf(fp_workload,"%lf %d %ld %u %x\n",curr_time, 0, sector_nb, length, 0);
	fprintf(fp_workload,"%lf %d %u %x\n",curr_time, sector_nb, length, 0);
	fclose(fp_workload);
#endif
#ifdef FTL_IO_LATENCY
	int64_t start_ftl_w, end_ftl_w;
	start_ftl_w = get_usec();
#endif
	ret = _FTL_WRITE(ssd, sector_nb, length);
#ifdef FTL_IO_LATENCY
	end_ftl_w = get_usec();
	if(length >= 128)
		fprintf(fp_ftl_w,"%ld\t%u\n", end_ftl_w - start_ftl_w, length);
#endif

    return ret;
}


int64_t _FTL_READ(struct ssdstate *ssd, int64_t sector_nb, unsigned int length)
{
    struct ssdconf *sc = &(ssd->ssdparams);
    int64_t SECTOR_NB = sc->SECTOR_NB;
    int64_t SECTORS_PER_PAGE = sc->SECTORS_PER_PAGE;
    int PLANES_PER_FLASH = sc->PLANES_PER_FLASH;
    int CHANNEL_NB = sc->CHANNEL_NB;
    int GC_MODE = sc->GC_MODE;
    int64_t *gc_slot = ssd->gc_slot;
    int64_t cur_need_to_emulate_tt = 0, max_need_to_emulate_tt = 0;

    int64_t curtime = get_usec();
#if 0
    if (curtime - last_time >= 1e7) { /* Coperd: every ten second */
        //printf("%s, %ld, %ld, %ld\n", __func__, pthread_self(), curtime, last_time);
        last_time = curtime;
        fprintf(statfp, "%d,%d,%d,%ld,%ld,%d,%d\n", 
                nb_blocked_reads, 
                nb_total_reads, 
                nb_total_writes, 
                nb_total_rd_sz, 
                nb_total_wr_sz,
                mygc_cnt,
                mycopy_page_nb);
                //total_empty_block_nb,
                //get_total_free_pages());
        fflush(statfp);

        /* Coperd: clear all related counters */
        nb_blocked_reads = 0;
        nb_total_reads = 0;
        nb_total_rd_sz = 0;
        nb_total_writes = 0;
        nb_total_wr_sz = 0;
        mygc_cnt = 0;
        mycopy_page_nb = 0;
    }
#endif

    /* Coperd: FTL layer blocked reads statistics */
    ssd->nb_total_reads++;
    ssd->nb_total_rd_sz += length;


#ifdef FTL_DEBUG
	printf("[%s] Start\n", __FUNCTION__);
#endif

	if(sector_nb + length > SECTOR_NB){
		printf("Error[%s] Exceed Sector number\n", __FUNCTION__); 
		return FAIL;	
	}

	int64_t lpn;
	int64_t fp;
	int64_t ppn;
	int64_t lba = sector_nb;
	unsigned int remain = length;
	unsigned int left_skip = sector_nb % SECTORS_PER_PAGE;
	unsigned int right_skip;
	unsigned int read_sects;

	unsigned int ret = FAIL;
	int read_page_nb = 0;
	int io_page_nb;

	nand_io_info* n_io_info = NULL;

    int num_flash = 0, num_blk = 0, num_channel = 0, num_plane = 0;
    int slot;

#ifdef FIRM_IO_BUFFER
	INCREASE_RB_FTL_POINTER(length);
#endif

	while(remain > 0){
		ppn = -1;
		if(remain > SECTORS_PER_PAGE - left_skip){
			right_skip = 0;
		}
		else{
			right_skip = SECTORS_PER_PAGE - left_skip - remain;
		}
		read_sects = SECTORS_PER_PAGE - left_skip - right_skip;

		lpn = lba / (int64_t)SECTORS_PER_PAGE;
		fp = GET_MAPPING_INFO(ssd, lpn);
		if (fp == -1) {
			printf("fp[%lld] not mapped!!!\n", fp);
		}
		else {
			ppn = GET_MAPPING_INFO_SECOND(ssd, fp);
		}
		

		if(ppn == -1){
#ifdef FIRM_IO_BUFFER
			INCREASE_RB_LIMIT_POINTER();
#endif
            printf("ppn[%lld] not mapped!!!\n", ppn);
			//return FAIL;
		}

		lba += read_sects;
		remain -= read_sects;
		left_skip = 0;
	}

	ssd->io_alloc_overhead = ALLOC_IO_REQUEST(ssd, sector_nb, length, READ, &io_page_nb);

	remain = length;
	lba = sector_nb;
	left_skip = sector_nb % SECTORS_PER_PAGE;

    /* 
     * Coperd: since the whole I/O submission path is single threaded, it's
     * safe to do this. "blocking_to" means the time we will block the
     * current I/O to. It will be finally decided by gc timestamps according 
     * to the GC mode you are using.
     */
    blocking_to = 0;

#if 0
    printf("req [%ld, %d] goes to ", sector_nb, length);
    if (GC_MODE == CHANNEL_BLOCKING) {
        printf("channel( ");
    } else if (GC_MODE == CHIP_BLOCKING) {
        printf("chip( ");
    }
#endif

	while(remain > 0) {		
		ppn = -1;
		if(remain > SECTORS_PER_PAGE - left_skip){
			right_skip = 0;
		}
		else{
			right_skip = SECTORS_PER_PAGE - left_skip - remain;
		}
		read_sects = SECTORS_PER_PAGE - left_skip - right_skip;

		lpn = lba / (int64_t)SECTORS_PER_PAGE;

#ifdef FTL_MAP_CACHE
		ppn = CACHE_GET_PPN(lpn);
#else
		fp = GET_MAPPING_INFO(ssd, lpn);
		if (fp == -1) {
			printf("ERROR[%s] No Mapping info\n", __FUNCTION__);
		}
		else {
			ppn = GET_MAPPING_INFO_SECOND(ssd, fp);
		}
		
#endif

		if(ppn == -1){
#ifdef FTL_DEBUG
			printf("ERROR[%s] No Mapping info\n", __FUNCTION__);
#endif
            ppn = 0;
		}

		/* Read data from NAND page */
        n_io_info = CREATE_NAND_IO_INFO(ssd, read_page_nb, READ, io_page_nb, ssd->io_request_seq_nb);


        num_flash = CALC_FLASH(ssd, ppn);
        num_blk = CALC_BLOCK(ssd, ppn);
        num_channel = num_flash %  CHANNEL_NB;
        num_plane = num_flash * PLANES_PER_FLASH + num_blk % PLANES_PER_FLASH;
        if (GC_MODE == WHOLE_BLOCKING) {
            slot = 0;
        } else if (GC_MODE == CHANNEL_BLOCKING) {
            slot = num_channel;
        } else if (GC_MODE == CHIP_BLOCKING) {
            slot = num_plane;
        }
        //printf("%d,", slot);

        if (gc_slot[slot] > blocking_to) {
            blocking_to = gc_slot[slot];
        }

		cur_need_to_emulate_tt = SSD_PAGE_READ(ssd, num_flash, num_blk, CALC_PAGE(ssd, ppn), n_io_info);

        if (cur_need_to_emulate_tt > max_need_to_emulate_tt) {
            max_need_to_emulate_tt = cur_need_to_emulate_tt;
        }

#ifdef FTL_DEBUG
		if(ret == SUCCESS){
			printf("\t read complete [%u]\n",ppn);
		}
		else if(ret == FAIL){
			printf("ERROR[%s] %u page read fail \n",__FUNCTION__, ppn);
		}
#endif
		read_page_nb++;

		lba += read_sects;
		remain -= read_sects;
		left_skip = 0;

	}
    //printf("Read, chnl: %d, chip: %d, LAT=%" PRId64 "\n", num_channel, num_flash, max_need_to_emulate_tt);

#if 0
    printf(")\n");

    printf("ftl(%ld, %d): GC-slot[] = ", sector_nb, length);
    int ti;
    for (ti = 0; ti < 8; ti++)
        printf("%ld, ", gc_slot[ti]);
    printf("\n");
#endif

    if (blocking_to > curtime) {
        ssd->nb_blocked_reads++;
    }

	INCREASE_IO_REQUEST_SEQ_NB(ssd);

#ifdef FIRM_IO_BUFFER
	INCREASE_RB_LIMIT_POINTER();
#endif

#ifdef MONITOR_ON
	char szTemp[1024];
	sprintf(szTemp, "READ PAGE %d ", length);
	WRITE_LOG(szTemp);
#endif

#ifdef FTL_DEBUG
	printf("[%s] Complete\n", __FUNCTION__);
#endif

	return max_need_to_emulate_tt;
}

int64_t _FTL_WRITE(struct ssdstate *ssd, int64_t sector_nb, unsigned int length)
{
	struct ssdconf *sc = &(ssd->ssdparams);
    int64_t SECTOR_NB = sc->SECTOR_NB;
    int64_t SECTORS_PER_PAGE = sc->SECTORS_PER_PAGE;
    int PLANES_PER_FLASH = sc->PLANES_PER_FLASH;
    int CHANNEL_NB = sc->CHANNEL_NB;
    int64_t *gc_slot = ssd->gc_slot;
    int GC_MODE = sc->GC_MODE;
    int EMPTY_TABLE_ENTRY_NB = sc->EMPTY_TABLE_ENTRY_NB;
    int64_t cur_need_to_emulate_tt = 0, max_need_to_emulate_tt = 0;
    int64_t curtime = get_usec();

    if (ssd->in_warmup_stage == 0) {
        ssd->nb_total_writes++;
        ssd->nb_total_wr_sz += length;
    }
	
	int io_page_nb;

	if(sector_nb + length > SECTOR_NB){
		printf("ERROR[%s] Exceed Sector number\n", __FUNCTION__);
                return FAIL;
        }
	else{
		ssd->io_alloc_overhead = ALLOC_IO_REQUEST(ssd, sector_nb, length, WRITE, &io_page_nb);
	}

	int64_t lba = sector_nb;
	int64_t lpn;
	int user;
	int64_t new_ppn;
	int64_t new_fp;
	int64_t old_fp = -1;
	int64_t old_ppn = -1;

	int64_t *finger_print = ssd->fingerprint;

	unsigned int remain = length;
	unsigned int left_skip = sector_nb % SECTORS_PER_PAGE;
	unsigned int right_skip;
	unsigned int write_sects;

	unsigned int ret = FAIL;
	int write_page_nb=0;
	nand_io_info* n_io_info = NULL;

    int num_channel = 0, num_flash = 0, num_blk = 0, num_plane = 0;
    int slot;

    /* 
     * Coperd: since the whole I/O submission path is single threaded, it's
     * safe to do this. "blocking_to" means the time we will block the
     * current I/O to. It will be finally decided by gc timestamps according 
     * to the GC mode you are using.
     */
    blocking_to = 0;

	while(remain > 0) {
		if(remain > SECTORS_PER_PAGE - left_skip){
			right_skip = 0;
		}
		else{
			right_skip = SECTORS_PER_PAGE - left_skip - remain;
		}

		write_sects = SECTORS_PER_PAGE - left_skip - right_skip;
		lpn = lba / (int64_t)SECTORS_PER_PAGE;

		user = CAL_USER_BY_LPN(ssd, lpn);
#ifdef DEBUG
		assert(user>=0 && user < ssd->user_num);
#endif //DEBUG
		
		old_fp = GET_MAPPING_INFO(ssd, lpn);
		new_fp = FP_GENERATOR(ssd, lpn);
		if (old_fp != -1) {	//之前已经存在映射，所以需要将旧映射取消
			UPDATE_OLD_PAGE_MAPPING(ssd, lpn);
		}
		if (finger_print[new_fp] != -1) {
			//不需要写操作
			UPDATE_NEW_PAGE_MAPPING(ssd, lpn, new_fp, finger_print[new_fp]);
		}
		else {
			ret = GET_NEW_PAGE(ssd, user, VICTIM_OVERALL, EMPTY_TABLE_ENTRY_NB, &new_ppn);
			if(ret == FAIL){
				printf("ERROR[%s] Get new page fail \n", __FUNCTION__);
				return FAIL;
			}
			n_io_info = CREATE_NAND_IO_INFO(ssd, write_page_nb, WRITE, io_page_nb, ssd->io_request_seq_nb);

			num_flash = CALC_FLASH(ssd, new_ppn);
			num_blk = CALC_BLOCK(ssd, new_ppn);
			num_channel = num_flash % CHANNEL_NB;
			num_plane = num_flash * PLANES_PER_FLASH + num_blk % PLANES_PER_FLASH;

			if (GC_MODE == WHOLE_BLOCKING) {
				slot = 0;
			} else if (GC_MODE == CHANNEL_BLOCKING) {
				slot = num_channel;
			} else if (GC_MODE == CHIP_BLOCKING) {
				slot = num_plane;
			}

			if (gc_slot[slot] > blocking_to) {
				blocking_to = gc_slot[slot];
			}

			if((left_skip || right_skip) && (old_ppn != -1)){
				cur_need_to_emulate_tt = SSD_PAGE_PARTIAL_WRITE(ssd,
					CALC_FLASH(ssd, old_ppn), CALC_BLOCK(ssd, old_ppn), CALC_PAGE(ssd, old_ppn),
					CALC_FLASH(ssd, new_ppn), CALC_BLOCK(ssd, new_ppn), CALC_PAGE(ssd, new_ppn),
					n_io_info);
			}
			else{
				cur_need_to_emulate_tt = SSD_PAGE_WRITE(ssd, CALC_FLASH(ssd, new_ppn), CALC_BLOCK(ssd, new_ppn), CALC_PAGE(ssd, new_ppn), n_io_info);
			}

			if (cur_need_to_emulate_tt > max_need_to_emulate_tt) {
				max_need_to_emulate_tt = cur_need_to_emulate_tt;
			}

			UPDATE_NEW_PAGE_MAPPING(ssd, lpn, new_fp, new_ppn);
			
			write_page_nb++;
		}
		
		lba += write_sects;
		remain -= write_sects;
		left_skip = 0;
	}

	if (blocking_to > curtime) {
        ssd->nb_blocked_writes++;
        //printf("%s,%.2f,%ld,%ld\n", ssd->ssdname, ssd->nb_blocked_writes*100.0/ssd->nb_total_writes, ssd->nb_blocked_writes, ssd->nb_total_writes);
    }

	INCREASE_IO_REQUEST_SEQ_NB(ssd);
#ifdef GC_ON
	GC_CHECK(ssd, user, CALC_FLASH(ssd, new_ppn), CALC_BLOCK(ssd, new_ppn));
#endif

	return max_need_to_emulate_tt; 
}

int femu_discard_process(struct ssdstate *ssd, uint32_t length, int64_t sector_nb) {
#if 0
	struct ssdconf *sc = &(ssd->ssdparams);
    int64_t SECTOR_NB = sc->SECTOR_NB;
    int64_t SECTORS_PER_PAGE = sc->SECTORS_PER_PAGE;
    int EMPTY_TABLE_ENTRY_NB = sc->EMPTY_TABLE_ENTRY_NB;

	if(sector_nb + length > SECTOR_NB){
		printf("ERROR[%s] Exceed Sector number\n", __FUNCTION__);
        return FAIL;
    }

	int64_t lba = sector_nb;
	int64_t lpn;
	int64_t old_fp;
	int64_t old_ppn;

	unsigned int remain = length;
	unsigned int left_skip = sector_nb % SECTORS_PER_PAGE;
	unsigned int right_skip;
	unsigned int write_sects;

	unsigned int ret = FAIL;

    /* 
     * Coperd: since the whole I/O submission path is single threaded, it's
     * safe to do this. "blocking_to" means the time we will block the
     * current I/O to. It will be finally decided by gc timestamps according 
     * to the GC mode you are using.
     */

	while(remain > 0){

		if(remain > SECTORS_PER_PAGE - left_skip){
			right_skip = 0;
		}
		else{
			right_skip = SECTORS_PER_PAGE - left_skip - remain;
		}

		write_sects = SECTORS_PER_PAGE - left_skip - right_skip;

		//add by hao

		//printf("hao_debug:_FTL_WRITEbbbbbbbbbbbbbbbbbbbbbb %d\n", bloom_temp);
		lpn = lba / (int64_t)SECTORS_PER_PAGE;
		old_fp = GET_MAPPING_INFO(ssd, lpn);
		assert(old_fp != -1);
		old_ppn = GET_MAPPING_INFO_SECOND(ssd, old_fp);
		//printf("hao_debug:_FTL_WRITE lpn old_ppn %d %d\n",lpn, old_ppn);

		if((left_skip || right_skip) && (old_ppn != -1)){
            //printf("hao_dubug4444444444444: ssd page partial write\n");
            /*cur_need_to_emulate_tt = SSD_PAGE_PARTIAL_WRITE(ssd,
				CALC_FLASH(ssd, old_ppn), CALC_BLOCK(ssd, old_ppn), CALC_PAGE(ssd, old_ppn),
				CALC_FLASH(ssd, new_ppn), CALC_BLOCK(ssd, new_ppn), CALC_PAGE(ssd, new_ppn),
				n_io_info);*/
		}

		UPDATE_OLD_PAGE_MAPPING(ssd, lpn);

		lba += write_sects;
		remain -= write_sects;
		left_skip = 0;
	}
#endif
	return true;
}

void myPanic(const char func[], const char msg[]) {
    printf("Error[%s], %s\n", func, msg);
	getchar();
	return;
}

int32_t sum(const int array[]) {
    int32_t res = 0;
    int32_t array_length = sizeof(array) / sizeof(int32_t);
    for (int32_t i = 0; i < array_length; ++i) {
        res += array[i];
    }
    return res;
}

#ifdef DEBUG
bool CHECK_MULTITENANT_LEGAL(struct ssdstate *ssd) {
    struct ssdconf *sc = &(ssd->ssdparams);
    int user_channel[] = USER_CHANNEL_ARRAY;
    int config_channel_num = sum(user_channel);
    int channel_num = sc->CHANNEL_NB;
    if (config_channel_num != channel_num) {
        myPanic(__FUNCTION__, "Channel Number is illegal!");
        return false;
    }
    return true;
}
#endif //DEBUG

void INIT_MULTITENANT_CONFIG(struct ssdstate *ssd) {
#ifdef DEBUG
    assert(CHECK_MULTITENANT_LEGAL(ssd));
#endif //DEBUG
    ssd->user_num = USER_NUM;
    int user_channel[] = USER_CHANNEL_ARRAY;
#ifdef DEBUG
    assert(ssd->user_num != 0);
#endif  //DEBUG
    ssd->user = (struct USER_INFO *)malloc(ssd->user_num * sizeof(struct USER_INFO));
    if(ssd->user == NULL) {
        myPanic(__FUNCTION__, "ssd->user malloc Error!");
        exit(0);
    }
    memset(ssd->user, 0, ssd->user_num * sizeof(struct USER_INFO));

    struct USER_INFO *user_head = ssd->user;
    struct ssdconf *sc = &(ssd->ssdparams);
	int flash_per_channel = sc->FLASH_NB / sc->CHANNEL_NB;
	int plane_per_channel = flash_per_channel * sc->PLANES_PER_FLASH;
    int page_per_channel = sc->PAGE_NB * sc->BLOCK_NB * flash_per_channel;
    int started_channel = 0;
    for (int i = 0; i < USER_NUM; ++i) {
        user_head->channel_num = user_channel[i];
        user_head->started_channel = started_channel;
        user_head->ended_channel = started_channel + user_head->channel_num;
		user_head->started_plane = started_channel * plane_per_channel;
		user_head->ended_plane = user_head->ended_channel * plane_per_channel;
		user_head->next_write_plane = user_head->started_plane;
        user_head->minLPN = started_channel * page_per_channel;
        user_head->maxLPN = user_head->ended_channel * page_per_channel;

		user_head->TOTAL_EMPTY_BLOCK_NB = sc->BLOCK_NB * (sc->FLASH_NB / sc->CHANNEL_NB) * user_channel[i];
		user_head->GC_THRESHOLD_BLOCK_NB = (int)((1-sc->GC_THRESHOLD) * (double)user_head->TOTAL_EMPTY_BLOCK_NB);
        user_head->free_block_num = user_head->TOTAL_EMPTY_BLOCK_NB;
        user_head->free_page_num = page_per_channel * user_channel[i];
		started_channel += user_channel[i];
        user_head ++;
    }
}

int CAL_USER_BY_CHANNEL(struct ssdstate *ssd, int channel) {
	 int res = -1;
	 struct USER_INFO *user_head = ssd->user;
	 int user_num = ssd->user_num;
	 
	 for (int i = 0; i < user_num; ++i) {
		 if (channel >= user_head->started_channel 
		 	&& channel < user_head->ended_channel) {
				 res = i;
				 break;
		 }
		 user_head ++;
	 }
	 if(res == -1) myPanic(__FUNCTION__, "Cannot find the user");
	 return res;
}

int CAL_USER_BY_LPN(struct ssdstate *ssd, int64_t lpn) {
	int res = -1;
	int user_num = ssd->user_num;
	struct USER_INFO *user_head = ssd->user;

	for (int i = 0; i < user_num; ++i) {
		if (lpn >= user_head->minLPN && lpn < user_head->maxLPN) {
			res = i;
			break;
		}
		user_head ++;
	}

	if(res == -1) myPanic(__FUNCTION__, "Cannot find the user");
	return res;
}

#ifdef DEBUG

void PRINT_USER_CONFIG(struct ssdstate *ssd){
	struct ssdconf *sc = &(ssd->ssdparams);
	int user_num = ssd->user_num;
	struct USER_INFO *user_head = ssd->user;
	for (int i = 0; i < user_num; ++i) {
		printf("********************************\n");
		printf("User id: %d\n", i);
		printf("started channel = %d, ended channel = %d, channel num = %d\n", user_head->started_channel,
			user_head->ended_channel, user_head->channel_num);
		printf("free block = %d, free page = %d\n", user_head->free_block_num, user_head->free_page_num);
		printf("********************************\n");
		user_head ++;
	}
	return;
}

void PRINT_USER_PLANE_STAT(struct ssdstate *ssd, int user) {
	struct ssdconf *sc = &(ssd->ssdparams);
	int FLASH_PER_CHANNEL = sc->FLASH_NB / sc->CHANNEL_NB;
	int PLANE_PER_CHANNEL = FLASH_PER_CHANNEL * sc->PLANES_PER_FLASH;
	
	struct USER_INFO *user_head = &(ssd->user[user]);
	int MIN_PLANE_NUM = user_head->started_channel * PLANE_PER_CHANNEL;
	int MAX_PLANE_NUM = user_head->ended_channel * PLANE_PER_CHANNEL;

	empty_block_root *empty_block_lists = (empty_block_root *)ssd->empty_block_list, *tmp1 = NULL;
	victim_block_root *victim_block_lists = (victim_block_root *)ssd->victim_block_list, *tmp2 = NULL;

	for (int i = MIN_PLANE_NUM; i < MAX_PLANE_NUM; ++i) {
		tmp1 = empty_block_lists + i;
		tmp2 = victim_block_lists + i;
		printf("plane %d: empty = %d, victim = %d\n", i, tmp1->empty_block_nb, tmp2->victim_block_nb);
	}

	return;
}

void PRINT_USER_STAT(struct ssdstate *ssd) {
	struct ssdconf *sc = &(ssd->ssdparams);
	int USER_NB = ssd->user_num;
	printf("---------------------------------------------\n");
	for (int i = 0; i < USER_NB; ++i) {
		struct USER_INFO *user_head = ssd->user + i;
		printf("user[%d], empty = %d, victim = %d\n", i,  user_head->free_block_num, user_head->used_block_num);
	}
	printf("---------------------------------------------\n");
	return;
}

void PRINT_PLANE_STAT(struct ssdstate *ssd) {
	struct ssdconf *sc = &(ssd->ssdparams);
	int USER_NB = ssd->user_num;
	printf("---------------------------------------------\n");
	for (int i = 0; i < USER_NB; i++)
	{
		PRINT_USER_PLANE_STAT(ssd, i);
	}
	printf("---------------------------------------------\n");
	return;
}

#endif //DEBUG

void INIT_zipf_AND_fingerprint(struct ssdstate *ssd)
{
	int i;
	double a=0.2, sum = 0.0;

	ssd->Pzipf=(double *)calloc(UNIQUE_PAGE_NB + 1, sizeof(double));
	ssd->fingerprint=(int64_t*)calloc(UNIQUE_PAGE_NB+1, sizeof(int64_t));

	for(i=0;i<=UNIQUE_PAGE_NB;i++)
	{
		ssd->Pzipf[i]=0.0;
		ssd->fingerprint[i]=-1;
	}

	for(i=1;i<=UNIQUE_PAGE_NB;i++)
	{
		sum+=1/pow((double)i, a);
	}

	for(i=1;i<=UNIQUE_PAGE_NB;i++)
	{
		ssd->Pzipf[i]=ssd->Pzipf[i-1]+1/pow((double)i, a)/sum;
	}
}

int64_t FP_GENERATOR(struct ssdstate *ssd, int64_t lpn){
	int64_t fp = -1;

	double data = ((double)rand() + 1) / ((double)RAND_MAX + 2);
	int64_t low = 0, high = UNIQUE_PAGE_NB, mid;
	while (low < high) {
		mid = low + (high - low + 1) / 2;

		if (data <= ssd->Pzipf[mid]) {
			if (data > ssd->Pzipf[mid - 1]) {
				fp = mid;
				break;
			}
			high = mid - 1;
		}
		else {
			low = mid;
		}
	}

	if (fp == -1) myPanic(__FUNCTION__, "fp == -1?");

	return fp;
}