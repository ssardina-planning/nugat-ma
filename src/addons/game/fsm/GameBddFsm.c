/**CFile***********************************************************************

  FileName    [ GameBddFsm.c ]

  PackageName [ game.fsm ]

  Synopsis    [ Defines the public interface for the class GameBddFsm. ]

  Description [ A GameBddFsm is a BDD representation of the game
                Finite State Machine (FSM). Essentially, a game FSM
                consists of two FSMs of the same kind, one for each
                player. See BddFsm.c for more details. ]

  SeeAlso     [ GameSexpFsm.h ]

  Author      [ Andrei Tchaltsev ]

  Copyright   [
  This file is part of the ``game.fsm'' package of NuGaT.
  Copyright (C) 2010 by FBK-irst.

  NuGaT is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  NuGaT is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA.

  For more information on NuGaT see <http://es.fbk.eu/tools/nugat>
  or email to <nugat-users@list.fbk.eu>.
  Please report bugs to <nugat-users@list.fbk.eu>.

  To contact the NuGaT development board, email to <nugat@list.fbk.eu>. ]

******************************************************************************/

#include "GameBddFsm.h"

#include "compile/compile.h"
#include "dd/dd.h"
#include "enc/enc.h"
#include "fsm/bdd/BddFsm.h"
#include "trans/bdd/BddTrans.h"
#include "utils/error.h"
#include "utils/ErrorMgr.h"
#include "utils/utils.h"

#include <stdio.h>

EXTERN int n_players;

static char rcsid[] UTIL_UNUSED = "$Id: GameBddFsm.c,v 1.1.2.4 2010-02-08 14:07:32 nusmv Exp $";

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/**Type************************************************************************

  Synopsis    [ Game BDD FSM, the type exported by this package ]

  Description [ A game BDD FSM consists of two usual FSMs of the same
                kind. ]

                See BddFsm_TAG for more info on BddFsm_ptr. ]

  SeeAlso     [ ]

******************************************************************************/
typedef struct GameBddFsm_TAG
{
  BddEnc_ptr enc;
  DDMgr_ptr dd;

  BddFsm_ptr *players;

  bdd_ptr *stateVarCubes;
  bdd_ptr *nextStateVarCubes;   /* created for efficiency only */
  bdd_ptr *stateFrozenVarCubes;

  BddStates *withSuccessorss;   /* states with successor for player 1 */
  BddStates *woSuccessorss;     /* states without successor for player 1 */
} GameBddFsm;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/
long gameBddFsm_notGoalTotalTime;
long gameBddFsm_andInvar2TotalTime;
long gameBddFsm_moveGoalAndInvar2TotalTime;
long gameBddFsm_trans2TotalTime;
long gameBddFsm_notTrans2TotalTime;
long gameBddFsm_moveInvar1TotalTime;
long gameBddFsm_andInvar1TotalTime;
long gameBddFsm_trans1TotalTime;
long gameBddFsm_andInvar1AndInvar2TotalTime;
long gameBddFsm_totalMoveTotalTime;

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/
static void game_bdd_fsm_init ARGS((GameBddFsm_ptr self,
                                    BddEnc_ptr enc,
                                    BddFsm_ptr *players,
                                    bdd_ptr *stateVarCubes,
                                    bdd_ptr *stateFrozenVarCubes));

static void game_bdd_fsm_copy ARGS((const GameBddFsm_ptr self,
                                    GameBddFsm_ptr copy));

static void game_bdd_fsm_deinit ARGS((GameBddFsm_ptr self));

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [ Constructor for GameBddFsm ]

  Description [ Takes two BddFsm, one for every player, and four BDDs
                representing cubes of players' variables. Self becomes
                the owner of these FSMs. The given BDDs are referenced
                by the constructor (the invoker remains their owner).

                Note: stateVarCube_... and stateFrozenVarCube_... are
                cubes of state variables excluding frozen variables,
                and both state and frozen variables, respectively. ]

  SideEffects [ ]

  SeeAlso     [ GameBddFsm_destroy ]

******************************************************************************/
GameBddFsm_ptr GameBddFsm_create(BddEnc_ptr enc,
                                 BddFsm_ptr *players,
                                 bdd_ptr *stateVarCubes,
                                 bdd_ptr *stateFrozenVarCubes)
{
  GameBddFsm_ptr self;
    boolean expr = true;
    int i;

  BDD_ENC_CHECK_INSTANCE(enc);
  for(i=0;i<n_players;i++) BDD_FSM_CHECK_INSTANCE(players[i]);

  self = ALLOC(GameBddFsm, 1);

  self->players = (BddFsm_ptr*)malloc(sizeof(BddFsm_ptr)*n_players);
  self->stateVarCubes = (bdd_ptr*)malloc(sizeof(bdd_ptr)*n_players);
  self->nextStateVarCubes = (bdd_ptr*)malloc(sizeof(bdd_ptr)*n_players);
  self->stateFrozenVarCubes = (bdd_ptr*)malloc(sizeof(bdd_ptr)*n_players);
  self->withSuccessorss = (BddStates*)malloc(sizeof(BddStates)*n_players);
  self->woSuccessorss = (BddStates*)malloc(sizeof(BddStates)*n_players);

  GAME_BDD_FSM_CHECK_INSTANCE(self);

  game_bdd_fsm_init(self, enc,
                    players, stateVarCubes, stateFrozenVarCubes);

  for(i=0;i<n_players;i++)
    expr &= bdd_is_true(self->dd, BddFsm_get_input_constraints(players[i]));

  nusmv_assert(expr);

  return self;
}

/**Function********************************************************************

  Synopsis    [ Destructor of class GameBddFsm. ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
void GameBddFsm_destroy(GameBddFsm_ptr self)
{
  GAME_BDD_FSM_CHECK_INSTANCE(self);

  game_bdd_fsm_deinit(self);
  FREE(self);
}

/**Function********************************************************************

  Synopsis    [ Copy constructor for GameBddFsm. ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
GameBddFsm_ptr GameBddFsm_copy(const GameBddFsm_ptr self)
{
  GameBddFsm_ptr copy;

  GAME_BDD_FSM_CHECK_INSTANCE(self);

  copy = ALLOC(GameBddFsm, 1);

  copy->players = (BddFsm_ptr*)malloc(sizeof(BddFsm_ptr)*n_players);
  copy->stateVarCubes = (bdd_ptr*)malloc(sizeof(bdd_ptr)*n_players);
  copy->nextStateVarCubes = (bdd_ptr*)malloc(sizeof(bdd_ptr)*n_players);
  copy->stateFrozenVarCubes = (bdd_ptr*)malloc(sizeof(bdd_ptr)*n_players);
  copy->withSuccessorss = (BddStates*)malloc(sizeof(BddStates)*n_players);
  copy->woSuccessorss = (BddStates*)malloc(sizeof(BddStates)*n_players);

  GAME_BDD_FSM_CHECK_INSTANCE(copy);

  game_bdd_fsm_copy(self, copy);

  return copy;
}

/**Function********************************************************************

  Synopsis    [ Returns the BDD FSM of player_i. ]

  Description [ Self keeps the ownership of the returned object. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
BddFsm_ptr GameBddFsm_get_player(const GameBddFsm_ptr self, int index)
{
  GAME_BDD_FSM_CHECK_INSTANCE(self);

  return self->players[index];
}

/**Function********************************************************************

  Synopsis    [ Returns the cube of state variables of the first
                player. ]

  Description [ This cube is a big AND of all boolean variables of the
                player excluding frozen variables. Self keeps the
                ownership of the returned object. ]

  SideEffects [ ]

  SeeAlso     [ GameBddFsm_get_state_var_cube_2 ]

******************************************************************************/
bdd_ptr GameBddFsm_get_state_var_cube(const GameBddFsm_ptr self, int index)
{
  GAME_BDD_FSM_CHECK_INSTANCE(self);

  return self->stateVarCubes[index];
}

