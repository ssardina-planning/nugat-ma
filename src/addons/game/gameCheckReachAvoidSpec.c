/**CFile***********************************************************************

  FileName    [ gameCheckReachAvoidSpec.c ]

  PackageName [ game ]

  Synopsis    [ Functions to perform checking game specifications of the
                form reach-or-avoid a target, i.e. REACHTARGET,
                AVOIDTARGET, REACHDEADLOCK, AVOIDDEADLOCK. ]

  Description [ ]

  SeeAlso     [ ]

  Author      [ Andrei Tchaltsev ]

  Copyright   [
  This file is part of the ``game'' package of NuGaT.
  Copyright (C) 2010 FBK-irst.

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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "game.h"
#include "gameInt.h"
#include "GamePlayer.h"
#include "GameStrategy.h"
#include "PropGame.h"

#include "enc/enc.h"
#include "node/node.h"
#include "opt/opt.h"
#include "parser/symbols.h"
#include "utils/utils.h"
#include "utils/error.h"
#include "utils/ErrorMgr.h"
#include "utils/ustring.h"
/* for variable ordering functions. Later it must be changed to public */
#include "enc/bdd/BddEnc.h"
#include "enc/bdd/BddEnc_private.h"

#include <stdio.h>

/*---------------------------------------------------------------------------*/
static char rcsid[] UTIL_UNUSED = "$Id: gameCheckReachAvoidSpec.c,v 1.1.2.5 2010-02-04 15:42:36 nusmv Exp $";
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/


EXTERN options_ptr options;
;

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [ Checks a reach-target game specification. ]

  Description [ 'prop' is a given property to be checked. It must be a
                REACHTARGET game specification.

                During checking (if required) all corresponding info
                is printed, such as strategy, success messages, etc. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
void Game_CheckReachTargetSpec(PropGame_ptr prop, gameParams_ptr params)
{
  boolean construct_strategy;
  Game_RealizabilityStatus status;
  GameStrategy_ptr strategy;

  NuSMVEnv_ptr env = EnvObject_get_environment(ENV_OBJECT(prop));
  OptsHandler_ptr opts = OPTS_HANDLER(NuSMVEnv_get_value(env, ENV_OPTS_HANDLER));

  nusmv_assert(PROP_GAME(NULL) != prop &&
               PropGame_ReachTarget == Prop_get_type(PROP(prop)));

  /* initialization and initial printing */
  strategy = GAME_STRATEGY(NULL);
  construct_strategy = (((params != (gameParams_ptr) NULL) &&
                         params->strategy_printout) ||
                        opt_game_print_strategy(opts));
  Game_BeforeCheckingSpec(prop);

  /* the checking itself */
  status = Game_UseStrongReachabilityAlgorithm(prop,
                                               (construct_strategy ?
                                                (&strategy) :
                                                (GameStrategy_ptr*) NULL));

  /* printing the results and cleaning up */
  Game_AfterCheckingSpec(prop, status, strategy, NULL, params);
}

/**Function********************************************************************

  Synopsis    [ Checks a reach-target game specification. ]

  Description [ 'prop' is a given property to be checked. It must be a
                REACHTARGET game specification.

                During checking (if required) all corresponding info
                is printed, such as strategy, success messages, etc. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
void Game_CheckAtlReachTargetSpec(PropGame_ptr prop, gameParams_ptr params)
{
    boolean construct_strategy;
    Game_RealizabilityStatus status;
    GameStrategy_ptr strategy;

    NuSMVEnv_ptr env = EnvObject_get_environment(ENV_OBJECT(prop));
    OptsHandler_ptr opts = OPTS_HANDLER(NuSMVEnv_get_value(env, ENV_OPTS_HANDLER));

    nusmv_assert(PROP_GAME(NULL) != prop &&
                 PropGame_AtlReachTarget == Prop_get_type(PROP(prop)));

    /* initialization and initial printing */
    strategy = GAME_STRATEGY(NULL);
    construct_strategy = (((params != (gameParams_ptr) NULL) &&
                           params->strategy_printout) ||
                          opt_game_print_strategy(opts));
    Game_BeforeCheckingSpec(prop);

    /* the checking itself */
    status = Game_UseStrongAtlReachabilityAlgorithm(prop,
                                                 (construct_strategy ?
                                                  (&strategy) :
                                                  (GameStrategy_ptr*) NULL));

    /* printing the results and cleaning up */
    Game_AfterCheckingSpec(prop, status, strategy, NULL, params);
}

