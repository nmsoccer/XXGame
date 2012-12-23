/*
 * xxmap.h
 *
 *  Created on: 2012-10-21
 *      Author: leiming
 */

#ifndef XXMAP_H_
#define XXMAP_H_

#include "mytypes.h"
#include "role_basic.h"


///////////////////////////////MACRO DEFINE/////////////////////////////////////////
//#define MAP_MAX_WIDTH	25600
//#define MAP_MAX_HEIGHT	19200
#define MAP_MAX_WIDTH 13660
#define MAP_MAX_HEIGHT 7680
#define MAX_NPC_NUMBER	64		/*最多的NPC数量*/
#define MAX_MONSTER_TYPE	10		/*一幅地图上最大的怪物数量*/
#define MAX_MONSTER_NUMBER	100	/*每种类型的最大怪物数量*/

///////////////////////////////DATA STRUCTURE//////////////////////////////////////

/*
 * 地图上的NPC
 */
struct _xxnpc{
	xxrole_basic	npc_baic;	/*NPC基本信息*/
	xxpoint npc_position;	/*NPC位置*/
};

typedef struct _xxnpc xxnpc;

/*
 * 地图上的怪物
 */
struct _xxmonster{
	xxrole_basic monster_basic;
	xxrole_move monseter_move;	/*怪物移动*/
	xxrole_attack monster_attack;
	xxrole_defend monster_defend;	/*怪物防御*/
};
typedef struct _xxmonster xxmonster;

/*
 * 地图
 */
struct _xxmap{
	int width;	/*地图长宽*/
	int height;
	u16 real_npc_nr;	/*实际的NPC数量*/
	xxnpc npcs[MAX_NPC_NUMBER];	/*地图所能容纳的最多NPC数量*/
	u16 real_monster_nr;	/*实际的怪物数量*/
	xxmonster monsters[MAX_MONSTER_NUMBER];	/*地图所能容纳的最多怪物数量*/
};

typedef struct _xxmap xxmap;

/*
 * 地图INDEX(一个地图INDEX定义了该层地图的特性，MAP+MAPINDEX就是一个完整地图；有多少张地图就有多少INDEX)
 */
struct _xxmap_index{
	u8 map_mask[MAP_MAX_HEIGHT][MAP_MAX_WIDTH];	/*地图掩码 注意屏幕<x , y> 中 x 对应在数组中为[y][x]*/
};

#endif /* XXMAP_H_ */