/**Function********************************************************************

  Synopsis    [ Returns the cube of state variables of the first player
                in the next state. ]

  Description [ This cube is a big AND of all boolean variables of the
                player excluding frozen variables. Self keeps the
                ownership of the returned object. ]

  SideEffects [ ]

  SeeAlso     [ GameBddFsm_get_next_state_var_cube_2 ]

******************************************************************************/
bdd_ptr GameBddFsm_get_next_state_var_cube(const GameBddFsm_ptr self, int index)
{
  GAME_BDD_FSM_CHECK_INSTANCE(self);

  return self->nextStateVarCubes[index];
}


/**Function********************************************************************

  Synopsis    [ Returns the cube of state and frozen variables of the
                first player. ]

  Description [ This cube is a big AND of all boolean state and frozen
                variables of the player. Self keeps the ownership of
                the returned object. ]

  SideEffects [ ]

  SeeAlso     [ GameBddFsm_get_state_frozen_var_cube_2 ]

******************************************************************************/
bdd_ptr GameBddFsm_get_state_frozen_var_cube(const GameBddFsm_ptr self, int index)
{
  GAME_BDD_FSM_CHECK_INSTANCE(self);

  return self->stateFrozenVarCubes[index];
}


/**Function********************************************************************

  Synopsis    [ Returns init (initial) condition of the first player. ]

  Description [ Returned BDD is referenced. ]

  SideEffects [ ]

  SeeAlso     [ GameBddFsm_get_init_2 ]

******************************************************************************/
BddStates GameBddFsm_get_init(const GameBddFsm_ptr self, int index)
{
  GAME_BDD_FSM_CHECK_INSTANCE(self);

  return BddFsm_get_init(self->players[index]);
}


/**Function********************************************************************

  Synopsis    [ Returns the invars of the first player. ]

  Description [ Returned BDD is referenced. Free it after use. ]

  SideEffects [ ]

  SeeAlso     [ GameBddFsm_get_invars_2 ]

******************************************************************************/
BddInvarStates GameBddFsm_get_invars(const GameBddFsm_ptr self, int index)
{
  GAME_BDD_FSM_CHECK_INSTANCE(self);

  return BddFsm_get_state_constraints(self->players[index]);
}

/**Function********************************************************************

  Synopsis    [ Returns the trans of the first player. ]

  Description [ Returned Trans belongs to self, i.e. do not modify or
                delete it.]

  SideEffects [ ]

  SeeAlso     [ GameBddFsm_get_trans_2 ]

******************************************************************************/
BddTrans_ptr GameBddFsm_get_trans(const GameBddFsm_ptr self, int index)
{
  GAME_BDD_FSM_CHECK_INSTANCE(self);

  return BddFsm_get_trans(self->players[index]);
}


/**Function********************************************************************

  Synopsis    [ Returns the trans of the first player as a monolitic BDD. ]

  Description [ NOTE: invariants are not added to the returned
                transition.

                ADVICE: Probably, GameBddFsm_get_move is required, not
                this function.

                Returned BDD is referenced. Free it after using. ]

  SideEffects [ ]

  SeeAlso     [ GameBddFsm_get_monolitic_trans_2 ]

******************************************************************************/
bdd_ptr GameBddFsm_get_monolitic_trans(const GameBddFsm_ptr self, int index)
{
  GAME_BDD_FSM_CHECK_INSTANCE(self);

  return BddFsm_get_monolithic_trans_bdd(self->players[index]);
}


/**Function********************************************************************

  Synopsis    [ Returns the set of states that have at least one legal
                successor. ]

  Description [ Paramater player defines for which player the states
                are computed. A state "s" is the result of the
                function if all the following conditions hold: 1) s is
                a state satisfying Invars for both players. 2) there
                is at least one transition from s which is leads to a
                state satisfying Invars for a given player.

                More formally, if p1 are the vars of first player and
                p2 are the vars of the second player then the returned
                states for the first and second player are,
                respectively:

                NS_1 = {<p1,p2> | Invar_1(p1) & Invar_2(p1,p2)
                                  & Exist p1'.Trans_1(p1,p2,p1') & Invar_1(p1')}

                NS_2 =  {<p1,p2,p1'> | Invar_1(p1) & Invar_2(p1,p2)
                                       & Trans_1(p1,p2,p1') & Invar_1(p1')
                                       & Exist p2'.Trans_2(p1,p2,p1',p2')
                                                   & Invar_2(p1',p2')}'

                The returned BDD belongs to this function. ]

  SideEffects [ Internal cache could change. ]

  SeeAlso     [ GameBddFsm_without_successor_states ]

******************************************************************************/
BddStates GameBddFsm_with_successor_states(GameBddFsm_ptr self,
                                           int player)
{
  BddStates* withSuccessors;
  BddStates* withoutSuccessors;
  int i;

  GAME_BDD_FSM_CHECK_INSTANCE(self);

  withSuccessors = &self->withSuccessorss[player-1];
  withoutSuccessors = &self->woSuccessorss[player-1];

  /* check the cache. Compute if result has not been computed before */
  if (BDD_STATES(NULL) == *withSuccessors) {
    bdd_ptr constrs[n_players], with, without;

    /* with and without successor states are computed here only */
    nusmv_assert((bdd_ptr) NULL == *withoutSuccessors);

    for(i=0;i<n_players;i++)
      constrs[i] = GameBddFsm_get_invars(self,i);

     /* ALL PLAYER */
      bdd_ptr tmp;
      tmp = BddEnc_state_var_to_next_state_var(self->enc, constrs[player-1]);
      with = BddTrans_get_backward_image_state(GameBddFsm_get_trans(self,player-1),
                                               tmp);
      without = bdd_not(self->dd, with);
      bdd_free(self->dd, tmp);


    if (player > 1 ) // TODO : use players and opponents
    { /* OTHER PLAYERS */
      bdd_ptr trans;

      tmp = BddEnc_state_var_to_next_state_var(self->enc, constrs[0]);
      bdd_and_accumulate(self->dd, &with, tmp);
      bdd_and_accumulate(self->dd, &without, tmp);
      bdd_free(self->dd, tmp);

      trans = BddFsm_get_monolithic_trans_bdd(self->players[0]);
      bdd_and_accumulate(self->dd, &with, trans);
      bdd_and_accumulate(self->dd, &without, trans);
      bdd_free(self->dd, trans);
    }

    for(i=0;i<n_players;i++) {

      bdd_and_accumulate(self->dd, &with, constrs[i]);
      bdd_and_accumulate(self->dd, &without, constrs[i]);
    }

    *withSuccessors = with;
    *withoutSuccessors = without;

    for(i=n_players-1;i>=0;i--)
      bdd_free(self->dd, constrs[i]);
  }

  return *withSuccessors;
}

