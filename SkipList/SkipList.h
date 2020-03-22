#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string>
#include <iostream>


using namespace std;

#define ZSKIPLIST_MAXLEVEL 10
#define ZSKIPLIST_P 0.25

//跳跃表节点中每级的信息
typedef struct zskiplistLevel
{

	// 前进指针
	struct zskiplistNode *forward;

	// 跨度
	unsigned int span;

};

/*
 * 跳跃表节点
 */
typedef struct zskiplistNode 
{

	// 成员对象
	int str;

	// 分值
	double score;

	// 后退指针
	struct zskiplistNode *backward;

	// 层
	zskiplistLevel level[];

} zskiplistNode;

/*
 * 跳跃表
 */
typedef struct zskiplist 
{

	// 表头节点和表尾节点
	struct zskiplistNode *header, *tail;

	// 表中节点的数量
	unsigned long length;

	// 表中最大的节点的层数
	int level;

} zskiplist;

/*
 * 创建一个层数为 level 的跳跃表节点，
 * 并将节点的成员对象设置为 str ，分值设置为 score 。
 *
 * 返回值为新创建的跳跃表节点
 *
 * T = O(1)
 */
zskiplistNode *zslCreateNode(int level, double score, int str) 
{

	// 分配空间
	zskiplistNode *zn = (zskiplistNode *)malloc(sizeof(zskiplistNode) + level * sizeof(zskiplistLevel));

	// 设置属性
	zn->score = score;
	zn->str = str;

	return zn;
}

/*
*初始化跳跃表
*/
zskiplist *zslCreate(void) 
{
	int j;
	zskiplist *zsl;

	// 分配空间
	zsl = (zskiplist *)malloc(sizeof(zskiplist));

	//printf("create zsl\n");

	// 设置高度和起始层数
	zsl->level = 1;
	zsl->length = 0;

	// 初始化表头节点
	// T = O(1)
	zsl->header = zslCreateNode(ZSKIPLIST_MAXLEVEL, 0, -1);

	//头节点每一层都要初始化
	for (j = 0; j < ZSKIPLIST_MAXLEVEL; j++) {
		zsl->header->level[j].forward = NULL;
		zsl->header->level[j].span = 0;
	}
	zsl->header->backward = NULL;

	// 设置表尾
	zsl->tail = NULL;

	return zsl;
}

/*
 * 释放给定的跳跃表节点
 *
 * T = O(1)
 */
void zslFreeNode(zskiplistNode *node) 
{
	free(node);
}

/*
 * 释放给定跳跃表，以及表中的所有节点
 *
 * T = O(N)
 */
void zslFree(zskiplist *zsl) 
{

	zskiplistNode *node = zsl->header->level[0].forward, *next;

	// 释放表头
	free(zsl->header);

	// 释放表中所有节点
	// T = O(N)
	while (node) 
	{

		next = node->level[0].forward;

		zslFreeNode(node);

		node = next;
	}

	// 释放跳跃表结构
	free(zsl);
}

/*
*随机返回一个值，代表层数
*/
int zslRandomLevel(void) 
{
	int level = 1;

	while ((rand() & 0xFFFF) < (ZSKIPLIST_P * 0xFFFF))
		level += 1;

	return (level < ZSKIPLIST_MAXLEVEL) ? level : ZSKIPLIST_MAXLEVEL;
}

/*
 * 创建一个成员为 str ，分值为 score 的新节点，
 * 并将这个新节点插入到跳跃表 zsl 中。
 *
 * 函数的返回值为新节点。
 *
 * T_wrost = O(N^2), T_avg = O(N log N)
 */
