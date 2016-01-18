/**CFile***********************************************************************

  FileName    [ gameBuildModel.c ]

  PackageName [ game ]

  Synopsis    [ Defines functions to build scalar (flat), boolean and BDD
                models of a game hierarchy. ]

  Description [ ]

  SeeAlso     [ compleCmd.h ]

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

#include <code/nusmv/core/bmc/bmcInt.h>
#include "gameInt.h"
#include "GameHierarchy.h"
#include "PropDbGame.h"
#include "fsm/GameBddFsm.h"
#include "fsm/GameSexpFsm.h"

#include "compile/compile.h"
#include "dd/dd.h"
#include "enc/enc.h"
#include "fsm/FsmBuilder.h"
#include "fsm/bdd/BddFsm.h"
#include "fsm/sexp/SexpFsm.h"
#include "opt/opt.h"
#include "prop/propPkg.h"
#include "set/set.h"
#include "utils/utils.h"
#include "utils/NodeList.h"
#include "utils/error.h"

static char rcsid[] UTIL_UNUSED = "$Id: gameBuildModel.c,v 1.1.2.8 2010-02-10 14:45:31 nusmv Exp $";

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/
EXTERN int nusmv_yylineno;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [ Partly performs the "build_model" command for games. ]

  Description [ This function is used in CommandBuildFlatModel to
                adopt "build_model" command to deal with a game
                declaration. ]

  SideEffects [ ]

  SeeAlso     [ compile_create_flat_model, Game_CommandBuildBddModel ]

******************************************************************************/
void Game_CommandBuildFlatModel(NuSMVEnv_ptr env)
{
  if (!NuSMVEnv_has_value(env, ENV_SEXP_FSM)) {

    SymbTable_ptr st;
    SymbLayer_ptr model_layers[n_players];
    GameSexpFsm_ptr scalar_fsm;
    Set_t set, sets[n_players];
    SymbLayerIter iters[n_players];
    SymbTableIter titer;
    int i;
    FlatHierarchy_ptr hierarchies[n_players];

    st = SYMB_TABLE(NuSMVEnv_get_value(env, ENV_SYMB_TABLE));

    for(i=0;i<n_players;i++) {
        char str[50];
        sprintf(str,"layer_of_PLAYER_%d",i+1);
        model_layers[i] = SymbTable_get_layer(st, str);
    }



     /*NEW_CODE_START*/
     SymbTable_gen_iter(st,&titer,STT_VAR);
     set = SymbTable_iter_to_set(st, titer);

     for(i=0;i<n_players;i++) {

         SymbLayer_gen_iter(model_layers[i], &iters[i], STT_VAR);
         sets[i] = SymbLayer_iter_to_set(model_layers[i], iters[i]);

         hierarchies[i] = GameHierarchy_get_player(mainGameHierarchy,i);
     }

     /*NEW_CODE_END*/

     /*OLD_CODE_START
    set = Set_Make(NodeList_to_node_ptr(SymbTable_get_vars(st)));
    set1 =
      Set_Make(NodeList_to_node_ptr(SymbLayer_get_all_vars(model_layers[0])));
    set2 =
      Set_Make(NodeList_to_node_ptr(SymbLayer_get_all_vars(model_layers[1])));
      *OLD_CODE_END*/
    scalar_fsm =
      GameSexpFsm_create(/* we assume that symbol table contains only variables
                            from the game */
                        env,
                         set,
                        hierarchies,
                         sets);

    NuSMVEnv_set_value(env, ENV_SEXP_FSM, scalar_fsm);

    for(i=1;i>=0;i--)
          Set_ReleaseSet(sets[i]);

    Set_ReleaseSet(set);
  }
}