/**Function********************************************************************

  Synopsis    [ Checks an avoid-target game specification. ]

  Description [ prop is a given property to be checked. It must be an
                AVOIDTARGET game specification.

                During checking all required info is printed, such as
                strategy, success messages, etc. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
void Game_CheckAvoidTargetSpec(PropGame_ptr prop, gameParams_ptr params)
{
  boolean construct_strategy;
  Game_RealizabilityStatus status;
  GameStrategy_ptr strategy;

  NuSMVEnv_ptr env = EnvObject_get_environment(ENV_OBJECT(prop));
  OptsHandler_ptr opts = OPTS_HANDLER(NuSMVEnv_get_value(env, ENV_OPTS_HANDLER));

  nusmv_assert(PROP_GAME(NULL) != prop &&
               PropGame_AvoidTarget == Prop_get_type(PROP(prop)));

  /* initialization and initial printing */
  strategy = GAME_STRATEGY(NULL);
  construct_strategy = (((params != (gameParams_ptr) NULL) &&
                         params->strategy_printout) ||
                        opt_game_print_strategy(opts));
  Game_BeforeCheckingSpec(prop);

  /* the checking itself */
  status = Game_UseStrongReachabilityAlgorithm(prop,
                                               (construct_strategy ?
                                                (&strategy) :
                                                (GameStrategy_ptr*) NULL));

  /* printing the results and cleaning up */
  Game_AfterCheckingSpec(prop, status, strategy, NULL, params);
}

/**Function********************************************************************

  Synopsis    [ Checks an avoid-target Atl-game specification. ]

  Description [ prop is a given property to be checked. It must be an
                AVOIDTARGET game specification.

                During checking all required info is printed, such as
                strategy, success messages, etc. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
void AtlGame_CheckAvoidTargetSpec(PropGame_ptr prop, gameParams_ptr params)
{
    boolean construct_strategy;
    Game_RealizabilityStatus status;
    GameStrategy_ptr strategy;

    NuSMVEnv_ptr env = EnvObject_get_environment(ENV_OBJECT(prop));
    OptsHandler_ptr opts = OPTS_HANDLER(NuSMVEnv_get_value(env, ENV_OPTS_HANDLER));

    nusmv_assert(PROP_GAME(NULL) != prop &&
                 PropGame_AvoidTarget == Prop_get_type(PROP(prop)));

    /* initialization and initial printing */
    strategy = GAME_STRATEGY(NULL);
    construct_strategy = (((params != (gameParams_ptr) NULL) &&
                           params->strategy_printout) ||
                          opt_game_print_strategy(opts));
    Game_BeforeCheckingSpec(prop);

    /* the checking itself */
    status = Game_UseStrongAtlReachabilityAlgorithm(prop,
                                                 (construct_strategy ?
                                                  (&strategy) :
                                                  (GameStrategy_ptr*) NULL));

    /* printing the results and cleaning up */
    Game_AfterCheckingSpec(prop, status, strategy, NULL, params);
}

/**Function********************************************************************

  Synopsis    [ Checks a reach-deadlock game specification, i.e., tries
                to build a strategy to reach the opponents deadlock
                states. ]

  Description [ prop is a given property to be checked. It must be a
                REACHDEADLOCK game specification.

                During checking all required info is printed, such as
                strategy, success messages, etc. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
void Game_CheckReachDeadlockSpec(PropGame_ptr prop, gameParams_ptr params)
{
  boolean construct_strategy;
  Game_RealizabilityStatus status;
  GameStrategy_ptr strategy;

  NuSMVEnv_ptr env = EnvObject_get_environment(ENV_OBJECT(prop));
  OptsHandler_ptr opts = OPTS_HANDLER(NuSMVEnv_get_value(env, ENV_OPTS_HANDLER));

  nusmv_assert(PROP_GAME(NULL) != prop &&
               PropGame_ReachDeadlock == Prop_get_type(PROP(prop)));

  /* initialization and initial printing */
  strategy = GAME_STRATEGY(NULL);
  construct_strategy = (((params != (gameParams_ptr) NULL) &&
                         params->strategy_printout) ||
                        opt_game_print_strategy(opts));
  Game_BeforeCheckingSpec(prop);

  /* the checking itself */
  status = Game_UseStrongReachabilityAlgorithm(prop,
                                               (construct_strategy ?
                                                (&strategy) :
                                                (GameStrategy_ptr*) NULL));

  /* printing the results and cleaning up */
  Game_AfterCheckingSpec(prop, status, strategy, NULL, params);
}

/**Function********************************************************************

  Synopsis    [ Checks an avoid-deadlock game specification, i.e., tries
                to build a strategy to avoid the players deadlock
                states. ]

  Description [ prop is a given property to be checked. It must be an
                AVOIDDEADLOCK game specification.

                During checking all required info is printed, such as
                strategy, success messages, etc. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
void Game_CheckAvoidDeadlockSpec(PropGame_ptr prop, gameParams_ptr params)
{
  boolean construct_strategy;
  Game_RealizabilityStatus status;
  GameStrategy_ptr strategy;

  NuSMVEnv_ptr env = EnvObject_get_environment(ENV_OBJECT(prop));
  OptsHandler_ptr opts = OPTS_HANDLER(NuSMVEnv_get_value(env, ENV_OPTS_HANDLER));

  nusmv_assert(PROP_GAME(NULL) != prop &&
               PropGame_AvoidDeadlock == Prop_get_type(PROP(prop)));

  /* initialization and initial printing */
  strategy = GAME_STRATEGY(NULL);
  construct_strategy = (((params != (gameParams_ptr) NULL) &&
                         params->strategy_printout) ||
                        opt_game_print_strategy(opts));
  Game_BeforeCheckingSpec(prop);

    /* the checking itself */
    status = Game_UseStrongReachabilityAlgorithm(prop,
                                                 (construct_strategy ?
                                                  (&strategy) :
                                                  (GameStrategy_ptr*) NULL));

  /* printing the results and cleaning up */
  Game_AfterCheckingSpec(prop, status, strategy, NULL, params);
}

