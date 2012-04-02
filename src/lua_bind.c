/* File: lua_bind.c */

/* Purpose: various lua bindings */

/*
 * Copyright (c) 2001 DarkGod
 *
 * This software may be copied and distributed for educational, research, and
 * not for profit purposes provided that this copyright and statement are
 * included in all such copies.
 */

#include "angband.h"

#include "lua.h"
#include "tolua.h"
extern lua_State *L;

magic_power *grab_magic_power(magic_power *m_ptr, int num)
{
	return (&m_ptr[num]);
}

bool_ lua_spell_success(magic_power *spell, int stat, char *oups_fct)
{
	int chance;
	int minfail = 0;

	/* Spell failure chance */
	chance = spell->fail;

	/* Reduce failure rate by "effective" level adjustment */
	chance -= 3 * (p_ptr->lev - spell->min_lev);

	/* Reduce failure rate by INT/WIS adjustment */
	chance -= 3 * (adj_mag_stat[p_ptr->stat_ind[stat]] - 1);

	/* Not enough mana to cast */
	if (spell->mana_cost > p_ptr->csp)
	{
		chance += 5 * (spell->mana_cost - p_ptr->csp);
	}

	/* Extract the minimum failure rate */
	minfail = adj_mag_fail[p_ptr->stat_ind[stat]];

	/* Failure rate */
	chance = clamp_failure_chance(chance, minfail);

	/* Failed spell */
	if (rand_int(100) < chance)
	{
		if (flush_failure) flush();
		msg_format("You failed to concentrate hard enough!");
		sound(SOUND_FAIL);

		if (oups_fct != NULL)
			exec_lua(format("%s(%d)", oups_fct, chance));
		return (FALSE);
	}
	return (TRUE);
}

/*
 * Create objects
 */
object_type *new_object()
{
	object_type *o_ptr;
	MAKE(o_ptr, object_type);
	return (o_ptr);
}

void end_object(object_type *o_ptr)
{
	FREE(o_ptr, object_type);
}

/*
 * Powers
 */
s16b add_new_power(cptr name, cptr desc, cptr gain, cptr lose, byte level, byte cost, byte stat, byte diff)
{
	/* Increase the size */
	reinit_powers_type(power_max + 1);

	/* Copy the strings */
	C_MAKE(powers_type[power_max - 1].name, strlen(name) + 1, char);
	strcpy(powers_type[power_max - 1].name, name);
	C_MAKE(powers_type[power_max - 1].desc_text, strlen(desc) + 1, char);
	strcpy(powers_type[power_max - 1].desc_text, desc);
	C_MAKE(powers_type[power_max - 1].gain_text, strlen(gain) + 1, char);
	strcpy(powers_type[power_max - 1].gain_text, gain);
	C_MAKE(powers_type[power_max - 1].lose_text, strlen(lose) + 1, char);
	strcpy(powers_type[power_max - 1].lose_text, lose);

	/* Copy the other stuff */
	powers_type[power_max - 1].level = level;
	powers_type[power_max - 1].cost = cost;
	powers_type[power_max - 1].stat = stat;
	powers_type[power_max - 1].diff = diff;

	return (power_max - 1);
}

static char *lua_item_tester_fct;
static bool_ lua_item_tester(object_type* o_ptr)
{
	int oldtop = lua_gettop(L);
	bool_ ret;

	lua_getglobal(L, lua_item_tester_fct);
	tolua_pushusertype(L, o_ptr, tolua_tag(L, "object_type"));
	lua_call(L, 1, 1);
	ret = lua_tonumber(L, -1);
	lua_settop(L, oldtop);
	return (ret);
}

void lua_set_item_tester(int tval, char *fct)
{
	if (tval)
	{
		item_tester_tval = tval;
	}
	else
	{
		lua_item_tester_fct = fct;
		item_tester_hook = lua_item_tester;
	}
}

char *lua_object_desc(object_type *o_ptr, int pref, int mode)
{
	static char buf[150];

	object_desc(buf, o_ptr, pref, mode);
	return (buf);
}

