/**CFile***********************************************************************

  FileName    [ gameGeneral.c ]

  PackageName [ game ]

  Synopsis    [ Auxiliary functions needed by game, but not strongly
                related to any particular part of game. ]

  Description [ ]

  SeeAlso     [ game.h ]

  Author      [ Andrei Tchaltsev, Viktor Schuppan ]

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
#include "GameStrategy.h"
#include "PropGame.h"
#include "fsm/GameBddFsm.h"

#include "compile/compile.h"
#include "enc/enc.h"
#include "node/node.h"
#include "opt/opt.h"
#include "prop/propPkg.h"
#include "utils/utils.h"
#include "utils/array.h"
#include "utils/error.h"
#include "utils/NodeList.h"
#include <stdio.h>

/*---------------------------------------------------------------------------*/
static char rcsid[] UTIL_UNUSED = "$Id: gameGeneral.c,v 1.1.2.9 2010-02-05 17:19:08 nusmv Exp $";
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/



/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/
static void game_print_prop_exp ARGS((OStream_ptr file, PropGame_ptr prop));

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [ Preparation for checking a game specification. ]

  Description [ Preparation consists of:
                - printing spec if required,
                - performing COI, and
                - creating FSM. ]

  SideEffects [ ]

  SeeAlso     [ Game_AfterCheckingSpec ]

******************************************************************************/
void Game_BeforeCheckingSpec(PropGame_ptr prop)
{
  GameBddFsm_ptr fsm;

  NuSMVEnv_ptr env = EnvObject_get_environment(ENV_OBJECT(prop));
  OptsHandler_ptr opts = OPTS_HANDLER(NuSMVEnv_get_value(env, ENV_OPTS_HANDLER));
  StreamMgr_ptr streams = STREAM_MGR(NuSMVEnv_get_value(env, ENV_STREAM_MANAGER));
  OStream_ptr errostream = StreamMgr_get_error_ostream(streams);

  PROP_GAME_CHECK_INSTANCE(prop);

  if (opt_verbose_level_gt(opts, 0)) {
    OStream_printf(errostream, "computing ");
    game_print_prop_exp(errostream, prop);
    OStream_printf(errostream, "\n");
  }

  if (opt_cone_of_influence(opts) == true) {
    /*
       The goal states also include opponent deadlock states =>
       variables from deadlock states should also be considered in the
       dependencies.

       The variables for COI should be taken from deadlock states,
       i.e. states with no successors(in BDD). Then if bdd_support of
       a var intersect with deadlock => create arbitrary expr with
       this var and so on for all var. Then invoke standard
       Prop_apply_coi_for_bdd
    */
    /*Prop_apply_coi_for_bdd(prop, global_fsm_builder);*/
  }

  fsm = PropGame_get_game_bdd_fsm(prop);
  if (fsm == GAME_BDD_FSM(NULL)) {
    Prop_set_environment_fsms(env, PROP(prop));
    fsm = PropGame_get_game_bdd_fsm(prop);
    GAME_BDD_FSM_CHECK_INSTANCE(fsm);
  }
}

/**Function********************************************************************

  Synopsis    [ Printing and cleaning after checking a game
                specification. ]

  Description [ This consists of:
                - printing a message of success or failure,
                - printing strategy if required,
                - setting prop status, and
                - freeing the strategy.

                Parameters:
                - prop: property to print
                - status: status of a property (realizable,
                  unrealizable, unknown)
                - strategy: strategy to print (can be NULL)
                - varList1: a list of additional variables which will
                  be considered as belonging to player 1 during
                  printing.  For example, these vars could have been
                  created just to construct the strategy. Normal vars
                  of player 1 will be printed in any case. Members
                  will be declared in the strategy module.
                - varList2: the same as varList1 but for player 2. ]

  SideEffects [ ]

  SeeAlso     [ Game_BeforeCheckingSpec ]

******************************************************************************/
void Game_AfterCheckingSpec(PropGame_ptr prop,
                            Game_RealizabilityStatus status,
                            GameStrategy_ptr strategy,
                            node_ptr *varLists,
                            gameParams_ptr params)

