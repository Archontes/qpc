/*****************************************************************************
* Model: game.qm
* File:  ./missile.c
*
* This code has been generated by QM tool (see state-machine.com/qm).
* DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*****************************************************************************/
/*${.::missile.c} ..........................................................*/
#include "qpc.h"
#include "bsp.h"
#include "game.h"

/* Q_DEFINE_THIS_FILE */

/* local objects -----------------------------------------------------------*/

#if ((QP_VERSION < 580) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8)))
#error qpc version 5.8.0 or higher required
#endif

/*${AOs::Missile} ..........................................................*/
typedef struct {
/* protected: */
    QActive super;

/* private: */
    uint8_t x;
    uint8_t y;
    uint8_t exp_ctr;
} Missile;

/* protected: */
static QState Missile_initial(Missile * const me, QEvt const * const e);
static QState Missile_armed(Missile * const me, QEvt const * const e);
static QState Missile_flying(Missile * const me, QEvt const * const e);
static QState Missile_exploding(Missile * const me, QEvt const * const e);

static Missile l_missile; /* the sole instance of the Missile active object */

/* Public-scope objects ----------------------------------------------------*/
QMActive * const AO_Missile = &l_missile.super; /* opaque AO pointer */

/* Active object definition ------------------------------------------------*/
/*${AOs::Missile_ctor} .....................................................*/
void Missile_ctor(void) {
    Missile *me = &l_missile;
    QActive_ctor(&me->super, Q_STATE_CAST(&Missile_initial));
}
/*${AOs::Missile} ..........................................................*/
/*${AOs::Missile::SM} ......................................................*/
static QState Missile_initial(Missile * const me, QEvt const * const e) {
    /* ${AOs::Missile::SM::initial} */
    (void)e;
    QActive_subscribe((QActive *)me, TIME_TICK_SIG);

    QS_OBJ_DICTIONARY(&l_missile);  /* object dictionary for Missile object */

    QS_FUN_DICTIONARY(&Missile_initial);    /* dictionaries for Missile HSM */
    QS_FUN_DICTIONARY(&Missile_armed);
    QS_FUN_DICTIONARY(&Missile_flying);
    QS_FUN_DICTIONARY(&Missile_exploding);

    QS_SIG_DICTIONARY(MISSILE_FIRE_SIG,   &l_missile);     /* local signals */
    QS_SIG_DICTIONARY(HIT_WALL_SIG,       &l_missile);
    QS_SIG_DICTIONARY(DESTROYED_MINE_SIG, &l_missile);
    return Q_TRAN(&Missile_armed);
}
/*${AOs::Missile::SM::armed} ...............................................*/
static QState Missile_armed(Missile * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /* ${AOs::Missile::SM::armed::MISSILE_FIRE} */
        case MISSILE_FIRE_SIG: {
            me->x = Q_EVT_CAST(ObjectPosEvt)->x;
            me->y = Q_EVT_CAST(ObjectPosEvt)->y;
            status_ = Q_TRAN(&Missile_flying);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*${AOs::Missile::SM::flying} ..............................................*/
static QState Missile_flying(Missile * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /* ${AOs::Missile::SM::flying::TIME_TICK} */
        case TIME_TICK_SIG: {
            /* ${AOs::Missile::SM::flying::TIME_TICK::[me->x+GAME_MISSILE_SPEED_X<GAME~} */
            if (me->x + GAME_MISSILE_SPEED_X < GAME_TUNNEL_WIDTH) {
                ObjectImageEvt *oie;
                me->x += GAME_MISSILE_SPEED_X;
                /*tell the Tunnel to draw the Missile and test for wall hits*/
                oie = Q_NEW(ObjectImageEvt, MISSILE_IMG_SIG);
                oie->x   = me->x;
                oie->y   = me->y;
                oie->bmp = MISSILE_BMP;
                QACTIVE_POST(AO_Tunnel, (QEvt *)oie, me);
                status_ = Q_HANDLED();
            }
            /* ${AOs::Missile::SM::flying::TIME_TICK::[else]} */
            else {
                status_ = Q_TRAN(&Missile_armed);
            }
            break;
        }
        /* ${AOs::Missile::SM::flying::HIT_WALL} */
        case HIT_WALL_SIG: {
            status_ = Q_TRAN(&Missile_exploding);
            break;
        }
        /* ${AOs::Missile::SM::flying::DESTROYED_MINE} */
        case DESTROYED_MINE_SIG: {
            QACTIVE_POST(AO_Ship, e, me);
            status_ = Q_TRAN(&Missile_armed);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*${AOs::Missile::SM::exploding} ...........................................*/
static QState Missile_exploding(Missile * const me, QEvt const * const e) {
    QState status_;
    switch (e->sig) {
        /* ${AOs::Missile::SM::exploding} */
        case Q_ENTRY_SIG: {
            me->exp_ctr = 0U;
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Missile::SM::exploding::TIME_TICK} */
        case TIME_TICK_SIG: {
            /* ${AOs::Missile::SM::exploding::TIME_TICK::[(me->x>=GAME_SPEED_X)&&(me->exp~} */
            if ((me->x >= GAME_SPEED_X) && (me->exp_ctr < 15U)) {
                ObjectImageEvt *oie;

                ++me->exp_ctr;           /* advance the explosion counter */
                me->x -= GAME_SPEED_X;   /* move the explosion by one step */

                /* tell the Tunnel to render the current stage of Explosion */
                oie = Q_NEW(ObjectImageEvt, EXPLOSION_SIG);
                oie->x   = me->x + 3U;   /* x-pos of explosion */
                oie->y   = (int8_t)((int)me->y - 4U); /* y-pos */
                oie->bmp = EXPLOSION0_BMP + (me->exp_ctr >> 2);
                QACTIVE_POST(AO_Tunnel, (QEvt *)oie, me);
                status_ = Q_HANDLED();
            }
            /* ${AOs::Missile::SM::exploding::TIME_TICK::[else]} */
            else {
                status_ = Q_TRAN(&Missile_armed);
            }
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}