/**Function********************************************************************

  Synopsis    [ Performs the "build_boolean_model" command for games. ]

  Description [ This function is used in CommandBuildBooleanModel to
                adopt "build_boolean_model" command to deal with a
                game declaration.

                New determinization layers are created and committed
                to both the boolean and bdd encoding. Note that these
                layers cannot have any variables and are added only
                for compatability with corresponding functions dealing
                with usual (not-game) models. ]

  SideEffects [ ]

  SeeAlso     [ compile_create_boolean_model, Game_CommandBuildFlatModel ]

******************************************************************************/
void Game_CommandBuildBooleanModel(NuSMVEnv_ptr env)
{
    if (!NuSMVEnv_has_value(env, ENV_BOOL_FSM)) {

    GameSexpFsm_ptr scalar_fsm;
    GameSexpFsm_ptr bool_fsm;
    SymbTable_ptr st;
    SymbLayer_ptr determ_layers[n_players];

    const ErrorMgr_ptr errmgr = ERROR_MGR(NuSMVEnv_get_value(env, ENV_ERROR_MANAGER));
    BddEnc_ptr  bdd_enc = BDD_ENC(NuSMVEnv_get_value(env, ENV_BDD_ENCODER));
    BoolEnc_ptr bool_enc = BOOL_ENC(NuSMVEnv_get_value(env, ENV_BOOL_ENCODER));

    int reord_status,i;
    dd_reorderingtype rt;
    DDMgr_ptr dd;

    /* temporary disables reordering */
    dd = BddEnc_get_dd_manager(bdd_enc);
    reord_status = dd_reordering_status(dd, &rt);
    if (reord_status == 1) { dd_autodyn_disable(dd); }

    /* create determinization layers */
    st = SYMB_TABLE(NuSMVEnv_get_value(env, ENV_SYMB_TABLE));

    for(i=0;i<n_players;i++) {
        char str[50];
        sprintf(str, "determ_layer_of_PLAYER_%d", i + 1);
        determ_layers[i] = SymbTable_create_layer(st,
                                                  str,
                                                  SYMB_LAYER_POS_BOTTOM);
    }

    /* convert existing scalar FSM to the boolean FSM */
    scalar_fsm = GAME_SEXP_FSM(NuSMVEnv_get_value(env, ENV_SEXP_FSM));

    bool_fsm = GameSexpFsm_scalar_to_boolean(scalar_fsm,
                                             bdd_enc,
                                             determ_layers);

      NuSMVEnv_set_value(env, ENV_BOOL_FSM, bool_fsm);

    /* commits layers to the encodings */
    for(i=0;i<n_players;i++) {
        char str[50];
        sprintf(str, "determ_layer_of_PLAYER_%d", i + 1);
        BaseEnc_commit_layer(BASE_ENC(bool_enc),
                             str);
        BaseEnc_commit_layer(BASE_ENC(bdd_enc),
                             str);
    }

     /*NEW_CODE_START*/
     SymbLayerIter iters[n_players];
     NodeList_ptr symss[n_players];
     bool expr = false;

    for(i=0;i<n_players;i++) {
         SymbLayer_gen_iter(determ_layers[i], &iters[i], STT_ALL);
         symss[i] = SymbLayer_iter_to_list(determ_layers[i], iters[i]);
        expr |= NodeList_get_length(symss[i])!= 0;
    }


     /* determinization variables are not supported in Game */
     if (expr)
     {
        nusmv_yylineno = 0; /* for error messages */
        ErrorMgr_rpterr(errmgr,"determinization variables are not supported by realizability "
                       "algorithms, \nbut were created during booleanisation\n"
                       "(check ASSIGN with boolean on the left and boolean-set on the "
                       "right)\n");
     }
     /*NEW_CODE_END*/

     /*OLD_CODE_START
    /*
    if (NodeList_get_length(SymbLayer_get_all_symbols(determ_layers[0])) != 0 ||
        NodeList_get_length(SymbLayer_get_all_symbols(determ_layers[1])) != 0) {
      nusmv_yylineno = 0;
      ErrorMgr_rpterr(errmgr,"determinization variables are not supported by realizability "
             "algorithms, \nbut were created during booleanisation\n"
             "(check ASSIGN with boolean on the left and boolean-set on the "
             "right)\n");
    }
      OLD_CODE_END*/
  } /* if */
}

/**Function********************************************************************

  Synopsis    [ Partly performs the "build_model" command for games. ]

  Description [ This function is used in CommandBuildModel to adopt
                "build_model" command to deal with a game declaration. ]

  SideEffects [ ]

  SeeAlso     [ CommandBuildModel, Game_CommandBuildFlatModel ]

******************************************************************************/
void Game_CommandBuildBddModel(NuSMVEnv_ptr env)
{
  PropDbGame_ptr propDbGame = PROP_DB_GAME(NuSMVEnv_get_value(env, ENV_PROP_DB));

  SymbTable_ptr st = SYMB_TABLE(NuSMVEnv_get_value(env, ENV_SYMB_TABLE));
  GameSexpFsm_ptr scalar_fsm = GAME_SEXP_FSM(NuSMVEnv_get_value(env, ENV_SEXP_FSM));
  OptsHandler_ptr opts = OPTS_HANDLER(NuSMVEnv_get_value(env, ENV_OPTS_HANDLER));
  FsmBuilder_ptr builder = FSM_BUILDER(NuSMVEnv_get_value(env, ENV_FSM_BUILDER));
  BddEnc_ptr bdd_enc = BDD_ENC(NuSMVEnv_get_value(env, ENV_BDD_ENCODER));
  SymbLayer_ptr model_layers[n_players];
  int i;

  for(i=0;i<n_players;i++) {
      char str[50];
      sprintf(str,"layer_of_PLAYER_%d",i+1);
      model_layers[i] = SymbTable_get_layer(st, str);
  }

  GameBddFsm_ptr bdd_fsm =
    Game_CreateGameBddFsm(builder,
                          bdd_enc,
                          scalar_fsm,
                          model_layers,
                          get_partition_method(opts));

    NuSMVEnv_set_value(env, ENV_BDD_FSM, bdd_fsm);
}

