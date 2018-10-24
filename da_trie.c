#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "da_trie.h"

da_trie_t *da_trie_create(void)
{
	da_trie_t *da_trie = NULL;
	
	da_trie = (da_trie_t *)malloc(sizeof(da_trie_t));
	check_pointer_error(da_trie, "da_trie malloc error", 1, NULL);

	memset(da_trie, 0, sizeof(da_trie_t));
	da_trie->bc_max = DATRIE_BC_INC;
	da_trie->matched_max = DATRIE_MATCHED_INC;
	da_trie->tail_max = DATRIE_TAIL_INC;
	da_trie->is_empty = 1;
	da_trie->tail_pos = 1;
	
	da_trie->base_array = (int32_t *)malloc(sizeof(int32_t) * da_trie->bc_max);
	check_pointer_error(da_trie->base_array, "base_array malloc error", 1, NULL);
	memset(da_trie->base_array, 0, sizeof(int32_t) * da_trie->bc_max);
	
	da_trie->check_array = (int32_t *)malloc(sizeof(int32_t) * da_trie->bc_max);
	check_pointer_error(da_trie->check_array, "check_array malloc error", 1, NULL);
	memset(da_trie->check_array, 0, sizeof(int32_t) * da_trie->bc_max);
	
	da_trie->match_array = (key_info_t *)malloc(sizeof(key_info_t) * da_trie->matched_max);
	check_pointer_error(da_trie->match_array, "match_array malloc error", 1, NULL);
	memset(da_trie->match_array, 0, sizeof(key_info_t) * da_trie->matched_max);

	da_trie->tail_array = (unsigned char *)malloc(sizeof(unsigned char) * da_trie->tail_max);
	check_pointer_error(da_trie->tail_array, "tail_array malloc error", 1, NULL);
	memset(da_trie->tail_array, 0, sizeof(unsigned char) * da_trie->tail_max);
	
	return da_trie;
}
static int datrie_bc_array_realloc(da_trie_t *entry)
{
	int32_t pre_max = 0;
	int32_t new_max = 0;

	check_pointer_error(entry, "stupid, datrie is NULL", 0, 0);
	
	pre_max = entry->bc_max;
	new_max = pre_max + DATRIE_BC_INC;

	entry->base_array = (int32_t *)realloc(entry->base_array, sizeof(int32_t) * new_max);
	check_pointer_error(entry->base_array, "base_array realloc error", 0, 0);
	memset(entry->base_array + pre_max, 0, sizeof(int32_t) * DATRIE_BC_INC);

	entry->check_array = (int32_t *)realloc(entry->check_array, sizeof(int32_t) * new_max);
	check_pointer_error(entry->check_array, "check_array realloc error", 0, 0);
	memset(entry->check_array + pre_max, 0, sizeof(int32_t) * DATRIE_BC_INC);

	entry->bc_max = new_max;

	pre_max = entry->matched_max;
	new_max = pre_max + DATRIE_MATCHED_INC;
	
	entry->match_array = (key_info_t *)realloc(entry->match_array, sizeof(key_info_t) * new_max);
	check_pointer_error(entry->match_array, "match_array realloc error", 0, 0);
	memset(entry->match_array + pre_max, 0, sizeof(key_info_t) * DATRIE_MATCHED_INC);

	entry->matched_max = new_max;
	return 0;
}
static int datrie_set_list(da_trie_t *entry, int state, uint16_t *list, int32_t *list_len)
{
	uint16_t ch = 0;
	
	check_pointer_error(entry, "stupid! datrie is NULL", 0, 0);
	*list_len = 0;

	for (ch = DATRIE_MIN_CODE; ch <= DATRIE_MAX_CODE; ++ch)
	{
		if (entry->base_array[state] + ch < entry->bc_max 
			&& entry->check_array[entry->base_array[state] + ch] == state)
		{
			list[(*list_len)++] = ch;	
		}	
	}
	return 0;
}
static int datrie_xcheck(da_trie_t *entry, uint16_t *list, int32_t len)
{
	int32_t i = 0;
	int32_t base = 1;
	int32_t found = 0;
	static int tmp = 0;
	
	check_pointer_error(entry, "stupid, datrie is NULL", 1, -1);
	while (!found)
	{
		for (i = 0; i < len; ++i)
		{
			while (base + list[i] >= entry->bc_max)
			{
				datrie_bc_array_realloc(entry);				
			}
			if (entry->check_array[base + list[i]])
			{
				break;
			}
		}
		if (i >= len)
		{
			found = 1;
			continue;
		}
		++base;
	}
	
	tmp++;
	return base;
}
static int datrie_wbase(da_trie_t *entry, int32_t state, int32_t base)
{
	check_pointer_error(entry, "stupid! datrie is NULL", 0, 0);
	while (state >= entry->bc_max)
	{
		datrie_bc_array_realloc(entry);
	}
	entry->base_array[state] = base;
	return 0;
}
static int datrie_wcheck(da_trie_t *entry, int32_t next, int32_t state)
{
	check_pointer_error(entry, "stupid! datrie is NULL", 0, 0);
	while (next >= entry->bc_max || state >= entry->bc_max)
	{
		datrie_bc_array_realloc(entry);
	}
	entry->check_array[next] = state;
	return 0;
}
static int datrie_wmatch(da_trie_t *entry, int32_t state, void *info)
{
	check_pointer_error(entry, "stupid! datrie is NULL", 0, 0);
	while (state >= entry->matched_max)
	{
		datrie_bc_array_realloc(entry);
	}
	if (entry->match_array)
	entry->match_array[state].info = info;
	return 0;
}

