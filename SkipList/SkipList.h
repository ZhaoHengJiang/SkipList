#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string>
#include <iostream>


using namespace std;

#define ZSKIPLIST_MAXLEVEL 10
#define ZSKIPLIST_P 0.25

//��Ծ��ڵ���ÿ������Ϣ
typedef struct zskiplistLevel
{

	// ǰ��ָ��
	struct zskiplistNode *forward;

	// ���
	unsigned int span;

};

/*
 * ��Ծ��ڵ�
 */
typedef struct zskiplistNode 
{

	// ��Ա����
	int str;

	// ��ֵ
	double score;

	// ����ָ��
	struct zskiplistNode *backward;

	// ��
	zskiplistLevel level[];

} zskiplistNode;

/*
 * ��Ծ��
 */
typedef struct zskiplist 
{

	// ��ͷ�ڵ�ͱ�β�ڵ�
	struct zskiplistNode *header, *tail;

	// ���нڵ������
	unsigned long length;

	// �������Ľڵ�Ĳ���
	int level;

} zskiplist;

/*
 * ����һ������Ϊ level ����Ծ��ڵ㣬
 * �����ڵ�ĳ�Ա��������Ϊ str ����ֵ����Ϊ score ��
 *
 * ����ֵΪ�´�������Ծ��ڵ�
 *
 * T = O(1)
 */
zskiplistNode *zslCreateNode(int level, double score, int str) 
{

	// ����ռ�
	zskiplistNode *zn = (zskiplistNode *)malloc(sizeof(zskiplistNode) + level * sizeof(zskiplistLevel));

	// ��������
	zn->score = score;
	zn->str = str;

	return zn;
}

/*
*��ʼ����Ծ��
*/
zskiplist *zslCreate(void) 
{
	int j;
	zskiplist *zsl;

	// ����ռ�
	zsl = (zskiplist *)malloc(sizeof(zskiplist));

	//printf("create zsl\n");

	// ���ø߶Ⱥ���ʼ����
	zsl->level = 1;
	zsl->length = 0;

	// ��ʼ����ͷ�ڵ�
	// T = O(1)
	zsl->header = zslCreateNode(ZSKIPLIST_MAXLEVEL, 0, -1);

	//ͷ�ڵ�ÿһ�㶼Ҫ��ʼ��
	for (j = 0; j < ZSKIPLIST_MAXLEVEL; j++) {
		zsl->header->level[j].forward = NULL;
		zsl->header->level[j].span = 0;
	}
	zsl->header->backward = NULL;

	// ���ñ�β
	zsl->tail = NULL;

	return zsl;
}

/*
 * �ͷŸ�������Ծ��ڵ�
 *
 * T = O(1)
 */
void zslFreeNode(zskiplistNode *node) 
{
	free(node);
}

/*
 * �ͷŸ�����Ծ���Լ����е����нڵ�
 *
 * T = O(N)
 */
void zslFree(zskiplist *zsl) 
{

	zskiplistNode *node = zsl->header->level[0].forward, *next;

	// �ͷű�ͷ
	free(zsl->header);

	// �ͷű������нڵ�
	// T = O(N)
	while (node) 
	{

		next = node->level[0].forward;

		zslFreeNode(node);

		node = next;
	}

	// �ͷ���Ծ��ṹ
	free(zsl);
}

/*
*�������һ��ֵ���������
*/
int zslRandomLevel(void) 
{
	int level = 1;

	while ((rand() & 0xFFFF) < (ZSKIPLIST_P * 0xFFFF))
		level += 1;

	return (level < ZSKIPLIST_MAXLEVEL) ? level : ZSKIPLIST_MAXLEVEL;
}

/*
 * ����һ����ԱΪ str ����ֵΪ score ���½ڵ㣬
 * ��������½ڵ���뵽��Ծ�� zsl �С�
 *
 * �����ķ���ֵΪ�½ڵ㡣
 *
 * T_wrost = O(N^2), T_avg = O(N log N)
 */
