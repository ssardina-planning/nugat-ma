/**CFile***********************************************************************

  FileName    [ GamePlayer.c ]

  PackageName [ game ]

  Synopsis    [ Implementaion of class 'GamePlayer' ]

  Description [ This is not much more than an enumeration. The main
                point of giving it its own file was to avoid having to
                declare it in GameBddFsm.h (which was done b/c of
                problems with order of includes). ]

  SeeAlso     [ ]

  Author      [ Andrei Tchaltsev, Viktor Schuppan ]

  Copyright   [
  This file is part of the ``game'' package of NuGaT.
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

#include "GamePlayer.h"

#include "utils/utils.h"
#include "utils/ustring.h"

EXTERN int n_players;

static char rcsid[] UTIL_UNUSED = "$Id: GamePlayer.c,v 1.1.2.1 2010-02-05 17:19:08 nusmv Exp $";

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
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

/*---------------------------------------------------------------------------*/
/* Definition of exported functions                                          */
/*---------------------------------------------------------------------------*/

/**Function********************************************************************

  Synopsis    [ Takes a player name as GamePlayer and returns it as a string. ]

  Description [ ]

  SideEffects [ ]

  SeeAlso     [ Game_StrToPlayer ]

******************************************************************************/
string_ptr Game_PlayerToStr(NuSMVEnv_ptr env,int player)
{
  char str[50];
  int i;

  sprintf(str,"%d",player);

  for(i=0;i<n_players;i++)
    if(player==i+1)
      return UStringMgr_find_string(USTRING_MGR(NuSMVEnv_get_value(env, ENV_STRING_MGR)),str);

  nusmv_assert(false); /* illegal value */
}

/**Function********************************************************************

  Synopsis    [ Takes a player name as a string and returns it as GamePlayer. ]

  Description [ Player names can be constructed only from
                " i+1" . ]

  SideEffects [ ]

  SeeAlso     [ Game_PlayerToStr ]

******************************************************************************/
int Game_StrToPlayer(NuSMVEnv_ptr env,string_ptr str)
{
  char tmp[50];
  int i;

  for(i=0;i<n_players;i++) {
    sprintf(tmp,"%d",i+1);
    if (UStringMgr_find_string(USTRING_MGR(NuSMVEnv_get_value(env, ENV_STRING_MGR)),tmp) == str) return (i+1);
  }

  nusmv_assert(false); /* illegal value */
}

/**AutomaticEnd***************************************************************/