/**Function********************************************************************

  Synopsis    [ The main function to compute the strong strategy for the
                player to reach a target defined in 'prop'. ]

  Description [

                prop     - specifies a type of the problem, target states
                           and a player.

                strategy - returns the computed strategy. It can be
                           zero if a strategy is not required.

                           NB: The invoker should free the content of
                           the strategy.

                The return value is the computed status of the
                problem, i.e., realizable, unrealizable, unknown.

                The property (problem) can only be one of:
                reach-target, reach-deadlock, avoid-target, or
                avoid-deadlock.

                A strong strategy is a set of pairs of player-opponent
                variables values, that if for every step and a current
                state the player's variables satisfy the strategy then
                the opponent will inevitably reach the target states
                independent of the values of the opponent variables
                (of course they must satisfy the transition relations
                as well as the invariant constraints). ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
Game_RealizabilityStatus Game_UseStrongReachabilityAlgorithm(PropGame_ptr prop,
                                                    GameStrategy_ptr* strategy)
{

  NuSMVEnv_ptr env = EnvObject_get_environment(ENV_OBJECT(prop));
  GameBddFsm_ptr fsm = PropGame_get_game_bdd_fsm(prop);
  BddEnc_ptr enc = BDD_ENC(NuSMVEnv_get_value(env, ENV_BDD_ENCODER));
  DDMgr_ptr dd_manager = BddEnc_get_dd_manager(enc);

  const NodeMgr_ptr nodemgr = NODE_MGR(NuSMVEnv_get_value(env, ENV_NODE_MGR));
  const UStringMgr_ptr strings = USTRING_MGR(NuSMVEnv_get_value(env, ENV_STRING_MGR));
  const ErrorMgr_ptr errmgr = ERROR_MGR(NuSMVEnv_get_value(env, ENV_ERROR_MANAGER));

  OptsHandler_ptr oh = OPTS_HANDLER(NuSMVEnv_get_value(env, ENV_OPTS_HANDLER));
  StreamMgr_ptr streams = STREAM_MGR(NuSMVEnv_get_value(env, ENV_STREAM_MANAGER));
  FILE* outstream = StreamMgr_get_output_stream(streams);
  FILE* errstream = StreamMgr_get_error_stream(streams);

    int i;

  PROP_GAME_CHECK_INSTANCE(prop);
  nusmv_assert(PropGame_ReachTarget == Prop_get_type(PROP(prop)) ||
               PropGame_AvoidTarget == Prop_get_type(PROP(prop)) ||
               PropGame_ReachDeadlock == Prop_get_type(PROP(prop)) ||
               PropGame_AvoidDeadlock == Prop_get_type(PROP(prop)));

  /* flag which player this game is for */

    int player;
    char str[50];

    for(i=0;i<n_players;i++) {
        sprintf(str,"%d",i+1);
        if(UStringMgr_find_string(strings,str) == PropGame_get_player(prop))
            player = i+1;
    }

  int opponent = 1 == player ? 2 : 1;
  char quantifiers = opt_game_game_initial_condition(oh);

  bdd_ptr inits[n_players], invars[n_players];
  bdd_ptr originalTarget;  /* A target to be reached. */
  bdd_ptr allReachStates;  /* All the states from which the target can
                              be reached. */
  node_ptr reachStateList; /* A list of consecutive states from which
                              the target can be reached. */
  boolean isTargetReached; /* True if the target is reachable by a
                              player (for reachability game) or an
                              opponent (for avoidance game). */
  boolean isFixedpointReached = false;
  int pathLength = 0;

    for(i=0;i<n_players;i++) {
      /* prepare the initial states (obtain them and add the invariants) */
      inits[i] = GameBddFsm_get_init(fsm,i);
      invars[i] = GameBddFsm_get_invars(fsm,i);

      bdd_and_accumulate(dd_manager, &inits[i], invars[i]);
    }
  /* initialize the original target states (reachability or avoidance target) */
  if (PropGame_ReachTarget == Prop_get_type(PROP(prop)) ||
      PropGame_AvoidTarget == Prop_get_type(PROP(prop))) {
    originalTarget =
      BddEnc_expr_to_bdd(enc, Prop_get_expr_core(PROP(prop)), Nil);
  }
  else {
    /* create empty set of target states */
    originalTarget = bdd_false(dd_manager);
  }

  for(i=0;i<n_players;i++)
    bdd_and_accumulate(dd_manager, &originalTarget, invars[i]);

  /* For avoidance games it is necessary to reverse the player and, as
     result, initial quantifiers (because now we play for the
     opponent).
  */
  if (PropGame_AvoidTarget == Prop_get_type(PROP(prop)) ||
      PropGame_AvoidDeadlock == Prop_get_type(PROP(prop))) {
    player = 1 == player ? 2 : 1;
    opponent = 1 == opponent ? 2 : 1;
    quantifiers = quantifiers == 'N' ? 'N'  /* 'N' does not change */
      : quantifiers == 'E' ? 'A'
      : quantifiers == 'A' ? 'E'
      : (ErrorMgr_internal_error(errmgr,"unknown quantifiers"), 0);
  }

  allReachStates = bdd_dup(originalTarget);
  reachStateList = cons(nodemgr,(node_ptr)bdd_dup(originalTarget), Nil);

  /* check whether the target can be reached at the initial state */
  isTargetReached = GameBddFsm_can_player_satisfy(env,fsm, inits,
                                                  allReachStates, player,
                                                  quantifiers);

  /* Makes a few checks and prints a few warning in the case of
     suspicious input, i.e., if init is zero, target is zero or one.
  */
  {
    /* init is zero */

      int i;
      bool expr = false, expr_loc;
      char str[50];

      for(i=0;i<n_players;i++) {
          expr_loc = bdd_is_false(dd_manager, inits[i]);
          expr |= expr_loc;

          if(expr_loc)
              sprintf(str,"%d",i+1);
      }
      /* init is zero */
      if (expr) {
          fprintf(errstream, "\n********   WARNING   ********\n"
                          "Initial states set for %s is empty.\n"
                          "******** END WARNING ********\n",
                  str);
      /* to skip the loop below */
      isFixedpointReached = true;
    }
    /* target is zero (check only reach-target and avoid-target specs) */
    if ((PropGame_ReachTarget == Prop_get_type(PROP(prop))
         || PropGame_AvoidTarget == Prop_get_type(PROP(prop)))
        && bdd_is_false(dd_manager, originalTarget)) {
      fprintf(errstream, "\n********   WARNING   ********\n"
              "The target states set is empty.\n"
              "******** END WARNING ********\n");
      /* continue the check because deadlock state may allow to win */
    }
    /* target is reached at step zero */
    if (isTargetReached) {
      fprintf(errstream, "\n********   WARNING   ********\n"
              "The target states are reached at step 0.\n"
              "Probably this is not what was intended.\n"
              "******** END WARNING ********\n");
    }
  }

  /* ------------ main loop ------------------*/
  /* loops stop if fixed-point is reached or target is reached from init */
  while (!isFixedpointReached && !isTargetReached) {

    bdd_ptr preImage;
    bdd_ptr previousReachStates = bdd_dup(allReachStates);

    if(opt_verbose_level_gt(oh, 0)) {
      fprintf(outstream, "\n-----------------------------\n"
              "Reach-target algorithm: iteration %d\n", pathLength);
    }

    // pre-image can be computed with not-reach-state constraint on
    // returned value, since only "new" reach states are of importance.
    // This can speed up computation of pre-image.

    /* compute the strong pre-image for reach states and given player. */
    preImage = GameBddFsm_get_strong_backward_image(fsm, allReachStates,
                                                    player);

    bdd_or_accumulate(dd_manager, &allReachStates, preImage);

    isFixedpointReached = (previousReachStates == allReachStates);

    /* check reachability only if fixpoint has not been reached */
    if (!isFixedpointReached) {
      isTargetReached = GameBddFsm_can_player_satisfy(env,fsm, inits,
                                                      allReachStates, player,
                                                      quantifiers);
    }

    /* add to the list of reach states sets */
    reachStateList = cons(nodemgr,(node_ptr)bdd_dup(allReachStates), reachStateList);
    pathLength++;

    bdd_free(dd_manager, preImage);
    bdd_free(dd_manager, previousReachStates);
  } /* while */


  /* auxiliary info */
  if(opt_verbose_level_gt(oh, 0)) {
    fprintf(outstream,
            "The number of iterations for strategy computation is %d.\n",
            pathLength);
  }


  /*-----------------------------------------------------------------------*/
  /*-------------         STRATEGY COMPUTATION        ---------------------*/
  /*-----------------------------------------------------------------------*/
  /* strategy is computed only if the container for the strategy is provided.
     The strategy is computed for the player if the game is won and for the
     opponent if the game is lost.

  */
  if ((GameStrategy_ptr*) NULL != strategy) {
    bdd_ptr trans = bdd_false(dd_manager);

    /* Compute the transitions for the strategy */

    /* The target is reached =>
       the player won in a reachability game or lost in an avoidance game
    */
    if (isTargetReached) {
      node_ptr iter;
      node_ptr targ, diff;
      node_ptr diffReachStateList;

      /* At first, create a list of differences between consecutive
         reach states.
         NB: reachStateList list is decreasing, i.e. first step is at
         the beginning, the original target is at the end.
      */
      for (iter = reachStateList, diffReachStateList = Nil;
           iter != Nil && cdr(iter) != Nil;
           iter = cdr(iter)) {
        bdd_ptr not_prev = bdd_not(dd_manager, (bdd_ptr)car(cdr(iter)));
        bdd_ptr and = bdd_and(dd_manager, (bdd_ptr)car(iter), not_prev);
        bdd_free(dd_manager, not_prev);
        diffReachStateList = cons(nodemgr,(node_ptr)and, diffReachStateList);
      }
      diffReachStateList = reverse(diffReachStateList);

      /* Compute moves (transitions). They should include also opponent-deadlock
         moves
      */
      targ = cdr(reachStateList);
      diff = diffReachStateList;

      while (targ != Nil) {
        bdd_ptr move;
        /* move =
           for player 1 :
           diff & trans1 & next(invar1) &
           Any p2', trans2 & next(invar2) -> next(targ)
           for player 2 :
           diff & trans1 & next(invar1) &
           trans2 & next(invar2) & next(targ)

           Actually, for the first player opponent-deadlock moves are
           removed by GameBddFsm_get_move.  They will be added later
           in a separate field.
        */
        move = GameBddFsm_get_move(fsm, (bdd_ptr)car(targ), player);
        bdd_and_accumulate(dd_manager, &move, (bdd_ptr)car(diff));

        bdd_or_accumulate(dd_manager, &trans, move);

        bdd_free(dd_manager, move);
        targ = cdr(targ);
        diff = cdr(diff);
        nusmv_assert((Nil == targ) == (Nil == diff));
      } /* while */

      /* add back the opponent-deadlock moves for the first player */
      if (1 == player) {
        bdd_or_accumulate(dd_manager, &trans,
                          GameBddFsm_without_successor_states(fsm, 2));
      }

      /* free the list of reach-states differences, i.e. diffReachStateList */
      while (diffReachStateList != Nil) {
        node_ptr n = cdr(diffReachStateList);
        bdd_free(dd_manager, (bdd_ptr)car(diffReachStateList));
        free_node(nodemgr,diffReachStateList);
        diffReachStateList = n;
      }

      /* construct the strategy */
      *strategy =
        GameStrategy_construct(env,
                               fsm,
                               player,
                               /* initial quantifiers have been
                                  reversed => reverse */
                               (quantifiers !=
                                opt_game_game_initial_condition(oh)),
                               originalTarget,
                               allReachStates,
                               trans);

    } /* end of if (isTargetReached) */

    /* fixed point is reached =>
       the player lost in a reachability game or won in an avoidance game
    */
    else if (isFixedpointReached) {
      bdd_ptr winStates, zero;

      /* for the player (opponent) it is just necessary to stay in the
         not-reach states */
      winStates = bdd_not(dd_manager, allReachStates);

      trans = GameBddFsm_get_move(fsm, winStates, opponent);
      bdd_and_accumulate(dd_manager, &trans, winStates);


      /* for player 1 GameBddFsm_get_move removes opponent deadlock states.
         Add them back now.
      */
      if (1 == opponent) {
        bdd_or_accumulate(dd_manager, &trans,
                          GameBddFsm_without_successor_states(fsm, 2));
      }

      zero = bdd_false(dd_manager);

      /* construct the strategy */
      *strategy =
        GameStrategy_construct(env,
                               fsm,
                               opponent,
                               /*initial quantifiers have been reversed
                                 => keep them */
                               (quantifiers ==
                                opt_game_game_initial_condition(oh)),
                               zero,
                               winStates,
                               trans);

      bdd_free(dd_manager, zero);
      bdd_free(dd_manager, winStates);
    } /* end of else if (isFixedpointReached)*/

    else {
      /* if the target is not reached then the fixpoint is reached always */
      nusmv_assert(false);
    }

  }

  /* end of reach- or avoid- strategy computation */


  /*-----    FREE THE CREATED OBJECTS    --------------------------------------*/

  /* free reachStateList */
  while (reachStateList != Nil) {
    node_ptr n = cdr(reachStateList);
    bdd_free(dd_manager, (bdd_ptr)car(reachStateList));
    free_node(nodemgr,reachStateList);
    reachStateList = n;
  }

  bdd_free(dd_manager, allReachStates);
  bdd_free(dd_manager, originalTarget);

  for(i=0;i<n_players;i++) {
    bdd_free(dd_manager, inits[i]);
    bdd_free(dd_manager, invars[i]);
  }

  return
    ( PropGame_ReachTarget == Prop_get_type(PROP(prop))
      || PropGame_ReachDeadlock == Prop_get_type(PROP(prop))
    )
    ? (isTargetReached ? GAME_REALIZABLE : GAME_UNREALIZABLE)
    : (isTargetReached ? GAME_UNREALIZABLE : GAME_REALIZABLE);
}

