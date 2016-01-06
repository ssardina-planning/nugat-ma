/**CFile***********************************************************************

  FileName    [ gameUnrealCore.c ]

  PackageName [ game ]

  Synopsis    [ This class is implements unrealizable core computation of
                a realizability problem. ]

  Description [ A first algorithm for computation of an unrealizable
                core is based on introduction of guarding parameters
                l_i for every INIT, TRANS, INVAR and temporal
                subformulas of a specification. Then a standard
                algorithm is used to compute winning states. State
                variables of both players are then abstracted from the
                winning states such that only parameters l_i are
                remained in the set. The negation of this set is
                actually a set of all possible assignment to l_i that
                make the original formula unrealiable. Then it is just
                necessary to find an assignment with least number of
                l_i set to true to find a minimal unrealizable core.

                A second algorithm removes constraints one by one and
                rechecks the modified problem.

                See also A. Cimatti, M. Roveri, V. Schuppan,
                A. Tchaltsev: Diagnostic Information for
                Realizability. VMCAI'08. ]

  SeeAlso     [ ]

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
#include "GamePlayer.h"
#include "PropGame.h"
#include "fsm/GameSexpFsm.h"
#include "fsm/GameBddFsm.h"
#include "parser/game_symbols.h"

#include "compile/compile.h"
#include "enc/enc.h"
#include "fsm/FsmBuilder.h"
#include "node/node.h"
#include "opt/opt.h"
#include "parser/symbols.h"
#include "prop/propPkg.h"
#include "prop/Prop_private.h"
#include "set/set.h"
#include "utils/utils.h"
#include "utils/assoc.h"
#include "utils/error.h"
#include "utils/ustring.h"
#include "utils/NodeList.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*---------------------------------------------------------------------------*/
static char rcsid[] UTIL_UNUSED = "$Id: gameUnrealCore.c,v 1.1.2.4 2010-01-11 15:07:53 nusmv Exp $";
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/**Type************************************************************************

  Synopsis    [ Definition of the public accessor. ]

  Description [ ]

  SeeAlso     [ ]

******************************************************************************/
typedef struct Game_UnrealizableCore_Struct_TAG*
  Game_UnrealizableCore_Struct_ptr;

/**Type************************************************************************

  Synopsis    [ This struct constains all FSMs required for a
                realizability check. ]

  Description [ ]

  SeeAlso     [ ]

******************************************************************************/
typedef struct GameGameFsms_TAG* GameGameFsms_ptr;

/**Type************************************************************************

  Synopsis    [ Functions of this type are used in
                game_minimize_players_constraints as a check that the
                game still has the required characteristics. ]

  Description [ A function of this type may check, for example, that
                the set of guarantees is still minimally
                unfulfillable.

                The parameters are exactly the same as those of
                game_minimize_players_constraints (except just_check
                and fun). ]

  SeeAlso     [ ]

******************************************************************************/
typedef boolean (*game_is_game_still_correct) (
                                          Game_UnrealizableCore_Struct_ptr self,
                                                    GameBddFsm_ptr fsm,
                                                    int playerToModify,
                                                    node_ptr property,
                                                    boolean realizable,
                                                    bdd_ptr winningStates);

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

/**Struct**********************************************************************

  Synopsis    [ Store input, result, and state information for
                unrealizable core algorithms. ]

  Description [ Global variables

                  oh: The options handler. Not owned.

                  st: The symbol table. Not owned.

                  bool_enc: The Boolean encoder. Not owned.

                  bdd_encc: The BDD encoder. Not owned.

                  dd_manager: The DD manager. Not owned.

                  gh: The mainGameHierarchy. Not owned.

                Input parameters

                  prop: The property to be minimized. Not owned.

                  algo: The algorithm to be used.

                  ct: The type of core to be computed.

                  min_init: True if INIT constraints are to be
                    minimized.

                  min_invar: True if INVAR constraints are to be
                    minimized.

                  min_trans: True if TRANS constraints are to be
                    minimized.

                  min_prop: True if the property is to be minimized.

                  w: Whom to minimize.

                  N: How many constraints are covered by a single
                    activation variable.

               Other variables

                 player: The player in prop.

                 assign_hash1: For backing up the assign_hash of players[0].

                 assign_hash2: For backing up the assign_hash of players[1].

               Used only by the explicit algorithm

                 init1:  For backing up INIT of players[0].

                 invar1:  For backing up INVAR of players[0].

                 trans1:  For backing up TRANS of players[0].

                 init2:  For backing up INIT of players[1].

                 invar2:  For backing up INVAR of players[1].

                 trans2:  For backing up TRANS of players[1].

               Used only by the activation variables algorithm

                 layer: For the activation variables.

                 parameterList_1: A list of all parameters introduced
                   for the player 1. The names are fully resolved. Set
                   up by game_guard_game_hierarchy_with_parameters.

                 parameterList_2: A list of all parameters introduced
                   for the player 2. The names are fully resolved. Set
                   up by game_guard_game_hierarchy_with_parameters.

                 parameterList_all: A list of all parameters
                   introduced for both player 1 and player 2. The list
                   is a concatenation of parameterList_1 and
                   parameterList_2. The names are fully resolved. Set
                   up by
                   game_guard_game_hierarchy_with_parameters. Using a
                   node_ptr rather than a NodeList_ptr for now turns
                   out to be slightly more convenient.

                 parameter2expression: A hash table of a (fully
                   resolved) parameter name and the expression it
                   guards. The expression is the same obtained after
                   flattening. Set up by
                   game_guard_game_hierarchy_with_parameters.

                 constraints_1_total_num: Total player 1 constraints.

                 constraints_2_total_num: Total player 2 constraints.

                 constraints_1_guarded_num: Number of player 1
                   constraints guarded by activation vars
                   (parameters).

                 constraints_2_guarded_num: Number of player 2
                   constraints guarded by activation vars
                   (parameters).

                 constraints_1_unique_num: An integer used to create
                   unique names for player 1 activation vars
                   (parameters), i.e. this is the number of activation
                   vars for player 1.

                 constraints_2_unique_num: An integer used to create
                   unique names for player 2 activation vars
                   (parameters), i.e. this is the number of activation
                   vars for player 2. ]

  SeeAlso     [ ]

******************************************************************************/
struct Game_UnrealizableCore_Struct_TAG {
  /* global variables */
  OptsHandler_ptr oh;
  SymbTable_ptr st;
  BoolEnc_ptr bool_enc;
  BddEnc_ptr bdd_enc;
  DDMgr_ptr dd_manager;
  GameHierarchy_ptr gh;

  /* input */
  PropGame_ptr prop;
  Game_UnrealizableCore_Algorithm algo;
  Game_UnrealizableCore_CoreType ct;
  boolean min_init;
  boolean min_invar;
  boolean min_trans;
  boolean min_prop;
  Game_Who w;
  int N;

  int player;

  /* partial copies of FlatHierarchies */
  hash_ptr *assign_hashs;

  /* specific to explicit algorithm */
  node_ptr *inits;
  node_ptr *invars;
  node_ptr *transs;

  /* specific to activation variables */
  SymbLayer_ptr layer;
  NodeList_ptr *parameterLists;
  /*node_ptr*/NodeList_ptr parameterList_all;
  hash_ptr parameter2expression;
  int *constraintss_total_num;
  int *constraintss_guarded_num;
  int *constraintss_unique_num;
};
typedef struct Game_UnrealizableCore_Struct_TAG Game_UnrealizableCore_Struct;