BddStates AtlGameBddFsm_with_successor_states(GameBddFsm_ptr self,
                                           int *players, int opponentsPlay)
{
  BddStates* withSuccessors;
  BddStates* withoutSuccessors;
  int i;

  GAME_BDD_FSM_CHECK_INSTANCE(self);

  withSuccessors = &self->withSuccessorss[players[0]-1];
  withoutSuccessors = &self->woSuccessorss[players[0]-1];

  /* check the cache. Compute if result has not been computed before */
  if (BDD_STATES(NULL) == *withSuccessors) {
    bdd_ptr constrs[n_players], with, without;

    /* with and without successor states are computed here only */
    nusmv_assert((bdd_ptr) NULL == *withoutSuccessors);

    for(i=0;i<n_players;i++)
      constrs[i] = GameBddFsm_get_invars(self,i);

    /* ALL PLAYER */
    bdd_ptr tmp;
    tmp = BddEnc_state_var_to_next_state_var(self->enc, constrs[players[0]-1]);
    with = BddTrans_get_backward_image_state(GameBddFsm_get_trans(self,players[0]-1),
                                             tmp);
    without = bdd_not(self->dd, with);
    bdd_free(self->dd, tmp);


    if (!opponentsPlay ) // TODO : use players and opponents
    { /* OTHER PLAYERS */
      bdd_ptr trans;

      tmp = BddEnc_state_var_to_next_state_var(self->enc, constrs[0]);
      bdd_and_accumulate(self->dd, &with, tmp);
      bdd_and_accumulate(self->dd, &without, tmp);
      bdd_free(self->dd, tmp);

      trans = BddFsm_get_monolithic_trans_bdd(self->players[0]);
      bdd_and_accumulate(self->dd, &with, trans);
      bdd_and_accumulate(self->dd, &without, trans);
      bdd_free(self->dd, trans);
    }

    for(i=0;i<n_players;i++) {

      bdd_and_accumulate(self->dd, &with, constrs[i]);
      bdd_and_accumulate(self->dd, &without, constrs[i]);
    }

    *withSuccessors = with;
    *withoutSuccessors = without;

    for(i=n_players-1;i>=0;i--)
      bdd_free(self->dd, constrs[i]);
  }

  return *withSuccessors;
}

/**Function********************************************************************

  Synopsis    [ Returns the set of states without successors. ]

  Description [ This method returns the set of states with no
                successor. Paramater 'player' defines for which player
                the no-successor states are computed. A state "ns" has
                no successor if all the following conditions hold: 1)
                ns is a state satisfying Invars for players. 2) no
                transition from ns exists which is leads to a state
                satisfying Invars for players.

                Formally, if p1 are the vars of first player and p2
                are the vars of the second player then the
                no-successor states for the first and second player
                are, respectively:

                NS_1 = {<p1,p2> | Invar_1(p1) & Invar_2(p1,p2)
                                  & not Exist p1'.Trans_1(p1,p2,p1')
                                                  & Invar_1(p1')}

                NS_2 =  {<p1,p2,p1'> | Invar_1(p1) & Invar_2(p1,p2)
                                       & Trans_1(p1,p2,p1') & Invar_1(p1')
                                       & not Exist p2'.Trans_2(p1,p2,p1',p2')
                                                       & Invar_2(p1',p2')}'

                The returned BDD belongs to this function. ]

  SideEffects [ Internal cache could change. ]

  SeeAlso     [ GameBddFsm_with_successor_states ]

******************************************************************************/
BddStates GameBddFsm_without_successor_states(GameBddFsm_ptr self,
                                              int player)
{
  BddStates* withoutSuccessors;

  GAME_BDD_FSM_CHECK_INSTANCE(self);

  withoutSuccessors = 1 == player
    ? &self->woSuccessorss[0] : &self->woSuccessorss[1];

  /* Check the cache. Compute if result has not been computed before. */
  if (BDD_STATES(NULL) == *withoutSuccessors) {
    GameBddFsm_with_successor_states(self, player);
  }
  nusmv_assert( (bdd_ptr)NULL != *withoutSuccessors);

  return *withoutSuccessors;
}

BddStates AtlGameBddFsm_without_successor_states(GameBddFsm_ptr self,
                                              int *players, bool opponentsPlay)
{
  BddStates* withoutSuccessors;

  GAME_BDD_FSM_CHECK_INSTANCE(self);

  withoutSuccessors = !opponentsPlay
                      ? &self->woSuccessorss[0] : &self->woSuccessorss[1];

  /* Check the cache. Compute if result has not been computed before. */
  if (BDD_STATES(NULL) == *withoutSuccessors) {
    AtlGameBddFsm_with_successor_states(self, players,opponentsPlay);
  }
  nusmv_assert( (bdd_ptr)NULL != *withoutSuccessors);

  return *withoutSuccessors;
}