zskiplistNode *zslInsert(zskiplist *zsl, double score, int str) 
{
	zskiplistNode *update[ZSKIPLIST_MAXLEVEL], *x; //update��¼��ѯ������½ڵ��·��
	unsigned int rank[ZSKIPLIST_MAXLEVEL];
	int i, level;


	// �ڸ�������ҽڵ�Ĳ���λ��
	// T_wrost = O(N^2), T_avg = O(N log N)
	x = zsl->header;
	for (i = zsl->level - 1; i >= 0; i--) {

		/* store rank that is crossed to reach the insert position */
		// ��� i ���� zsl->level-1 ��
		// ��ô i �����ʼ rank ֵΪ i+1 ��� rank ֵ
		// ������� rank ֵһ����ۻ�
		// ���� rank[0] ��ֵ��һ�����½ڵ��ǰ�ýڵ����λ
		// rank[0] ���ں����Ϊ���� span ֵ�� rank ֵ�Ļ���
		// rank-kth���½ڵ�������λ��rank-kth = rank[0]
		rank[i] = i == (zsl->level - 1) ? 0 : rank[i + 1];

		// ����ǰ��ָ�������Ծ��
		// T_wrost = O(N^2), T_avg = O(N log N)
		while (x->level[i].forward &&
			(x->level[i].forward->score < score ||
				// �ȶԷ�ֵ
			(x->level[i].forward->score == score &&
				// �ȶԳ�Ա�� T = O(N)
				x->level[i].forward->str < str)))
		{

			// ��¼��;��Խ�˶��ٸ��ڵ�
			rank[i] += x->level[i].span;

			// �ƶ�����һָ��
			x = x->level[i].forward;
		}
		// ��¼��Ҫ���½ڵ������ӵĽڵ�
		update[i] = x; 
	}

	/* we assume the key is not already inside, since we allow duplicated
	 * scores, and the re-insertion of score and redis object should never
	 * happen since the caller of zslInsert() should test in the hash table
	 * if the element is already inside or not.
	 *
	 * zslInsert() �ĵ����߻�ȷ��ͬ��ֵ��ͬ��Ա��Ԫ�ز�����֣�
	 * �������ﲻ��Ҫ��һ�����м�飬����ֱ�Ӵ�����Ԫ�ء�
	 */

	 // ��ȡһ�����ֵ��Ϊ�½ڵ�Ĳ���
	 // T = O(N)
	level = zslRandomLevel();

	// ����½ڵ�Ĳ����ȱ��������ڵ�Ĳ�����Ҫ��
	// ��ô��ʼ����ͷ�ڵ���δʹ�õĲ㣬�������Ǽ�¼�� update ������
	// ����Ҳָ���½ڵ�
	if (level > zsl->level) {

		// ��ʼ��δʹ�ò�
		// T = O(1)
		for (i = zsl->level; i < level; i++) {
			rank[i] = 0;
			update[i] = zsl->header;
			update[i]->level[i].span = zsl->length; //�µĲ�Ӧ��forward��ָ��β�ڵ㣬���spanΪlength
		}

		// ���±��нڵ�������
		zsl->level = level;
	}

	// �����½ڵ�
	x = zslCreateNode(level, score, str);

	// ��ǰ���¼��ָ��ָ���½ڵ㣬������Ӧ������
	// T = O(1)
	for (i = 0; i < level; i++) 
	{

		// �����½ڵ�� forward ָ��
		x->level[i].forward = update[i]->level[i].forward;

		// ����;��¼�ĸ����ڵ�� forward ָ��ָ���½ڵ�
		update[i]->level[i].forward = x;

		/* update span covered by update[i] as x is inserted here */
		// �����½ڵ��Խ�Ľڵ�����
		x->level[i].span = update[i]->level[i].span - (rank[0] - rank[i]);

		// �����½ڵ����֮����;�ڵ�� span ֵ
		// ���е� +1 ��������½ڵ�
		update[i]->level[i].span = (rank[0] - rank[i]) + 1;
	}

	/* increment span for untouched levels */
	// δ�Ӵ��Ľڵ�� span ֵҲ��Ҫ��һ����Щ�ڵ�ֱ�Ӵӱ�ͷָ���½ڵ�
	// T = O(1)
	for (i = level; i < zsl->level; i++) 
	{
		update[i]->level[i].span++;
	}

	// �����½ڵ�ĺ���ָ��
	x->backward = (update[0] == zsl->header) ? NULL : update[0];
	if (x->level[0].forward)
		x->level[0].forward->backward = x;
	else
		zsl->tail = x;

	// ��Ծ��Ľڵ������һ
	zsl->length++;

	return x;
}