/**Type************************************************************************

  Synopsis    [ This struct constains all FSMs required for a
                realizability check. ]

  Description [ ]

  SeeAlso     [ ]

******************************************************************************/
typedef struct GameGameFsms_TAG {
  GameSexpFsm_ptr sexp;
  GameBddFsm_ptr bdd;
} GameGameFsms_TAG;
typedef struct GameGameFsms_TAG GameGameFsms;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**Macro***********************************************************************

  Synopsis    [ To cast and check instances of
                Game_UnrealizableCore_Struct. ]

  Description [ These macros must be used respectively to cast and to
                check instances of Game_UnrealizableCore_Struct. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
#define GAME_UNREALIZABLE_CORE_STRUCT(x) \
  ((Game_UnrealizableCore_Struct_ptr) x)
#define GAME_UNREALIZABLE_CORE_STRUCT_CHECK_INSTANCE(x) \
  ( nusmv_assert(GAME_UNREALIZABLE_CORE_STRUCT(x) != \
                 GAME_UNREALIZABLE_CORE_STRUCT(NULL)) )

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/**Variable*******************************************************************

  Synopsis    [ An integer used to create unique names. Increased upon
                each invocation of
                game_compute_core_using_parameters. ]

  Description [ ]

  SeeAlso     [ ]

******************************************************************************/
static int game_unrealizable_core_unique_num = 0;




/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/
static Game_UnrealizableCore_Struct_ptr Game_UnrealizableCore_Struct_create
(NuSMVEnv_ptr env,
      PropGame_ptr prop,
      Game_UnrealizableCore_Algorithm algo,
      Game_UnrealizableCore_CoreType ct,
      boolean min_init,
      boolean min_invar,
      boolean min_trans,
      boolean min_prop,
      Game_Who w,
      int N);
static void Game_UnrealizableCore_Struct_destroy
(Game_UnrealizableCore_Struct_ptr self);
static void Game_UnrealizableCore_Struct_save_assign_hashes
(Game_UnrealizableCore_Struct_ptr self);
static void Game_UnrealizableCore_Struct_restore_assign_hashes
(Game_UnrealizableCore_Struct_ptr self);
static void Game_UnrealizableCore_Struct_save_flat_hierarchies
(Game_UnrealizableCore_Struct_ptr self);
static void Game_UnrealizableCore_Struct_restore_flat_hierarchies
(NodeMgr_ptr nodemgr,Game_UnrealizableCore_Struct_ptr self);

static node_ptr copy_opposite_list (NodeMgr_ptr nodemgr,node_ptr l);
static void free_opposite_list (NodeMgr_ptr nodemgr,node_ptr l);
static node_ptr opposite_reverse (node_ptr x);

static GameGameFsms_ptr game_construct_game_fsms
(NuSMVEnv_ptr env,Game_UnrealizableCore_Struct_ptr self);
static void game_free_game_fsms
(GameGameFsms_ptr fsm);

static node_ptr game_create_new_param
(Game_UnrealizableCore_Struct_ptr self,
       node_ptr expr,
       int kind,
      int player);
static void game_guard_exprs_by_parameters
(Game_UnrealizableCore_Struct_ptr self,
      node_ptr exprs,
      int kind,
      int player);
static void game_guard_gamespecs_by_parameters
(Game_UnrealizableCore_Struct_ptr self);
static void game_guard_game_hierarchy_with_parameters
(Game_UnrealizableCore_Struct_ptr self);
static void game_unguard_exprs_by_parameters
(NodeMgr_ptr nodemgr,Game_UnrealizableCore_Struct_ptr self,
      node_ptr exprs,
      int kind,
      int player);
static void game_unguard_gamespecs_by_parameters
(NodeMgr_ptr nodemgr,Game_UnrealizableCore_Struct_ptr self);
static void game_unguard_game_hierarchy_with_parameters
(NodeMgr_ptr nodemgr,Game_UnrealizableCore_Struct_ptr self);


static void game_process_unrealizable_core_with_params
(Game_UnrealizableCore_Struct_ptr self,
      GameBddFsm_ptr fsm,
      bdd_ptr winningCore);

static void game_compute_core_using_parameters
(NuSMVEnv_ptr env,Game_UnrealizableCore_Struct_ptr self);

static void game_compute_core_switching_constraints
(Game_UnrealizableCore_Struct_ptr self);

static void game_output_spec_without_params(Game_UnrealizableCore_Struct_ptr self, MasterPrinter_ptr wffprint, FILE* out);

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [ Extracts an (un)realizable core. ]

  Description [ For details see
                gameCmd.c::CommandExtractUnrealizableCore. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
int Game_CheckGameSpecAndComputeCores(NuSMVEnv_ptr env,
                                      NodeMgr_ptr nodemgr,
                                      PropGame_ptr prop,
                                      Game_UnrealizableCore_Algorithm algo,
                                      Game_UnrealizableCore_CoreType ct,
                                      boolean min_init,
                                      boolean min_invar,
                                      boolean min_trans,
                                      boolean min_prop,
                                      Game_Who w,
                                      int N)
{
  Game_UnrealizableCore_Struct_ptr cls;

  OptsHandler_ptr opts = OPTS_HANDLER(NuSMVEnv_get_value(env, ENV_OPTS_HANDLER));
  StreamMgr_ptr streams = STREAM_MGR(NuSMVEnv_get_value(env, ENV_STREAM_MANAGER));
  FILE* errstream = StreamMgr_get_error_stream(streams);

    PROP_GAME_CHECK_INSTANCE(prop);
  nusmv_assert(Prop_get_type(PROP(prop)) == PropGame_GenReactivity);

  bool expr = false;
  int i;

  for(i=0;i<n_players;i++)
    expr |= ( w == i+1 );

  nusmv_assert((algo == GAME_UNREALIZABLE_CORE_ALGORITHM_ACTIVATION_VARIABLES &&
                (ct == GAME_UNREALIZABLE_CORE_CORE_TYPE_CORE ||
                 ct == GAME_UNREALIZABLE_CORE_CORE_TYPE_FIX) &&
                (expr ||
                 w == GAME_WHO_BOTH) &&
                (N >= -1)) ||
               (algo == GAME_UNREALIZABLE_CORE_ALGORITHM_EXPLICIT &&
                (ct == GAME_UNREALIZABLE_CORE_CORE_TYPE_CORE) &&
                (w == GAME_WHO_LOSER ||
                 w == GAME_WHO_BOTH) &&
                (ct == GAME_UNREALIZABLE_CORE_CORE_TYPE_CORE) &&
                (N == -1)));

  if (opt_game_print_strategy(opts)) {
    fprintf(errstream,
            "Strategy computation is not implemented when "
            "realizability/unrealizability core computation is enabled.\n");
    return 1;
  }

  cls = Game_UnrealizableCore_Struct_create(env,
                                            PROP_GAME(prop),
                                            algo,
                                            ct,
                                            min_init,
                                            min_invar,
                                            min_trans,
                                            min_prop,
                                            w,
                                            N);

  switch (algo) {
  case GAME_UNREALIZABLE_CORE_ALGORITHM_ACTIVATION_VARIABLES:
    game_compute_core_using_parameters(env,cls);
    break;
  case GAME_UNREALIZABLE_CORE_ALGORITHM_EXPLICIT:
    game_compute_core_switching_constraints(cls);
    break;
  default:
    nusmv_assert(false);
  }

  Game_UnrealizableCore_Struct_destroy(cls);

  return 0;
}

/**Function********************************************************************

  Synopsis    [ const char* to Game_UnrealizableCore_Algorithm ]

  Description [ Caller retains ownership of passed string. ]

  SideEffects [ None. ]

  SeeAlso     [ ]

******************************************************************************/
Game_UnrealizableCore_Algorithm Game_UnrealizableCore_Algorithm_from_string(
                                                                  const char* s)
{
  if (strcmp(s,
           GAME_UNREALIZABLE_CORE_ALGORITHM_ACTIVATION_VARIABLES_STRING) == 0) {
    return GAME_UNREALIZABLE_CORE_ALGORITHM_ACTIVATION_VARIABLES;
  } else if (strcmp(s, GAME_UNREALIZABLE_CORE_ALGORITHM_EXPLICIT_STRING) == 0) {
    return GAME_UNREALIZABLE_CORE_ALGORITHM_EXPLICIT;
  } else {
    return GAME_UNREALIZABLE_CORE_ALGORITHM_INVALID;
  }
  nusmv_assert(false);
}

/**Function********************************************************************

  Synopsis    [ Game_UnrealizableCore_Algorithm to const char*. ]

  Description [ Returned string is statically allocated and must not be
                freed. ]

  SideEffects [ None. ]

  SeeAlso     [ ]

******************************************************************************/
const char* Game_UnrealizableCore_Algorithm_to_string(
                                        const Game_UnrealizableCore_Algorithm a)
{
  switch (a) {
  case GAME_UNREALIZABLE_CORE_ALGORITHM_INVALID:
    return GAME_UNREALIZABLE_CORE_ALGORITHM_INVALID_STRING;
  case GAME_UNREALIZABLE_CORE_ALGORITHM_ACTIVATION_VARIABLES:
    return GAME_UNREALIZABLE_CORE_ALGORITHM_ACTIVATION_VARIABLES_STRING;
  case GAME_UNREALIZABLE_CORE_ALGORITHM_EXPLICIT:
    return GAME_UNREALIZABLE_CORE_ALGORITHM_EXPLICIT_STRING;
  default:
    nusmv_assert(false);
  }
}

/**Function********************************************************************

  Synopsis    [ const char* to Game_UnrealizableCore_CoreType ]

  Description [ Caller retains ownership of passed string. ]

  SideEffects [ None. ]

  SeeAlso     [ ]

******************************************************************************/
Game_UnrealizableCore_CoreType Game_UnrealizableCore_CoreType_from_string(
                                                                  const char* s)
{
  if (strcmp(s,
           GAME_UNREALIZABLE_CORE_CORE_TYPE_CORE_STRING) == 0) {
    return GAME_UNREALIZABLE_CORE_CORE_TYPE_CORE;
  } else if (strcmp(s, GAME_UNREALIZABLE_CORE_CORE_TYPE_FIX_STRING) == 0) {
    return GAME_UNREALIZABLE_CORE_CORE_TYPE_FIX;
  } else {
    return GAME_UNREALIZABLE_CORE_CORE_TYPE_INVALID;
  }
  nusmv_assert(false);
}

/**Function********************************************************************

  Synopsis    [ Game_UnrealizableCore_CoreType to const char*. ]

  Description [ Returned string is statically allocated and must not be
                freed. ]

  SideEffects [ None. ]

  SeeAlso     [ ]

******************************************************************************/
const char* Game_UnrealizableCore_CoreType_to_string(
                                        const Game_UnrealizableCore_CoreType ct)
{
  switch (ct) {
  case GAME_UNREALIZABLE_CORE_CORE_TYPE_INVALID:
    return GAME_UNREALIZABLE_CORE_CORE_TYPE_INVALID_STRING;
  case GAME_UNREALIZABLE_CORE_CORE_TYPE_CORE:
    return GAME_UNREALIZABLE_CORE_CORE_TYPE_CORE_STRING;
  case GAME_UNREALIZABLE_CORE_CORE_TYPE_FIX:
    return GAME_UNREALIZABLE_CORE_CORE_TYPE_FIX_STRING;
  default:
    nusmv_assert(false);
  }
}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [ Constructor for Game_UnrealizableCore_Struct. ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
static Game_UnrealizableCore_Struct_ptr
Game_UnrealizableCore_Struct_create (NuSMVEnv_ptr env,
                                     PropGame_ptr prop,
                                     Game_UnrealizableCore_Algorithm algo,
                                     Game_UnrealizableCore_CoreType ct,
                                     boolean min_init,
                                     boolean min_invar,
                                     boolean min_trans,
                                     boolean min_prop,
                                     Game_Who w,
                                     int N) {
  Game_UnrealizableCore_Struct_ptr cls;

  PROP_GAME_CHECK_INSTANCE(prop);
  nusmv_assert(Prop_get_type(PROP(prop)) == PropGame_GenReactivity);

  bool expr = false;
  int i;

  for (i = 0; i < n_players; i++)
    expr |= (w == i + 1);

  nusmv_assert((algo == GAME_UNREALIZABLE_CORE_ALGORITHM_ACTIVATION_VARIABLES &&
                (ct == GAME_UNREALIZABLE_CORE_CORE_TYPE_CORE ||
                 ct == GAME_UNREALIZABLE_CORE_CORE_TYPE_FIX) &&
                (expr ||
                 w == GAME_WHO_BOTH) &&
                (N >= -1)) ||
               (algo == GAME_UNREALIZABLE_CORE_ALGORITHM_EXPLICIT &&
                (ct == GAME_UNREALIZABLE_CORE_CORE_TYPE_CORE) &&
                (w == GAME_WHO_LOSER ||
                 w == GAME_WHO_BOTH) &&
                (ct == GAME_UNREALIZABLE_CORE_CORE_TYPE_CORE) &&
                (N == -1)));

  cls = ALLOC(Game_UnrealizableCore_Struct, 1);
  GAME_UNREALIZABLE_CORE_STRUCT_CHECK_INSTANCE(cls);

  cls->oh = OPTS_HANDLER(NuSMVEnv_get_value(env, ENV_OPTS_HANDLER));
  cls->st = SYMB_TABLE(NuSMVEnv_get_value(env, ENV_SYMB_TABLE));
  cls->bool_enc = BoolEncClient_get_bool_enc(BOOL_ENC_CLIENT(NULL));
  cls->bdd_enc = NuSMVEnv_get_value(env, ENV_BDD_ENCODER);
  cls->dd_manager = BddEnc_get_dd_manager(cls->bdd_enc);
  cls->gh = mainGameHierarchy;

  cls->prop = prop;
  cls->algo = algo;
  cls->ct = ct;
  cls->min_init = min_init;
  cls->min_invar = min_invar;
  cls->min_trans = min_trans;
  cls->min_prop = min_prop;
  cls->w = w;
  cls->N = N;

  cls->player = Game_StrToPlayer(env, PropGame_get_player(prop));

  cls->inits = (node_ptr *) malloc(sizeof(node_ptr) * n_players);
  cls->invars = (node_ptr *) malloc(sizeof(node_ptr) * n_players);
  cls->transs = (node_ptr *) malloc(sizeof(node_ptr) * n_players);
  cls->assign_hashs = (hash_ptr *) malloc(sizeof(hash_ptr) * n_players);
  cls->parameterLists = (NodeList_ptr *) malloc(sizeof(NodeList_ptr) * n_players);
  cls->constraintss_total_num = (int *) malloc(sizeof(int) * n_players);
  cls->constraintss_guarded_num = (int *) malloc(sizeof(int) * n_players);
  cls->constraintss_unique_num = (int *) malloc(sizeof(int) * n_players);

  for(i=0;i<n_players;i++) {
    cls->inits[i] = Nil;
    cls->invars[i] = Nil;
    cls->transs[i] = Nil;
    cls->assign_hashs[i] = (hash_ptr) NULL;
    cls->parameterLists[i] = NODE_LIST(NULL);
    cls->constraintss_total_num[i] = 0;
    cls->constraintss_guarded_num[i] = 0;
    cls->constraintss_unique_num[i] = 0;
  }

  cls->layer = SYMB_LAYER(NULL);
  cls->parameterList_all = NODE_LIST(NULL); //Nil;
  cls->parameter2expression = (hash_ptr) NULL;

  return cls;
}

/**Function********************************************************************

  Synopsis    [ Destructor for Game_UnrealizableCore_Struct. ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
static void Game_UnrealizableCore_Struct_destroy(
                                          Game_UnrealizableCore_Struct_ptr self)
{
  int i;

  GAME_UNREALIZABLE_CORE_STRUCT_CHECK_INSTANCE(self);

  for(i=0;i<n_players;i++) {
    nusmv_assert(self->inits[i] == Nil);
    nusmv_assert(self->invars[i] == Nil);
    nusmv_assert(self->transs[i] == Nil);
    nusmv_assert(self->assign_hashs[i] == (hash_ptr) NULL);
    nusmv_assert(self->parameterLists[i] == NODE_LIST(NULL));
  }
  nusmv_assert(self->layer == SYMB_LAYER(NULL));
  nusmv_assert(self->parameterList_all == /*Nil*/NODE_LIST(NULL));
  nusmv_assert(self->parameter2expression == (hash_ptr) NULL);

  FREE(self);
}

/**Function********************************************************************

  Synopsis    [ Saves the assign_hashes. ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ Game_UnrealizableCore_Struct_restore_assign_hashes ]

******************************************************************************/
static void Game_UnrealizableCore_Struct_save_assign_hashes(
                                          Game_UnrealizableCore_Struct_ptr self)
{
  FlatHierarchy_ptr fhs[n_players];
  int i;
  GAME_UNREALIZABLE_CORE_STRUCT_CHECK_INSTANCE(self);

  for(i=0;i<n_players;i++)
    nusmv_assert(self->assign_hashs[i] == (hash_ptr) NULL);

  for(i=0;i<n_players;i++){
    fhs[i] = GameHierarchy_get_player((self->gh),i);
    self->assign_hashs[i] = copy_assoc(FlatHierarchy_get_var_expr_associations(fhs[i]));
  }
}

/**Function********************************************************************

  Synopsis    [ Restores the assign_hashes. ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ Game_UnrealizableCore_Struct_save_assign_hashes ]

******************************************************************************/
static void Game_UnrealizableCore_Struct_restore_assign_hashes(
                                         Game_UnrealizableCore_Struct_ptr self)
{
  FlatHierarchy_ptr fhs[n_players];
  int i;

  GAME_UNREALIZABLE_CORE_STRUCT_CHECK_INSTANCE(self);

  for(i=0;i<n_players;i++)
    nusmv_assert(self->assign_hashs[i] != (hash_ptr) NULL);

  for(i=0;i<n_players;i++)
    fhs[i] = GameHierarchy_get_player((self->gh),i);

  for(i=0;i<n_players;i++) {
      free_assoc(FlatHierarchy_get_var_expr_associations(fhs[i]));
      FlatHierarchy_set_var_expr_associations(fhs[i], self->assign_hashs[i]);
      self->assign_hashs[i] = (hash_ptr) NULL;
  }
}

/**Function********************************************************************

  Synopsis    [ Saves parts of the FlatHierarchies ]

  Description [ Saves:
                - init_expr
                - invar_expr
                - trans_expr
                - assign_hash ]

  SideEffects [ ]

  SeeAlso     [ Game_UnrealizableCore_Struct_restore_flat_hieararchies ]

******************************************************************************/
static void Game_UnrealizableCore_Struct_save_flat_hierarchies(
                                          Game_UnrealizableCore_Struct_ptr self)
{
  FlatHierarchy_ptr fhs[n_players];
  int i;

    const NuSMVEnv_ptr env = EnvObject_get_environment(ENV_OBJECT(self));
    const NodeMgr_ptr nodemgr = NODE_MGR(NuSMVEnv_get_value(env, ENV_NODE_MGR));

  GAME_UNREALIZABLE_CORE_STRUCT_CHECK_INSTANCE(self);

  for(i=0;i<n_players;i++) {
      nusmv_assert(self->inits[i] == Nil);
      nusmv_assert(self->invars[i] == Nil);
      nusmv_assert(self->transs[i] == Nil);
      nusmv_assert(self->assign_hashs[i] == (hash_ptr) NULL);
  }

  for(i=0;i<n_players;i++) {
    fhs[i] = GameHierarchy_get_player((self->gh), i);

    self->inits[i] = FlatHierarchy_get_init(fhs[i]);
    FlatHierarchy_set_init(fhs[i], copy_opposite_list(nodemgr, self->inits[i]));
    self->invars[i] = FlatHierarchy_get_invar(fhs[i]);
    FlatHierarchy_set_invar(fhs[i], copy_opposite_list(nodemgr, self->invars[i]));
    self->transs[i] = FlatHierarchy_get_trans(fhs[i]);
    FlatHierarchy_set_trans(fhs[i], copy_opposite_list(nodemgr, self->transs[i]));
  }

  Game_UnrealizableCore_Struct_save_assign_hashes(self);
}

/**Function********************************************************************

  Synopsis    [ Restores parts of the FlatHierarchies. ]

  Description [ See Game_UnrealizableCore_Struct_save_flat_hieararchies. ]

  SideEffects [ ]

  SeeAlso     [ Game_UnrealizableCore_Struct_save_flat_hieararchies ]

******************************************************************************/
static void Game_UnrealizableCore_Struct_restore_flat_hierarchies(NodeMgr_ptr nodemgr,
                                         Game_UnrealizableCore_Struct_ptr self)
{
  FlatHierarchy_ptr fhs[n_players];
  int i;

  GAME_UNREALIZABLE_CORE_STRUCT_CHECK_INSTANCE(self);
  for(i=0;i<n_players;i++)
    nusmv_assert(self->assign_hashs[i] != (hash_ptr) NULL);

  for(i=0;i<n_players;i++)
    fhs[i] = GameHierarchy_get_player((self->gh),i);

  for(i=0;i<n_players;i++) {
      free_opposite_list(nodemgr,FlatHierarchy_get_init(fhs[i]));
      FlatHierarchy_set_init(fhs[i], self->inits[i]);
      self->inits[i] = Nil;
      free_opposite_list(nodemgr,FlatHierarchy_get_invar(fhs[i]));
      FlatHierarchy_set_invar(fhs[i], self->invars[i]);
      self->invars[i] = Nil;
      free_opposite_list(nodemgr,FlatHierarchy_get_trans(fhs[i]));
      FlatHierarchy_set_trans(fhs[i], self->transs[i]);
      self->transs[i] = Nil;
  }

  Game_UnrealizableCore_Struct_restore_assign_hashes(self);
}

/**Function********************************************************************

  Synopsis    [ Returns a copy of a list in which, contrary to a normal
                lists, the head is on RHS (cdr) and the tail is on LHS
                (car). ]

  Description [ An invoker should free the returned list. ]

  SideEffects [ ]

  SeeAlso     [ free_opposite_list ]

******************************************************************************/
node_ptr copy_opposite_list(NodeMgr_ptr nodemgr,node_ptr list)
{
  node_ptr new_list;

  /* create a reversed copy of the list */
  for (new_list = Nil; list != Nil; list = car(list)) {
    new_list = new_node(nodemgr,node_get_type(list), new_list, cdr(list));
  }

  /* reverse the created list */
  new_list = opposite_reverse(new_list);

  return new_list;
}

/**Function********************************************************************

  Synopsis    [ Frees all the elements of the list in which, opposite to
                a normal lists, the head is on RHS (cdr) and the tail
                is on LHS (car). ]

  Description [ Frees all the elements of the list for further use. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
void free_opposite_list(NodeMgr_ptr nodemgr,node_ptr l) {
  while(l != Nil) {
    node_ptr tmp = l;

    l = car(l);
    free_node(nodemgr,tmp);
  }
}

/**Function********************************************************************

  Synopsis    [ Reverse a list in which, opposite to a normal lists, the
                head is on RHS (cdr) and the tail is on LHS (car). ]

  Description [ Returns a new sequence containing the same elements as
                X but in reverse order. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
node_ptr opposite_reverse(node_ptr x)
{
  node_ptr y=Nil;

  while (x != Nil) {
    node_ptr z = x->left.nodetype;

    x->left.nodetype = y;
    y = x;
    x = z;
  }
  return(y);
}

/**Function********************************************************************

  Synopsis    [ From the game hierarchy mainGameHierarchy constructs
                first a Sexp FSM and then a BDD FSM. ]

  Description [ The constructed FSMs are set as fields of the output
                struct. The invoker is responsible for freeing the
                output struct with game_free_game_fsms. ]

  SideEffects [ ]

  SeeAlso     [ game_free_game_fsms ]

******************************************************************************/
static GameGameFsms_ptr game_construct_game_fsms(NuSMVEnv_ptr env,
                                          Game_UnrealizableCore_Struct_ptr self)
{
  /* FSMs are not created yet */
  GameGameFsms_ptr fsms = ALLOC(GameGameFsms, 1);
  nusmv_assert(fsms != NULL);
  SymbLayerIter iters[n_players];
  SymbTableIter titer;
  SymbLayer_ptr model_layers[n_players];
  int i;
    FlatHierarchy_ptr hierarchies[n_players];

  for(i=0;i<n_players;i++) {
    char str[50];
    sprintf(str, "layer_of_PLAYER_%d", i + 1);
    model_layers[i] = SymbTable_get_layer(self->st, str);
  }

  FsmBuilder_ptr builder = FSM_BUILDER(NuSMVEnv_get_value(env, ENV_FSM_BUILDER));

   /*NEW_CODE_START*/
   SymbTable_gen_iter(self->st,&titer,STT_VAR);
   Set_t set = SymbTable_iter_to_set(self->st, titer);
   Set_t sets[n_players];
   for(i=0;i<n_players;i++){
       SymbLayer_gen_iter(model_layers[i], &iters[i], STT_VAR);
       sets[i] = SymbLayer_iter_to_set(model_layers[i], iters[i]);
   }

   /*NEW_CODE_END*/

   /*OLD_CODE_START
  Set_t set = Set_Make(NodeList_to_node_ptr(SymbTable_get_vars(self->st)));

  Set_t set1 =
    Set_Make(NodeList_to_node_ptr(SymbLayer_get_all_vars(model_layer_1)));
  Set_t set2 =
    Set_Make(NodeList_to_node_ptr(SymbLayer_get_all_vars(model_layer_2)));
    OLD_CODE_END*/
  Set_Freeze(set);
  for(i=0;i<n_players;i++)
    Set_Freeze(sets[i]);

  for(i=0;i<n_players;i++)
    hierarchies[i] = GameHierarchy_get_player(self->gh,i);

  /* We assume that symbol table contains only variables from the
     game. */
  fsms->sexp = GameSexpFsm_create(env,
                                  set,
                                  hierarchies,
                                  sets);

  fsms->bdd = Game_CreateGameBddFsm(builder,
                                    self->bdd_enc,
                                    fsms->sexp,
                                    model_layers,
                                    get_partition_method(self->oh));
  for(i=1;i>=0;i--)
    Set_ReleaseSet(sets[i]);

  Set_ReleaseSet(set);

  return fsms;
}

/**Function********************************************************************

  Synopsis    [ Frees the input struct and its BDD and SEXP FSMs
                constructed by game_construct_game_fsms. ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ game_construct_game_fsms ]

******************************************************************************/
static void game_free_game_fsms(GameGameFsms_ptr fsms)
{
  nusmv_assert((GameGameFsms_ptr) NULL != fsms);
  nusmv_assert(GAME_SEXP_FSM(NULL) != fsms->sexp);
  nusmv_assert(GAME_BDD_FSM(NULL) != fsms->bdd);

  GameBddFsm_destroy(fsms->bdd);
  GameSexpFsm_destroy(fsms->sexp);

  fsms->bdd = GAME_BDD_FSM(NULL);
  fsms->sexp = GAME_SEXP_FSM(NULL);

  FREE(fsms);
}

/**Function********************************************************************

  Synopsis    [ This function creates a new frozen variable with a unique
                name. ]

  Description [ The created var is expected to be used as a parameter
                guarding expressions during unrealizable core
                computation. After creation the name is fully
                resolved. The name is guaranted to be unique. The name
                is added to global lists of players' parametes
                parameterList_1 and parameterList_2. The association
                name->expr is added to hash table
                parameter2expression.

                The created name is returned. ]

  SideEffects [ ]

  SeeAlso     [ game_get_param_exp, game_get_param_exp_kind ]

******************************************************************************/
static node_ptr game_create_new_param(Game_UnrealizableCore_Struct_ptr self,
                                      node_ptr expr,
                                      int kind,
                                      int player)
{
    int i;

    const NuSMVEnv_ptr env = EnvObject_get_environment(ENV_OBJECT(self));
    const ExprMgr_ptr exprs = EXPR_MGR(NuSMVEnv_get_value(env, ENV_EXPR_MANAGER));
    const NodeMgr_ptr nodemgr = NODE_MGR(NuSMVEnv_get_value(env, ENV_NODE_MGR));

    for(i=0;i<n_players;i++)
        if (player == i) ++(self->constraintss_total_num[i]);

  if (self->N == 0) {
    return NULL;
  }
  if ((kind == INIT && !self->min_init) ||
      (kind == INVAR && !self->min_invar) ||
      (kind == TRANS && !self->min_trans) ||
      (((kind == REACHTARGET) ||
        (kind == ATLREACHTARGET) ||
        (kind == AVOIDTARGET) ||
        (kind == REACHDEADLOCK) ||
        (kind == AVOIDDEADLOCK) ||
        (kind == BUCHIGAME) ||
        (kind == LTLGAME) ||
        (kind == GENREACTIVITY)) && !self->min_prop)) {
    return NULL;
  }

  bool bexpr = true;

  for(i=0;i<n_players;i++)
    bexpr &= ( self->w != i+1  || player != i+1 );

  if (self->w != GAME_WHO_BOTH &&
          bexpr) {
    return NULL;
  }

  NodeList_ptr parameterList;
  int* uniqueNum;
  int guardedConstr;

  for(i=0;i<n_players;i++)
        if(i+1 == player) {
            parameterList = self->parameterLists[i];
            uniqueNum = &(self->constraintss_unique_num[i]);
            guardedConstr = self->constraintss_guarded_num[i];
        }

  /* A new activ. var is created for opt_game_unreal_core_N number of
     constraints. */
  boolean is_new_var = (0 == (guardedConstr % self->N));

  if (is_new_var) ++*uniqueNum;

  char name[100]; /* 100 is enough for a name */
  /* the name is with a space => it is always unique */
  snprintf(name,
           100,
           (player == self->player) ? " guarn_%d_%d" : " assmp_%d_%d",
           game_unrealizable_core_unique_num,
           *uniqueNum);

  node_ptr var = find_node(nodemgr,DOT, Nil, sym_intern(env,name));

  if (is_new_var) {
    nusmv_assert(SymbLayer_can_declare_var(self->layer, var));

    SymbType_ptr symbolicType = SymbType_create(env,SYMB_TYPE_ENUM, ExprMgr_boolean_range(exprs));

    SymbLayer_declare_frozen_var(self->layer, var, symbolicType);

    /* Add the var to the player vars list and create association var
       -> expression it guards. */
    NodeList_append(parameterList, var);
  }

  /* Wrap the expression into its high level kind. */
  node_ptr old_exp = find_assoc(self->parameter2expression, var);
  old_exp = cons(nodemgr,new_node(nodemgr,kind, expr, Nil), old_exp);
  insert_assoc(self->parameter2expression, var, old_exp);

        for(i=0;i<n_players;i++)
            if(i+1 == player)
                ++(self->constraintss_guarded_num[i]);

  return var;
}

/**Function********************************************************************

  Synopsis    [ Takes a list of high level constraint (INIT, TRANS and
                INVAR) and guards every expression with a newly
                created parameter. ]

  Description [ For every high level expression exp a frozen variable
                l in the provided layer is created and exp is
                substituted by "l -> exp".

                The modified expression is returned.

                The given expression is expected to be a list of
                expressions obtained after flattening, i.e., to have a
                particular structure. If the format of data is changed
                in the flattener then this function has to be changed
                as well. ]

  SideEffects [ Input expression exprs is modified. ]

  SeeAlso     [ game_unguard_exprs_by_parameters ]

******************************************************************************/
static void game_guard_exprs_by_parameters(Game_UnrealizableCore_Struct_ptr self,
                                           node_ptr exprs,
                                           int kind,
                                           int player)
{
  node_ptr iter;

    const NuSMVEnv_ptr env = EnvObject_get_environment(ENV_OBJECT(self));
    const NodeMgr_ptr nodemgr = NODE_MGR(NuSMVEnv_get_value(env, ENV_NODE_MGR));

  /* The expected format of exprs is a list connected by AND, right
     child is a head, left child is a tail, the last element is
     Nil. */

  for (iter = exprs; iter != Nil; iter = car(iter)) {
    node_ptr param, exp;
    nusmv_assert(AND == node_get_type(iter));
    nusmv_assert(find_atom(nodemgr,iter) != iter); /* the list is not find_node'd */

    exp = cdr(iter);
    param = game_create_new_param(self, exp, kind, player);
    if (param != NULL) {
      exp = new_node(nodemgr,IMPLIES, param, exp);
      setcdr(iter, exp);
    }
  }
}

/**Function********************************************************************

  Synopsis    [ Takes a list of game specifications and guards their
                subexpressions with a newly created parameter. ]

  Description [ For every high level expression exp a frozen variable
                l in the given layer is created and exp is substituted
                by "l -> exp".

                The modified expression is returned.

                kind is the kind of the specification.

                The given expression is expected to be a game
                specification just before being added to the game
                hierarchy and to the property database. If the format
                of data is changed in flattener then this function has
                to be changed as well. ]

  SideEffects [ The input expression may be changed. ]

  SeeAlso     [ game_unguard_gamespecs_by_parameters ]

******************************************************************************/
static void game_guard_gamespecs_by_parameters(
                                          Game_UnrealizableCore_Struct_ptr self)
{

  const NuSMVEnv_ptr env = EnvObject_get_environment(ENV_OBJECT(self));
  const NodeMgr_ptr nodemgr = NODE_MGR(NuSMVEnv_get_value(env, ENV_NODE_MGR));

  node_ptr exp;
  node_ptr param;
  node_ptr rhs;
  int kind;

  /* Get expression of prop. Need to be ugly here b/c the game version
     of Prop_get_expr actually returns the cdr of the prop of an
     Prop_ptr. */
  exp = (PROP(self->prop))->prop;

  /* This is a game spec. */
  nusmv_assert(GAME_SPEC_WRAPPER == node_get_type(exp));

  switch (Prop_get_type(PROP(self->prop))) {
  case PropGame_ReachTarget:
    /* For reachability the parameter is added to the expression as
       implicant. */
    kind = REACHTARGET;
    /* fall through */

  case PropGame_AtlReachTarget:
    /* For reachability the parameter is added to the expression as
       implicant. */
    kind = ATLREACHTARGET;
        /* fall through */

  case PropGame_LtlGame:
    /* Very crude for now: just guard the whole LTL formula with a
       single parameter. */
    /* Check for fall-through from PropGame_ReachTarget. */
    if (Prop_get_type(PROP(self->prop)) == PropGame_LtlGame) {
      kind = LTLGAME;
    }
    param = game_create_new_param(self, cdr(exp), kind, self->player);
    if (param != Nil) {
      rhs = new_lined_node(nodemgr,IMPLIES,
                           param,
                           cdr(exp),
                           node_get_lineno(car(exp)));
      setcdr(exp, rhs);
    }
    break;

  case PropGame_AvoidTarget:
    /* For avoidance the parameter is added to the expressions as
       conjunct. */
    kind = AVOIDTARGET;
    param = game_create_new_param(self, cdr(exp), kind, self->player);
    if (param != Nil) {
      rhs = new_lined_node(nodemgr,AND,
                           param,
                           cdr(exp),
                           node_get_lineno(car(exp)));
      setcdr(exp, rhs);
    }
    break;

  case PropGame_ReachDeadlock:
    /* No expression to guard. */
    kind = REACHDEADLOCK;
    break;

  case PropGame_AvoidDeadlock:
    /* No expression to guard. */
    kind = AVOIDDEADLOCK;
    break;

  case PropGame_BuchiGame:
    /* Guard every Buchi condition with a new parameter. */
    kind = BUCHIGAME;
    nusmv_assert(GAME_EXP_LIST == node_get_type(cdr(exp)));
    {
      node_ptr iter;
      for (iter = car(cdr(exp)); iter; iter = cdr(iter)) {
        node_ptr cond = car(iter);
        param = game_create_new_param(self, cond, kind, self->player);
        if (param != Nil) {
          cond = new_lined_node(nodemgr,IMPLIES,
                                param,
                                cond,
                                node_get_lineno(cond));
          nusmv_assert(iter != find_atom(nodemgr,iter)); /* We are going to
                                                    modify it. */
          setcar(iter, cond);
        }
      }
    }
    break;

  case PropGame_GenReactivity:
    /* Guard every gen-reactivity condition with a new parameter. */
    kind = GENREACTIVITY;
    nusmv_assert(GAME_TWO_EXP_LISTS == node_get_type(cdr(exp)));
    {
      node_ptr lists[] = {car(cdr(exp)), cdr(cdr(exp)), Nil};
      node_ptr* one_list;
      int p = self->player == 1 ? 2 : 1;

      /* The left list is guarded by the parameters of the opponent
         and the right list is guared by the parameters of the
         player. */
      for (one_list = lists; *one_list; ++one_list) {
        node_ptr iter;
        for (iter = *one_list; iter; iter = cdr(iter)) {
          node_ptr cond = car(iter);
          param = game_create_new_param(self, cond, kind, p);
          if (param != Nil) {
            cond = new_lined_node(nodemgr,IMPLIES,
                                  param,
                                  cond,
                                  node_get_lineno(cond));
            /* We are going to modify it. */
            nusmv_assert(iter != find_atom(nodemgr,iter));
            setcar(iter, cond);
          }
        } /* for (iter) */
        p = p == 1 ? 2 : 1; /* Ok as the loop has
                                                    two iterations. */
      } /* for (one_list) */
    }
    break;

  default: nusmv_assert(false); /* impossible code */
  }
}

/**Function********************************************************************

  Synopsis    [ A function takes a game hierarchy and specifications and
                guards every high-level expression (constraint or
                specification) with newly created activation
                variables. ]

  Description [ For every expression exp a new frozen variable l is
                created, and then exp is substituted by "l ->
                exp". The variable is then defined in
                MODEL_LAYER_UNREAL_CORE layer, and added to global
                parameterList.  Global parameter2expression will
                contain association of var name and the exp it guards.

                The names of parameters are added fully resolved.

                This function is expected to be invoked after the
                hierachy is flattened and checked for being correctly
                written (and after the properties are added to the
                property database). ]

  SideEffects [ The provided expressions representing specifications
                and the hierarchy are modified. ]

  SeeAlso     [ game_unguard_game_hierarchy_with_parameters ]

******************************************************************************/
static void
game_guard_game_hierarchy_with_parameters(Game_UnrealizableCore_Struct_ptr self)
{
    int i;

  nusmv_assert(self->layer == SYMB_LAYER(NULL));

  for(i=0;i<n_players;i++)
    nusmv_assert(self->parameterLists[i] == NODE_LIST(NULL));

    nusmv_assert(self->parameterList_all == /*Nil*/NODE_LIST(NULL));
  nusmv_assert(self->parameter2expression == (hash_ptr) NULL);

  FlatHierarchy_ptr players[n_players];

  for(i=0;i<n_players;i++)
      players[i]  = GameHierarchy_get_player((self->gh),i);

  /* Save assign_hashes. */
  Game_UnrealizableCore_Struct_save_assign_hashes(self);

  /* -- at first, initialize global vars and create a new layer */

  for(i=0;i<n_players;i++)
    self->parameterLists[i] = NodeList_create();

  self->parameterList_all = NodeList_create(); //Nil;

  self->parameter2expression = new_assoc();

  self->layer = SymbTable_create_layer(self->st, NULL, SYMB_LAYER_POS_TOP);

    for(i=0;i<n_players;i++) {
        game_guard_exprs_by_parameters(self,
                                       FlatHierarchy_get_init(players[i]),
                                       INIT,
                                       i+1);
        game_guard_exprs_by_parameters(self,
                                       FlatHierarchy_get_invar(players[i]),
                                       INVAR,
                                       i+1);
        game_guard_exprs_by_parameters(self,
                                       FlatHierarchy_get_trans(players[i]),
                                       TRANS,
                                       i+1);
    }


  /* Processes are not implemented for games. */
  nusmv_assert(Nil == cdr(FlatHierarchy_get_assign(players[0])));
  nusmv_assert(Nil == cdr(FlatHierarchy_get_assign(players[1])));

  /* Assignments are not implemented. See warning half a page below! */
  nusmv_assert(Nil == cdr(car(FlatHierarchy_get_assign(players[0]))) &&
               Nil == cdr(car(FlatHierarchy_get_assign(players[1]))));

  /* Standard properties are not implemented for games. */
  nusmv_assert(Nil == GameHierarchy_get_ctlspec(self->gh) &&
               Nil == GameHierarchy_get_ltlspec(self->gh) &&
               Nil == GameHierarchy_get_invarspec(self->gh) &&
               Nil == GameHierarchy_get_pslspec(self->gh) &&
               Nil == GameHierarchy_get_compute(self->gh));

  /* Since the properties in the game hierarchy and in the database
     are the same (the same pointer) we can modify here or there. */
  game_guard_gamespecs_by_parameters(self);

  /* After creating all the parameter create a total list. */

   /*NEW_CODE_START*/
  for(i=0;i<n_players;i++)
    NodeList_concat(self->parameterList_all, self->parameterLists[i]);
   /*NEW_CODE_END*/

   /*OLD_CODE_START
  self->parameterList_all =
    append_ns(NodeList_to_node_ptr(self->parameterList_1),
              NodeList_to_node_ptr(self->parameterList_2));

  OLD_CODE_END*/

  /* Remove the association between variables and expressions they are
     used in. This is necessary to do because expressions in the hash
     tables are without guards and these expressions are used to
     (incorrectly) construct the SexpFsm later.

     Note: this clearing makes COI or any dependencies check
           impossible. Be careful to invoke this function after all
           checkes are done. */
  FlatHierarchy_clear_var_expr_associations(players[0]);
  FlatHierarchy_clear_var_expr_associations(players[1]);

  /* Encode the created vars. */
  BaseEnc_commit_layer(BASE_ENC(self->bool_enc),
                       SymbLayer_get_name(self->layer));
  BaseEnc_commit_layer(BASE_ENC(self->bdd_enc),
                       SymbLayer_get_name(self->layer));

  /* DEBUGGING: OUTPUT ENCODING ORDER */
  //{
  //  BddEnc_write_var_ordering(Enc_get_bdd_encoding(),
  //                            "game_unreal_core.ord",
  //                      opt_write_order_dumps_bits(opts)
  //                            ? DUMP_BITS : DUMP_DEFAULT);
  //}
}

/**Function********************************************************************

  Synopsis    [ Undoes the effects of game_guard_exprs_by_parameters. ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ game_guard_exprs_by_parameters ]

******************************************************************************/
static void game_unguard_exprs_by_parameters(NodeMgr_ptr nodemgr,
                                          Game_UnrealizableCore_Struct_ptr self,
                                             node_ptr exprs,
                                             int kind,
                                             int player)
{
  node_ptr iter;

  bool expr = false;
  int i;

  for(i=0;i<n_players;i++)
    expr |= (self->w == i+1 && player == i+1);

  nusmv_assert(self->N > 0);
  nusmv_assert((kind == INIT && self->min_init) ||
               (kind == INVAR && self->min_invar) ||
               (kind == TRANS && self->min_trans) ||
               (((kind == REACHTARGET) ||
                 (kind == ATLREACHTARGET) ||
                 (kind == AVOIDTARGET) ||
                 (kind == REACHDEADLOCK) ||
                 (kind == AVOIDDEADLOCK) ||
                 (kind == BUCHIGAME) ||
                 (kind == LTLGAME) ||
                 (kind == GENREACTIVITY)) && self->min_prop));
  nusmv_assert(self->w == GAME_WHO_BOTH ||
                       expr);

  /* The expected format of exprs is a list connected by AND, right
     child is a head, left child is a tail, the last element is
     Nil. */

  for (iter = exprs; iter != Nil; iter = car(iter)) {
    node_ptr tmp, rhs;
    nusmv_assert(AND == node_get_type(iter));

    tmp = cdr(iter);
    nusmv_assert(IMPLIES == node_get_type(tmp) &&
                 DOT == node_get_type(car(tmp)) &&
                 ATOM == node_get_type(cdr(car(tmp))) &&
                 ' ' == UStringMgr_get_string_text((string_ptr)car(cdr(car(tmp))))[0]);
    rhs = cdr(tmp);
    setcdr(iter, rhs);
    free_node(nodemgr,tmp);
  }
}

/**Function********************************************************************

  Synopsis    [ Undoes the effects of
                game_guard_gamespecs_by_parameters. ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ game_guard_gamespecs_by_parameters ]

******************************************************************************/
static void game_unguard_gamespecs_by_parameters(NodeMgr_ptr nodemgr,
                                          Game_UnrealizableCore_Struct_ptr self)
{
  node_ptr exp;
  node_ptr expcore;
  node_ptr tmp;
  node_ptr rhs;

  /* Get expression of prop. Need to be ugly here b/c the game version
     of Prop_get_expr actually returns the cdr of the prop of an
     Prop_ptr. */
  exp = (PROP(self->prop))->prop;

  /* This is a game spec. */
  nusmv_assert(GAME_SPEC_WRAPPER == node_get_type(exp));

  expcore = cdr(exp);

  switch (Prop_get_type(PROP(self->prop))) {
  case PropGame_ReachTarget:
    /* For reachability the parameter is added to the expression as
       implicant. */
    /* fall through */
  case PropGame_AvoidTarget:
    /* For avoidance the parameter is added to the expressions as
       conjunct. */
    /* fall through */
  case PropGame_LtlGame:
    /* Very crude for now: just guard the whole LTL formula with a
       single parameter. */
    tmp = expcore;
    nusmv_assert((((Prop_get_type(PROP(self->prop)) == PropGame_ReachTarget ||
                    Prop_get_type(PROP(self->prop)) == PropGame_LtlGame) &&
                   node_get_type(tmp) == IMPLIES) ||
                  (Prop_get_type(PROP(self->prop)) == PropGame_AvoidTarget &&
                   node_get_type(tmp) == AND)) &&
                 node_get_type(car(tmp)) == DOT &&
                 node_get_type(cdr(car(tmp))) == ATOM &&
                 UStringMgr_get_string_text((string_ptr)car(cdr(car(tmp))))[0] == ' ');
    rhs = cdr(expcore);
    setcdr(exp, rhs);
    free_node(nodemgr,tmp);
    break;

  case PropGame_ReachDeadlock:
    /* No expression to guard. */
    /* fall through */
  case PropGame_AvoidDeadlock:
    /* No expression to guard. */
    nusmv_assert(expcore == Nil);
    break;

  case PropGame_BuchiGame:
    /* Guard every Buchi condition with a new parameter. */
    nusmv_assert(GAME_EXP_LIST == node_get_type(expcore));
    {
      node_ptr iter;
      for (iter = car(expcore); iter; iter = cdr(iter)) {
        tmp = car(iter);
        nusmv_assert(IMPLIES == node_get_type(tmp) &&
                     DOT == node_get_type(car(tmp)) &&
                     ATOM == node_get_type(cdr(car(tmp))) &&
                     ' ' == UStringMgr_get_string_text((string_ptr)car(cdr(car(tmp))))[0]);
        rhs = cdr(tmp);
        setcar(iter, rhs);
        free_node(nodemgr,tmp);
      }
    }
    break;

  case PropGame_GenReactivity:
    /* Guard every gen-reactivity condition with a new parameter. */
    nusmv_assert(GAME_TWO_EXP_LISTS == node_get_type(expcore));
    {
      node_ptr iter;
      bool expr = false;
      int i;

      for(i=0;i<n_players;i++)
        expr |= (self->w == i+1 && self->player == n_players-i);

      if (self->w == GAME_WHO_BOTH ||
          expr) {
        for (iter = car(expcore); iter; iter = cdr(iter)) {
          tmp = car(iter);
          nusmv_assert(IMPLIES == node_get_type(tmp) &&
                       DOT == node_get_type(car(tmp)) &&
                       ATOM == node_get_type(cdr(car(tmp))) &&
                       ' ' == UStringMgr_get_string_text((string_ptr)car(cdr(car(tmp))))[0]);
          rhs = cdr(tmp);
          setcar(iter, rhs);
          free_node(nodemgr,tmp);
        } /* for (iter) */
      }

      expr = false;
      for(i=0;i<n_players;i++)
        expr |= (self->w == i+1 && self->player == i+1);

      if (self->w == GAME_WHO_BOTH ||
          expr) {
        for (iter = cdr(expcore); iter; iter = cdr(iter)) {
          tmp = car(iter);
          nusmv_assert(IMPLIES == node_get_type(tmp) &&
                       DOT == node_get_type(car(tmp)) &&
                       ATOM == node_get_type(cdr(car(tmp))) &&
                       ' ' == UStringMgr_get_string_text((string_ptr)car(cdr(car(tmp))))[0]);
          rhs = cdr(tmp);
          setcar(iter, rhs);
          free_node(nodemgr,tmp);
        } /* for (iter) */
      }
    }
    break;

  default: nusmv_assert(false); /* impossible code */
  }
}

/**Function********************************************************************

  Synopsis    [ Undoes the effects of
                game_guard_game_hierarchy_with_parameters. ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ game_guard_game_hierarchy_with_parameters ]

******************************************************************************/
static void game_unguard_game_hierarchy_with_parameters(NodeMgr_ptr nodemgr,
                                          Game_UnrealizableCore_Struct_ptr self)
{
    int i;

  SYMB_LAYER_CHECK_INSTANCE(self->layer);

  for(i=0;i<n_players;i++)
    NODE_LIST_CHECK_INSTANCE(self->parameterLists[i]);

    nusmv_assert(self->parameter2expression != (hash_ptr) NULL);
  FlatHierarchy_ptr players[n_players];


  for(i=0;i<n_players;i++)
    players[i] = GameHierarchy_get_player((self->gh),i);

  /* Remove layer. */
  {
    if (BaseEnc_layer_occurs(BASE_ENC(self->bdd_enc),
                             SymbLayer_get_name(self->layer))) {
      BaseEnc_remove_layer(BASE_ENC(self->bdd_enc),
                           SymbLayer_get_name(self->layer));
    }
    if (BaseEnc_layer_occurs(BASE_ENC(self->bool_enc),
                             SymbLayer_get_name(self->layer))) {
      BaseEnc_remove_layer(BASE_ENC(self->bool_enc),
                           SymbLayer_get_name(self->layer));
    }
    SymbTable_remove_layer(self->st, self->layer);
    self->layer = SYMB_LAYER(NULL);
  }

  if (self->N > 0) {

    for(i=0;i<n_players;i++)
      if (self->w == GAME_WHO_BOTH || self->w == i+1) {
        if (self->min_init) {
          game_unguard_exprs_by_parameters(nodemgr,self,
                                           FlatHierarchy_get_init(players[i]),
                                           INIT,
                                           i+1);
        }
        if (self->min_invar) {
          game_unguard_exprs_by_parameters(nodemgr,self,
                                           FlatHierarchy_get_invar(players[i]),
                                           INVAR,
                                           i+1);
        }
        if (self->min_trans) {
          game_unguard_exprs_by_parameters(nodemgr,self,
                                           FlatHierarchy_get_trans(players[i]),
                                           TRANS,
                                           i+1);
        }
      }

    bool expr = false;
    int i;

    for(i=0;i<n_players;i++)
      expr |= (self->w == i+1 && self->player == i+1) ||
              (self->w == i+1 &&
              self->player == n_players-i &&
              Prop_get_type(PROP(self->prop)) == PropGame_GenReactivity);


    if (self->w == GAME_WHO_BOTH ||
            expr) {
      if (self->min_prop) {
        game_unguard_gamespecs_by_parameters(nodemgr,self);
      }
    }
  }

  free_assoc(self->parameter2expression);
  self->parameter2expression = (hash_ptr) NULL;

  for(i=0;i<n_players;i++) {
    NodeList_destroy(self->parameterLists[i]);
    self->parameterLists[i] = NODE_LIST(NULL);
  }

  //free_list(0,self->parameterList_all);
  //self->parameterList_all = Nil;
   NodeList_destroy(self->parameterList_all);
   self->parameterList_all = NODE_LIST(NULL);

  /* Restore assign_hashes. */
  Game_UnrealizableCore_Struct_restore_assign_hashes(self);
}

/**Function********************************************************************

  Synopsis    [ This function takes a BDD representing all possible
                assignments to parameters making the problem
                realizable, and outputs the unrealizable core. ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
void game_process_unrealizable_core_with_params(
                                          Game_UnrealizableCore_Struct_ptr self,
                                                GameBddFsm_ptr fsm,
                                                bdd_ptr winningCore)
{
  boolean is_realizable;

    const NuSMVEnv_ptr env = EnvObject_get_environment(ENV_OBJECT(self));
    const ExprMgr_ptr exprs = EXPR_MGR(NuSMVEnv_get_value(env, ENV_EXPR_MANAGER));
    MasterPrinter_ptr wffprint = MASTER_PRINTER(NuSMVEnv_get_value(env, ENV_WFF_PRINTER));
    StreamMgr_ptr streams = STREAM_MGR(NuSMVEnv_get_value(env, ENV_STREAM_MANAGER));
    FILE* outstream = StreamMgr_get_output_stream(streams);
    FILE* errstream = StreamMgr_get_error_stream(streams);

  nusmv_assert(PropGame_PropGame_Type_First < Prop_get_type(PROP(self->prop)) &&
               PropGame_PropGame_Type_Last > Prop_get_type(PROP(self->prop)));

  /* Check if it is realizable or not. The problem is realizable if
     winningCore has an assignment making all the parameters true, and
     it is false otherwise. */
  {
    bdd_ptr iter = winningCore;
    while (!add_isleaf(iter)) {
      bdd_ptr tmp = iter;
      iter = bdd_then(self->dd_manager, iter);
      if (bdd_iscomplement(self->dd_manager, tmp)) { /* negate the
                                                        child as
                                                        well */
        iter = bdd_not(self->dd_manager, iter);
        /* It is OK to free the BDD before using it because its parent
           still keeps the child. */
        bdd_free(self->dd_manager, iter);
      }
    }
    is_realizable = bdd_is_true(self->dd_manager, iter);
  }

  /* Output the specification and whether it is realizable or not. */
  {
    fprintf(outstream, "--   ");
    game_output_spec_without_params(self, wffprint, outstream);
    fprintf(outstream,
            is_realizable ?
            " : the strategy has been found\n" :
            " : no strategy exists\n");
  }

  fprintf(outstream, "\nThere are:\n");
  fprintf(outstream,
          " %d assumptions\n"
          " %d assumptions are guarded\n"
          " %d assumption unique activation vars\n",
          (self->player == 1 ?
           self->constraintss_total_num[1] :
           self->constraintss_total_num[0]),
          (self->player == 1 ?
           self->constraintss_guarded_num[1] :
           self->constraintss_guarded_num[0]),
          (self->player == 1 ?
           self->constraintss_unique_num[1] :
           self->constraintss_unique_num[0]));
  fprintf(outstream,
          " %d guarantees in total \n"
          " %d guarantees are guarded\n"
          " %d guarantee unique activation vars\n",
          (self->player == 1 ?
           self->constraintss_total_num[0] :
           self->constraintss_total_num[1]),
          (self->player == 1 ?
           self->constraintss_guarded_num[0] :
           self->constraintss_guarded_num[1]),
          (self->player == 1 ?
           self->constraintss_unique_num[0] :
           self->constraintss_unique_num[1]));

  /*  Output all the parameters. */
  {
    ListIter_ptr iter;
    char* player_names[n_players+1];
    char** player_name;
    char str[50];
    int i;
    fprintf(outstream, "\nLabels of Expressions (label, kind, expression)\n");

    for(i=0;i<n_players;i++) {
      sprintf(str,"PLAYER_%d",i+1);
      strcpy(player_names[i],str);
    }
    player_names[i] = (char*) NULL;

    NodeList_ptr parameters;

    for (player_name = player_names; *player_name != NULL; ++player_name) {

        for(i=0;i<n_players;i++) {
            sprintf(str,"PLAYER_%d",i+1);
            if(strcmp(*player_name, str) == 0)
                parameters = self->parameterLists[i];
        }

      /* Skip the player if it does not have any guarded expressions. */
      if (NodeList_get_length(parameters) == 0) continue;

      fprintf(outstream, "\n%s labels:\n", *player_name);

      for (iter = NodeList_get_first_iter(parameters);
           !ListIter_is_end(iter);
           iter =  ListIter_get_next(iter)) {
         //FIX START: extracted node_ptr from parameters to pass to car
        node_ptr v = NodeList_get_elem_at(parameters, iter);
        node_ptr param = car(v);
         //FIX END
        node_ptr exp_list = find_assoc(self->parameter2expression, param);
        nusmv_assert(exp_list != Nil); /* it is impossible to have a
                                          parameter without exp */

        for (; exp_list != Nil; exp_list = cdr(exp_list)) {
          nusmv_assert(CONS == node_get_type(exp_list)); /* list is expected */
          int kind = node_get_type(car(exp_list));
          node_ptr exp = car(car(exp_list));

          print_node(wffprint,outstream, param);
          fprintf(outstream, " \t");
          switch (kind) {
          case INIT:
            fprintf(outstream, " INIT\t");
            break;
          case TRANS:
            fprintf(outstream, " TRANS\t");
            break;
          case INVAR:
            fprintf(outstream, " INVAR\t");
            break;
          case REACHTARGET:
            fprintf(outstream, " REACHTARGET\t");
            break;
          case ATLREACHTARGET:
            fprintf(outstream, " ATLREACHTARGET\t");
            break;
          case AVOIDTARGET:
            fprintf(outstream, " AVOIDTARGET\t");
            break;
          case REACHDEADLOCK:
          case AVOIDDEADLOCK:
            nusmv_assert(false); /* No expression to guard for
                                    these. */
            break;
          case BUCHIGAME:
            fprintf(outstream, " part of BUCHIGAME\t");
            break;
          case LTLGAME:
            fprintf(outstream, " LTLGAME\t");
            break;
          case GENREACTIVITY:
            fprintf(outstream, " part of GENREACTIVITY\t");
            break;
          default: nusmv_assert(false); /* unsupported kind */
          }

          print_node(wffprint,outstream, exp);
          fprintf(outstream, "\n");
        } /* for (exp_list) */
      } /* for (activation vars)*/
    } /* for (player) */
  }

  /* Returns immediately if the specifications do not influence the
     realizability (this happens for the empty module, for
     example). */
  if (bdd_is_true(self->dd_manager, winningCore) ||
      bdd_is_false(self->dd_manager, winningCore)) {
    fprintf(outstream,
            "\nThe problem is found to be %s "
            "independent of the guarded high-level constraints\n",
            is_realizable ? "realizable" : "unrealizable");
    return;
  }

  /* Now output assignments of parameters changing the realizability
     of the problem. */
  {
    int player_to_remove;

    bdd_ptr core;

    switch (self->ct) {
    case GAME_UNREALIZABLE_CORE_CORE_TYPE_CORE:
      fprintf(outstream,
              "\n To keep the specification %s "
              "it is necessary to do one of the following :\n",
              is_realizable ? "realizable" : "unrealizable");
      if (!is_realizable) {
        player_to_remove = self->player == 1 ? 2 : 1;
        core = bdd_not(self->dd_manager, winningCore);
      }
      else {
        player_to_remove = self->player;
        core = bdd_dup(winningCore);
      }
      break;
    case GAME_UNREALIZABLE_CORE_CORE_TYPE_FIX:
      fprintf(outstream,
              "\n To make the specification %s "
              "it is necessary to do one of the following :\n",
              is_realizable ? "unrealizable" : "realizable");
      if (is_realizable) {
        player_to_remove = self->player == 1 ? 2 : 1;
        core = bdd_not(self->dd_manager, winningCore);
      }
      else {
        player_to_remove = self->player;
        core = bdd_dup(winningCore);
      }
      break;
    default:
      nusmv_assert(false);
    }

     SymbTableIter titer;
     SymbTable_gen_iter(self->st,&titer,STT_FROZEN_VAR);


    if (opt_verbose_level_ge(self->oh, 4)) {
      fprintf(errstream,
              "\ngame_process_unrealizable_core_with_params: the set of cores "
              "is:\n");
      BddEnc_print_bdd_wff(self->bdd_enc,
                           core,
                           //SymbTable_get_frozen_vars(self->st),
                           SymbTable_iter_to_list(self->st, titer),
                           false,
                           false,
                           0,
                           (OStream_ptr)outstream);
      fprintf(errstream,
              "\ngame_process_unrealizable_core_with_params: end set of "
              "cores\n");
    }
    ///* debug */ fprintf(outstream, "debugging: the core is \n");
    ///* debug */ dd_printminterm(self->dd_manager, core);
    ///* debug */ fprintf(outstream, "\n");

    while (!bdd_is_false(self->dd_manager, core)) {
      node_ptr assignments;
      bdd_ptr assigns_used = bdd_true(self->dd_manager);
      /* get one assigment */
      assignments = BddEnc_assign_symbols(self->bdd_enc,
                                          core,
                                          self->parameterList_all,
                                          true,
                                          NULL);

      /* Print the disabling and enabling parameters. The aim is to
         remove constraints of the player and keep constraints of the
         opponent, but not vice versa. Note: at first always go
         parameters of player 1 and then the parameters of player 2.
      */
      node_ptr iter;
      boolean remove = player_to_remove == 1;
      boolean something_printed = false;

      fprintf(outstream, remove ? "remove [" : "keep [");

      for (iter = assignments; iter; iter = cdr(iter)) {
        /* a list of EQUALS connected by ANDS */
        nusmv_assert(AND == node_get_type(iter) &&
                     EQUAL == node_get_type(car(iter)));
        node_ptr exp = cdr(car(iter));
        node_ptr var = car(car(iter));

        /* Check if the vars of player 1 just finished and this var is
           of player 2. */
        if ((remove == (player_to_remove == 1)) &&
            NodeList_belongs_to(self->parameterLists[1], var)) {
          remove = !remove;
          something_printed = false;
          fprintf(outstream, " ] and %s", remove ? "remove [" : "keep [");
        }

        /* only 0 and 1 can be assigned */
        nusmv_assert(ExprMgr_number(exprs, 1) == exp || ExprMgr_number(exprs, 0) == exp);
        /* Do not keep constraints of the player and do not remove the
           constraints of the opponent. */
        if ((remove && ExprMgr_number(exprs, 0) == exp) || (!remove && ExprMgr_number(exprs, 1) == exp))  {
          if (something_printed) fprintf(outstream, ",");
          print_node(wffprint,outstream, var);
          something_printed = true;

          /* collect BDDs of all used assignments */
          bdd_ptr tmp = BddEnc_expr_to_bdd(self->bdd_enc, car(iter), Nil);
          bdd_and_accumulate(self->dd_manager, &assigns_used, tmp);
          bdd_free(self->dd_manager, tmp);
        }

      }; /* for (assignments) */
      fprintf(outstream, " ]\n");

      free_list(0,assignments);

      /* Now remove already used assignments from the set of all
         allowed assignments ("core").
         Note: we could directly obtain BDD of assignments from
         BddEnc_assign_symbols and then remove it from "core", but
         then more useless assingments would be printed */
      bdd_ptr tmp = bdd_not(self->dd_manager, assigns_used);
      bdd_and_accumulate(self->dd_manager, &core, tmp);
      bdd_free(self->dd_manager, tmp);
      bdd_free(self->dd_manager, assigns_used);

    } /* while (core) */

    fprintf(outstream,
            "\nExpressions not mentioned do not influence "
            "the realizability/unrealizablity of the specification");
  }

  fprintf(outstream,
          "\n\nEnd of Realizability/Unrealizability Core computation\n");
}