/**Function********************************************************************

  Synopsis    [ Returns the strong backward image of a set of states. ]

  Description [ This method computes the set of states that have at
                least one legal (satisfying INVAR) successor and such
                that player can force the whole system to take
                transition to the given (input) set of states.

                'player' is a player which forces the whole system to
                take transition to the input set of states.

                The strong backward image of S(P1, P2) is computed as
                follows (P1 and P2 are state vars of player 1 and 2,
                respectively):

                For player 1:
                S_1 (P1, P2) = {<p1, p2> |
                                Invar_1(p1) & Invar_2(p1,p2) &
                                Exist p1'.Tr_1(p1,p2,p1') & Invar_1(p1') &
                                Any p2'.(Tr_2(p1,p2,p1',p2') & Invar_2(p1',p2'))
                                        -> goal(p1',p2')}

                For player 2:
                S_2 (P1, P2) = {<p1, p2> |
                                Invar_1(p1) & Invar_2(p1,p2) &
                                Any p1'.(Tr_1(p1,p2,p1') & Invar_1(p1')) ->
                                Exist p2'.Tr_2(p1,p2,p1',p2')
                                          & Invar_2(p1',p2') & goal(p1',p2')}

                Note: frozen variables do not participate in quantifications.

                The returned BDD is referenced. ]

  SideEffects [ ]

******************************************************************************/
BddStates GameBddFsm_get_strong_backward_image(const GameBddFsm_ptr self,
                                               BddStates goal,
                                               int player)
{
  /* Rewriting of the above formulas:

     S_1 = Inv1 & Inv2 & E p1'.Tr1 & Inv1' & not E p2'.Tr2 & Inv2' & not goal

     S_2 = Inv1 & Inv2 & not E p1'.Tr1 & Inv1' & not E p2'.Tr2 & Inv2' & goal
  */

  bdd_ptr tmp, constrs[n_players], result;

  GAME_BDD_FSM_CHECK_INSTANCE(self);

  long time;

       gameBddFsm_totalMoveTotalTime -= util_cpu_time();
       time = util_cpu_time();


  constrs[0] = GameBddFsm_get_invars(self,0);
  constrs[1] = GameBddFsm_get_invars(self,1);

  /* depending on the player 'goal' or 'not goal' is used */
  result = 1 == player ? bdd_not(self->dd, goal) : bdd_dup(goal);

      gameBddFsm_notGoalTotalTime += util_cpu_time() - time;
      time = util_cpu_time();

  /* Invariants of the next states MUST be added to the constraints,
     i.e.  it is not enough to have invars in the input 'goal'.
     Otherwise, for example, some states would be erroneously removed
     from the result.
  */

  /* add the second player constraints and move to the next state */
  bdd_and_accumulate(self->dd, &result, constrs[1]);
        gameBddFsm_andInvar2TotalTime += util_cpu_time() - time;
        time = util_cpu_time();
  tmp = BddEnc_state_var_to_next_state_var(self->enc, result);
  bdd_free(self->dd, result);
  result = tmp;
        gameBddFsm_moveGoalAndInvar2TotalTime += util_cpu_time() - time;
        time = util_cpu_time();

  /* apply Exist p2'.Tr_2 and negate. NOTE: there should be no input vars. */
  tmp = BddTrans_get_backward_image_state_input(GameBddFsm_get_trans(self,1),
                                                result);
  bdd_free(self->dd, result);
        gameBddFsm_trans2TotalTime += util_cpu_time() - time;
        time = util_cpu_time();
  result = bdd_not(self->dd, tmp);
  bdd_free(self->dd, tmp);
        gameBddFsm_notTrans2TotalTime += util_cpu_time() - time;
        time = util_cpu_time();

  /* add the first player constraints moved to the next state */
  tmp = BddEnc_state_var_to_next_state_var(self->enc, constrs[0]);
        gameBddFsm_moveInvar1TotalTime += util_cpu_time() - time;
        time = util_cpu_time();
  bdd_and_accumulate(self->dd, &result, tmp);
  bdd_free(self->dd, tmp);
        gameBddFsm_andInvar1TotalTime += util_cpu_time() - time;
        time = util_cpu_time();

  /* apply Exist p1'.Tr_1. NOTE: there should be no input vars. */
  tmp = BddTrans_get_backward_image_state_input(GameBddFsm_get_trans(self,0),
                                                result);
  bdd_free(self->dd, result);
  /* negate if the game is for player 2 */
  result = 2 == player ? bdd_not(self->dd, tmp) : bdd_dup(tmp);
  bdd_free(self->dd, tmp);
        gameBddFsm_trans1TotalTime += util_cpu_time() - time;
        time = util_cpu_time();


  /* apply both constraints on the current state */
  bdd_and_accumulate(self->dd, &result, constrs[0]);
  bdd_and_accumulate(self->dd, &result, constrs[1]);
    gameBddFsm_andInvar1AndInvar2TotalTime += util_cpu_time() - time;
        time = util_cpu_time();

  bdd_free(self->dd, constrs[0]);
  bdd_free(self->dd, constrs[1]);

  /* this would make the player win only if opponent survive.
     Probably, this is not good because if this is a goal state it may not
     have a transition (i.e. it may be a deadlock and this is OK).
   */
       gameBddFsm_totalMoveTotalTime += util_cpu_time();
  return BDD_STATES(result);
}

BddStates AtlGameBddFsm_get_strong_backward_image(const GameBddFsm_ptr self,
                                               BddStates goal,
                                               int *players, int np, int *opponents, bool avoidTarget)
{
  /* Rewriting of the above formulas:

     S_1 = Inv1 & Inv2 & E p1'.Tr1 & Inv1' & not E p2'.Tr2 & Inv2' & not goal

     S_2 = Inv1 & Inv2 & not E p1'.Tr1 & Inv1' & not E p2'.Tr2 & Inv2' & goal
  */

  bdd_ptr tmp, constrs[n_players], result;
  int i;
  
  GAME_BDD_FSM_CHECK_INSTANCE(self);

  long time;

  gameBddFsm_totalMoveTotalTime -= util_cpu_time();
  time = util_cpu_time();

  for(i=0;i<n_players;i++)
    constrs[i] = GameBddFsm_get_invars(self,i);

  /* depending on the player 'goal' or 'not goal' is used */
  result = !avoidTarget ? bdd_not(self->dd, goal) : bdd_dup(goal);

  gameBddFsm_notGoalTotalTime += util_cpu_time() - time;
  time = util_cpu_time();

  /* Invariants of the next states MUST be added to the constraints,
     i.e.  it is not enough to have invars in the input 'goal'.
     Otherwise, for example, some states would be erroneously removed
     from the result.
  */
    for(i=0;i<n_players-np;i++)
        if(i==0) tmp = bdd_dup(constrs[opponents[i]-1]);
        else bdd_or_accumulate(self->dd, &tmp, bdd_dup(constrs[opponents[i]-1]));

    bdd_and_accumulate(self->dd, &result, tmp);

    /* add the enemies constraints and move to the next state */
    gameBddFsm_andInvar2TotalTime += util_cpu_time() - time;
    time = util_cpu_time();
    tmp = BddEnc_state_var_to_next_state_var(self->enc, result);
    bdd_free(self->dd, result);
    result = tmp;
    gameBddFsm_moveGoalAndInvar2TotalTime += util_cpu_time() - time;
    time = util_cpu_time();

    bdd_free(self->dd, tmp);

    /* apply Exist p2'.Tr_2 and negate. NOTE: there should be no input vars. */
    for(i=0;i<n_players-np;i++)
        if(i==0) tmp = bdd_dup(BddTrans_get_backward_image_state_input(GameBddFsm_get_trans(self, opponents[i]-1), result));
        else bdd_or_accumulate(self->dd, &tmp, bdd_dup(BddTrans_get_backward_image_state_input(GameBddFsm_get_trans(self, opponents[i]-1), result)));

    bdd_free(self->dd, result);
    gameBddFsm_trans2TotalTime += util_cpu_time() - time;
    time = util_cpu_time();
    result = bdd_not(self->dd, tmp);
    bdd_free(self->dd, tmp);
    gameBddFsm_notTrans2TotalTime += util_cpu_time() - time;
    time = util_cpu_time();

  /* add the coalition constraints moved to the next state */
    gameBddFsm_moveInvar1TotalTime += util_cpu_time() - time;
    time = util_cpu_time();

    for(i=0;i<np;i++)
        if(i==0) tmp = bdd_dup(BddEnc_state_var_to_next_state_var(self->enc, constrs[players[i]-1]));
        else bdd_and_accumulate(self->dd, &tmp, bdd_dup(BddEnc_state_var_to_next_state_var(self->enc, constrs[players[i]-1])));

    gameBddFsm_andInvar1TotalTime += util_cpu_time() - time;
    time = util_cpu_time();

    bdd_and_accumulate(self->dd, &result, tmp);
    bdd_free(self->dd, tmp);

  /* apply Exist p1'.Tr_1. NOTE: there should be no input vars. */
    for(i=0;i<np;i++)
        if(i==0) tmp = bdd_dup(BddTrans_get_backward_image_state_input(GameBddFsm_get_trans(self, players[i] - 1),result));
        else bdd_and_accumulate(self->dd, &tmp, bdd_dup(BddTrans_get_backward_image_state_input(GameBddFsm_get_trans(self, players[i] - 1),result)));

  gameBddFsm_trans1TotalTime += util_cpu_time() - time;
  time = util_cpu_time();

  bdd_free(self->dd, result);
  /* negate if the game is for player 2 */
  result = avoidTarget ? bdd_not(self->dd, tmp) : bdd_dup(tmp);
  bdd_free(self->dd, tmp);


  /* apply both constraints on the current state */
  for(i=0;i<n_players;i++)
    bdd_and_accumulate(self->dd, &result, constrs[i]);

  gameBddFsm_andInvar1AndInvar2TotalTime += util_cpu_time() - time;
  time = util_cpu_time();

  for(i=0;i<n_players;i++)
    bdd_free(self->dd, constrs[i]);

  /* this would make the player win only if opponent survive.
     Probably, this is not good because if this is a goal state it may not
     have a transition (i.e. it may be a deadlock and this is OK).
   */
  gameBddFsm_totalMoveTotalTime += util_cpu_time();
  return BDD_STATES(result);
}

