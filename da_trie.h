#ifndef _DA_TRIE_H_
#define _DA_TRIE_H_

#include <stdint.h>

#define DATRIE_TAIL_INC		128	
#define DATRIE_BC_INC		256	
#define DATRIE_MATCHED_INC	256

#define DATRIE_INIT_STATE	1

#define DATRIE_ENCODE(t) 	(t + 1)

#define DATRIE_MIN_CODE		1
#define DATRIE_MAX_CODE 	256

#define DATRIE_WALK_FAILED	1	
#define DATRIE_WALK_NORMAL	2
#define DATRIE_MATCHED_NORMAL	3
#define DATRIE_MATCHED_END	4

#define check_pointer_error(ptr, msg, flag, ret)	\
	do { if (ptr == NULL) {printf("%s\n", msg); if (flag == 1) {return ret;}}} while (0)

#define check_ret_error(val, std, msg, flag, ret)	\
	do { if (val != std) {printf("%s\n", msg); if (flag == 1) {return ret;}}} while (0)
typedef struct _key_info
{
	void *info;
	struct _key_info *next;
}key_info_t;

typedef struct _trie_match_session
{
	int32_t valid;
	unsigned char *text;
	int32_t len; //文本串的剩余字符
	int32_t state;
}trie_match_session_t;

typedef struct _da_trie
{
	int32_t is_empty;
	int32_t bc_max;
	int32_t matched_max;
	int32_t tail_max;
	//int32_t state_num;
	int32_t pattern_num;
	int32_t tail_pos;
	int32_t *base_array;
	int32_t *check_array;
	key_info_t **match_array;
	unsigned char *tail_array; 
}da_trie_t;
/**
 * 函数功能: 创建da_trie_t 结构指针并分配空间
 * 函数参数:无
 * 返回值:da_trie_t结构指针
 */
da_trie_t *da_trie_create(void);

/**
 * 函数功能: 向da_trie结构中添加模式串
 * 函数参数: 
 *	entry, da_trie_t结构中指针
 *	pattern, 模式串指针
 *	len, 模式串长度
 *	info, 模式串的额外信息
 * 返回值:
 *	0, 添加成功
 *	-1, 添加失败
 */
int32_t da_trie_add_pattern(da_trie_t *entry, unsigned char *pattern, int32_t len, void *info);

/**
 * 函数功能: 在当前状态下向前走一步到下一个状态
 * 函数参数: 
 *	entry, da_trie_t结构指针
 *	state, 当前状态
 *	text, 待匹配文本串
 *	len, 待匹配文本串长度
 *	info, 用于存储匹配到的一些信息
 * 返回值:
 *	函数会返回四种状态,分别为
 *	DATRIE_WALK_FAILED, 失败
 *	DATRIE_WALK_NORMAL, 正常向前走一步
 *	DATRIE_MATCHED_NORMAL, 正常向前走一步并发生匹配
 *	DATRIE_MATCHED_END, 走到最后一个状态并发生匹配
 */
int da_trie_walk(da_trie_t *entry, int32_t *state, unsigned char *text, int32_t len, void **info);

/**
 * 函数功能: 在da_trie_t结构中检索字符串是否出现
 * 函数参数: 
 *	entry, da_trie_t结构指针
 *	text, 带检索文本串指针
 *	len, 带检索文本串长度
 *	session, 检索时的session信息
 *	info, 一些额外信息
 * 返回值:
 *	0, 检索成功
 *	-1, 检索失败
 */
int32_t da_trie_find_one(da_trie_t *entry, unsigned char *text, int32_t len, trie_match_session_t *session, void **info);

/**
 * 函数功能: 销毁da_trie_t结构占用的空间
 */
void da_trie_destroy(da_trie_t *entry);

/**
 * 函数功能: 计算da_trie_t结构占用的内存
 */
size_t da_trie_mem(da_trie_t *entry);

#endif