/*
 * Monsters
 */

void find_position(int y, int x, int *yy, int *xx)
{
	int attempts = 500;

	do
	{
		scatter(yy, xx, y, x, 6);
	}
	while (!(in_bounds(*yy, *xx) && cave_floor_bold(*yy, *xx)) && --attempts);
}

static char *summon_lua_okay_fct;
bool_ summon_lua_okay(int r_idx)
{
	int oldtop = lua_gettop(L);
	bool_ ret;

	lua_getglobal(L, lua_item_tester_fct);
	tolua_pushnumber(L, r_idx);
	lua_call(L, 1, 1);
	ret = lua_tonumber(L, -1);
	lua_settop(L, oldtop);
	return (ret);
}

bool_ lua_summon_monster(int y, int x, int lev, bool_ friend_, char *fct)
{
	summon_lua_okay_fct = fct;

	if (!friend_)
		return summon_specific(y, x, lev, SUMMON_LUA);
	else
		return summon_specific_friendly(y, x, lev, SUMMON_LUA, TRUE);
}

/*
 * Quests
 */
s16b add_new_quest(char *name)
{
	int i;

	/* Increase the size */
	reinit_quests(max_q_idx + 1);
	quest[max_q_idx - 1].type = HOOK_TYPE_LUA;
	strncpy(quest[max_q_idx - 1].name, name, 39);

	for (i = 0; i < 10; i++)
		strncpy(quest[max_q_idx - 1].desc[i], "", 39);

	return (max_q_idx - 1);
}

void desc_quest(int q_idx, int d, char *desc)
{
	if (d >= 0 && d < 10)
		strncpy(quest[q_idx].desc[d], desc, 79);
}

/*
 * Misc
 */
bool_ get_com_lua(cptr prompt, int *com)
{
	char c;

	if (!get_com(prompt, &c)) return (FALSE);
	*com = c;
	return (TRUE);
}

/* Spell schools */
s16b new_school(int i, cptr name, s16b skill)
{
	schools[i].name = string_make(name);
	schools[i].skill = skill;
	return (i);
}

s16b new_spell(int i, cptr name)
{
	school_spells[i].name = string_make(name);
	school_spells[i].level = 0;
	school_spells[i].level = 0;
	return (i);
}

spell_type *grab_spell_type(s16b num)
{
	return (&school_spells[num]);
}

school_type *grab_school_type(s16b num)
{
	return (&schools[num]);
}

/* Change this fct if I want to switch to learnable spells */
s32b lua_get_level(s32b s, s32b lvl, s32b max, s32b min, s32b bonus)
{
	s32b tmp;

	tmp = lvl - ((school_spells[s].skill_level - 1) * (SKILL_STEP / 10));

	if (tmp >= (SKILL_STEP / 10)) /* We require at least one spell level */
		tmp += bonus;

	tmp = (tmp * (max * (SKILL_STEP / 10)) / (SKILL_MAX / 10));

	if (tmp < 0) /* Shift all negative values, so they map to appropriate integer */
		tmp -= SKILL_STEP / 10 - 1;

	/* Now, we can safely divide */
	lvl = tmp / (SKILL_STEP / 10);

	if (lvl < min)
		lvl = min;

	return lvl;
}

/** This is the function to use when casting through a stick */
s32b get_level_device(s32b s, s32b max, s32b min)
{
	int lvl;
	int get_level_use_stick = exec_lua("return get_level_use_stick");
	int get_level_max_stick = exec_lua("return get_level_max_stick");

	/* No max specified ? assume 50 */
	if (max <= 0) {
		max = 50;
	}
	/* No min specified ? */
	if (min <= 0) {
		min = 1;
	}

	lvl = s_info[SKILL_DEVICE].value;
	lvl = lvl + (get_level_use_stick * SKILL_STEP);

	/* Sticks are limited */
	if (lvl - ((school_spells[s].skill_level + 1) * SKILL_STEP) >= get_level_max_stick * SKILL_STEP)
	{
		lvl = (get_level_max_stick + school_spells[s].skill_level - 1) * SKILL_STEP;
	}

	/* / 10 because otherwise we can overflow a s32b and we can use a u32b because the value can be negative
	-- The loss of information should be negligible since 1 skill = 1000 internally
	*/
	lvl = lvl / 10;
	lvl = lua_get_level(s, lvl, max, min, 0);

	return lvl;
}