/**Function********************************************************************

  Synopsis    [ Creates a GameBddFsm instance from a given GameSexpFsm. ]

  Description [ Very similar to FsmBuilder_create_bdd_fsm, but creates
                a Game BDD FSM instead of usual BDD FSM. The main
                differences of Game BDD FSM from usual BDD FSM are:
                1. A game consist of two BDD FSM (one for each
                   player).
                2. There cannot be input variables.
                3. Variables are divided into two sets (one for every
                   player) and their cubes are also kept separately.

                Parameters layers[0] and layers[1] are model layers, which
                contain all the (state) variables of the first and
                second player, respectively. It is presumed that the
                system does not have any other variables. ]

  SideEffects [ ]

  SeeAlso     [ FsmBuilder_create_bdd_fsm ]

******************************************************************************/
GameBddFsm_ptr Game_CreateGameBddFsm(const FsmBuilder_ptr self,
                                     BddEnc_ptr enc,
                                     const GameSexpFsm_ptr sexp_fsm,
                                     const SymbLayer_ptr *layers,
                                     const TransType trans_type)
{
  DDMgr_ptr dd;
  bdd_ptr one;
  SexpFsm_ptr players[n_players];
  GameBddFsm_ptr gameFsm;
  BddFsm_ptr bddfsms[n_players];
  BddVarSet_ptr curVarsCubes[n_players],
    nextVarsCubes[n_players],
    curFrozVarsCubes[n_players];

    int i;
    bool expr = true;

  FSM_BUILDER_CHECK_INSTANCE(self);
  BDD_ENC_CHECK_INSTANCE(enc);
  GAME_SEXP_FSM_CHECK_INSTANCE(sexp_fsm);

  for(i=0;i<n_players;i++)
    SYMB_LAYER_CHECK_INSTANCE(layers[i]);

  dd = BddEnc_get_dd_manager(enc);
  one = bdd_true(dd);

  for(i=0;i<n_players;i++)
    players[i] = GameSexpFsm_get_player(sexp_fsm,i);

  /* ---------------------------------------------------------------------- */
  /* Players variables cubes constraction                                   */
  /* ---------------------------------------------------------------------- */

  /* Game cannot have input variables */

    for(i=0;i<n_players;i++)
        expr &= (0 == SymbLayer_get_input_vars_num(layers[i])) ;

   nusmv_assert(expr);

   /*OLD_CODE_START
  nusmv_assert(0 == NodeList_get_length(SymbLayer_get_input_vars(layers[0])) &&
               0 == NodeList_get_length(SymbLayer_get_input_vars(layers[1])));
  OLD_CODE_END*/

  /* create a cube of current state, next state and
     both state and frozen variables for player 1 */
    for(i=0;i<n_players;i++)
  curVarsCubes[i] = BddEnc_get_layer_vars_cube(enc, layers[i], VFT_CURRENT);

    for(i=0;i<n_players;i++)
  nextVarsCubes[i] = BddEnc_state_var_to_next_state_var(enc, curVarsCubes[i]);

    for(i=0;i<n_players;i++)
  curFrozVarsCubes[i] = BddEnc_get_layer_vars_cube(enc, layers[i], VFT_CURR_FROZEN);

  /* ---------------------------------------------------------------------- */
  /* Construct BDD FSM for players and then Game BDD FSM                    */
  /* ---------------------------------------------------------------------- */

    for(i=0;i<n_players;i++)
  bddfsms[i] = FsmBuilder_create_bdd_fsm_of_vars(self,
                                               players[i],
                                               trans_type,
                                               enc,
                                               curVarsCubes[i],
                                               one,
                                               nextVarsCubes[i]);

  gameFsm = GameBddFsm_create(enc,
                              bddfsms, curVarsCubes, curFrozVarsCubes);
    for(i=0;i<n_players;i++) {
      bdd_free(dd, curVarsCubes[i]);
      bdd_free(dd, nextVarsCubes[i]);
      bdd_free(dd, curFrozVarsCubes[i]);
    }

  bdd_free(dd, one);

  return gameFsm;
}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/