/**Function********************************************************************

  Synopsis    [ Computes (un)realizable cores. ]

  Description [ The function modifies self->gh by introducing
                parameters (activation vars), then computes cores and
                outputs infos.

                Note: No FSM should be created before invoking this
                function. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
static void game_compute_core_using_parameters(NuSMVEnv_ptr env,
                                          Game_UnrealizableCore_Struct_ptr self)
{
  GameGameFsms_ptr fsm;
  bdd_ptr winningStates;

  const NodeMgr_ptr nodemgr = NODE_MGR(NuSMVEnv_get_value(env, ENV_NODE_MGR));

  /* only gen-reactivity is implemented */
  nusmv_assert(PropGame_GenReactivity == Prop_get_type(PROP(self->prop)));

  /* generate unique variable names */
  game_unrealizable_core_unique_num++;

  /* introduce activation variables (parameters) */
  game_guard_game_hierarchy_with_parameters(self);

  /* create all the necessary FSM for the property */
  fsm = game_construct_game_fsms(env,self);

  /* the prop has not been used yet */
  nusmv_assert(GAME_BDD_FSM(NULL) == PropGame_get_game_bdd_fsm(self->prop));

  /* only gen-reactivity is implemented */
  Game_ComputeGenReactivity(env,
                            Prop_get_expr_core(PROP(self->prop)),
                            self->player,
                            fsm->bdd,
                            /* It seems that GAME_INIT_TERM_CONJUNCT
                               typically gives slight speed up but
                               sometimes causes huge slow down. So
                               GAME_INIT_TERM_NONE is used here. */
                            GAME_INIT_TERM_NONE,
                            NULL,
                            NULL,
                            &winningStates);
  /* calculate winning core */
  {
      bdd_ptr inits[n_players];
      bdd_ptr invar[n_players];
      int i;

    for(i=0;i<n_players;i++) {
      inits[i] = GameBddFsm_get_init(fsm->bdd,i);
      invar[i] = GameBddFsm_get_invars(fsm->bdd,i);
      bdd_and_accumulate(self->dd_manager, &inits[i], invar[i]);
    }

    bdd_ptr winningCore = GameBddFsm_player_satisfies_from(fsm->bdd,
                                                           inits,
                                                           winningStates,
                                                           self->player,
                                     opt_game_game_initial_condition(self->oh));
    game_process_unrealizable_core_with_params(self,
                                               fsm->bdd,
                                               winningCore);
    bdd_free(self->dd_manager, winningCore);
    bdd_free(self->dd_manager, invar[1]);
    bdd_free(self->dd_manager, invar[0]);
    bdd_free(self->dd_manager, inits[1]);
    bdd_free(self->dd_manager, inits[0]);
  }
  bdd_free(self->dd_manager, winningStates);

  /* Free FSMs */
  game_free_game_fsms(fsm);

  /* Remove activation variables (parameters). */
  game_unguard_game_hierarchy_with_parameters(nodemgr,self);
}