s32b lua_spell_chance(s32b chance, int level, int skill_level, int mana, int cur_mana, int stat)
{
	int minfail;
	/* Reduce failure rate by "effective" level adjustment */
	chance -= 3 * (level - 1);

	/* Reduce failure rate by INT/WIS adjustment */
	chance -= 3 * (adj_mag_stat[p_ptr->stat_ind[stat]] - 1);

	/* Not enough mana to cast */
	if (chance < 0) chance = 0;
	if (mana > cur_mana)
	{
		chance += 15 * (mana - cur_mana);
	}

	/* Extract the minimum failure rate */
	minfail = adj_mag_fail[p_ptr->stat_ind[stat]];

	/*
	        * Non mage characters never get too good
	 */
	if (!(has_ability(AB_PERFECT_CASTING)))
	{
		if (minfail < 5) minfail = 5;
	}

	/* Hack -- Priest prayer penalty for "edged" weapons  -DGK */
	if ((forbid_non_blessed()) && (p_ptr->icky_wield)) chance += 25;

	/* Return the chance */
	return clamp_failure_chance(chance, minfail);
}

s32b lua_spell_device_chance(s32b chance, int level, int base_level)
{
	int minfail;

	/* Reduce failure rate by "effective" level adjustment */
	chance -= (level - 1);

	/* Extract the minimum failure rate */
	minfail = 15 - get_skill_scale(SKILL_DEVICE, 25);

	/* Return the chance */
	return clamp_failure_chance(chance, minfail);
}

/* Cave */
cave_type *lua_get_cave(int y, int x)
{
	return (&(cave[y][x]));
}

void set_target(int y, int x)
{
	target_who = -1;
	target_col = x;
	target_row = y;
}

void get_target(int dir, int *y, int *x)
{
	int ty, tx;

	/* Use the given direction */
	tx = p_ptr->px + (ddx[dir] * 100);
	ty = p_ptr->py + (ddy[dir] * 100);

	/* Hack -- Use an actual "target" */
	if ((dir == 5) && target_okay())
	{
		tx = target_col;
		ty = target_row;
	}
	*y = ty;
	*x = tx;
}

/* Level gen */
void get_map_size(char *name, int *ysize, int *xsize)
{
	*xsize = 0;
	*ysize = 0;
	init_flags = INIT_GET_SIZE;
	process_dungeon_file(name, ysize, xsize, cur_hgt, cur_wid, TRUE, TRUE);
}

void load_map(char *name, int *y, int *x)
{
	/* Set the correct monster hook */
	set_mon_num_hook();

	/* Prepare allocation table */
	get_mon_num_prep();

	init_flags = INIT_CREATE_DUNGEON;
	process_dungeon_file(name, y, x, cur_hgt, cur_wid, TRUE, TRUE);
}

bool_ alloc_room(int by0, int bx0, int ysize, int xsize, int *y1, int *x1, int *y2, int *x2)
{
	int xval, yval, x, y;

	/* Try to allocate space for room.  If fails, exit */
	if (!room_alloc(xsize + 2, ysize + 2, FALSE, by0, bx0, &xval, &yval)) return FALSE;

	/* Get corner values */
	*y1 = yval - ysize / 2;
	*x1 = xval - xsize / 2;
	*y2 = yval + (ysize) / 2;
	*x2 = xval + (xsize) / 2;

	/* Place a full floor under the room */
	for (y = *y1 - 1; y <= *y2 + 1; y++)
	{
		for (x = *x1 - 1; x <= *x2 + 1; x++)
		{
			cave_type *c_ptr = &cave[y][x];
			cave_set_feat(y, x, floor_type[rand_int(100)]);
			c_ptr->info |= (CAVE_ROOM);
			c_ptr->info |= (CAVE_GLOW);
		}
	}
	return TRUE;
}