/**Function********************************************************************

  Synopsis    [ Returns the weak forward image of a set of states. ]

  Description [ This method computes the set of states that CAN be a
                successor of the given states. Note that parameter
                'states' can potentially have constraints on the
                current as well as on the next states.

                The computed formula (p1,p2 - current variables of
                players 1 and 2, respectively) is:

                next_to_current(Exist p1 p2, states(p1,p2,p1',p2')
                                             & inv1 & trans1 & inv1'
                                             & inv2 & trans2 & inv2')

                Note: frozen variables do not participate in
                quantifications.

                The returned BDD is referenced. ]

  SideEffects [ ]

******************************************************************************/
BddStates GameBddFsm_get_weak_forward_image(const GameBddFsm_ptr self,
                                            BddStates states)
{
  /* The function BddTrans_get_forward_image_state cannot be used
     TWICE (one for every player) because during the second
     application it will introduce the opponent state vars (in
     transtion) which just have been abstracted.
  */

  bdd_ptr constrs[n_players], transs[n_players], tmp, result;
  int i;

  GAME_BDD_FSM_CHECK_INSTANCE(self);

  for (i = 0; i < n_players; i++) {
    constrs[i] = GameBddFsm_get_invars(self,i);
    transs[i] = BddFsm_get_monolithic_trans_bdd(self->players[i]);
  }

  /* compute the formula
     Exist p1 (Exist p2, tr2 & (tr1 & constrs[0] & constrs[1] & states))
  */
  tmp = bdd_dup(states);

  for (i = 0; i < n_players; i++)
    bdd_and_accumulate(self->dd, &tmp, constrs[i]);

  bdd_and_accumulate(self->dd, &tmp, transs[0]);

  result = BddTrans_get_forward_image_state(GameBddFsm_get_trans(self,1), tmp);
  bdd_free(self->dd, tmp);

  tmp = bdd_forsome(self->dd, result, GameBddFsm_get_state_var_cube(self,0));
  bdd_free(self->dd, result);

  /* move to the current state and add the remaining invariants */
  result = BddEnc_next_state_var_to_state_var(self->enc, tmp);
  bdd_free(self->dd, tmp);

  for (i = 0; i < n_players; i++)
    bdd_and_accumulate(self->dd, &result, constrs[i]);

  for (i = 0; i < n_players; i++) {
    bdd_free(self->dd, constrs[i]);
    bdd_free(self->dd, transs[i]);
  }

  return BDD_STATES(result);
}

/**Function********************************************************************

  Synopsis    [ Returns a move of a player, which leads to a set of states. ]

  Description [ This method computes the set of current-next states
                which are legal (obey invars), there is (at least one)
                transition from current state to the next one, and the
                next state satisfy the given condition 'toState'.
                More accurately:

                For player 1 the returned formula is

                {<p1,p2,p1'> | Invar_1(p1) & Trans_1(p1,p2,p1')
                               & Invar_1(p1') & Invar_2(p1,p2)
                               & Exist p2',Trans_2(p1,p2,p1',p2')
                                           & Invar_2(p1',p2')
                               & Any p2',(Trans_2(p1,p2,p1',p2')
                                          & Invar_2(p1',p2'))
                                         -> toState(p1',p2') }

                This means that there must be at least one legal
                successor, and ALL possible moves of p2 go to
                'states'.

                For player 2:

                {<p1,p2,p1',p2'> | Invar_1(p1) & Trans_1(p1,p2,p1')
                                   & Invar_1(p1') & Invar_2(p1,p2)
                                   & Trans_2(p1,p2,p1',p2') & Invar_2(p1',p2')
                                   & toState(p1',p2') }

                Note: frozen variables do not participate in
                quantifications.

                The returned BDD is referenced. ]

  SideEffects [ ]

******************************************************************************/
BddStates GameBddFsm_get_move(const GameBddFsm_ptr self,
                              BddStates toState,
                              int player)
{
  bdd_ptr tmp, constrs[n_players], result;
  int i;

  GAME_BDD_FSM_CHECK_INSTANCE(self);

  for (i = 0; i < n_players; i++)
    constrs[i] = GameBddFsm_get_invars(self,i);

  if (1 == player) {
    /* Any p2', Trans_2(p1,p2,p1',p2') & Invar_2(p1',p2') -> States(p1',p2')
         is the same as
       not Exist p2', trans & invar' & not toState'
    */
    tmp = bdd_not(self->dd, toState);
    bdd_and_accumulate(self->dd, &tmp, constrs[1]);

    result = BddEnc_state_var_to_next_state_var(self->enc, tmp);
    bdd_free(self->dd, tmp);

    tmp = BddTrans_get_backward_image_state(GameBddFsm_get_trans(self,1),
                                            result);
    bdd_free(self->dd, result);
    result = bdd_not(self->dd, tmp);
    bdd_free(self->dd, tmp);

    bdd_and_accumulate(self->dd,
                       &result,
                       GameBddFsm_with_successor_states(self, 2));
   }
  else {
    tmp = bdd_and(self->dd, toState, constrs[0]);
    bdd_and_accumulate(self->dd, &tmp, constrs[1]);
    result = BddEnc_state_var_to_next_state_var(self->enc, tmp);
    bdd_free(self->dd, tmp);

    for (i = 0; i < n_players; i++) {
      tmp = BddFsm_get_monolithic_trans_bdd(self->players[i]);
      bdd_and_accumulate(self->dd, &result, tmp);
      bdd_free(self->dd, tmp);
    }

    for (i = 0; i < n_players; i++)
      bdd_and_accumulate(self->dd, &result, constrs[i]);
  }

  for (i = 0; i < n_players; i++)
    bdd_free(self->dd, constrs[i]);

  return result;
}