/**Function********************************************************************

  Synopsis    [ Given a game specification guarded by parameters this
                function outputs the spec without parameters. ]

  Description [ The spec must be a game spec and it must be guarded by
                parameters. A spec is output in the form similar to as
                Prop_print does. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
static void game_output_spec_without_params(
                                          Game_UnrealizableCore_Struct_ptr self,
                                          MasterPrinter_ptr wffprint,
                                          FILE* out)
{
  node_ptr exp = Prop_get_expr(PROP(self->prop));
  node_ptr iter;
  bool expr = false , expr_inv;
  int i;

  for(i=0;i<n_players;i++) {
    expr |= (self->w == i+1 && self->player == i+1);
    expr_inv |= (self->w == i+1 && self->player == n_players-i );
  }

  /* only game property can be met here */
  nusmv_assert((((Prop_get_type(PROP(self->prop)) == PropGame_ReachDeadlock) ||
                 (Prop_get_type(PROP(self->prop)) == PropGame_AvoidDeadlock)) &&
                exp == Nil) ||
               (((Prop_get_type(PROP(self->prop)) == PropGame_ReachTarget) ||
                 (Prop_get_type(PROP(self->prop)) == PropGame_AvoidTarget) ||
                 (Prop_get_type(PROP(self->prop)) == PropGame_BuchiGame) ||
                 (Prop_get_type(PROP(self->prop)) == PropGame_LtlGame) ||
                 (Prop_get_type(PROP(self->prop)) == PropGame_GenReactivity)) &&
                exp != Nil));
  /* game property can not be in arbitrary modules */
  nusmv_assert(exp == Nil || node_get_type(exp) != CONTEXT);

  fprintf(out,
          " %s %s ",
          Prop_get_type_as_string(PROP(self->prop)),
          (char*)UStringMgr_get_string_text(PropGame_get_player(self->prop)));

  switch (Prop_get_type(PROP(self->prop))) {
  case PropGame_ReachTarget:
  case PropGame_LtlGame:
    if (self->N > 0 &&
        self->min_prop &&
        (self->w == GAME_WHO_BOTH ||
                expr )) {
      nusmv_assert(IMPLIES == node_get_type(exp) &&
                   DOT == node_get_type(car(exp)) &&
                   ATOM == node_get_type(cdr(car(exp))) &&
                   ' ' == UStringMgr_get_string_text((string_ptr)car(cdr(car(exp))))[0]);
      /* act.var was introduced if
         exp is implication with act.var as left child */
      print_node(wffprint,out, cdr(exp)); /* remove the parameter */
    }
    else {
      nusmv_assert(IMPLIES != node_get_type(exp) ||
                   DOT != node_get_type(car(exp)) ||
                   ATOM != node_get_type(cdr(car(exp))) ||
                   ' ' != UStringMgr_get_string_text((string_ptr)car(cdr(car(exp))))[0]);
      print_node(wffprint,out, exp);
    }
    break;

  case PropGame_AvoidTarget:
    if (self->N > 0 &&
        self->min_prop &&
        (self->w == GAME_WHO_BOTH ||
                expr )) {
      nusmv_assert(AND == node_get_type(exp) &&
                   DOT == node_get_type(car(exp)) &&
                   ATOM == node_get_type(cdr(car(exp))) &&
                   ' ' == UStringMgr_get_string_text((string_ptr)car(cdr(car(exp))))[0]);
      /* act.var was introduced if exp is
         conjunct with parameter as left child */
      print_node(wffprint,out, cdr(exp)); /* remove the parameter */
    }
    else {
      nusmv_assert(AND != node_get_type(exp) ||
                   DOT != node_get_type(car(exp)) ||
                   ATOM != node_get_type(cdr(car(exp))) ||
                   ' ' != UStringMgr_get_string_text((string_ptr)car(cdr(car(exp))))[0]);
      print_node(wffprint,out, exp);
    }
    break;

  case PropGame_ReachDeadlock:
  case PropGame_AvoidDeadlock:
    break;

  case PropGame_BuchiGame:
    nusmv_assert(GAME_EXP_LIST == node_get_type(exp));
    fprintf(out, "(");
    for (iter = car(exp); iter != Nil; iter = cdr(iter)) {
      nusmv_assert(CONS == node_get_type(iter)); /* a list of exps */
      node_ptr cond = car(iter);
      if (self->N > 0 &&
          self->min_prop &&
          (self->w == GAME_WHO_BOTH ||
                  expr )) {
        nusmv_assert(IMPLIES == node_get_type(cond) &&
                     DOT == node_get_type(car(cond)) &&
                     ATOM == node_get_type(cdr(car(cond))) &&
                     ' ' == UStringMgr_get_string_text((string_ptr)car(cdr(car(cond))))[0]);
        /* act.var was introduced if exp is
           an implication with parameter as left child */
        print_node(wffprint,out, cdr(cond)); /* remove the parameter */
      }
      else {
        nusmv_assert(IMPLIES != node_get_type(cond) ||
                     DOT != node_get_type(car(cond)) ||
                     ATOM != node_get_type(cdr(car(cond))) ||
                     ' ' != UStringMgr_get_string_text((string_ptr)car(cdr(car(cond))))[0]);
        print_node(wffprint,out, cond);
      }

      if (cdr(iter)) fprintf(out, ", ");
    }
    fprintf(out, ")");
    break;

  case PropGame_GenReactivity:
    nusmv_assert(GAME_TWO_EXP_LISTS == node_get_type(exp));
    fprintf(out, "(");
    for (iter = car(exp); iter != Nil; iter = cdr(iter)) {
      nusmv_assert(CONS == node_get_type(iter)); /* a list of exps */
      node_ptr cond = car(iter);
      if (self->N > 0 &&
          self->min_prop &&
          (self->w == GAME_WHO_BOTH ||
           /* note: switched players */
           expr_inv )) {
        nusmv_assert(IMPLIES == node_get_type(cond) &&
                     DOT == node_get_type(car(cond)) &&
                     ATOM == node_get_type(cdr(car(cond))) &&
                     ' ' == UStringMgr_get_string_text((string_ptr)car(cdr(car(cond))))[0]);
        /* act.var was introduced if exp is
           an implication with parameter as left child */
        print_node(wffprint,out, cdr(cond)); /* remove the parameter */
      }
      else {
        nusmv_assert(IMPLIES != node_get_type(cond) ||
                     DOT != node_get_type(car(cond)) ||
                     ATOM != node_get_type(cdr(car(cond))) ||
                     ' ' != UStringMgr_get_string_text((string_ptr)car(cdr(car(cond))))[0]);
        print_node(wffprint,out, cond);
      }
      if (cdr(iter)) fprintf(out, ", ");
    }
    fprintf(out, ") -> (");
    for (iter = cdr(exp); iter != Nil; iter = cdr(iter)) {
      nusmv_assert(CONS == node_get_type(iter)); /* a list of exps */
      node_ptr cond = car(iter);
      if (self->N > 0 &&
          self->min_prop &&
          (self->w == GAME_WHO_BOTH ||
           expr)) {
        nusmv_assert(IMPLIES == node_get_type(cond) &&
                     DOT == node_get_type(car(cond)) &&
                     ATOM == node_get_type(cdr(car(cond))) &&
                     ' ' == UStringMgr_get_string_text((string_ptr)car(cdr(car(cond))))[0]);
        /* act.var was introduced if exp is
           an implication with parameter as left child */
        print_node(wffprint,out, cdr(cond)); /* remove the parameter */
      }
      else {
        nusmv_assert(IMPLIES != node_get_type(cond) ||
                     DOT != node_get_type(car(cond)) ||
                     ATOM != node_get_type(cdr(car(cond))) ||
                     ' ' != UStringMgr_get_string_text((string_ptr)car(cdr(car(cond))))[0]);
        print_node(wffprint,out, cond);
      }
      if (cdr(iter)) fprintf(out, ", ");
    }
    fprintf(out, ")");
    break;

  default: nusmv_assert(false); /* unsupported kind of a specificataion */
  }
}