/* Internal function used by zslDelete, zslDeleteByScore and zslDeleteByRank
 *
 * �ڲ�ɾ��������
 * �� zslDelete �� zslDeleteRangeByScore �� zslDeleteByRank �Ⱥ������á�
 *
 * T = O(1)
 */
void zslDeleteNode(zskiplist *zsl, zskiplistNode *x, zskiplistNode **update) 
{
	int i;

	// �������кͱ�ɾ���ڵ� x �йصĽڵ��ָ�룬�������֮��Ĺ�ϵ
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

	// ���±�ɾ���ڵ� x ��ǰ���ͺ���ָ��
	if (x->level[0].forward) 
	{
		x->level[0].forward->backward = x->backward;
	}
	else 
	{
		zsl->tail = x->backward;
	}

	// ������Ծ����������ֻ�ڱ�ɾ���ڵ�����Ծ������ߵĽڵ�ʱ��ִ�У�
	// T = O(1)
	while (zsl->level > 1 && zsl->header->level[zsl->level - 1].forward == NULL)
		zsl->level--;

	// ��Ծ��ڵ��������һ
	zsl->length--;
}

/* Delete an element with matching score/object from the skiplist.
 *
 * ����Ծ�� zsl ��ɾ�����������ڵ� score ���Ҵ���ָ������ obj �Ľڵ㡣
 *
 * T_wrost = O(N^2), T_avg = O(N log N)
 */
int zslDelete(zskiplist *zsl, double score, int str) {
	zskiplistNode *update[ZSKIPLIST_MAXLEVEL], *x;
	int i;

	// ������Ծ������Ŀ��ڵ㣬����¼������;�ڵ�
	// T_wrost = O(N^2), T_avg = O(N log N)
	x = zsl->header;
	for (i = zsl->level - 1; i >= 0; i--) {

		// ������Ծ��ĸ��Ӷ�Ϊ T_wrost = O(N), T_avg = O(log N)
		while (x->level[i].forward &&
			(x->level[i].forward->score < score ||
				// �ȶԷ�ֵ
			(x->level[i].forward->score == score &&
				// �ȶԶ���T = O(N)
				x->level[i].forward->str < str)))

			// ����ǰ��ָ���ƶ�
			x = x->level[i].forward;

		// ��¼��;�ڵ�
		update[i] = x;
	}

	/* We may have multiple elements with the same score, what we need
	 * is to find the element with both the right score and object.
	 *
	 * ����ҵ���Ԫ�� x ��ֻ�������ķ�ֵ�Ͷ�����ͬʱ���Ž���ɾ����
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
 * ������λ����Ծ���в���Ԫ�ء���λ����ʼֵΪ 1 ��
 *
 * �ɹ����ҷ�����Ӧ����Ծ��ڵ㣬û�ҵ��򷵻� NULL ��
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

		// ������Ծ���ۻ�Խ���Ľڵ�����
		while (x->level[i].forward && (traversed + x->level[i].span) <= rank)
		{
			traversed += x->level[i].span;
			x = x->level[i].forward;
		}

		// ���Խ���Ľڵ������Ѿ����� rank
		// ��ô˵���Ѿ�����Ҫ�ҵĽڵ�
		if (traversed == rank) {
			return x;
		}

	}

	// û�ҵ�Ŀ��ڵ�
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