/**Function********************************************************************

  Synopsis    [ The function returns true if a given player can satisfy
                (stay in, choose) goal-states taking into account
                interpretation (quantification) of player roles and
                players' constraints. ]

  Description [ The interpretation of players' roles is given by
                parameter "quantifiers" (stored typically in
                opt_game_game_initial_condition).

                The formal description of the result is the following.

                If "quantifiers" is "N" (normal)
                for player 1:
                  Exist p1, constrs[0](p1)
                            & Any p2, constrs[1](p1,p2) -> GoalStates(p1,p2)
                for player 2:
                  Any p1, constrs[0](p1)
                          -> Exist p2, constrs[1](p1,p2) & GoalStates(p1,p2)

                If "quantifiers" is "A" (universal)
                for both players
                  Any p1, constrs[0](p1)
                          -> (Any p2, constrs[1](p1,p2) -> GoalStates(p1,p2))

                If "quantifiers" is "E" (existential)
                for both players
                  Exist p1, constrs[0](p1)
                            & Exist p2, constrs[1](p1,p2) & GoalStates(p1,p2)

                Note: there is no transition here.

                Note: p1 and p2 are both state and frozen variables.

                Note: all provided constraints must be already
                      conjoined with invariants, i.e.

                      constrs[0] <= GameBddFsm_get_invars(,0),
                      constrs[1] <= GameBddFsm_get_invars(,1),
                      goalStates <= GameBddFsm_get_invars(,0)
                                    & GameBddFsm_get_invars(,1).

                Note: all provided bdd_ptr are expected to have
                      current-state and frozen vars only, and contain
                      only the given Game FSM vars. Thus, the result
                      is always a constant. If there are other
                      variables see function
                      GameBddFsm_player_satisfies_from. ]

  SideEffects [ ]

  SeeAlso     [ GameBddFsm_player_satisfies_from ]

******************************************************************************/
EXTERN boolean GameBddFsm_can_player_satisfy(NuSMVEnv_ptr env,
                                             const GameBddFsm_ptr self,
                                             BddStates *constrs,
                                             BddStates goalStates,
                                             int player,
                                             char quantifiers)
{
  /* For developers: if the code of this function changes then
     implementation of GameBddFsm_player_satisfies_from has to be
     changed as well !!!
  */

  DDMgr_ptr dd_manager;
  bdd_ptr tmp, result;
  boolean isOne;
  boolean goalNegation, p2Negation, p1Negation;

  const ErrorMgr_ptr errmgr = ERROR_MGR(NuSMVEnv_get_value(env, ENV_ERROR_MANAGER));

  GAME_BDD_FSM_CHECK_INSTANCE(self);

  dd_manager = BddEnc_get_dd_manager(self->enc);

  /* The above formulas can be rewritten as
     Interpretation is "N":
       for player 1 : E p1, Cnstr1 & not E p2, Cnstr2 & not Goal
       for player 2 : not E p1, Cnstr1 & not E p2, Cnstr2 & Goal
     Interpretation is "A":
       for both players : not E p1, Cnstr1 & E p2, Cnstr2 & not Goal
     Interpretation is "E":
       for both players : E p1, Cnstr1 & E p2, Cnstr2 & Goal

     The difference is only in negations. Lets identify them.
  */

  switch (quantifiers) {
  case 'N': /* Normal interpretation of initial conditions */
    if (1 == player) {
      p1Negation = false;
      p2Negation = true;
      goalNegation = true;
    }
    else {
      p1Negation = true;
      p2Negation = true;
      goalNegation = false;
    }
    break;

  case 'A': /* Universal interpration of initial conditions */
    p1Negation = true;
    p2Negation = false;
    goalNegation = true;
    break;

  case 'E': /* Existential interpration of initial conditions */
    p1Negation = false;
    p2Negation = false;
    goalNegation = false;
    break;

  default:
    ErrorMgr_internal_error(errmgr,"unknown intial condition interpretation");
  } /* switch */

  /* negate the goal states */
  result = goalNegation ? bdd_not(dd_manager, goalStates) : bdd_dup(goalStates);

  bdd_and_accumulate(dd_manager, &result, constrs[1]);
  tmp = bdd_forsome(dd_manager,
                    result,
                    GameBddFsm_get_state_frozen_var_cube(self,1));
  bdd_free(dd_manager, result);

  result = p2Negation ? bdd_not(dd_manager, tmp) : bdd_dup(tmp);
  bdd_free(dd_manager, tmp);

  bdd_and_accumulate(dd_manager, &result, constrs[0]);

  /* NEW_CODE */
  /* Here is an optimisation:  (E p.G) != 0 <=> G != 0 and
     (not E p.G) != 0 <=> G == 0. I.e. quantification of p1 is not required.
     Benchmarking showed that this code is more efficient.
  */
  isOne = bdd_isnot_false(dd_manager, result);
  if (p1Negation) isOne = !isOne;
  bdd_free(dd_manager, result);

  return isOne;
}

