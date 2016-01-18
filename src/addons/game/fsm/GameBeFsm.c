/**CFile***********************************************************************

  FileName    [ GameBeFsm.c ]

  PackageName [ game.fsm ]

  Synopsis    [ Implementation of class GameBeFsm ]

  Description [ ]

  SeeAlso     [ ]

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

#include "GameBeFsm.h"
#include "GameSexpFsm.h"

#include "enc/be/BeEnc.h"
#include "fsm/be/BeFsm.h"
#include "fsm/sexp/SexpFsm.h"
#include "utils/utils.h"

EXTERN int n_players;

static char rcsid[] UTIL_UNUSED = "$Id: GameBeFsm.c,v 1.1.2.6 2010-02-10 14:57:17 nusmv Exp $";

/*---------------------------------------------------------------------------*/
/* Constant declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/**Struct*********************************************************************

  Synopsis    [ Game boolean expression FSM, the type exported by this
                package. ]

  Description [ A game boolean expression FSM consists of two usual
                FSMs of the same kind. ]

                See BeFsm_TAG for more info on BeFsm_ptr.

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
typedef struct GameBeFsm_TAG {
  BeFsm_ptr *players;
} GameBeFsm;

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Macro declarations                                                        */
/*---------------------------------------------------------------------------*/

/**AutomaticStart*************************************************************/

/*---------------------------------------------------------------------------*/
/* Static function prototypes                                                */
/*---------------------------------------------------------------------------*/
static void game_be_fsm_init ARGS((GameBeFsm_ptr self,
                                   BeFsm_ptr *players));

static void game_be_fsm_deinit ARGS((GameBeFsm_ptr self));

/**AutomaticEnd***************************************************************/

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [ Class GameBeFsm constructor ]

  Description [ Creates a Game BE FSM from two BE FSMs of players.

                If the mask for enumeratives has not been taken into
                account while building the BeFsms player_1 and
                player_2, then this FSM will not have these
                constraints. ]

  SideEffects [ ]

  SeeAlso     [ GameBeFsm_destroy ]

******************************************************************************/
GameBeFsm_ptr GameBeFsm_create(BeFsm_ptr *players)
{
  GameBeFsm_ptr self;
  int i;

  for(i=0;i<n_players;i++)
    BE_FSM_CHECK_INSTANCE(players[i]);

  self = ALLOC(GameBeFsm, 1);
  self->players = (BeFsm_ptr*)malloc(sizeof(BeFsm_ptr)*n_players);

  GAME_BE_FSM_CHECK_INSTANCE(self);

  game_be_fsm_init(self, players);

  return self;
}

/**Function********************************************************************

  Synopsis    [ Copy constructor for class GameBeFsm ]

  Description [ Creates a new independent copy of the given FSM
                instance.  The returned instance must be destroyed by
                invoking the class destructor when it is no longer
                needed. ]

  SideEffects [ ]

  SeeAlso     [ GameBeFsm_destroy ]

******************************************************************************/
GameBeFsm_ptr GameBeFsm_copy(GameBeFsm_ptr self)
{
  GameBeFsm_ptr copy;
    BeFsm_ptr players[n_players];
    int i;

  GAME_BE_FSM_CHECK_INSTANCE(self);

    for(i=0;i<n_players;i++)
    players[i] = BeFsm_copy(self->players[i]);

  game_be_fsm_init(copy,
                   players);

  return copy;
}

/**Function********************************************************************

  Synopsis    [ Class GameBeFsm constructor ]

  Description [ Creates a new instance of GameBeFsm, getting
                information from an instance of GameSexpFsm.

                If the mask for enumeratives has not been taken into
                account while building the BeFsms player_1 and
                player_2, then this FSM will not have these
                constraints. ]

  SideEffects [ ]

  SeeAlso     [ GameBeFsm_create, GameBeFsm_destroy ]

******************************************************************************/
GameBeFsm_ptr GameBeFsm_create_from_sexp_fsm(BeEnc_ptr be_enc,
                                             const GameSexpFsm_ptr bfsm)
{
  GameBeFsm_ptr self;
  BoolSexpFsm_ptr bsexpfsms[n_players];
  BeFsm_ptr befsms[n_players];
  int i;

  BE_ENC_CHECK_INSTANCE(be_enc);
  GAME_SEXP_FSM_CHECK_INSTANCE(bfsm);

  for(i=0;i<n_players;i++) {
    bsexpfsms[i] = BOOL_SEXP_FSM(GameSexpFsm_get_player(bfsm,i));
    befsms[i] = BeFsm_create_from_sexp_fsm(be_enc, bsexpfsms[i]);
  }

  self = GameBeFsm_create(befsms);

  return self;
}

/**Function********************************************************************

  Synopsis    [ Class GameBeFsm destructor ]

  Description [ ]

  SideEffects [ self will be invalidated ]

  SeeAlso     [ GameBeFsm_create, GameBeFsm_create_from_sexp_fsm ]

******************************************************************************/
void GameBeFsm_destroy(GameBeFsm_ptr self)
{
  GAME_BE_FSM_CHECK_INSTANCE(self);

  game_be_fsm_deinit(self);
  FREE(self);
}

/**Function********************************************************************

  Synopsis    [ Returns the BE FSM of the first player ]

  Description [ self keeps ownership of the returned FSM. ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
BeFsm_ptr GameBeFsm_get_player(const GameBeFsm_ptr self, int index)
{
  GAME_BE_FSM_CHECK_INSTANCE(self);

  return self->players[index];
}


/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [ Private service to initialize the internal members. ]

  Description [ ]

  SideEffects [ self will change internally. ]

  SeeAlso     [ ]

******************************************************************************/
static void game_be_fsm_init(GameBeFsm_ptr self,
                             BeFsm_ptr *players)
{
    int i;

    for(i=0;i<n_players;i++)
  self->players[i] = players[i];
}

/**Function********************************************************************

  Synopsis    [ Private service to deinitialize the internal members. ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ ]

******************************************************************************/
static void game_be_fsm_deinit(GameBeFsm_ptr self)
{
    int i;

    for(i=0;i<n_players;i++)
  BeFsm_destroy(self->players[i]);
}
