/*
 * Contains functions for dealing with things like
 * potions and scrolls
 *
 * @(#)things.c	3.37 (Berkeley) 6/15/81
 */

#include "things.h"

#include "armor.h"
#include "daemon.h"
#include "daemons.h"
#include "io.h"
#include "list.h"
#include "main.h"
#include "misc.h"
#include "move.h"
#include "pack.h"
#include "rings.h"
#include "sticks.h"
#include "weapons.h"

#include <ctype.h>

/*
 * inv_name:
 *	return the name of something as it would appear in an
 *	inventory.
 */
char *inv_name(struct object *obj, bool drop)
{
    char *pb;

    switch(obj->o_type) {
    case SCROLL:
        if(obj->o_count == 1) {
            strcpy(prbuf, "A scroll ");
        }
        else {
            sprintf(prbuf, "%d scrolls ", obj->o_count);
        }
        
        pb = &prbuf[strlen(prbuf)];
        
        if(s_know[obj->o_which]) {
            sprintf(pb, "of %s", s_magic[obj->o_which].mi_name);
        }
        else if(s_guess[obj->o_which]) {
            sprintf(pb, "called %s", s_guess[obj->o_which]);
        }
        else {
		sprintf(pb, "titled '%s'", s_names[obj->o_which]);
        }
        break;
    case POTION:
        if(obj->o_count == 1) {
            strcpy(prbuf, "A potion ");
        }
        else {
            sprintf(prbuf, "%d potions ", obj->o_count);
        }
        
        pb = &prbuf[strlen(prbuf)];
        
        if(p_know[obj->o_which]) {
            sprintf(pb,
                    "of %s(%s)",
                    p_magic[obj->o_which].mi_name,
                    p_colors[obj->o_which]);
        }
        else if(p_guess[obj->o_which]) {
            sprintf(pb,
                    "called %s(%s)",
                    p_guess[obj->o_which],
		    p_colors[obj->o_which]);
        }
        else if(obj->o_count == 1) {
            sprintf(prbuf,
                    "A%s %s potion",
		    vowelstr(p_colors[obj->o_which]),
		    p_colors[obj->o_which]);
        }
        else {
            sprintf(prbuf,
                    "%d %s potions",
                    obj->o_count,
		    p_colors[obj->o_which]);
        }
        break;
    case FOOD:
        if(obj->o_which == 1) {
            if(obj->o_count == 1) {
                sprintf(prbuf, "A%s %s", vowelstr(fruit), fruit);
            }
            else {
                sprintf(prbuf, "%d %ss", obj->o_count, fruit);
            }
        }
        else {
            if(obj->o_count == 1) {
                strcpy(prbuf, "Some food");
            }
            else {
		    sprintf(prbuf, "%d rations of food", obj->o_count);
            }
        }
        break;
    case WEAPON:
        if(obj->o_count > 1) {
            sprintf(prbuf, "%d ", obj->o_count);
        }
        else {
            strcpy(prbuf, "A ");
        }
        
        pb = &prbuf[strlen(prbuf)];
        
        if(obj->o_flags & ISKNOW) {
            sprintf(pb,
                    "%s %s",
                    num(obj->o_hplus, obj->o_dplus),
                    w_names[obj->o_which]);
        }
        else {
            sprintf(pb, "%s", w_names[obj->o_which]);
        }
        
        if(obj->o_count > 1) {
            strcat(prbuf, "s");
        }
        break;
    case ARMOR:
        if(obj->o_flags & ISKNOW) {
            sprintf(prbuf,
                    "%s %s",
                    num(a_class[obj->o_which] - obj->o_ac, 0),
		    a_names[obj->o_which]);
        }
        else {
            sprintf(prbuf, "%s", a_names[obj->o_which]);
        }
        break;
    case AMULET:
        strcpy(prbuf, "The Amulet of Yendor");
        break;
    case STICK:
        sprintf(prbuf, "A %s ", ws_type[obj->o_which]);
        pb = &prbuf[strlen(prbuf)];
        
        if(ws_know[obj->o_which]) {
            sprintf(pb,
                    "of %s%s(%s)",
                    ws_magic[obj->o_which].mi_name,
		    charge_str(obj),
                    ws_made[obj->o_which]);
        }
        else if(ws_guess[obj->o_which]) {
            sprintf(pb,
                    "called %s(%s)",
                    ws_guess[obj->o_which],
		    ws_made[obj->o_which]);
        }
        else {
            sprintf(&prbuf[2],
                    "%s %s",
                    ws_made[obj->o_which],
		    ws_type[obj->o_which]);
        }
        break;
    case RING:
        if(r_know[obj->o_which]) {
            sprintf(prbuf,
                    "A%s ring of %s(%s)",
                    ring_num(obj),
		    r_magic[obj->o_which].mi_name,
                    r_stones[obj->o_which]);
        }
        else if (r_guess[obj->o_which]) {
            sprintf(prbuf,
                    "A ring called %s(%s)",
		    r_guess[obj->o_which],
                    r_stones[obj->o_which]);
        }
        else {
            sprintf(prbuf,
                    "A%s %s ring",
                    vowelstr(r_stones[obj->o_which]),
		    r_stones[obj->o_which]);
        }
        break;
    default:
        if(wizard) {
            msg("Picked up something funny", 0);
        }
        
        sprintf(prbuf, "Something bizarre %s", unctrl(obj->o_type));
    }
    
    if(obj == cur_armor) {
	strcat(prbuf, " (being worn)");
    }
    
    if(obj == cur_weapon) {
	strcat(prbuf, " (weapon in hand)");
    }
    
    if(obj == cur_ring[LEFT]) {
	strcat(prbuf, " (on left hand)");
    }
    else if(obj == cur_ring[RIGHT]) {
	strcat(prbuf, " (on right hand)");
    }
    
    if(drop && isupper(prbuf[0])) {
	prbuf[0] = tolower(prbuf[0]);
    }
    else if(!drop && islower(*prbuf)) {
	*prbuf = toupper(*prbuf);
    }
    
    if(!drop) {
	strcat(prbuf, ".");
    }
    
    return prbuf;
}

