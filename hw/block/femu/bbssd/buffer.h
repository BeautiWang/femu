#ifndef __FEMU_FTL_BUFFER_H
#define __FEMU_FTL_BUFFER_H

/*  AVLTree  */

#define AVL_NULL		(TREE_NODE *)0

#define EH_FACTOR	0
#define LH_FACTOR	1
#define RH_FACTOR	-1
#define LEFT_MINUS	0
#define RIGHT_MINUS	1


#define ORDER_LIST_WANTED

#define INSERT_PREV	0
#define INSERT_NEXT	1

enum {
    BUFFER_READ =  0,
    BUFFER_WRITE = 1,

    BUFFER_READ_LATENCY = 1000,
    BUFFER_PROG_LATENCY = 1000,
};

typedef struct _AVL_TREE_NODE
{
#ifdef ORDER_LIST_WANTED
	struct _AVL_TREE_NODE *prev;
	struct _AVL_TREE_NODE *next;
#endif
	struct _AVL_TREE_NODE *tree_root;
	struct _AVL_TREE_NODE *left_child;
	struct _AVL_TREE_NODE *right_child;
	int  bf;    /*Balance Factor. abs(bf)>=2 means imbalanced.*/
}TREE_NODE;

typedef struct buffer_info
{
	unsigned long read_hit;
	unsigned long read_miss_hit;  
	unsigned long write_hit;   
	unsigned long write_miss_hit;

	struct buffer_group *buffer_head;            /*as LRU head which is most recently used*/
	struct buffer_group *buffer_tail;            /*as LRU tail which is least recently used*/
	TREE_NODE	*pTreeHeader;     				 /*for search target lsn is LRU table*/

	unsigned int max_buffer_sector;
	unsigned int buffer_sector_count;

#ifdef ORDER_LIST_WANTED
	TREE_NODE	*pListHeader;
	TREE_NODE	*pListTail;
#endif
	unsigned int	count;		                 /*Total number of AVL nodes*/
	int 			(*keyCompare)(TREE_NODE * , TREE_NODE *);
	int				(*free)(TREE_NODE *);
} tAVLTree;

typedef struct buffer_group{
	TREE_NODE node;                     //The structure of the tree node must be placed at the top of the user-defined structure
	struct buffer_group *LRU_link_next;	// next node in LRU list
	struct buffer_group *LRU_link_pre;	// previous node in LRU list

	unsigned int group;                 //the first data logic sector number of a group stored in buffer 
	unsigned int stored;                //indicate the sector is stored in buffer or not. 1 indicates the sector is stored and 0 indicate the sector isn't stored.EX.  00110011 indicates the first, second, fifth, sixth sector is stored in buffer.
	unsigned int dirty_clean;           //it is flag of the data has been modified, one bit indicates one subpage. EX. 0001 indicates the first subpage is dirty
	int flag;			                //indicates if this node is the last 20% of the LRU list	
	unsigned int page_type;				//buff page type:0--full_page  1--partial_page
} buf_node;

int avlTreeHigh(TREE_NODE *);
int avlTreeCheck(tAVLTree *,TREE_NODE *);
void R_Rotate(TREE_NODE **);
void L_Rotate(TREE_NODE **);
void LeftBalance(TREE_NODE **);
void RightBalance(TREE_NODE **);
int avlDelBalance(tAVLTree *,TREE_NODE *,int);
void AVL_TREE_LOCK(tAVLTree *,int);
void AVL_TREE_UNLOCK(tAVLTree *);
void AVL_TREENODE_FREE(tAVLTree *,TREE_NODE *);

#ifdef ORDER_LIST_WANTED
int orderListInsert(tAVLTree *,TREE_NODE *,TREE_NODE *,int);
int orderListRemove(tAVLTree *,TREE_NODE *);
TREE_NODE *avlTreeFirst(tAVLTree *);
TREE_NODE *avlTreeLast(tAVLTree *);
TREE_NODE *avlTreeNext(TREE_NODE *pNode);
TREE_NODE *avlTreePrev(TREE_NODE *pNode);
#endif

int avlTreeInsert(tAVLTree *,TREE_NODE **,TREE_NODE *,int *);
int avlTreeRemove(tAVLTree *,TREE_NODE *);
TREE_NODE *avlTreeLookup(tAVLTree *,TREE_NODE *,TREE_NODE *);
tAVLTree *avlTreeCreate(int *,int *);
int avlTreeDestroy(tAVLTree *);
int avlTreeFlush(tAVLTree *);
int avlTreeAdd(tAVLTree *,TREE_NODE *);
int avlTreeDel(tAVLTree *,TREE_NODE *);
TREE_NODE *avlTreeFind(tAVLTree *,TREE_NODE *);
unsigned int avlTreeCount(tAVLTree *);

int keyCompareFunc(TREE_NODE *p , TREE_NODE *p1);
int freeFunc(TREE_NODE *pNode);

#endif //__FEMU_FTL_BUFFER_H