{
  boolean has_params;

  NuSMVEnv_ptr env = EnvObject_get_environment(ENV_OBJECT(prop));
  const ErrorMgr_ptr errmgr = ERROR_MGR(NuSMVEnv_get_value(env, ENV_ERROR_MANAGER));
  OptsHandler_ptr opts = OPTS_HANDLER(NuSMVEnv_get_value(env, ENV_OPTS_HANDLER));
  StreamMgr_ptr streams = STREAM_MGR(NuSMVEnv_get_value(env, ENV_STREAM_MANAGER));
  OStream_ptr errostream = StreamMgr_get_error_ostream(streams);
  OStream_ptr outostream = StreamMgr_get_output_ostream(streams);

  PROP_GAME_CHECK_INSTANCE(prop);

  has_params = ((gameParams_ptr) NULL != params);

  /* ----------  Print the result ----------- */
  OStream_printf(outostream, "-- ");
  game_print_prop_exp(outostream, prop);

  /* strategy reached */
  switch (status) {
  case GAME_REALIZABLE:
    Prop_set_status(PROP(prop), Prop_True);
    OStream_printf(outostream, " : the strategy has been found\n");
    break;
  case GAME_UNREALIZABLE:
    Prop_set_status(PROP(prop), Prop_False);
    OStream_printf(outostream, " : no strategy exists\n");
    break;
  case GAME_UNKNOWN:
    /* status should remain unknown */
    nusmv_assert(Prop_Unchecked == Prop_get_status(PROP(prop)));
    OStream_printf(outostream, " : existence of a strategy is unknown\n");
    break;
  default: ErrorMgr_internal_error(errmgr,"unknown status of a problem");
  }

  /* print a strategy for a player or an opponent */
  if (((has_params && params->strategy_printout)
       || opt_game_print_strategy(opts)) &&
      ((status == GAME_REALIZABLE) ||
       (status == GAME_UNREALIZABLE))) {

    if (GAME_STRATEGY(NULL) != strategy) {
      array_t* layers;
      BddEnc_ptr enc;
      SymbTable_ptr st;
      NodeList_ptr vars;
      NodeList_ptr vars_to_decl;
      NodeList_ptr varss[n_players];
      int i;
      char str[50];

      GAME_STRATEGY_CHECK_INSTANCE(strategy);

      /* Symbol table local reference */
      enc = GameStrategy_get_bdd_enc(strategy);
      st = BaseEnc_get_symb_table(BASE_ENC(enc));

      /* extract variables from game layers */
      layers = array_alloc(char *, n_players*2);

      for(i=0;i<n_players;i++) {
        sprintf(str, "layer_of_PLAYER_%d", i + 1);
        array_insert_last(char *, layers, str);
      }
      /* There shouldnt be any variables in the determinization layers. */
      {

        SymbLayer_ptr dls;
        SymbLayerIter iters;
        NodeList_ptr symss;

        for(i=0;i<n_players;i++) {
          sprintf(str, "determ_layer_of_PLAYER_%d", i + 1);
          dls = SymbTable_get_layer(st, str);
          /**OLD_CODE_START
          nusmv_assert((dl1 == SYMB_LAYER(NULL)) ||
                       (NodeList_get_length(SymbLayer_get_all_symbols(dl1)) ==
                        0));
          nusmv_assert((dl2 == SYMB_LAYER(NULL)) ||
                       (NodeList_get_length(SymbLayer_get_all_symbols(dl2)) ==
                        0));
          OLD_CODE_END**/

          /**NEW_CODE_START**/

          SymbLayer_gen_iter(dls, &iters, STT_ALL);
          symss = SymbLayer_iter_to_list(dls, iters);
        
          //OStream_printf(outostream, "Hi\n");
          nusmv_assert((dls == SYMB_LAYER(NULL)) ||
                       (NodeList_get_length(symss) ==
                        0));
        }
        /**NEW_CODE_END**/
      }

      /* obtain variables */
      vars = SymbTable_get_layers_sf_i_vars(st, layers);
      vars_to_decl = NodeList_create();
      
      for(i=0;i<n_players;i++) {

        varss[i] = NodeList_create_from_list(varLists[i]);

        NodeList_concat(vars, varss[i]);
        NodeList_concat(vars_to_decl, varss[i]);
      }
      
      GameStrategy_print_module(strategy, vars, vars_to_decl, params);

      /* free local variables */
      NodeList_destroy(vars);
      NodeList_destroy(vars_to_decl);
      
      for(i=0;i<n_players;i++)
        NodeList_destroy(varss[i]);
      
      array_free(layers);

      /* free strategy */
      GameStrategy_destroy(strategy);
    } else {
      OStream_printf(errostream,
              "\nWarning: strategy printing requested, but strategy = NULL.\n");
    }
  }

  OStream_printf(outostream,"\n");
  return;
}