/*
 * money:
 *	Add to characters purse
 */
int money()
{
    struct room *rp;

    for(rp = rooms; rp < &rooms[MAXROOMS]; ++rp) {
        if((player.t_pos.x == rp->r_gold.x) && (player.t_pos.y == rp->r_gold.y)) {
	    if(notify) {
		if(!terse) {
		    addmsg("You found ", 0);
                }
                
                int args[] = { rp->r_goldval };
		msg("%d gold pieces.", args);
	    }
            
	    purse += rp->r_goldval;
	    rp->r_goldval = 0;
            move(rp->r_gold.y, rp->r_gold.x);
	    addch(FLOOR);
	    return 0;
	}
    }
    
    msg("That gold must have been counterfeit", 0);

    return 0;
}

/*
 * drop:
 *	put something down
 */
int drop()
{
    register char ch;
    register struct linked_list *obj, *nobj;
    register struct object *op;

    ch = mvwinch(stdscr, player.t_pos.y, player.t_pos.x);
    if (ch != FLOOR && ch != PASSAGE)
    {
	msg("There is something there already", 0);
	return 0;
    }
    if ((obj = get_item("drop", 0)) == NULL)
	return 0;
    op = (struct object *)obj->l_data;
    if (!dropcheck(op))
	return 0;
    /*
     * Take it out of the pack
     */
    if((op->o_count >= 2) && (op->o_type != WEAPON)) {
	nobj = new_item(sizeof *op);
	op->o_count--;
	op = (struct object *)nobj->l_data;
	*op = *((struct object *)obj->l_data);
	op->o_count = 1;
	obj = nobj;
        
	if(op->o_group != 0) {
            ++inpack;
        }
    }
    else {
	_detach(&player.t_pack, obj);
    }
    
    --inpack;
    /*
     * Link it into the level object list
     */
    _attach(&lvl_obj, obj);
    mvaddch(player.t_pos.y, player.t_pos.x, op->o_type);
    op->o_pos = player.t_pos;
    msg("Dropped %s", inv_name(op, TRUE));

    return 0;
}

/*
 * do special checks for dropping or unweilding|unwearing|unringing
 */
int dropcheck(struct object *op)
{
    str_t save_max;

    if (op == NULL)
	return TRUE;
    if (op != cur_armor && op != cur_weapon
	&& op != cur_ring[LEFT] && op != cur_ring[RIGHT])
	    return TRUE;
    if (op->o_flags & ISCURSED)
    {
	msg("You can't.  It appears to be cursed.", 0);
	return FALSE;
    }
    if (op == cur_weapon)
	cur_weapon = NULL;
    else if (op == cur_armor)
    {
	waste_time();
	cur_armor = NULL;
    }
    else if (op == cur_ring[LEFT] || op == cur_ring[RIGHT])
    {
	switch (op->o_which)
	{
	    case R_ADDSTR:
		save_max = max_stats.s_str;
		chg_str(-op->o_ac);
		max_stats.s_str = save_max;
		break;
	    case R_SEEINVIS:
		player.t_flags &= ~CANSEE;
		extinguish(unsee);
		light(&player.t_pos);
		mvwaddch(cw, player.t_pos.y, player.t_pos.x, PLAYER);
		break;
	}

        if(op == cur_ring[LEFT]) {
            cur_ring[LEFT] = NULL;
        }
        else {
            cur_ring[RIGHT] = NULL;
        }
    }
    return TRUE;
}