static int datrie_bc_change(da_trie_t *entry, int32_t state, int32_t *pre, int32_t new_base, uint16_t *list, int32_t list_len)
{
	int32_t tmp_base = 0;
	int32_t tmp_node1 = 0;
	int32_t tmp_node2 = 0;
	uint16_t tmp_list[DATRIE_MAX_CODE + 1] = {0};
	int32_t tmp_list_len = 0;
	int j = 0;
	int k = 0;

	check_pointer_error(entry, "stupid! datrie is NULL", 0, 0);
	tmp_base = entry->base_array[state];
	datrie_wbase(entry, state, new_base);
	for (j = 0; j < list_len; ++j)
	{
		//旧孩子
		tmp_node1 = tmp_base + list[j];
		//新孩子
		tmp_node2 = entry->base_array[state] + list[j];
		/*
 		 * 下面这句代码我用了至少3天才找到的, 
 		 * 曾多次反复参看论文并在纸上画图构思DATrie树的结构。 
 		 * 如果没有 if(*pre == tmp_node1) *pre=tmp_node2; 这句代码,
 		 * 测试插入论文中的例举数据 "bachelor","jar","badge","baby"不会出问题: 
 		 * 但是插入如下字符串会出问题 "bac","bacd","be","bae"
 		 */
		if (pre && *pre == tmp_node1)
		{
			*pre = tmp_node2;
		}
		//移交旧孩子的base
		datrie_wbase(entry, tmp_node2, entry->base_array[tmp_node1]);
		//设置新孩子的check
		datrie_wcheck(entry, tmp_node2, entry->check_array[tmp_node1]);
		//设置新孩子的匹配信息
		datrie_wmatch(entry, tmp_node2, entry->match_array[tmp_node1].info);

		//移交旧孩子的孩子
		if (entry->base_array[tmp_node1] > 0)
		{
			//找到所有孙子，即旧孩子的孩子
			datrie_set_list(entry, tmp_node1, tmp_list, &tmp_list_len);
			for (k = 0; k < tmp_list_len; ++k)
			{
				//给孙子赋值新爸
				datrie_wcheck(entry, entry->base_array[tmp_node1] + tmp_list[k], tmp_node2);
			}	
			datrie_wbase(entry, tmp_node1, 0);
			datrie_wcheck(entry, tmp_node1, 0);
			datrie_wmatch(entry, tmp_node1, NULL);
		}
		else if (entry->base_array[tmp_node1] < 0)
		{
			datrie_wbase(entry, tmp_node1, 0);
			datrie_wcheck(entry, tmp_node1, 0);
			datrie_wmatch(entry, tmp_node1, NULL);
		}
		else
		{
			printf("impossible! There is an exception!\n");
			return -1;
		}
	}
	return 0;
}
static int datrie_copy_into_tail(da_trie_t *entry, unsigned char *pattern, int32_t len)
{
	uint16_t tail_len = len;
	
	check_pointer_error(entry, "stupid! datrie is NULL", 1, -1);
	while (entry->tail_pos + sizeof(uint16_t) + tail_len >= entry->tail_max)
	{
		entry->tail_max += DATRIE_TAIL_INC;
		entry->tail_array = (unsigned char *)realloc(entry->tail_array, entry->tail_max);
		memset(entry->tail_array + entry->tail_max - DATRIE_TAIL_INC, 0, DATRIE_TAIL_INC);
	}
	memcpy(entry->tail_array + entry->tail_pos, &tail_len, sizeof(uint16_t));
	memcpy(entry->tail_array + entry->tail_pos + sizeof(uint16_t), pattern, tail_len);
	entry->tail_pos += sizeof(uint16_t) + tail_len;		
	return 0;
}
#if 0
static int datrie_tail_compact(da_trie_t *entry, int32_t border, int32_t offset)
{
	int32_t tmp_pos = 0;
	int32_t i = 0;
	
	check_pointer_error(entry, "stupid! datrie is NULL", 0, 0);
	for (i = border; i < entry->tail_max; ++i)
	{
		entry->tail_array[i - offset] = entry->tail_array[i];
		entry->tail_array[i] = 0;
	}
	
	for (i = 0; i < entry->bc_max; ++i)
	{
		tmp_pos = entry->base_array[i] * -1;
		if (tmp_pos >= border)
		{
			datrie_wbase(entry, i, entry->base_array[i] + offset);
		}
	}
	entry->tail_pos -= offset;
	return 0;
}
#endif
static int datrie_tail_shift_forward(da_trie_t *entry, int32_t pos, int32_t len)
{
	uint16_t tail_len = 0;
	int32_t i = 0;
	int32_t border = 0;

	check_pointer_error(entry, "stupid! datrie is NULL", 0, 0);
	tail_len = *(uint16_t *)(entry->tail_array + pos);
	if (tail_len < len)
	{
		return -1;
	}
	//向前移动len的长度
	border = pos + sizeof(uint16_t) + tail_len;
	for (i = pos + sizeof(uint16_t) + len; i < border; ++i)
	{
		entry->tail_array[i - len] = entry->tail_array[i];
	}
	//写回新的长度值
	tail_len -= len;
	memcpy(entry->tail_array + pos, &tail_len, sizeof(uint16_t));

	//datrie_tail_compact(entry, border, len);
	return 0;
}
static int datrie_remove_tail(da_trie_t *entry, int32_t pos)
{
	uint16_t tail_len = 0;
	int32_t border = 0;

	check_pointer_error(entry, "stupid! datrie is NULL", 0, 0);
	//写回新的长度值
	memcpy(entry->tail_array + pos, &tail_len, sizeof(uint16_t));

	tail_len = *(uint16_t *)(entry->tail_array + pos);
	border = pos + sizeof(uint16_t) + tail_len;
		
	if (tail_len == 0)
	{
		tail_len = sizeof(uint16_t);
	}
	//datrie_tail_compact(entry, border, tail_len);

	return 0;
} 
int32_t da_trie_add_pattern(da_trie_t *entry, unsigned char *pattern, int32_t len, void *info)
{
	int32_t state = DATRIE_INIT_STATE;	 
	uint16_t list1[DATRIE_MAX_CODE + 1] = {0};
	uint16_t list2[DATRIE_MAX_CODE + 1] = {0};
	int32_t list_len1 = 0; 
	int32_t list_len2 = 0;
	uint16_t ch = 0;
	int32_t s1 = 0;
	int32_t s2 = 0;
	int32_t i = 0;
	int32_t j = 0;
	int32_t k = 0;
	int32_t n = 0;
	int32_t new_base = 0;
	int32_t tmp_pos = 0;
	int32_t tmp_base = 0;
	uint16_t max_prefix_len = 0;
	uint16_t two_child[2] = {0};
	uint16_t *common_prefix_list = NULL;
	unsigned char *tmp_ptr = NULL;
	void *tmp_info = NULL;	
	int ret = 0; 

    if (pattern == NULL || len < 1)
    {
        return -1;
    }
	check_pointer_error(entry, "stupid, datrie is NULL", 1, -1);
	if (entry->is_empty)
	{
		ch = DATRIE_ENCODE(*pattern);

		tmp_base = datrie_xcheck(entry, &ch, 1);
		datrie_wbase(entry, DATRIE_INIT_STATE, tmp_base);	
		n = entry->base_array[DATRIE_INIT_STATE] + ch;
		datrie_wcheck(entry, n, DATRIE_INIT_STATE);
		datrie_wbase(entry, n, -1 * entry->tail_pos);
		datrie_wmatch(entry, n, info);

		ret = datrie_copy_into_tail(entry, ++pattern, --len);
		check_ret_error(ret, 0, "copy suffix into tail failed!", 1, -1);
		entry->is_empty = 0;	
		entry->pattern_num++;
		return 0;
	}	
	for (i = 0; i < len; ++i)
	{
		ch = DATRIE_ENCODE(pattern[i]);
		n = entry->base_array[state] + ch;
		while (n >= entry->bc_max)
		{
			datrie_bc_array_realloc(entry);
		}
		//case1：此位置无冲突，添进后缀
		if (entry->check_array[n] == 0)
		{
			datrie_wcheck(entry, n, state);
			datrie_wbase(entry, n, -1 * entry->tail_pos);
			datrie_wmatch(entry, n, info);

			ret = datrie_copy_into_tail(entry, pattern + i + 1, len - i - 1);
			check_ret_error(ret, 0, "copy suffix into tail failed!", 1, -1);
			entry->pattern_num++;
			return 0;
		}
		else if (entry->check_array[n] == state)
		{
			state = n;
			if (entry->base_array[state] < 0)
			{
				tmp_pos = -1 * entry->base_array[state];
				tmp_ptr = entry->tail_array + tmp_pos;
				max_prefix_len = *(uint16_t *)tmp_ptr;
				tmp_ptr += sizeof(uint16_t);
				//保存模式串info指针，移除该状态的匹配信息
				tmp_info = entry->match_array[state].info;

				common_prefix_list = (uint16_t *)malloc(sizeof(uint16_t) * (max_prefix_len + 1));
				memset(common_prefix_list, 0, sizeof(uint16_t) * (max_prefix_len + 1));
				check_pointer_error(common_prefix_list, "get common prefix failed", 1, -1);

				for (j = 0; j < max_prefix_len && j < len - i - 1; ++j)
				{
					//i所对应的字符已被接受，因此下一字符为i+1
					if (tmp_ptr[j] != pattern[i + 1 + j])
					{
						break;
					}
					ch = DATRIE_ENCODE(tmp_ptr[j]);
					common_prefix_list[j] = ch;
				}
				if (j == max_prefix_len && j == len - i - 1)
				{
					free(common_prefix_list);
					return -1;		
				}
				datrie_wmatch(entry, state, NULL);
				for (k = 0; k < j; ++k)
				{
					tmp_base = datrie_xcheck(entry, common_prefix_list + k, 1);
					datrie_wbase(entry, state, tmp_base);
					n = entry->base_array[state] + common_prefix_list[k];
					datrie_wcheck(entry, n, state);
					state = n;
				}
				if (j == max_prefix_len || j == len - i - 1)
				{
					if (max_prefix_len < len - i - 1)
					{
						//新位置赋旧值
						datrie_wmatch(entry, state, tmp_info);
						//全部移除在tail中的后缀，而不是shift_forwad
						datrie_remove_tail(entry, tmp_pos);
				
						ch = DATRIE_ENCODE(pattern[i + 1 + j]);
						tmp_base = datrie_xcheck(entry, &ch, 1);
						datrie_wbase(entry, state, tmp_base);
						n = entry->base_array[state] + ch;
						datrie_wcheck(entry, n, state);
						datrie_wbase(entry, n, -1 * entry->tail_pos);
						datrie_wmatch(entry, n, info);
						//添进新的后缀
						ret = datrie_copy_into_tail(entry, pattern + i + j + 2, len - i - j - 2);
						check_ret_error(ret, 0, "copy suffix into tail failed!", 1, -1);
					}
					else 
					{
						//新位置赋新值
						datrie_wmatch(entry, state, info);		
					
						ch = DATRIE_ENCODE(tmp_ptr[j]);
						tmp_base = datrie_xcheck(entry, &ch, 1);
						datrie_wbase(entry, state, tmp_base);
						n = entry->base_array[state] + ch;
						datrie_wcheck(entry, n, state);
						datrie_wbase(entry, n, -1 * tmp_pos);
						datrie_wmatch(entry, n, tmp_info);
						//向前滑动
						datrie_tail_shift_forward(entry, tmp_pos, j + 1);
					}
				}
				else
				{	
					two_child[0] = DATRIE_ENCODE(tmp_ptr[j]);
					two_child[1] = DATRIE_ENCODE(pattern[i + 1 + j]);
					tmp_base = datrie_xcheck(entry, two_child, 2);
					datrie_wbase(entry, state, tmp_base);

					n = entry->base_array[state] + two_child[0];
					datrie_wcheck(entry, n, state);
					datrie_wbase(entry, n, -1 * tmp_pos);
					datrie_wmatch(entry, n, tmp_info);
					//向前滑动
					datrie_tail_shift_forward(entry, tmp_pos, j + 1);

					n = entry->base_array[state] + two_child[1];
					datrie_wcheck(entry, n, state);
					datrie_wbase(entry, n, -1 * entry->tail_pos);
					datrie_wmatch(entry, n, info);
					//添进新的后缀
					ret = datrie_copy_into_tail(entry, pattern + i + j + 2, len - i - j - 2);
					check_ret_error(ret, 0, "copy suffix into tail failed!", 1, -1);
				}
				entry->pattern_num++;
				free(common_prefix_list);
				return 0;
			}
			else if (entry->base_array[state] == 0)
			{
				printf("haha !\n");
			}
		}
		else
		{
			s1 = state;
			s2 = entry->check_array[n]; 
			datrie_set_list(entry, s1, list1, &list_len1);
			datrie_set_list(entry, s2, list2, &list_len2);
			if (list_len1 + 1 < list_len2)	
			{
				//移动s1
				list1[list_len1] = ch;
				new_base = datrie_xcheck(entry, list1, list_len1 + 1);	
				datrie_bc_change(entry, s1, NULL, new_base, list1, list_len1);		

				n = entry->base_array[s1] + ch;
			}
			else
			{
				//移动s2
				new_base = datrie_xcheck(entry, list2, list_len2);
				datrie_bc_change(entry, s2, &state, new_base, list2, list_len2);		
			}	
			datrie_wbase(entry, n, -1 * entry->tail_pos);
			datrie_wcheck(entry, n, state);
			datrie_wmatch(entry, n, info);
			ret = datrie_copy_into_tail(entry, pattern + i + 1, len - (i + 1));
			check_ret_error(ret, 0, "copy suffix into tail failed!", 1, -1);

			entry->pattern_num++;
			return 0;
		}
	}	
    if (entry->match_array[n].info == NULL)
    {
        entry->match_array[n].info = info;
    }
    else
    {
        return -1;
    }
	
	return 0;
}