/* ========================================================================== */
/* ========================================================================== */
/* == Below are functions computing cores without using activation vars  ==== */
/* ========================================================================== */
/* ========================================================================== */

/**Function********************************************************************

  Synopsis    [ This function takes a game hierarchy and tries to remove
                constraints while maintaining the realizability of the
                game. ]

  Description [ The function removes (substitutes by TRUE) high level
                exps (INIT, TRANS, etc) and rechecks realizability of
                the spec.

                Parameters:

                fsm - game BDD FSM of the mainGameHierarchy;

                playerToModify - the player whose exps are substituted
                  by TRUE;

                property - a property being checked;

                realizable - realizability of the given game;

                winningStates - exact winning states of the given game;

                just_check - if true no exps are substitued (the game
                  hierarchy remains the same); the returned value
                  only is of importance;

                fun - a function (which is invoked after substitution
                  and checking that realizability did not change)
                  additionally checks whether the game lost some of
                  its characteristics after last substitution; if fun
                  is NULL no additional check is done.

                Returned value: true if some exp has been removed and
                false otherwise. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
static boolean game_minimize_players_constraints(
                                           Game_UnrealizableCore_Struct_ptr self,
                                                 GameBddFsm_ptr fsm,
                                                 int playerToModify,
                                                 node_ptr property,
                                                 boolean realizable,
                                                 bdd_ptr winningStates,
                                                 boolean just_check,
                                                 game_is_game_still_correct fun)
{

  const NuSMVEnv_ptr env = EnvObject_get_environment(ENV_OBJECT(self));
  const NodeMgr_ptr nodemgr = NODE_MGR(NuSMVEnv_get_value(env, ENV_NODE_MGR));
  MasterPrinter_ptr wffprint = MASTER_PRINTER(NuSMVEnv_get_value(env, ENV_WFF_PRINTER));
  const StreamMgr_ptr streams = STREAM_MGR(NuSMVEnv_get_value(env, ENV_STREAM_MANAGER));
  FILE* errstream = StreamMgr_get_error_stream(streams);
  int i;

  node_ptr trueConst = find_node(nodemgr,TRUEEXP, Nil, Nil);

  boolean somethingChanged = false;

  nusmv_assert(node_get_type(property) == GAME_TWO_EXP_LISTS);

  FlatHierarchy_ptr playerModified = (1 == playerToModify) ?
    GameHierarchy_get_player((self->gh),0) :
    GameHierarchy_get_player((self->gh),1);

  node_ptr propertyModified =
    (self->player == playerToModify) ? cdr(property) : car(property);

  /* ---- at first, process init constraints (if required) ---- */
  /* This is done because removing initial state constraints doesnt
     require a full realizability check but can use
     GameBddFsm_can_player_satisfy. */
    bdd_ptr inits[n_players];
    bdd_ptr invars[n_players];

  if (self->min_init && FlatHierarchy_get_init(playerModified) != Nil) {

    for(i=0;i<n_players;i++) {
      inits[i] = GameBddFsm_get_init(fsm,i);
      invars[i] = GameBddFsm_get_invars(fsm,i);
      bdd_and_accumulate(self->dd_manager, &inits[i], invars[i]);
    }

    bdd_ptr* init = playerToModify == 1 ? &inits[0] : &inits[1];
    bdd_ptr invar = playerToModify == 1 ? invars[0] : invars[1];

    node_ptr iter_init, iter_conj, iter_orig;
    /* construct a list of bdd : i1, i2, i3, i4 ... iN. */
    node_ptr bdd_inits = Nil;
    node_ptr bdd_conjuncts = Nil;

    for (iter_orig = FlatHierarchy_get_init(playerModified);
         iter_orig != Nil;
         iter_orig = car(iter_orig)) {
      bdd_inits = new_node(nodemgr,AND,
                           bdd_inits,
                           (node_ptr) BddEnc_expr_to_bdd(self->bdd_enc,
                                                         cdr(iter_orig),
                                                         Nil));
    }

    /* construct a list of bdd: i2^..^iN, i3^..^iN , .. , iN, true */
    bdd_conjuncts = new_node(nodemgr,AND, Nil, (node_ptr) bdd_true(self->dd_manager));
    for (iter_init = bdd_inits; iter_init != Nil; iter_init = car(iter_init)) {
      bdd_conjuncts = new_node(nodemgr,AND,
                               bdd_conjuncts,
                               (node_ptr) bdd_and(self->dd_manager,
                                                  (bdd_ptr) cdr(bdd_conjuncts),
                                                  (bdd_ptr) cdr(iter_init)));
    }
    /* debug checking: conjunct of all inits is equal to the init from FSM */
    {
      bdd_ptr bdd_tmp = bdd_and(self->dd_manager,
                                (bdd_ptr) cdr(bdd_conjuncts),
                                invar);
      nusmv_assert(bdd_tmp == *init);
      bdd_free(self->dd_manager, bdd_tmp);
    }

    /* remove the last element of bdd_conjuncts, i.e. bdd of i1^i2^..^iN */
    {
      node_ptr tmp = bdd_conjuncts;
      bdd_conjuncts = car(bdd_conjuncts);
      bdd_free(self->dd_manager, (bdd_ptr) cdr(tmp));
      free_node(nodemgr,tmp);
    }

    /* restore right order of bdd_inits */
    bdd_inits = opposite_reverse(bdd_inits);

    /* now check whether removal of one init changes realizability */
    bdd_free(self->dd_manager, *init);
    *init = bdd_dup(invar); /* "*init" will contain inits which stay for sure */

    for (iter_orig = FlatHierarchy_get_init(playerModified),
           iter_conj = bdd_conjuncts,
           iter_init = bdd_inits;
         iter_orig != Nil;
         iter_orig = car(iter_orig),
           iter_conj = car(iter_conj),
           iter_init = car(iter_init)) {
      /* remove constraint from the game */
      node_ptr exp = cdr(iter_orig);

      /* if expression is a memory-shared true-constant => it means it
         was already processed => just skip it.
      */
      if (exp != trueConst) {
        setcdr(iter_orig, trueConst);

        bdd_ptr old_init = *init;
        /* conjunct without a given init condition */
        *init = bdd_and(self->dd_manager, *init, (bdd_ptr) cdr(iter_conj));
        boolean newReal = GameBddFsm_can_player_satisfy(env,fsm,
                                                        inits,
                                                        winningStates,
                                                        self->player,
                                     opt_game_game_initial_condition(self->oh));
        bdd_free(self->dd_manager, *init);
        *init = old_init;

        /* realizability did not change => keep the constraint removed.
           if fun is set => invoke it.

           Note: Since game hierarchy changed we cannot pass the same
           FSM => pass NULL.
        */
        if (newReal == realizable &&
            (fun == NULL ||
             fun(self,
                 NULL,
                 playerToModify,
                 property,
                 realizable,
                 winningStates))) {
          somethingChanged = true;
          if (just_check) { /* nothing should be changed => restore
                               exp and break */
            setcdr(iter_orig, exp);
            break;
          }

          if (opt_verbose_level_ge(self->oh, 2)) {
            fprintf(errstream, "\nINIT ");
            print_node(wffprint,errstream, exp);
            fprintf(errstream, " is removed\n");
          }
        }
        else { /* realizability changed => restore the constraint and
                  remember this constraint in *init */
          setcdr(iter_orig, exp);
          bdd_and_accumulate(self->dd_manager, init, (bdd_ptr) cdr(iter_init));
        }
      } /* if */
    } /* for */

    /* all lists has the same length */
    nusmv_assert((just_check && somethingChanged) ||
                 (Nil == iter_orig && Nil == iter_conj && Nil == iter_init));

    /* free all the lists and constructed bdd */
    for (iter_conj = bdd_conjuncts;
         iter_conj != Nil;
         iter_conj = car(iter_conj)) {
      bdd_free(self->dd_manager, (bdd_ptr) cdr(iter_conj));
    }
    free_opposite_list(nodemgr,bdd_conjuncts);

    for (iter_init = bdd_inits; iter_init != Nil; iter_init = car(iter_init)) {
      bdd_free(self->dd_manager, (bdd_ptr) cdr(iter_init));
    }
    free_opposite_list(nodemgr,bdd_inits);

    bdd_free(self->dd_manager, invars[1]);
    bdd_free(self->dd_manager, invars[0]);
    bdd_free(self->dd_manager, inits[1]);
    bdd_free(self->dd_manager, inits[0]);

    if (just_check && somethingChanged) return true;
  }

  /* ---- Process all other constraints (INVAR, TRANS, PROPERTY) ---- */
  bdd_ptr overApproximation = NULL;
  bdd_ptr underApproximation = NULL;

    /* -- if assumptions (oppenonent's constraints) are removed =>
       winning state can only decrease => previous winning states can
       be used over-approximation for next iteration.

       if guarantees (player's constraints) are removed => winning
       state can only increase => previous winning states can be used
       under-approximation for next iteration.
    */
  bdd_ptr* approximation =
    (playerToModify != self->player ?
     &overApproximation :
     &underApproximation);
  *approximation = bdd_dup(winningStates);

  node_ptr constraints[3] = {FlatHierarchy_get_invar(playerModified),
                             FlatHierarchy_get_trans(playerModified),
                             propertyModified};
  /* Note: in init, trans, invar a head of a list is on the right
     (cdr) and tail is on the left (cdr). In property the situation
     is opposite(i.e. normal; as it should be) */
  struct functions {
    node_ptr (*elmnt) (node_ptr);
    void (*set_elmnt) (node_ptr, node_ptr);
    node_ptr (*next) (node_ptr);
    const char* name;
    boolean doMinimize;
  };

  /* this list must correspond to list "constraints" */
  struct functions function [3] = {
    {.elmnt = cdr,
     .set_elmnt = setcdr,
     .next = car,
     .name = "INVAR",
     .doMinimize = self->min_invar},
    {.elmnt = cdr,
     .set_elmnt = setcdr,
     .next = car,
     .name = "TRANS",
     .doMinimize = self->min_trans},
    {.elmnt = car,
     .set_elmnt = setcar,
     .next = cdr,
     .name = "part of property",
     .doMinimize = self->min_prop},
  };

  /* traverse all the constraints and disable them one by one */
  for (i = 0; i < 3; ++i) {
    node_ptr iter;
    /* if this kind of constraint should be checked */
    if (function[i].doMinimize) {

      for (iter = constraints[i]; iter != Nil; iter = function[i].next(iter)) {

        nusmv_assert(iter != find_atom(nodemgr,iter)); /* it cannot be mem-shared */
        node_ptr exp = function[i].elmnt(iter);
        /* if expression is a memory-shared true-constant => it means it
           was already processed => just skip it.
        */
        if (exp != trueConst) {
          if (opt_verbose_level_ge(self->oh, 2)) {
            fprintf(errstream,
                    "\n.... %s is CHECKED .... \n",
                    function[i].name);
          }

          /* at every iteration disable one constraint and then recheck the game
             (disabling == substituting by TRUE) */
          function[i].set_elmnt(iter, trueConst);

          /* create all the necessary FSMs */
          GameGameFsms_ptr new_fsm = game_construct_game_fsms(env,self);

          boolean newReal = Game_ComputeGenReactivity(env,
                                                      property,
                                                      self->player,
                                                      new_fsm->bdd,
                        /* for a realizable game win-states are exact
                           always, but for unrealizable game with earlier
                           termination win-states is overapproximation and
                           therefore cannot be used as underapproximation for
                           next iteration => we disable earlier termination */
                                                      (realizable ?
                                                       GAME_INIT_TERM_NORMAL :
                                                       GAME_INIT_TERM_NONE),
                                                      overApproximation,
                                                      underApproximation,
                                                      &winningStates);
          /* the realizability did not change => remove this
             constraint forever */
          if (newReal == realizable &&
              (fun == NULL ||
               fun(self,
                   new_fsm->bdd,
                   playerToModify,
                   property,
                   realizable,
                   winningStates))) {
            /* update the approximation */
            bdd_free(self->dd_manager, *approximation);
            *approximation = winningStates;

            somethingChanged = true;
            if (just_check) { /* nothing should be changed => restore
                                 exp and break */
              function[i].set_elmnt(iter, exp);
              game_free_game_fsms(new_fsm);
              break;
            }

            if (opt_verbose_level_ge(self->oh, 2)) {
              fprintf(errstream, "\n%s ", function[i].name);
              print_node(wffprint,errstream, exp);
              fprintf(errstream, " is removed\n");
            }
          }
          else {
            /* the realizability changed => restore exp and winningStates */
            function[i].set_elmnt(iter, exp);
            /* realizability change => leave approximation as it is */
            bdd_free(self->dd_manager, winningStates);
          }

          game_free_game_fsms(new_fsm);
        } /* if */
      } /* for (iter)*/

      if (just_check && somethingChanged) {
        bdd_free(self->dd_manager, *approximation);
        return true;
      };
    } /* if */
  } /* for (i) */

  bdd_free(self->dd_manager, *approximation);

 /* if just checking was done => true is always returned earlier */
  nusmv_assert(!just_check || !somethingChanged);

  return somethingChanged;
}