/* Files */
void lua_print_hook(cptr str)
{
	fprintf(hook_file, "%s", str);
}


/*
 * Finds a good random bounty monster
 * Im too lazy to write it in lua since the lua API for monsters is not very well yet
 */

/*
 * Hook for bounty monster selection.
 */
static bool_ lua_mon_hook_bounty(int r_idx)
{
	monster_race* r_ptr = &r_info[r_idx];


	/* Reject uniques */
	if (r_ptr->flags1 & RF1_UNIQUE) return (FALSE);

	/* Reject those who cannot leave anything */
	if (!(r_ptr->flags9 & RF9_DROP_CORPSE)) return (FALSE);

	/* Accept only monsters that can be generated */
	if (r_ptr->flags9 & RF9_SPECIAL_GENE) return (FALSE);
	if (r_ptr->flags9 & RF9_NEVER_GENE) return (FALSE);

	/* Reject pets */
	if (r_ptr->flags7 & RF7_PET) return (FALSE);

	/* Reject friendly creatures */
	if (r_ptr->flags7 & RF7_FRIENDLY) return (FALSE);

	/* Accept only monsters that are not breeders */
	if (r_ptr->flags4 & RF4_MULTIPLY) return (FALSE);

	/* Forbid joke monsters */
	if (r_ptr->flags8 & RF8_JOKEANGBAND) return (FALSE);

	/* Accept only monsters that are not good */
	if (r_ptr->flags3 & RF3_GOOD) return (FALSE);

	/* The rest are acceptable */
	return (TRUE);
}

int lua_get_new_bounty_monster(int lev)
{
	int r_idx;

	/*
	 * Set up the hooks -- no bounties on uniques or monsters
	 * with no corpses
	 */
	get_mon_num_hook = lua_mon_hook_bounty;
	get_mon_num_prep();

	/* Set up the quest monster. */
	r_idx = get_mon_num(lev);

	/* Undo the filters */
	get_mon_num_hook = NULL;
	get_mon_num_prep();

	return r_idx;
}

/*
 * Some misc functions
 */
char *lua_input_box(cptr title, int max)
{
	static char buf[80];
	int wid, hgt;

	strcpy(buf, "");
	Term_get_size(&wid, &hgt);
	if (!input_box(title, hgt / 2, wid / 2, buf, (max > 79) ? 79 : max))
		return "";
	return buf;
}

char lua_msg_box(cptr title)
{
	int wid, hgt;

	Term_get_size(&wid, &hgt);
	return msg_box(title, hgt / 2, wid / 2);
}

list_type *lua_create_list(int size)
{
	list_type *l;
	cptr *list;

	MAKE(l, list_type);
	C_MAKE(list, size, cptr);
	l->list = list;
	return l;
}

void lua_delete_list(list_type *l, int size)
{
	int i;

	for (i = 0; i < size; i++)
		string_free(l->list[i]);
	C_FREE(l->list, size, cptr);
	FREE(l, list_type);
}

void lua_add_to_list(list_type *l, int idx, cptr str)
{
	l->list[idx] = string_make(str);
}

void lua_display_list(int y, int x, int h, int w, cptr title, list_type* list, int max, int begin, int sel, byte sel_color)
{
	display_list(y, x, h, w, title, list->list, max, begin, sel, sel_color);
}

/*
 * Gods
 */
s16b add_new_gods(char *name)
{
	int i;

	/* Increase the size */
	reinit_gods(max_gods + 1);
	deity_info[max_gods - 1].name = string_make(name);

	for (i = 0; i < 10; i++)
		strncpy(deity_info[max_gods - 1].desc[i], "", 39);

	return (max_gods - 1);
}