EXTERN boolean AtlGameBddFsm_can_player_satisfy(NuSMVEnv_ptr env,
                                             const GameBddFsm_ptr self,
                                             BddStates *constrs,
                                             BddStates goalStates,
                                             int *players, int np, int *opponents,
                                             char quantifiers)
{
  /* For developers: if the code of this function changes then
     implementation of GameBddFsm_player_satisfies_from has to be
     changed as well !!!
  */

  DDMgr_ptr dd_manager;
  bdd_ptr tmp,tmp2, result;
  boolean isOne;
  boolean goalNegation, p2Negation, p1Negation;
  int i;
  const ErrorMgr_ptr errmgr = ERROR_MGR(NuSMVEnv_get_value(env, ENV_ERROR_MANAGER));

  GAME_BDD_FSM_CHECK_INSTANCE(self);

  dd_manager = BddEnc_get_dd_manager(self->enc);

  /* The above formulas can be rewritten as
     Interpretation is "N":
       for player 1 : E p1, Cnstr1 & not E p2, Cnstr2 & not Goal
       for player 2 : not E p1, Cnstr1 & not E p2, Cnstr2 & Goal
     Interpretation is "A":
       for both players : not E p1, Cnstr1 & E p2, Cnstr2 & not Goal
     Interpretation is "E":
       for both players : E p1, Cnstr1 & E p2, Cnstr2 & Goal

     The difference is only in negations. Lets identify them.
  */

  switch (quantifiers) {
  case 'N': /* Normal interpretation of initial conditions */
     /* Start to play always the coalition (players) for first */
      p1Negation = false;
      p2Negation = true;
      goalNegation = true;
    break;

  case 'A': /* Universal interpration of initial conditions */
    p1Negation = true;
    p2Negation = false;
    goalNegation = true;
    break;

  case 'E': /* Existential interpration of initial conditions */
    p1Negation = false;
    p2Negation = false;
    goalNegation = false;
    break;

  default:
    ErrorMgr_internal_error(errmgr,"unknown intial condition interpretation");
} /* switch */

  /* negate the goal states */
  result = goalNegation ? bdd_not(dd_manager, goalStates) : bdd_dup(goalStates);

  for(i=0;i<n_players-np;i++)
    if(i==0) tmp2 = bdd_dup(constrs[opponents[i]-1]);
    else bdd_and_accumulate(dd_manager, &tmp2, bdd_dup(constrs[opponents[i]-1]));

  bdd_and_accumulate(dd_manager, &result, tmp2);
  bdd_free(dd_manager, tmp2);

  for(i=0;i<n_players-np;i++)
    if(i==0) tmp2 = bdd_dup(GameBddFsm_get_state_frozen_var_cube(self, opponents[i]-1));
    else bdd_and_accumulate(dd_manager, &tmp2, bdd_dup(GameBddFsm_get_state_frozen_var_cube(self, opponents[i]-1)));

  tmp = bdd_forsome(dd_manager,
                    result,
                    tmp2);

  bdd_free(dd_manager, tmp2);
  bdd_free(dd_manager, result);

  result = p2Negation ? bdd_not(dd_manager, tmp) : bdd_dup(tmp);
  bdd_free(dd_manager, tmp);

  for(i=0;i<np;i++)
    if(i==0) tmp2 = bdd_dup(constrs[players[i]-1]);
    else bdd_and_accumulate(dd_manager, &tmp2, bdd_dup(constrs[players[i]-1]));

  bdd_and_accumulate(dd_manager, &result, tmp2);

  /* NEW_CODE */
  /* Here is an optimisation:  (E p.G) != 0 <=> G != 0 and
     (not E p.G) != 0 <=> G == 0. I.e. quantification of p1 is not required.
     Benchmarking showed that this code is more efficient.
  */
  isOne = bdd_isnot_false(dd_manager, result);
  if (p1Negation) isOne = !isOne;
  bdd_free(dd_manager, result);

  return isOne;
}

/**Function********************************************************************

  Synopsis    [ The function returns a set of states such that a given
                player can satisfy (stay in, choose) goal-states
                taking into account interpretation (quantification) of
                player roles and players' constraints. ]

  Description [ This function is the same as
                GameBddFsm_can_player_satisfy but instead of returning
                true or false returns a set of states.

                The function has meaning only if there are variables
                not belonging to current-state and frozen variable of
                the given FSM. The players' variables are quantified
                out and the returned BDD can consist only of that
                external or next-state variables.

                See the description of GameBddFsm_can_player_satisfy
                for the exact definition of the result.

                Invoker is responsible to de-reference the returned
                BDD. ]

  SideEffects [ ]

  SeeAlso     [ GameBddFsm_can_player_satisfy ]

******************************************************************************/
EXTERN BddStates GameBddFsm_player_satisfies_from(const GameBddFsm_ptr self,
                                                  BddStates constrs[n_players],
                                                  BddStates goalStates,
                                                  int player,
                                                  char quantifiers)
{
  /* For developers: this is exactly the same as in
     GameBddFsm_can_player_satisfy except that quantification is
     performed completely and the remaining BDD is returned.

     If the code of this function changes then implementation of
     GameBddFsm_can_player_satisfies has likely to be changed as well
     !!!
  */

  DDMgr_ptr dd_manager;
  bdd_ptr tmp, result;
  boolean goalNegation, p2Negation, p1Negation;

  const NuSMVEnv_ptr env = EnvObject_get_environment(ENV_OBJECT(self));
  const ErrorMgr_ptr errmgr = ERROR_MGR(NuSMVEnv_get_value(env, ENV_ERROR_MANAGER));

  GAME_BDD_FSM_CHECK_INSTANCE(self);

  dd_manager = BddEnc_get_dd_manager(self->enc);

  /* The above formulas can be rewritten as
     Interpretation is "N":
       for player 1 : E p1, Cnstr1 & not E p2, Cnstr2 & not Goal
       for player 2 : not E p1, Cnstr1 & not E p2, Cnstr2 & Goal
     Interpretation is "A":
       for both players : not E p1, Cnstr1 & E p2, Cnstr2 & not Goal
     Interpretation is "E":
       for both players : E p1, Cnstr1 & E p2, Cnstr2 & Goal

     The difference is only in negations. Lets identify them.
  */

  switch (quantifiers) {
  case 'N': /* Normal interpretation of initial conditions */
    if (1 == player) {
      p1Negation = false;
      p2Negation = true;
      goalNegation = true;
    }
    else {
      p1Negation = true;
      p2Negation = true;
      goalNegation = false;
    }
    break;

  case 'A': /* Universal interpration of initial conditions */
    p1Negation = true;
    p2Negation = false;
    goalNegation = true;
    break;

  case 'E': /* Existential interpration of initial conditions */
    p1Negation = false;
    p2Negation = false;
    goalNegation = false;
    break;

  default:
    ErrorMgr_internal_error(errmgr,"unknown intial condition interpretation");
  } /* switch */

  /* negate the goal states */
  result = goalNegation ? bdd_not(dd_manager, goalStates) : bdd_dup(goalStates);

  bdd_and_accumulate(dd_manager, &result, constrs[1]);
  tmp = bdd_forsome(dd_manager,
                    result,
                    GameBddFsm_get_state_frozen_var_cube(self,1));
  bdd_free(dd_manager, result);

  result = p2Negation ? bdd_not(dd_manager, tmp) : bdd_dup(tmp);
  bdd_free(dd_manager, tmp);

  bdd_and_accumulate(dd_manager, &result, constrs[0]);

  /* --- THIS PART IS DIFFERENT FROM GameBddFsm_can_player_satisfy --- */
  tmp = bdd_forsome(dd_manager,
                    result,
                    GameBddFsm_get_state_frozen_var_cube(self,0));
  bdd_free(dd_manager, result);

  result = p1Negation ? bdd_not(dd_manager, tmp) : bdd_dup(tmp);
  bdd_free(dd_manager, tmp);

  /* Here the result should may be not only a constant TRUE or FALSE,
     because original BDD may contain not only currect-state and
     frozen vars of given FSM.
  */

  return result;
}