int da_trie_walk(da_trie_t *entry, int32_t *state, unsigned char *text, int32_t len, void **info)
{
	int32_t next = 0;
	unsigned char *tail_ptr = NULL;
	int32_t tail_len = 0;

	check_pointer_error(entry, "stupid! datrie is NULL!", 1, -1);
	next = entry->base_array[*state] + DATRIE_ENCODE(*text);
	if (entry->check_array[next] != *state)
	{
		return DATRIE_WALK_FAILED;
	}
	if (entry->match_array[next].info)
	{
		if (entry->base_array[next] < 0)
		{
			tail_ptr = entry->tail_array + (entry->base_array[next] * -1);
			tail_len = *(uint16_t *)tail_ptr;
			tail_ptr += sizeof(uint16_t);
			++text;
			--len;
			if (len < tail_len || memcmp(text, tail_ptr, tail_len))		
			{
				return DATRIE_WALK_FAILED;
			}
			*info = entry->match_array[next].info;
			return DATRIE_MATCHED_END;
		}
		else
		{
			*info = entry->match_array[next].info;
			*state = next;
			return DATRIE_MATCHED_NORMAL;	
		}
	}
	else
	{
		*state = next;
		return DATRIE_WALK_NORMAL;
	}
	return -1;
}

int32_t da_trie_find_one(da_trie_t *entry, unsigned char *text, int32_t len, trie_match_session_t *session, void **info)
{
	int32_t state = 0;
	unsigned char *t = NULL;
	int32_t l = 0;
	int tail_len = 0;
	unsigned char *tail_ptr = NULL;
	uint16_t ch = 0;
	int32_t n = 0;

//	check_pointer_error(entry, "stupid! datrie is NULL", 0, 0);
	if (!session->valid)
	{
		session->valid = 1;
		session->state = DATRIE_INIT_STATE;
		session->text = text;	
		session->len = len;
	}
	state = session->state;
	t = session->text;
	l = session->len;
	while (l > 0)
	{
		ch = DATRIE_ENCODE(*t);
		n = entry->base_array[state] + ch;
		if (entry->check_array[n] == state)
		{
			--l;
			++t;
			state = n;
			if (entry->match_array[state].info)
			{
				if (entry->base_array[state] < 0)
				{
					tail_ptr = entry->tail_array + (-1) * entry->base_array[state];
					tail_len = *(uint16_t *)tail_ptr;
					tail_ptr += sizeof(uint16_t);	
					if (l < tail_len || memcmp(t, tail_ptr, tail_len))
					{
						break;
					}
					//一旦进入到tail数组来比较，不管是否比较成功，整个trie树的查找都将完成
					*info = entry->match_array[n].info;
					session->valid = 0;

					return 0;	
				}
				else
				{
					*info = entry->match_array[n].info;
					session->state = state;
					session->text = t;
					session->len = l;
					
					return 0;
				}
			}
		}
		else
		{
			break;
		}
	}	

	session->valid = 0;
	return -1;	
}
static void datrie_free(void **ptr)
{
	if (ptr && *ptr)
	{
		free(*ptr);
		*ptr = NULL;
	}
}
void da_trie_destroy(da_trie_t *entry)
{
	if (entry == NULL)	
	{
		return;
	}
	datrie_free((void **)&entry->base_array);
	datrie_free((void **)&entry->check_array);
	datrie_free((void **)&entry->match_array);
	datrie_free((void **)&entry->tail_array);	
}

size_t da_trie_mem(da_trie_t *entry)
{
	size_t mem = 0;
	check_pointer_error(entry, "stupid! datrie is NULL", 1, 0);

	mem += entry->bc_max * sizeof(int32_t) * 2;
	mem += entry->matched_max * sizeof(key_info_t);
	mem += entry->tail_max * sizeof(unsigned char);

	return mem;
}