void desc_god(int g_idx, int d, char *desc)
{
	if (d >= 0 && d < 10)
		strncpy(deity_info[g_idx].desc[d], desc, 79);
}

/*
 * Returns the direction of the compass that y2, x2 is from y, x
 * the return value will be one of the following: north, south,
 * east, west, north-east, south-east, south-west, north-west,
 * or "close" if it is within 2 tiles.
 */
cptr compass(int y, int x, int y2, int x2)
{
	static char compass_dir[64];

	// is it close to the north/south meridian?
	int y_diff = y2 - y;

	// determine if y2, x2 is to the north or south of y, x
	const char *y_axis;
	if ((y_diff > -3) && (y_diff < 3))
	{
		y_axis = 0;
	}
	else if (y2 > y)
	{
		y_axis = "south";
	}
	else
	{
		y_axis = "north";
	}

	// is it close to the east/west meridian?
	int x_diff = x2 - x;

	// determine if y2, x2 is to the east or west of y, x
	const char *x_axis;
	if ((x_diff > -3) && (x_diff < 3))
	{
		x_axis = 0;
	}
	else if (x2 > x)
	{
		x_axis = "east";
	}
	else
	{
		x_axis = "west";
	}

	// Maybe it is very close
	if ((!x_axis) && (!y_axis)) { strcpy(compass_dir, "close"); }
	// Maybe it is (almost) due N/S
	else if (!x_axis) { strcpy(compass_dir, y_axis); }
	// Maybe it is (almost) due E/W
	else if (!y_axis) { strcpy(compass_dir, x_axis); }
	//  or if it is neither
	else { sprintf(compass_dir, "%s-%s", y_axis, x_axis); }
	return compass_dir;
}

/* Returns a relative approximation of the 'distance' of y2, x2 from y, x. */
cptr approximate_distance(int y, int x, int y2, int x2)
{
	//  how far to away to the north/south?
	int y_diff = abs(y2 - y);
	// how far to away to the east/west?
	int x_diff = abs(x2 - x);
	// find which one is the larger distance
	int most_dist = x_diff;
	if (y_diff > most_dist) {
		most_dist = y_diff;
	}

	// how far away then?
	if (most_dist >= 41) {
		return "a very long way";
	} else if (most_dist >= 25) {
		return "a long way";
	} else if (most_dist >= 8) {
		return "quite some way";
	} else {
		return "not very far";
	}
}

bool_ drop_text_left(byte c, cptr str, int y, int o)
{
	int i = strlen(str);
	int x = 39 - (strlen(str) / 2) + o;
	while (i > 0)
	{
		int a = 0;
		int time = 0;

		if (str[i-1] != ' ')
		{
			while (a < x + i - 1)
			{
				Term_putch(a - 1, y, c, 32);
				Term_putch(a, y, c, str[i-1]);
				time = time + 1;
				if (time >= 4)
				{
					Term_xtra(TERM_XTRA_DELAY, 1);
					time = 0;
				}
				Term_redraw_section(a - 1, y, a, y);
				a = a + 1;

				inkey_scan = TRUE;
				if (inkey()) {
					return TRUE;
				}
			}
		}
		
		i = i - 1;
	}
	return FALSE;
}

bool_ drop_text_right(byte c, cptr str, int y, int o)
{
	int x = 39 - (strlen(str) / 2) + o;
	int i = 1;
	while (i <= strlen(str))
	{
		int a = 79;
		int time = 0;

		if (str[i-1] != ' ') {
			while (a >= x + i - 1)
			{
				Term_putch(a + 1, y, c, 32);
				Term_putch(a, y, c, str[i-1]);
				time = time + 1;
				if (time >= 4) {
					Term_xtra(TERM_XTRA_DELAY, 1);
					time = 0;
				}
				Term_redraw_section(a, y, a + 1, y);
				a = a - 1;

				inkey_scan = TRUE;
				if (inkey()) {
					return TRUE;
				}
			}
		}

		i = i + 1;
	}
	return FALSE;
}