/**Function********************************************************************

  Synopsis    [ Prints some information about this GameBddFsm. ]

  Description [ ]

  SideEffects [ None ]

  SeeAlso     [ ]

******************************************************************************/
void GameBddFsm_print_info(const GameBddFsm_ptr self, OStream_ptr file)
{
    int i;

    GAME_BDD_FSM_CHECK_INSTANCE(self);
  nusmv_assert((OStream_ptr) NULL != file);

  OStream_printf(file, "Statistics on Game BDD FSM.\n");

    for(i=0;i<n_players;i++) {
        OStream_printf(file, "Statistics on player %d :\n", i);
        BddFsm_print_info(self->players[i], file);
    }
}

/**Function********************************************************************

  Synopsis    [ Performs the synchronous product of two GameBddFsms. ]

  Description [ The product is mostly formed by constructing the
                product of the component BddFsms.

                Note that the component products are formed only on
                the two components respective variable sets.

                The cached fields for states w/, w/o successors are
                reset. ]

  SideEffects [ self will change ]

  SeeAlso     [ BddFsm_apply_synchronous_product_custom_varsets ]

******************************************************************************/
void GameBddFsm_apply_synchronous_product(GameBddFsm_ptr self,
                                          const GameBddFsm_ptr other)
{
  GAME_BDD_FSM_CHECK_INSTANCE(self);
  GAME_BDD_FSM_CHECK_INSTANCE(other);

  /* check for the same encoding and dd manager */
  nusmv_assert(self->enc == other->enc);
  nusmv_assert(self->dd == other->dd);

    int i;

    /* var cubes: union of players' cubes */
  {
    bdd_ptr tmp;

    for (i = 0; i < n_players; i++) {
      tmp = self->stateVarCubes[i];
      self->stateVarCubes[i] = bdd_cube_union(self->dd,
                                              self->stateVarCubes[i],
                                              other->stateVarCubes[i]);
      bdd_free(self->dd, tmp);
    }

    for (i = 0; i < n_players; i++) {
      tmp = self->nextStateVarCubes[i];
      self->nextStateVarCubes[i] = bdd_cube_union(self->dd,
                                                  self->nextStateVarCubes[i],
                                                  other->nextStateVarCubes[i]);
      bdd_free(self->dd, tmp);
    }

    for (i = 0; i < n_players; i++) {
      tmp = self->stateFrozenVarCubes[i];
      self->stateFrozenVarCubes[i] = bdd_cube_union(self->dd,
                                                    self->stateFrozenVarCubes[i],
                                                    other->stateFrozenVarCubes[i]);
      bdd_free(self->dd, tmp);
    }

  }

  /* w, w/o successors cache: reset */
  for (i = 0; i < n_players; i++)
    if (self->withSuccessorss[i]) {
      bdd_free(self->dd, self->withSuccessorss[i]);
      self->withSuccessorss[i] = BDD_STATES(NULL);
    }

  for (i = 0; i < n_players; i++)
    if (self->woSuccessorss[i]) {
      bdd_free(self->dd, self->woSuccessorss[i]);
      self->woSuccessorss[i] = BDD_STATES(NULL);
    }

  /* player_1, 2: synchronous product of players */
  {
    bdd_ptr one;

    one = bdd_true(self->dd);
    for(i=0;i<n_players;i++)
      BddFsm_apply_synchronous_product_custom_varsets(self->players[i],
                                                    other->players[i],
                                                    self->stateVarCubes[i],
                                                    one,
                                                    self->nextStateVarCubes[i]);
    bdd_free(self->dd, one);
  }
}

/* ---------------------------------------------------------------------- */
/*                         Static functions                               */
/* ---------------------------------------------------------------------- */

/**Function********************************************************************

  Synopsis    [ Private initializer ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
static void game_bdd_fsm_init(GameBddFsm_ptr self,
                              BddEnc_ptr enc,
                              BddFsm_ptr *players,
                              bdd_ptr *stateVarCubes,
                              bdd_ptr *stateFrozenVarCubes)
{
    int i;

    self->enc = enc;
  self->dd = BddEnc_get_dd_manager(enc);

    for(i=0;i<n_players;i++) {

        self->players[i] = players[i];

        self->stateVarCubes[i] = bdd_dup(stateVarCubes[i]);

        self->nextStateVarCubes[i] = BddEnc_state_var_to_next_state_var(enc, stateVarCubes[i]);

        self->stateFrozenVarCubes[i] = bdd_dup(stateFrozenVarCubes[i]);

        self->withSuccessorss[i] = BDD_STATES(NULL);

        self->woSuccessorss[i] = BDD_STATES(NULL);

    }

}

/**Function********************************************************************

  Synopsis    [ Private copy constructor ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
static void game_bdd_fsm_copy(const GameBddFsm_ptr self, GameBddFsm_ptr copy)
{
    int i;

  copy->enc = self->enc;
  copy->dd = self->dd;

    for(i=0;i<n_players;i++) {

        copy->players[i] = BddFsm_copy(self->players[i]);

        copy->stateVarCubes[i] = bdd_dup(self->stateVarCubes[i]);

        copy->nextStateVarCubes[i] = bdd_dup(self->nextStateVarCubes[i]);

        copy->stateFrozenVarCubes[i] = bdd_dup(self->stateFrozenVarCubes[i]);

        copy->withSuccessorss[i] = self->withSuccessorss[i] != BDD_STATES(NULL)
                                   ? bdd_dup(self->withSuccessorss[i]) : BDD_STATES(NULL);

        copy->woSuccessorss[i] = self->woSuccessorss[i] != BDD_STATES(NULL)
                                 ? bdd_dup(self->woSuccessorss[i]) : BDD_STATES(NULL);

    }
}


/**Function********************************************************************

  Synopsis    [ Private member called by the destructor ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
static void game_bdd_fsm_deinit(GameBddFsm_ptr self)
{
    int i;

    for(i=0;i<n_players;i++) {
        BddFsm_destroy(self->players[i]);

        bdd_free(self->dd, self->stateVarCubes[i]);

        bdd_free(self->dd, self->nextStateVarCubes[i]);

        bdd_free(self->dd, self->stateFrozenVarCubes[i]);

        if (self->withSuccessorss[i]) bdd_free(self->dd, self->withSuccessorss[i]);

        if (self->woSuccessorss[i]) bdd_free(self->dd, self->woSuccessorss[i]);
    }
}
