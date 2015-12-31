/**CFile*****************************************************************

  FileName    [ GameSexpFsm.c ]

  PackageName [ game.fsm ]

  Synopsis    [ The GameSexpFsm (Game Scalar FSM) implementation ]

  Description [ This class is a Game Scalar-expression FSM. In
                general, a Game FSM consists of two usual FSMs, one
                for each player. Another distinction from a usual FSM
                is that a Game FSM also divides variables into two
                subsets -- one for each player. ]

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

#include "code/nusmv/core/utils/ErrorMgr.h"
#include "GameSexpFsm.h"

#include "compile/compile.h"
#include "enc/enc.h"
#include "fsm/sexp/SexpFsm.h"
#include "node/node.h"
#include "set/set.h"
#include "utils/utils.h"
#include "utils/error.h"
#include "utils/NodeList.h"

EXTERN int n_players;

static char rcsid[] UTIL_UNUSED = "$Id: GameSexpFsm.c,v 1.1.2.9 2010-02-05 22:42:22 nusmv Exp $";

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/**Type************************************************************************

  Synopsis    [ Game scalar FSM, the type exported by this package ]

  Description [ A scalar (or boolean) game FSM consists of two usual
                FSMs of the same kind. ]

                See SexpFsm_TAG for more info on SexpFsm_ptr. ]

  SeeAlso     [ ]

******************************************************************************/
typedef struct GameSexpFsm_TAG {
  SexpFsm_ptr *players;

  /* Each player's scalar FSMs contain a complete set of variables
     (which is used for COI).  These fields are the subsets of
     variables belonging to a particular player. */
  Set_t *vars_sets;

} GameSexpFsm;

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/
static void game_sexp_fsm_init ARGS((GameSexpFsm_ptr self,
                                     SexpFsm_ptr *players,
                                     Set_t vars_sets[n_players]));

static void game_sexp_fsm_deinit ARGS((GameSexpFsm_ptr self));

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [ Constructor for a scalar and boolean game scalar fsm. If
                det_layer's are null, then a scalar fsm will be
                created, otherwise a boolean fsm will be created. ]

  Description [ If provided, the layers will be filled with the
                determinization variables that might derive from the
                booleanization.

                Note that layers both should be Nil or both not Nil.

                The hierarchies are used to provide all the TRANS,
                INVARS, INIT and ASSIGN to the corresponding players.

                all_vars_set is the set of variables for which the FSM
                should be created. Parameters vars_set_1 and
                vars_set_2 are non-intersecting sets of variables
                belonging to the first and second players,
                respectively. Note that some player's variables may
                not be in all_vars_set, for example, if COI is
                enabled.

                The pointer to the parameters vars_set_n are
                remembered, i.e., copies are created. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
GameSexpFsm_ptr GameSexpFsm_create(NuSMVEnv_ptr env,
                                   Set_t all_vars_set,
                                   FlatHierarchy_ptr hierarchies[n_players],
                                   Set_t vars_sets[n_players])
{
  GameSexpFsm_ptr self;
    SexpFsm_ptr players[n_players];
    int i;
    boolean expr = false;

  const ErrorMgr_ptr errmgr = ERROR_MGR(NuSMVEnv_get_value(env, ENV_ERROR_MANAGER));

    for(i=0;i<n_players;i++)
  FLAT_HIERARCHY_CHECK_INSTANCE(hierarchies[i]);

  self = ALLOC(GameSexpFsm, 1);
  GAME_SEXP_FSM_CHECK_INSTANCE(self);

    for(i=0;i<n_players;i++)
    players[i] = SexpFsm_create(hierarchies[i], all_vars_set);

  game_sexp_fsm_init(self,
                     players,
                     vars_sets);

    for(i=0;i<2-1;i++)
    expr = Nil != FlatHierarchy_get_invar(hierarchies[i]) || Nil != FlatHierarchy_get_invar(hierarchies[i+1]);

  if (expr) {
    ErrorMgr_rpterr(errmgr,"Game has an invariant construct (INVAR or ASSIGN)."
           "This is not implemented at the moment.\n");
    /* For developers: the problem is how images are
       computed. Currently invars are added at the end to the
       computed, for example, strong backward image. This is not
       correct because invar MUST behave the same as trans, i.e., if
       an environment cannot satisfy its invars then the system
       wins. */
  }

  return self;
}

/**Function********************************************************************

  Synopsis    [ Copy constructor ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
GameSexpFsm_ptr GameSexpFsm_copy(const GameSexpFsm_ptr self)
{
  GameSexpFsm_ptr copy;
    int i;
    SexpFsm_ptr players[n_players];

  GAME_SEXP_FSM_CHECK_INSTANCE(self);

  copy = ALLOC(GameSexpFsm, 1);
  GAME_SEXP_FSM_CHECK_INSTANCE(copy);

    for(i=0;i<n_players;i++)
        players[i] = SexpFsm_copy(self->players[i]);

  game_sexp_fsm_init(copy,
                     players,
                     self->vars_sets);

  return copy;
}

/**Function********************************************************************

  Synopsis    [ Destructor ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
void GameSexpFsm_destroy(GameSexpFsm_ptr self)
{
  GAME_SEXP_FSM_CHECK_INSTANCE(self);

  game_sexp_fsm_deinit(self);
  FREE(self);
}

/**Function********************************************************************

  Synopsis    [ Construct a boolean game fsm from a scalar one ]

  Description [ The given layers will be filled with the
                determinization variables coming from the
                booleanization process. det_layer_1 will be used
                during booleanization of the first player, and
                det_layer_2 for the second player. Returns a copy if
                self is already booleanized.

                Note the argument FSM should be scalar, i.e., not
                boolean. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
GameSexpFsm_ptr GameSexpFsm_scalar_to_boolean(const GameSexpFsm_ptr self,
                                              BddEnc_ptr enc,
                                              SymbLayer_ptr det_layers[n_players])
{
  GameSexpFsm_ptr result;
  int i;

  GAME_SEXP_FSM_CHECK_INSTANCE(self);
  BDD_ENC_CHECK_INSTANCE(enc);
  for(i=0;i<n_players;i++)
    SYMB_LAYER_CHECK_INSTANCE(det_layers[i]); /* must be a valid layer */
  nusmv_assert(!SexpFsm_is_boolean(self->players[0]));

  result = ALLOC(GameSexpFsm, 1);
  GAME_SEXP_FSM_CHECK_INSTANCE(result);

  result->players = (SexpFsm_ptr *)malloc(sizeof(SexpFsm_ptr)*n_players);
  result->vars_sets = (Set_t *)malloc(sizeof(Set_t)*n_players);

    for(i=0;i<n_players;i++)
  result->players[i] = SEXP_FSM(BoolSexpFsm_create_from_scalar_fsm(self->players[i],
                                                                 enc,
                                                                 det_layers[i]));
    for(i=0;i<n_players;i++)
  result->vars_sets[i] = Set_Copy(self->vars_sets[i]);

  return result;
}

