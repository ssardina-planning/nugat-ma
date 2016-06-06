#!/usr/bin/env bash

# param: input.ispl
echo "MODULE b1

GAME

" > $1.auto.smv

cat $1 | sed -E '/end (Agent|Evolution|Obsvars|Vars|Evaluation|InitStates|Formulae|Groups)/d' \
       | sed -E '/(Fairness|RedStates|Lobsvars)/d' \
       | sed 's/Vars:/VAR/g' \
       | sed 's/Environment.//g' \
       | sed -E 's/Evolution(.*)/TRANS/g' \
       | sed -E 's/end Protocol/ esac/g' | sed -E 's/Protocol(.*)/TRANS\n     case/g' \
       | sed -e 's/\(\S.*\) if \(.*\);/\2 -> next(\1);/g' \
       | sed -e 's/ or / | /g' | sed -e 's/ and / \& /g' \
       | sed -e 's/ or/ |/g' | sed -e 's/ and/ \&/g' \
       | sed -e 's/true/TRUE/g' | sed -e 's/false/FALSE/g' \
       | awk 'BEGIN { cntr = -1 }
                   /Agent / {cntr++;print cntr," -- agent:",$2}
                   / state / {gsub(/state/, "state"cntr);}
                   / serviced / {gsub(/serviced/, "serviced"cntr);}
                   / broken / {gsub(/broken/, "broken"cntr);}
                   /Actions =/ {
                       split($0,arr,"Actions =");
                       print "        actions"cntr,":"arr[2];
                   }
                   /Action(\s*)=/ {gsub(/Action/, "actions"cntr);}
                   !/(Agent |Actions =| state |Action(\s*)=)/ { print $0 }

               END {



               }

               ' >> $1.auto.smv

# InitStates -> split into agents
# Formulae -> ATLREACHTARGET (1)
# define Evaluation VAR

#      | awk '/  -- / {idA[$3".Action"]=$1;}
#              /(.*).Action/ {print "actions"idA[$1];}
#             !/([0-9] -- (.*)|.Action)/ { print $0 }'
#      | sed -e 's/\(\S.*\).Action/actions\1/g'

#