/*
 * return a new thing
 */
struct linked_list *new_thing()
{
    struct linked_list *item;
    struct object *cur;
    int j, k;

    item = new_item(sizeof *cur);
    cur = (struct object *)item->l_data;
    cur->o_hplus = cur->o_dplus = 0;
    cur->o_damage = cur->o_hurldmg = "0d0";
    cur->o_ac = 11;
    cur->o_count = 1;
    cur->o_group = 0;
    cur->o_flags = 0;

    // Decide what kind of object it will be,
    // if we haven't had food for a while, let it be food.
    int value = pick_one(things, NUMTHINGS);

    if(no_food > 3) {
        value = 2;
    }
    
    switch(value) {
    case 0:
        cur->o_type = POTION;
        cur->o_which = pick_one(p_magic, MAXPOTIONS);
        break;
    case 1:
        cur->o_type = SCROLL;
        cur->o_which = pick_one(s_magic, MAXSCROLLS);
    case 2:
        no_food = 0;
        cur->o_type = FOOD;
        
        if(rnd(100) > 10) {
            cur->o_which = 0;
        }
        else {
            cur->o_which = 1;
        }
        break;
    case 3:
        cur->o_type = WEAPON;
        cur->o_which = rnd(MAXWEAPONS);
        init_weapon(cur, cur->o_which);
        
        k = rnd(100);
        if(k < 10) {
            cur->o_flags |= ISCURSED;
            cur->o_hplus -= (rnd(3) + 1);
        }
        else if(k < 15) {
            cur->o_hplus += (rnd(3) + 1);
        }
        break;
    case 4:
        cur->o_type = ARMOR;
        
        for(j = 0, k = rnd(100); j < MAXARMORS; ++j) {
            if(k < a_chances[j]) {
                break;
            }
        }
        
        if(j == MAXARMORS) {
            if(wizard) {
                int args[] = { k };
                msg("Picked a bad armor %d", args);
            }
            
            j = 0;
        }
        
        cur->o_which = j;
        cur->o_ac = a_class[j];
        
        k = rnd(100);
        
        if(k < 20) {
            cur->o_flags |= ISCURSED;
            cur->o_ac += (rnd(3) + 1);
        }
        else if(k < 28){
            cur->o_ac -= (rnd(3) + 1);
        }
        break;
    case 5:
        cur->o_type = RING;
        cur->o_which = pick_one(r_magic, MAXRINGS);
        
        switch(cur->o_which) {
        case R_ADDSTR:
        case R_PROTECT:
        case R_ADDHIT:
        case R_ADDDAM:
            cur->o_ac = rnd(3);
            
            if(cur->o_ac == 0) {
                cur->o_ac = -1;
                cur->o_flags |= ISCURSED;
            }
            break;
        case R_AGGR:
        case R_TELEPORT:
            cur->o_flags |= ISCURSED;
        }
        break;
    case 6:
        cur->o_type = STICK;
        cur->o_which = pick_one(ws_magic, MAXSTICKS);
        fix_stick(cur);
        break;
    default:
        if(wizard) {
            msg("Picked a bad kind of object", 0);
        }
        
        wait_for(' ');
    }
    
    return item;
}

/*
 * pick an item out of a list of nitems possible magic items
 */
int pick_one(struct magic_item *magic, int nitems)
{
    register struct magic_item *end;
    register int i;
    register struct magic_item *start;

    start = magic;
    for (end = &magic[nitems], i = rnd(100); magic < end; magic++)
	if (i < magic->mi_prob)
	    break;
    if (magic == end)
    {
	if (wizard)
	{
            int args[] = { i, nitems };
            msg("bad pick_one: %d from %d items", args);
	    for(magic = start; magic < end; magic++) {
                msg("%s: ", magic->mi_name);
                int args[] = { magic->mi_prob };
                msg("%d%%", args);
            }
	}
	magic = start;
    }
    return magic - start;
}