zskiplistNode *zslInsert(zskiplist *zsl, double score, int str) 
{
	zskiplistNode *update[ZSKIPLIST_MAXLEVEL], *x; //update记录查询插入的新节点的路径
	unsigned int rank[ZSKIPLIST_MAXLEVEL];
	int i, level;


	// 在各个层查找节点的插入位置
	// T_wrost = O(N^2), T_avg = O(N log N)
	x = zsl->header;
	for (i = zsl->level - 1; i >= 0; i--) {

		/* store rank that is crossed to reach the insert position */
		// 如果 i 不是 zsl->level-1 层
		// 那么 i 层的起始 rank 值为 i+1 层的 rank 值
		// 各个层的 rank 值一层层累积
		// 最终 rank[0] 的值加一就是新节点的前置节点的排位
		// rank[0] 会在后面成为计算 span 值和 rank 值的基础
		// rank-kth是新节点插入的排位，rank-kth = rank[0]
		rank[i] = i == (zsl->level - 1) ? 0 : rank[i + 1];

		// 沿着前进指针遍历跳跃表
		// T_wrost = O(N^2), T_avg = O(N log N)
		while (x->level[i].forward &&
			(x->level[i].forward->score < score ||
				// 比对分值
			(x->level[i].forward->score == score &&
				// 比对成员， T = O(N)
				x->level[i].forward->str < str)))
		{

			// 记录沿途跨越了多少个节点
			rank[i] += x->level[i].span;

			// 移动至下一指针
			x = x->level[i].forward;
		}
		// 记录将要和新节点相连接的节点
		update[i] = x; 
	}

	/* we assume the key is not already inside, since we allow duplicated
	 * scores, and the re-insertion of score and redis object should never
	 * happen since the caller of zslInsert() should test in the hash table
	 * if the element is already inside or not.
	 *
	 * zslInsert() 的调用者会确保同分值且同成员的元素不会出现，
	 * 所以这里不需要进一步进行检查，可以直接创建新元素。
	 */

	 // 获取一个随机值作为新节点的层数
	 // T = O(N)
	level = zslRandomLevel();

	// 如果新节点的层数比表中其他节点的层数都要大
	// 那么初始化表头节点中未使用的层，并将它们记录到 update 数组中
	// 将来也指向新节点
	if (level > zsl->level) {

		// 初始化未使用层
		// T = O(1)
		for (i = zsl->level; i < level; i++) {
			rank[i] = 0;
			update[i] = zsl->header;
			update[i]->level[i].span = zsl->length; //新的层应的forward该指向尾节点，因此span为length
		}

		// 更新表中节点最大层数
		zsl->level = level;
	}

	// 创建新节点
	x = zslCreateNode(level, score, str);

	// 将前面记录的指针指向新节点，并做相应的设置
	// T = O(1)
	for (i = 0; i < level; i++) 
	{

		// 设置新节点的 forward 指针
		x->level[i].forward = update[i]->level[i].forward;

		// 将沿途记录的各个节点的 forward 指针指向新节点
		update[i]->level[i].forward = x;

		/* update span covered by update[i] as x is inserted here */
		// 计算新节点跨越的节点数量
		x->level[i].span = update[i]->level[i].span - (rank[0] - rank[i]);

		// 更新新节点插入之后，沿途节点的 span 值
		// 其中的 +1 计算的是新节点
		update[i]->level[i].span = (rank[0] - rank[i]) + 1;
	}

	/* increment span for untouched levels */
	// 未接触的节点的 span 值也需要增一，这些节点直接从表头指向新节点
	// T = O(1)
	for (i = level; i < zsl->level; i++) 
	{
		update[i]->level[i].span++;
	}

	// 设置新节点的后退指针
	x->backward = (update[0] == zsl->header) ? NULL : update[0];
	if (x->level[0].forward)
		x->level[0].forward->backward = x;
	else
		zsl->tail = x;

	// 跳跃表的节点计数增一
	zsl->length++;

	return x;
}

/* Internal function used by zslDelete, zslDeleteByScore and zslDeleteByRank
 *
 * 内部删除函数，
 * 被 zslDelete 、 zslDeleteRangeByScore 和 zslDeleteByRank 等函数调用。
 *
 * T = O(1)
 */