/**Function********************************************************************

  Synopsis    [ This function takes a game hierarchy, FSM, etc, as it is
                given to game_minimize_players_constraints and checks
                if the opponent's constraints are still minimal, i.e.,
                cannot be removed without changing the
                realizability. ]

  Description [ The actual parameters must be the same as those given
                to game_minimize_players_constraints.

                The function returns true iff it is impossible to
                remove opponent's constraints (i.e., other to
                playerToModify player) without changing realizability.

                Note: "fsm" may be null; then new FSMs are created.

                Note: The game hierarchy can be modified during
                checking but will be restored before return. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
static boolean game_is_opponent_constraint_minimal(NuSMVEnv_ptr env,
                                           Game_UnrealizableCore_Struct_ptr self,
                                                   GameBddFsm_ptr fsm,
                                                   int playerToModify,
                                                   node_ptr property,
                                                   boolean realizable,
                                                   bdd_ptr winningStates)
{
  GameGameFsms_ptr new_fsm;
  /* create new FSM if required */
  if (GAME_BDD_FSM(NULL) == fsm) {
    new_fsm = game_construct_game_fsms(env,self);
    fsm = new_fsm->bdd;
  }
  else new_fsm = (GameGameFsms_ptr) NULL;

  /* The player to modify has to be changed since the opponent is
     analysed now. */
  playerToModify = 1 == playerToModify ? 2 : 1;

  boolean somethingChanged = game_minimize_players_constraints(self,
                                                               fsm,
                                                               playerToModify,
                                                               property,
                                                               realizable,
                                                               winningStates,
                                                               true,
                                                               NULL);

  if (new_fsm != (GameGameFsms_ptr) NULL) game_free_game_fsms(new_fsm);

  return !somethingChanged;
}