/**Function********************************************************************

  Synopsis    [ Returns true if self if a booleanized fsm, false if it is
                scalar. ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
boolean GameSexpFsm_is_boolean(const GameSexpFsm_ptr self)
{
  GAME_SEXP_FSM_CHECK_INSTANCE(self);
    boolean expr = false;
    int i;

    for(i=0;i<2-1;i++)
        expr += SexpFsm_is_boolean(self->players[i]) == SexpFsm_is_boolean(self->players[i+1]);

  nusmv_assert(expr);

  /* players are both boolean or both scalar */
  return SexpFsm_is_boolean(self->players[0]);
}

/**Function********************************************************************

  Synopsis    [ Returns all the variables this FSM depends on. ]

  Description [ This list of vars is provided during
                construction. self keeps ownership (i.e., do not
                change it). ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
NodeList_ptr GameSexpFsm_get_all_vars_set(const GameSexpFsm_ptr self)
{
  GAME_SEXP_FSM_CHECK_INSTANCE(self);

  return SexpFsm_get_vars_list(self->players[0]); // only player1
}

/**Function********************************************************************

  Synopsis    [ Returns the list of variables belonging to the first
                player only. ]

  Description [ This list of vars is provided during
                construction. self keeps ownership (i.e., do not
                change it). ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
NodeList_ptr GameSexpFsm_get_vars_list(const GameSexpFsm_ptr self, int index)
{
  GAME_SEXP_FSM_CHECK_INSTANCE(self);

  return Set_Set2List(self->vars_sets[index]);
}


/**Function********************************************************************

  Synopsis    [ Returns the scalar FSM of the first player. ]

  Description [ Returned object continues to belong to self. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
SexpFsm_ptr GameSexpFsm_get_player(const GameSexpFsm_ptr self, int index)
{
  GAME_SEXP_FSM_CHECK_INSTANCE(self);

  return self->players[index];
}

/*---------------------------------------------------------------------------*/
/* Static function definitions                                               */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [ Initialization function ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
static void game_sexp_fsm_init(GameSexpFsm_ptr self,
                               SexpFsm_ptr *players,
                               Set_t *vars_sets)
{
  int i;

  self->players = (SexpFsm_ptr*)malloc(sizeof(SexpFsm_ptr)*n_players);
  self->vars_sets = (Set_t*)malloc(sizeof(Set_t)*n_players);

  for(i=0;i<n_players;i++) {

    self->players[i] = players[i];
    self->vars_sets[i] = Set_Copy(vars_sets[i]);
  }
}

/**Function********************************************************************

  Synopsis    [ De-initialization function ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
static void game_sexp_fsm_deinit(GameSexpFsm_ptr self)
{
    int i;

    for(i=0;i<n_players;i++)
  SexpFsm_destroy(self->players[i]);

    for(i=0;i<n_players;i++)
  Set_ReleaseSet(self->vars_sets[i]);
}