void zslDeleteNode(zskiplist *zsl, zskiplistNode *x, zskiplistNode **update) 
{
	int i;

	// 更新所有和被删除节点 x 有关的节点的指针，解除它们之间的关系
	// T = O(1)
	for (i = 0; i < zsl->level; i++) 
	{
		if (update[i]->level[i].forward == x) 
		{
			update[i]->level[i].span += x->level[i].span - 1;
			update[i]->level[i].forward = x->level[i].forward;
		}
		else 
		{
			update[i]->level[i].span -= 1;
		}
	}

	// 更新被删除节点 x 的前进和后退指针
	if (x->level[0].forward) 
	{
		x->level[0].forward->backward = x->backward;
	}
	else 
	{
		zsl->tail = x->backward;
	}

	// 更新跳跃表最大层数（只在被删除节点是跳跃表中最高的节点时才执行）
	// T = O(1)
	while (zsl->level > 1 && zsl->header->level[zsl->level - 1].forward == NULL)
		zsl->level--;

	// 跳跃表节点计数器减一
	zsl->length--;
}

/* Delete an element with matching score/object from the skiplist.
 *
 * 从跳跃表 zsl 中删除包含给定节点 score 并且带有指定对象 obj 的节点。
 *
 * T_wrost = O(N^2), T_avg = O(N log N)
 */
int zslDelete(zskiplist *zsl, double score, int str) {
	zskiplistNode *update[ZSKIPLIST_MAXLEVEL], *x;
	int i;

	// 遍历跳跃表，查找目标节点，并记录所有沿途节点
	// T_wrost = O(N^2), T_avg = O(N log N)
	x = zsl->header;
	for (i = zsl->level - 1; i >= 0; i--) {

		// 遍历跳跃表的复杂度为 T_wrost = O(N), T_avg = O(log N)
		while (x->level[i].forward &&
			(x->level[i].forward->score < score ||
				// 比对分值
			(x->level[i].forward->score == score &&
				// 比对对象，T = O(N)
				x->level[i].forward->str < str)))

			// 沿着前进指针移动
			x = x->level[i].forward;

		// 记录沿途节点
		update[i] = x;
	}

	/* We may have multiple elements with the same score, what we need
	 * is to find the element with both the right score and object.
	 *
	 * 检查找到的元素 x ，只有在它的分值和对象都相同时，才将它删除。
	 */
	x = x->level[0].forward;
	if (x && score == x->score && x->str < str) 
	{
		// T = O(1)
		zslDeleteNode(zsl, x, update);
		// T = O(1)
		zslFreeNode(x);
		return 1;
	}
	else 
	{
		return 0; /* not found */
	}

	return 0; /* not found */
}

/* Finds an element by its rank. The rank argument needs to be 1-based.
 *
 * 根据排位在跳跃表中查找元素。排位的起始值为 1 。
 *
 * 成功查找返回相应的跳跃表节点，没找到则返回 NULL 。
 *
 * T_wrost = O(N), T_avg = O(log N)
 */
zskiplistNode* zslGetElementByRank(zskiplist *zsl, unsigned long rank) 
{
	zskiplistNode *x;
	unsigned long traversed = 0;
	int i;

	// T_wrost = O(N), T_avg = O(log N)
	x = zsl->header;
	for (i = zsl->level - 1; i >= 0; i--) 
	{

		// 遍历跳跃表并累积越过的节点数量
		while (x->level[i].forward && (traversed + x->level[i].span) <= rank)
		{
			traversed += x->level[i].span;
			x = x->level[i].forward;
		}

		// 如果越过的节点数量已经等于 rank
		// 那么说明已经到达要找的节点
		if (traversed == rank) {
			return x;
		}

	}

	// 没找到目标节点
	return NULL;
}

zskiplistNode* zslGetElementByScore(zskiplist* zsl, double score)
{
	zskiplistNode* x;
	int i = zsl->level-1;

	x = zsl->header;
	for (; i >= 0; i--)
	{
		while (x && x->score < score)
		{
			if (x->level[i].forward)
				x = x->level[i].forward;
			else
				break;
		}

		if (x != NULL && x->score == score)
			return x;
	}

	return NULL;
}

void zslPrint(zskiplist* zsl)
{
	zskiplistNode* x;
	int i = zsl->level-1;

	x = zsl->header;

	for (; i >= 0; i--)
	{
		x = zsl->header;
		while (x)
		{
			int j = x->level[i].span;
			cout << x->str << " " << x->score;
			for (; j > 0; j--)
				printf("\t");
			x = x->level[i].forward;
		}
		printf("\n");
	}
}