/**Function********************************************************************

  Synopsis    [ Outputs the game hierarchy self->gh after minimization. ]

  Description [ Parameters:

                realizable -- a flag whether the game is realizable;

                explanation -- a player whose constraints were
                  minimized to obtain the core;

                property -- a specification of the game;

                Note: self->gh and property are expected to be already
                minimized. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
static void game_output_game_after_minimization(
                                           Game_UnrealizableCore_Struct_ptr self,
                                                boolean realizable,
                                                int explanation,
                                                node_ptr property,
                                                MasterPrinter_ptr wffprint)
{
  nusmv_assert(node_get_type(property) == GAME_TWO_EXP_LISTS);

  const NuSMVEnv_ptr env = EnvObject_get_environment(ENV_OBJECT(self));
  StreamMgr_ptr streams = STREAM_MGR(NuSMVEnv_get_value(env, ENV_STREAM_MANAGER));
  FILE* outstream = StreamMgr_get_output_stream(streams);

  /* a list consisting of the player in explanation and the other
     player */
    FlatHierarchy_ptr players[n_players];

  players[0] = 1 == explanation ? GameHierarchy_get_player((self->gh),0) :
                              GameHierarchy_get_player((self->gh),1);

  players[1] = 1 == explanation ? GameHierarchy_get_player((self->gh),1) :
                              GameHierarchy_get_player((self->gh),0);

  node_ptr constraints[8] = {FlatHierarchy_get_init(players[0]),
                             FlatHierarchy_get_invar(players[0]),
                             FlatHierarchy_get_trans(players[0]),
                             realizable ? car(property) : cdr(property),

                             FlatHierarchy_get_init(players[1]),
                             FlatHierarchy_get_invar(players[1]),
                             FlatHierarchy_get_trans(players[1]),
                             realizable ? cdr(property) : car(property)
  };

  struct functions {
    node_ptr (*elmnt) (node_ptr);
    void (*set_elmnt) (node_ptr, node_ptr);
    node_ptr (*next) (node_ptr);
    const char* name;
    boolean doMinimize;
  };

  /* Note: in init, trans, invar a head of a list is on the right
     (cdr) and tail is on the left (car). In property the situation is
     opposite (i.e. normal; as it should be). */
  /* this list must correspond to list "constraints" */
  struct functions function [4] = {
    {.elmnt = cdr,
     .set_elmnt = setcdr,
     .next = car,
     .name = "INIT",
     .doMinimize = self->min_init},
    {.elmnt = cdr,
     .set_elmnt = setcdr,
     .next = car,
     .name = "INVAR",
     .doMinimize = self->min_invar},
    {.elmnt = cdr,
     .set_elmnt = setcdr,
     .next = car,
     .name = "TRANS",
     .doMinimize = self->min_trans},
    {.elmnt = car,
     .set_elmnt = setcar,
     .next = cdr,
     .name = "part of property",
     .doMinimize = self->min_prop}
  };

  const char* messages[2] = {
    realizable ? "minimally sufficient set of assumptions"
               : "minimally unfulfillable set of guarantees",
    realizable ? "minimized guarantees"
               : "minimized assumptions",
  };

  /* --- output the results ---- */
  fprintf(outstream,
          "\nThe specification is %s.\n\n",
          realizable ? "REALIZABLE" : "UNREALIZABLE");

  int plr, base;
  /* condition is that only 2 player are output and second player is
     output only if isOpponentMinimized is set on */
  for (plr = 0, base = 0;
       plr < 2 && (plr == 0 || (self->w == GAME_WHO_BOTH));
       ++plr, base += 4) {
    int remainedConstr = 0, removedConstr = 0;

    /* --- output the results ---- */
    fprintf(outstream, "Below is a list of %s:\n", messages[plr]);

    boolean something_was_printed = false;
    int i;
    for (i = 0; i < 4; ++i) {
      node_ptr iter;
      if (!(function[i].doMinimize)) {
        fprintf(outstream,
                "NOTE : All %s constraints are left in by command line option\n",
                function[i].name);
      }

      for (iter = constraints[i + base];
           iter != Nil;
           iter = function[i].next(iter)) {
        node_ptr exp = function[i].elmnt(iter);
        if (node_get_type(exp) != TRUEEXP ||
            !function[i].doMinimize) { /* this exp has not been removed */
          fprintf(outstream, "%s ", function[i].name);
          print_node(wffprint,outstream, exp);
          fprintf(outstream, "\n");
          something_was_printed = true;
          ++remainedConstr;
        }
        else {
          ++removedConstr;
        }
      } /* for iter */
    } /* for */

    if (!something_was_printed) {
      fprintf(outstream, "No constraints are required\n");
    }
    fprintf(outstream,
            "%d constraints remained (%d of %d, i.e., %.1f%%, were removed)\n",
            remainedConstr,
            removedConstr,
            removedConstr + remainedConstr,
            ((double) removedConstr*100/(removedConstr + remainedConstr)));
    fprintf(outstream, "\n");
  } /* for (plr) */
}

