/*
 * role_basic.h
 * 角色的基本信息
 * 适用于玩家与怪物
 *
 *  Created on: 2012-10-21
 *      Author: leiming
 */

#ifndef ROLE_BASIC_H_
#define ROLE_BASIC_H_

//////////////////////////////MACRO DEFINE//////////////////////////////
#define MAX_ROLE_NAME	32		/*一般名字的最长字符*/
#define MAX_MAGIC_ATTACK_TYPE 4		/*魔法攻击类型数量*/
#define MAX_MAGIC_DEFND_TYPE	4	/*防御类型数量*/


//////////////////////////////DATA SRUCT//////////////////////////////////
/*
 * 地图上的点
 */
struct _xxpoint{
	int x;
	int y;
};
typedef struct _xxpoint xxpoint;

/*
 * 任意物体的尺寸
 */
struct _xxsize{
	short width;
	short height;
};
typedef struct _xxsize xxsize;


/*角色基本信息*/
struct _xxrole_basic{
	char type;	/*角色类型*/
	char name[MAX_ROLE_NAME];	/*角色名*/
	xxsize shape;	/*角色尺寸*/
	int life;	/*生命值*/
	short life_growth;	/*生命成长 每秒*/
};
typedef struct _xxrole_basic xxrole_basic;


/*角色移动信息*/
struct _xxrole_move{
	char direction;	/*移动方向*/
	xxpoint position;	/*当前位置*/
	short speed;		/*移动速度*/
};
typedef struct _xxrole_move xxrole_move;

/*角色攻击信息*/
struct _xxrole_attack{
	char magic_attack_type_count;	/*魔法攻击类型的数量*/
	char magic_attack_cd[MAX_MAGIC_ATTACK_TYPE];	/*魔法攻击CD 秒*/
	int magic_min_damage[MAX_MAGIC_ATTACK_TYPE];	/*最小~最大各种魔法类型伤害值*/
	int magic_max_damage[MAX_MAGIC_ATTACK_TYPE];

	int physic_attack_gap;	/*物理攻击间隔 单位毫秒*/
	short crit_chance;		/*暴击机率*/
	short crit_damage;			/*暴击伤害 百分比*/
	int physic_min_damage;	/*最小~最大物理攻击伤害值*/
	int physic_max_damage;

};
typedef struct _xxrole_attack xxrole_attack;

/*角色防御信息*/
struct _xxrole_defend{
	int physic_defend;	/*物理防御*/
	int magic_defend[MAX_MAGIC_DEFND_TYPE];	/*各类魔法防御值*/
};
typedef struct _xxrole_defend xxrole_defend;


#endif /* ROLE_BASIC_H_ */