/******************************************************************************/
Game_RealizabilityStatus Game_UseStrongAtlReachabilityAlgorithm(PropGame_ptr prop,
                                                             GameStrategy_ptr* strategy)
{

    NuSMVEnv_ptr env = EnvObject_get_environment(ENV_OBJECT(prop));
    GameBddFsm_ptr fsm = PropGame_get_game_bdd_fsm(prop);
    BddEnc_ptr enc = BDD_ENC(NuSMVEnv_get_value(env, ENV_BDD_ENCODER));
    DDMgr_ptr dd_manager = BddEnc_get_dd_manager(enc);

    const NodeMgr_ptr nodemgr = NODE_MGR(NuSMVEnv_get_value(env, ENV_NODE_MGR));
    const UStringMgr_ptr strings = USTRING_MGR(NuSMVEnv_get_value(env, ENV_STRING_MGR));
    const ErrorMgr_ptr errmgr = ERROR_MGR(NuSMVEnv_get_value(env, ENV_ERROR_MANAGER));

    OptsHandler_ptr oh = OPTS_HANDLER(NuSMVEnv_get_value(env, ENV_OPTS_HANDLER));
    StreamMgr_ptr streams = STREAM_MGR(NuSMVEnv_get_value(env, ENV_STREAM_MANAGER));
    FILE* outstream = StreamMgr_get_output_stream(streams);
    FILE* errstream = StreamMgr_get_error_stream(streams);

    int i,j, *players, *opponents, found;

    PROP_GAME_CHECK_INSTANCE(prop);
    nusmv_assert(PropGame_AtlReachTarget == Prop_get_type(PROP(prop)) ||
                 PropGame_AtlAvoidTarget == Prop_get_type(PROP(prop)) ||
                 PropGame_AtlReachDeadlock == Prop_get_type(PROP(prop)) ||
                 PropGame_AtlAvoidDeadlock == Prop_get_type(PROP(prop)));

    /* flag which player this game is for */
    int np = 0, no = 0;
    bool avoidTarget = false;
    char str[50];

    char  tmp[50];

    strcpy(tmp,PropGame_get_player(prop)->text); // contain multiple players "1 PLAYER_2 ..."
    int k,count=0;
    char *p;
    int length = strlen(tmp);

    for (k = 0; k < length; k++)
        if (tmp[k]==' ')
            count++;

    string_ptr playersProp[count];
    k = 0;
    p = strtok (tmp," ");
    while (p != NULL)
    {
        playersProp[k++] = UStringMgr_find_string(strings,p);
        p = strtok (NULL, " ");
    }

    players = (int*)malloc(sizeof(int)*count);
    opponents = (int*)malloc(sizeof(int)*(n_players-count));

    for(i=0;i<n_players;i++) {
        found = 0;
        sprintf(str,"%d",i+1);
        for(j=0;j<count&&!found;j++)
            if(UStringMgr_find_string(strings,str) == playersProp[j])
                found = 1;

        if(found)
            players[np++] = i+1;
        else
            opponents[no++] = i+1;
    }

    char quantifiers = opt_game_game_initial_condition(oh);

    bdd_ptr inits[n_players], invars[n_players];
    bdd_ptr originalTarget;  /* A target to be reached. */
    bdd_ptr allReachStates;  /* All the states from which the target can
                              be reached. */
    node_ptr reachStateList; /* A list of consecutive states from which
                              the target can be reached. */
    boolean isTargetReached; /* True if the target is reachable by a
                              player (for reachability game) or an
                              opponent (for avoidance game). */
    boolean isFixedpointReached = false;
    int pathLength = 0;

    for(i=0;i<n_players;i++) {
        /* prepare the initial states (obtain them and add the invariants) */
        inits[i] = GameBddFsm_get_init(fsm,i);
        invars[i] = GameBddFsm_get_invars(fsm,i);

        bdd_and_accumulate(dd_manager, &inits[i], invars[i]);
    }

    /* initialize the original target states (reachability or avoidance target) */
    if (PropGame_AtlReachTarget == Prop_get_type(PROP(prop)) ||
        PropGame_AtlAvoidTarget == Prop_get_type(PROP(prop))) {
        originalTarget =
                BddEnc_expr_to_bdd(enc, Prop_get_expr_core(PROP(prop)), Nil);
    }
    else {
        /* create empty set of target states */
        originalTarget = bdd_false(dd_manager);
    }

    for(i=0;i<n_players;i++)
        bdd_and_accumulate(dd_manager, &originalTarget, invars[i]);

    /* For avoidance games it is necessary to reverse the player and, as
       result, initial quantifiers (because now we play for the
       opponent).
    */
    if (PropGame_AtlAvoidTarget == Prop_get_type(PROP(prop)) ||
        PropGame_AtlAvoidDeadlock == Prop_get_type(PROP(prop))) {

        int *tmp2;
        *tmp2 = *players;
        *players = *opponents;
        *opponents = *tmp2;
        avoidTarget = true;

        quantifiers = quantifiers == 'N' ? 'N'  /* 'N' does not change */
                                         : quantifiers == 'E' ? 'A'
                                                              : quantifiers == 'A' ? 'E'
                                                                                   : (ErrorMgr_internal_error(errmgr,"unknown quantifiers"), 0);
    }

    allReachStates = bdd_dup(originalTarget);
    reachStateList = cons(nodemgr,(node_ptr)bdd_dup(originalTarget), Nil);

    /* check whether the target can be reached at the initial state */
    isTargetReached = AtlGameBddFsm_can_player_satisfy(env,fsm, inits,
                                                        allReachStates, players, np, opponents,
                                                        quantifiers);

    /* Makes a few checks and prints a few warning in the case of
       suspicious input, i.e., if init is zero, target is zero or one.
    */
    {
        int i;
        bool expr = false, expr_loc;
        char str[50];

        for(i=0;i<n_players;i++) {
            expr_loc = bdd_is_false(dd_manager, inits[i]);
            expr |= expr_loc;

            if(expr_loc)
                sprintf(str,"%d",i+1);
        }
        /* init is zero */
        if (expr) {
            fprintf(errstream, "\n********   WARNING   ********\n"
                            "Initial states set for %s is empty.\n"
                            "******** END WARNING ********\n",
                    str);
            /* to skip the loop below */
            isFixedpointReached = true;
        }
        /* target is zero (check only reach-target and avoid-target specs) */
        if ((PropGame_AtlReachTarget == Prop_get_type(PROP(prop))
             || PropGame_AtlAvoidTarget == Prop_get_type(PROP(prop)))
            && bdd_is_false(dd_manager, originalTarget)) {
            fprintf(errstream, "\n********   WARNING   ********\n"
                    "The target states set is empty.\n"
                    "******** END WARNING ********\n");
            /* continue the check because deadlock state may allow to win */
        }
        /* target is reached at step zero */
        if (isTargetReached) {
            fprintf(errstream, "\n********   WARNING   ********\n"
                    "The target states are reached at step 0.\n"
                    "Probably this is not what was intended.\n"
                    "******** END WARNING ********\n");
        }
    }

    /* ------------ main loop ------------------*/
    /* loops stop if fixed-point is reached or target is reached from init */
    while (!isFixedpointReached && !isTargetReached) {

        bdd_ptr preImage;
        bdd_ptr previousReachStates = bdd_dup(allReachStates);

        if(opt_verbose_level_gt(oh, 0)) {
            fprintf(outstream, "\n-----------------------------\n"
                    "Reach-target algorithm: iteration %d\n", pathLength);
        }

        // pre-image can be computed with not-reach-state constraint on
        // returned value, since only "new" reach states are of importance.
        // This can speed up computation of pre-image.

        /* compute the strong pre-image for reach states and given player. */

        preImage = AtlGameBddFsm_get_strong_backward_image(fsm, allReachStates,
                                                        players, np, opponents, avoidTarget);

        bdd_or_accumulate(dd_manager, &allReachStates, preImage);

        isFixedpointReached = (previousReachStates == allReachStates);

        /* check reachability only if fixpoint has not been reached */
        if (!isFixedpointReached) {
            isTargetReached = AtlGameBddFsm_can_player_satisfy(env,fsm, inits,
                                                            allReachStates, players, np, opponents,
                                                            quantifiers);
        }

        /* add to the list of reach states sets */
        reachStateList = cons(nodemgr,(node_ptr)bdd_dup(allReachStates), reachStateList);
        pathLength++;

        bdd_free(dd_manager, preImage);
        bdd_free(dd_manager, previousReachStates);
    } /* while */

    /* auxiliary info */
    if(opt_verbose_level_gt(oh, 0)) {
        fprintf(outstream,
                "The number of iterations for strategy computation is %d.\n",
                pathLength);
    }


    /*-----------------------------------------------------------------------*/
    /*-------------         STRATEGY COMPUTATION        ---------------------*/
    /*-----------------------------------------------------------------------*/
    /* strategy is computed only if the container for the strategy is provided.
       The strategy is computed for the player if the game is won and for the
       opponents if the game is lost.

    */
    if ((GameStrategy_ptr*) NULL != strategy) {
        bdd_ptr trans = bdd_false(dd_manager);

        /* Compute the transitions for the strategy */

        /* The target is reached =>
           the player won in a reachability game or lost in an avoidance game
        */
        if (isTargetReached) {
            node_ptr iter;
            node_ptr targ, diff;
            node_ptr diffReachStateList;

            /* At first, create a list of differences between consecutive
               reach states.
               NB: reachStateList list is decreasing, i.e. first step is at
               the beginning, the original target is at the end.
            */
            for (iter = reachStateList, diffReachStateList = Nil;
                 iter != Nil && cdr(iter) != Nil;
                 iter = cdr(iter)) {
                bdd_ptr not_prev = bdd_not(dd_manager, (bdd_ptr)car(cdr(iter)));
                bdd_ptr and = bdd_and(dd_manager, (bdd_ptr)car(iter), not_prev);
                bdd_free(dd_manager, not_prev);
                diffReachStateList = cons(nodemgr,(node_ptr)and, diffReachStateList);
            }
            diffReachStateList = reverse(diffReachStateList);

            /* Compute moves (transitions). They should include also opponent-deadlock
               moves
            */
            targ = cdr(reachStateList);
            diff = diffReachStateList;

            while (targ != Nil) {
                bdd_ptr move;
                /* move =
                   for player 1 :
                   diff & trans1 & next(invar1) &
                   Any p2', trans2 & next(invar2) -> next(targ)
                   for player 2 :
                   diff & trans1 & next(invar1) &
                   trans2 & next(invar2) & next(targ)

                   Actually, for the first player opponent-deadlock moves are
                   removed by GameBddFsm_get_move.  They will be added later
                   in a separate field.
                */
                for(i=0;i<np;i++) {
                    move = GameBddFsm_get_move(fsm, (bdd_ptr)car(targ), players[i]);
                    bdd_and_accumulate(dd_manager, &move, (bdd_ptr)car(diff));
                }
                bdd_or_accumulate(dd_manager, &trans, move);

                bdd_free(dd_manager, move);
                targ = cdr(targ);
                diff = cdr(diff);
                nusmv_assert((Nil == targ) == (Nil == diff));
            } /* while */

            /* add back the opponent-deadlock moves for the first player */
            if (avoidTarget==0) {

                for(i=0;i<no;i++)
                    bdd_or_accumulate(dd_manager, &trans,
                                      GameBddFsm_without_successor_states(fsm, opponents[i]));


            }

            /* free the list of reach-states differences, i.e. diffReachStateList */
            while (diffReachStateList != Nil) {
                node_ptr n = cdr(diffReachStateList);
                bdd_free(dd_manager, (bdd_ptr)car(diffReachStateList));
                free_node(nodemgr,diffReachStateList);
                diffReachStateList = n;
            }

            /* construct the strategy */
            *strategy =
                    AtlGameStrategy_construct(env,
                                           fsm,
                                           players, np, opponents, avoidTarget,
                            /* initial quantifiers have been
                               reversed => reverse */
                                           (quantifiers !=
                                            opt_game_game_initial_condition(oh)),
                                           originalTarget,
                                           allReachStates,
                                           trans);

        } /* end of if (isTargetReached) */

            /* fixed point is reached =>
               the player lost in a reachability game or won in an avoidance game
            */
        else if (isFixedpointReached) {
            bdd_ptr winStates, zero;

            /* for the player (opponent) it is just necessary to stay in the
               not-reach states */
            winStates = bdd_not(dd_manager, allReachStates);

            for(i=0;i<no;i++)
                trans = GameBddFsm_get_move(fsm, winStates, opponents[i]);

            bdd_and_accumulate(dd_manager, &trans, winStates);


            /* for player 1 GameBddFsm_get_move removes opponent deadlock states.
               Add them back now.
            */
            if (avoidTarget) {

                for(i=0;i<np;i++)
                    bdd_or_accumulate(dd_manager, &trans,
                                      GameBddFsm_without_successor_states(fsm, players[i]));

            }

            zero = bdd_false(dd_manager);

            /* construct the strategy */
            *strategy =
                    AtlGameStrategy_construct(env,
                                           fsm,
                                           opponents, np, players, !avoidTarget,
                            /*initial quantifiers have been reversed
                              => keep them */
                                           (quantifiers ==
                                            opt_game_game_initial_condition(oh)),
                                           zero,
                                           winStates,
                                           trans);

            bdd_free(dd_manager, zero);
            bdd_free(dd_manager, winStates);
        } /* end of else if (isFixedpointReached)*/

        else {
            /* if the target is not reached then the fixpoint is reached always */
            nusmv_assert(false);
        }

    }

    /* end of reach- or avoid- strategy computation */


    /*-----    FREE THE CREATED OBJECTS    --------------------------------------*/

    /* free reachStateList */
    while (reachStateList != Nil) {
        node_ptr n = cdr(reachStateList);
        bdd_free(dd_manager, (bdd_ptr)car(reachStateList));
        free_node(nodemgr,reachStateList);
        reachStateList = n;
    }

    bdd_free(dd_manager, allReachStates);
    bdd_free(dd_manager, originalTarget);

    for(i=0;i<n_players;i++) {
        bdd_free(dd_manager, inits[i]);
        bdd_free(dd_manager, invars[i]);
    }


    return
            ( PropGame_AtlReachTarget == Prop_get_type(PROP(prop))
              || PropGame_AtlReachDeadlock == Prop_get_type(PROP(prop))
            )
            ? (isTargetReached ? GAME_REALIZABLE : GAME_UNREALIZABLE)
            : (isTargetReached ? GAME_UNREALIZABLE : GAME_REALIZABLE);
}


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/
