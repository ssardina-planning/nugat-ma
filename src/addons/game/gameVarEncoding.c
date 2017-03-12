/**CFile***********************************************************************

  FileName    [ gameVarEncoding.c ]

  PackageName [ game ]

  Synopsis    [ Defines functions to perform variable encoding of a game
                hierarchy. ]

  Description [ ]

  SeeAlso     [ compileFlatten.h ]

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

#include "gameInt.h"

#include "compile/compile.h"
#include "enc/enc.h"
#include "opt/opt.h"
#include "utils/utils.h"
#include "utils/error.h"
#include "code/nusmv/core/utils/ErrorMgr.h"
#include "utils/ucmd.h"

#include <stdio.h>


/*---------------------------------------------------------------------------*/
static char rcsid[] UTIL_UNUSED = "$Id: gameVarEncoding.c,v 1.1.2.2 2010-02-08 16:19:04 nusmv Exp $";
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Structure declarations                                                    */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Type declarations                                                         */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Variable declarations                                                     */
/*---------------------------------------------------------------------------*/

EXTERN cmp_struct_ptr cmps;

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

  Synopsis    [ The function is used to perform the "encode_variables"
                command. ]

  Description [ This function is used in CommandEncodeVariables to
                adopt "encode_variables" command to deal with a game
                declaration. Returns 0 if everything is ok, and 1
                otherwise. ]

  SideEffects [ ]

  SeeAlso     [ CommandGameEncodeVariables, compile_encode_variables ]

******************************************************************************/
int Game_CommandEncodeVariables(NuSMVEnv_ptr env, char* input_order_file_name)
{
  BoolEnc_ptr bool_enc;
  BddEnc_ptr bdd_enc;

  ErrorMgr_ptr const errmgr = ERROR_MGR(NuSMVEnv_get_value(env, ENV_ERROR_MANAGER));
  OptsHandler_ptr opts = OPTS_HANDLER(NuSMVEnv_get_value(env, ENV_OPTS_HANDLER));
  StreamMgr_ptr streams = STREAM_MGR(NuSMVEnv_get_value(env, ENV_STREAM_MANAGER));
  OStream_ptr errostream = StreamMgr_get_error_ostream(streams);
  int i;
  
  if (opt_verbose_level_gt(opts, 0)) {
    OStream_printf(errostream, "Building variables...");
  }

  if (input_order_file_name != NIL(char)) {
    set_input_order_file(opts, input_order_file_name);
  }

    /* Creates the bool encoding, and commits the model layer, which was
    created during the flattening phase. */
    Enc_init_bool_encoding(env);
    bool_enc = NuSMVEnv_get_value(env, ENV_BOOL_ENCODER);
  for(i=0;i<n_players;i++) {
    char str[50];
    sprintf(str,"layer_of_PLAYER_%d",i+1);
    BaseEnc_commit_layer(BASE_ENC(bool_enc), str);
  }

    /* Creates the bdd encoding, and again commits the model layer. */
    Enc_init_bdd_encoding(env,input_order_file_name);
    bdd_enc = NuSMVEnv_get_value(env, ENV_BDD_ENCODER);

    for(i=0;i<n_players;i++) {
      char str[50];
      sprintf(str, "layer_of_PLAYER_%d", i + 1);
      BaseEnc_commit_layer(BASE_ENC(bdd_enc), str);
    }

  cmp_struct_set_encode_variables(cmps);

  if (!opt_reorder(opts) &&
      !is_default_order_file(opts) &&
      !util_is_string_null(get_output_order_file(opts))) {
    VarOrderingType dump_type;
    if (opt_write_order_dumps_bits(opts)) {
      dump_type = DUMP_BITS;
    }
    else dump_type = DUMP_DEFAULT;

    BddEnc_write_var_ordering(bdd_enc,
                              get_output_order_file(opts),
                              dump_type);

    /* batch mode: */
    if (opt_batch(opts)) { ErrorMgr_nusmv_exit(errmgr,0); }
  }

  if (opt_verbose_level_gt(opts, 0)) {
    OStream_printf(errostream, "...done\n");
  }

  return 0;
}

/*---------------------------------------------------------------------------*/
/* Definition of static functions                                            */
/*---------------------------------------------------------------------------*/

/**AutomaticEnd***************************************************************/
