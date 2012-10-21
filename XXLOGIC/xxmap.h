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
#define MAP_MAX_WIDTH	25600
#define MAP_MAX_HEIGHT	19200
#define MAX_NPC_NUMBER	64		/*最多的NPC数量*/
#define MAX_MONSTER_TYPE	10		/*一幅地图上最大的怪物数量*/
#define MAX_MONSTER_NUMBER	100	/*每种类型的最大怪物数量*/

///////////////////////////////DATA STRUCTURE//////////////////////////////////////

/*
 * 地图上的NPC
 */
struct _xxnpc{
	char type;	/*NPC类型*/
	char name[MAX_ROLE_NAME];	/*npc 名字*/
	xxpoint position;	/*NPC位置*/
	xxsize shape;	/*NPC形状*/
};

typedef struct _xxnpc xxnpc;

/*
 * 地图上的怪物
 */

struct _xxmonster{
	char type;		/*怪物类型*/
	char name[MAX_ROLE_NAME];	/*怪物名字*/
	xxrole_move monseter_move;	/*怪物移动*/
};

/*
 * 地图
 */
struct _xxmap{
	unsigned char map_mask[MAP_MAX_HEIGHT][MAP_MAX_WIDTH];	/*地图掩码 注意屏幕<x , y> 中 x 对应在数组中为[y][x]*/



};


#endif /* XXMAP_H_ */