/**Function********************************************************************

  Synopsis    [ const char* to Game_Who ]

  Description [ Caller retains ownership of passed string. ]

  SideEffects [ None. ]

  SeeAlso     [ ]

******************************************************************************/
Game_Who Game_Who_from_string(const char* s)
{
  int i;

  if (strcmp(s, GAME_WHO_NONE_STRING) == 0) {
    return GAME_WHO_NONE;
  } else if (strcmp(s, GAME_WHO_BOTH_STRING) == 0) {
    return GAME_WHO_BOTH;
  } else if (strcmp(s, GAME_WHO_PROTAGONIST_STRING) == 0) {
    return GAME_WHO_PROTAGONIST;
  } else if (strcmp(s, GAME_WHO_ANTAGONIST_STRING) == 0) {
    return GAME_WHO_ANTAGONIST;
  } else if (strcmp(s, GAME_WHO_WINNER_STRING) == 0) {
    return GAME_WHO_WINNER;
  } else if (strcmp(s, GAME_WHO_LOSER_STRING) == 0) {
    return GAME_WHO_LOSER;
  } else {
    return GAME_WHO_INVALID;
  }

  for(i=0;i<n_players;i++) {
    char str[50];
    sprintf(str, "%d", i + 1);

    if (strcmp(s, str) == 0) {
      return (i + 1);
    }
  }

    nusmv_assert(false);
}

/**Function********************************************************************

  Synopsis    [ Game_Who to const char*. ]

  Description [ Returned string is statically allocated and must not be
                freed. ]

  SideEffects [ None. ]

  SeeAlso     [ ]

******************************************************************************/
const char* Game_Who_to_string(const Game_Who w)
{
  int i;

  switch (w) {
  case GAME_WHO_INVALID:     return GAME_WHO_INVALID_STRING;
  case GAME_WHO_NONE:        return GAME_WHO_NONE_STRING;
  case GAME_WHO_BOTH:        return GAME_WHO_BOTH_STRING;
  case GAME_WHO_PROTAGONIST: return GAME_WHO_PROTAGONIST_STRING;
  case GAME_WHO_ANTAGONIST:  return GAME_WHO_ANTAGONIST_STRING;
  case GAME_WHO_WINNER:      return GAME_WHO_WINNER_STRING;
  case GAME_WHO_LOSER:       return GAME_WHO_LOSER_STRING;
  default:

      for (i = 0; i < n_players; i++) {
        if(w==i+1) {
          char *str;
          str = (char*)malloc(sizeof(char)*50);
          sprintf(str,"%d",i+1);
          return str;
        }
      }
      nusmv_assert(false);
    }
}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis           [ Prints the property. ]

  Description        [ Prints the kind of the property and its expression. ]

  SideEffects        []

  SeeAlso            []

******************************************************************************/
static void game_print_prop_exp(OStream_ptr file, PropGame_ptr prop)
{
  OStream_printf(file, " ");
  Prop_print(PROP(prop), file, PROP_PRINT_FMT_FORMULA);
}