/**Function********************************************************************

  Synopsis    [ Computes (un)realizable cores. ]

  Description [ The function modifies mainGameHierarchy by iteratively
                substituting high-level exps (INIT, TRANS, etc) by
                TRUE and checking realizability of the spec.

                Note: No FSM should be created before invoking this
                function. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
static void game_compute_core_switching_constraints(
                                          Game_UnrealizableCore_Struct_ptr self)
{
  bdd_ptr winningStates;
  boolean realizable;
  boolean somethingChanged;
  GameGameFsms_ptr fsm;
  int playerToModify;

  const NuSMVEnv_ptr env = EnvObject_get_environment(ENV_OBJECT(self));
  const NodeMgr_ptr nodemgr = NODE_MGR(NuSMVEnv_get_value(env, ENV_NODE_MGR));
  MasterPrinter_ptr wffprint = MASTER_PRINTER(NuSMVEnv_get_value(env, ENV_WFF_PRINTER));
  StreamMgr_ptr streams = STREAM_MGR(NuSMVEnv_get_value(env, ENV_STREAM_MANAGER));
  FILE* errstream = StreamMgr_get_error_stream(streams);

  node_ptr spec = Prop_get_expr_core(PROP(self->prop));
  /* the prop has not been used yet */
  nusmv_assert(GAME_BDD_FSM(NULL) == PropGame_get_game_bdd_fsm(self->prop));

  long init_time, minim_1_time, minim_2_time;
  init_time = util_cpu_time();

  FlatHierarchy_ptr p1 = GameHierarchy_get_player((self->gh),0);
  FlatHierarchy_ptr p2 = GameHierarchy_get_player((self->gh),1);

  /* ----------------------------------------------------- */
  /* Some checks. */

  /* Processes are not implemented for games. */
  nusmv_assert(Nil == cdr(FlatHierarchy_get_assign(p1)));
  nusmv_assert(Nil == cdr(FlatHierarchy_get_assign(p2)));

  /* Assignments are not implemented. See comment above. */
  nusmv_assert(Nil == cdr(car(FlatHierarchy_get_assign(p1))) &&
               Nil == cdr(car(FlatHierarchy_get_assign(p2))));

  /* Only gen-reactivity is implemented. */
  nusmv_assert(PropGame_GenReactivity == Prop_get_type(PROP(self->prop)));
  nusmv_assert(GAME_TWO_EXP_LISTS == node_get_type(spec));

  /* ----------------------------------------------------- */
  /* Save flat hierarchies. Then clear the associations between
     variables and expressions. */

  Game_UnrealizableCore_Struct_save_flat_hierarchies(self);

  /* Remove the association between variables and expressions they are
     used in. This is necessary to do disabled exps stay in that
     association and will be reintroduced again in Sexp FSM.
  */
  FlatHierarchy_clear_var_expr_associations(p1);
  FlatHierarchy_clear_var_expr_associations(p2);

  spec = new_node(nodemgr,node_get_type(spec),
                  copy_list(0,car(spec)),
                  copy_list(0,cdr(spec)));

  init_time = util_cpu_time() - init_time;
  minim_1_time = util_cpu_time();

  /* ------- perform the check of the original spec ------ */
  /* create all the necessary FSMs */
  fsm = game_construct_game_fsms(env,self);

  realizable = Game_ComputeGenReactivity(env,
                                         spec,
                                         self->player,
                                         fsm->bdd,
                                         /* NONE is used to obtain
                                            exact winning state
                                            set. */
                                         GAME_INIT_TERM_NONE,
                                         NULL,
                                         NULL,
                                         &winningStates);

  /* ------- minimize corresponding player's constraints ------ */
  {
    /* -- if game is realizable we are going to remove assumptions
       (oppenonent's constraints) as long as the game is still
       realizable => winning state can only decrease => previous
       winning states can be used over-approximation for the next
       iteration.

       if game is unrealizable we are going to remove guarantees
       (player's constraints) as long as the game is still
       unrealizable => winning state can only increase => previous
       winning states can be used under-approximation for next
       iteration.
    */
    playerToModify =
      (((realizable && 1 == self->player) ||
        (!realizable && 2 == self->player)) ?
       2 :
       1);

    somethingChanged = game_minimize_players_constraints(self,
                                                         fsm->bdd,
                                                         playerToModify,
                                                         spec,
                                                         realizable,
                                                         winningStates,
                                                         false,
                                                         NULL);
  }

  minim_1_time = util_cpu_time() - minim_1_time;
  fprintf(errstream, "Minimization time of 1th player (preliminary): %f\n",
          minim_1_time/(double)1000);
  minim_2_time = util_cpu_time();

  /* ------- minimize  opponent's constraints ------ */
  if (self->w == GAME_WHO_BOTH) {
    /* the game changed => it is necessary to recompute the FSM and exact
       winning states */
    if (somethingChanged) {
      boolean realizable2;
      game_free_game_fsms(fsm);
      fsm = game_construct_game_fsms(env,self);

      bdd_ptr tmp = winningStates;
      realizable2 = Game_ComputeGenReactivity(env,
                                              spec,
                                              self->player,
                                              fsm->bdd,
                                              /* NONE is used to
                                                 obtain exact winning
                                                 state set. */
                                              GAME_INIT_TERM_NONE,
                                              /* Use the previous
                                                 winning states as
                                                 over- or
                                                 under-approximation. */
                                              realizable ? tmp : NULL,
                                              realizable ? NULL : tmp,
                                              &winningStates);
      nusmv_assert(realizable == realizable2);
      bdd_free(self->dd_manager, tmp);
    }

    /* Since we have changed the player it is necessary to change the
       player to modify. */
    game_minimize_players_constraints(self,
                                      fsm->bdd,
                                      (1 == playerToModify ?
                                       2 :
                                       1),
                                      spec,
                                      realizable,
                                      winningStates,
                                      false,
                                      (game_is_game_still_correct)game_is_opponent_constraint_minimal);
  }
  bdd_free(self->dd_manager, winningStates);
  game_free_game_fsms(fsm);

  minim_2_time = util_cpu_time() - minim_2_time;
  init_time = util_cpu_time() - init_time;

  /* output the results */
  game_output_game_after_minimization(self,
                                      realizable,
                                      playerToModify,
                                      spec,
                                      wffprint);

  /* Restore flat hierarchies. */
  Game_UnrealizableCore_Struct_restore_flat_hierarchies(nodemgr,self);

  free_list(0,car(spec));
  free_list(0,cdr(spec));
  free_node(nodemgr,spec);

  init_time = util_cpu_time() - init_time;
  fprintf(errstream, "Initialization time: %f\n"
          "Minimization time of 1th player: %f\n"
          "Minimization time of 2nd player: %f\n"
          "Total time: %f\n",
          init_time/(double)1000,
          minim_1_time/(double)1000,
          minim_2_time/(double)1000,
          (init_time + minim_1_time + minim_2_time)/(double)1000);
}
