#!/usr/bin/env bash

# param: input.ispl
echo $1
echo "MODULE b1

GAME
" > $1.auto.smv

cat $1 | sed -E '/Fairness|RedStates|Lobsvars|RedStates/d' \
       | sed -E '/end (Agent|Evolution|Obsvars|Vars|Evaluation|InitStates|Formulae|Groups)/d' \
       | sed -E 's/Formulae|Groups//g' \
       | sed -E 's/(Vars:|Obsvars:)/VAR/g' \
       | sed -E 's/InitStates/INIT/g' \
       | sed -E 's/Evaluation/ TRANS -- Evaluation/g' \
       | sed -E 's/Other(\s*):/TRUE:/g' \
       | sed 's/Environment.Action/actions0/g' \
       | sed 's/Environment.//g' \
       | sed -E 's/Evolution(.*):/TRANS/g' \
       | sed -E 's/end Protocol/ esac/g' | sed -E 's/Protocol(.*)/TRANS\n   case/g' \
       | sed -E 's/(\S.*) if (.*?);/\2 -> next(\1);/g' \
       | sed -e 's/ or / | /g' | sed -e 's/ and / \& /g' \
       | sed -e 's/ or/ |/g' | sed -e 's/ and/ \&/g' \
       | sed -e 's/true/TRUE/g' | sed -e 's/false/FALSE/g' \
       | sed -E 's/([a-zA-Z]*)([0-9])\.([a-zA-Z]*)/\3\2/g' \
       | sed -E 's/Action([0-9])/actions\1/g' \
       | sed -E '/^(\s*)g(.*)=/d' \
       | sed -E 's/    <g(.*)>F/ATLREACHTARGET (\1)/g' \
       | awk 'BEGIN { cntr = -1 }
                   /Agent / {cntr++;din=":";din2=";";print cntr," -- ",$2; agents[$2]}
                   / Action(\s*)=/ {gsub(/ Action(\s*)=/, " actions"cntr" =");}
                   / Actions(\s*)=/ {gsub(/ Actions(\s*)=/, "   actions"cntr" :");}

                   /case/ { din=": next(actions"cntr") in";din2=";"; }
                   /:/ { gsub(/:/,din);}
                   /esac/ { din=":" }

                   /TRANS/ { din2=";\n  TRANS" }
                   /;/ { gsub(/;/,din2);}
                   /VAR|INIT/ { din2=";" }

                   /see/ {gsub(/see/, "see"cntr);}
                   /mem/ {gsub(/mem/, "mem"cntr);}
                   /othersayknow/ {gsub(/othersayknow/, "othersayknow"cntr);}

                   /Evaluation/ {cntr=""}

                   !/(Agent |Actions =| see | mem | othersayknow | Action(\s*)=)/ { print $0 }

               '  >> $1.auto.smv


#while read line
#do
    #ida="$(echo $line | awk '/agent:/ { print $1 }')"
    #idname = $(echo $line | awk '/agent:/ { print $4 }')
    #if ["$ida" -neq ""]; then
    #    echo $line
    #fi
    #agents = $();
    #echo $agents
#done < $1.auto.smv

# InitStates -> split into agents
# Formulae -> ATLREACHTARGET (1)
# define Evaluation VAR

#      | awk '/  -- / {idA[$3".Action"]=$1;}
#              /(.*).Action/ {print "actions"idA[$1];}
#             !/([0-9] -- (.*)|.Action)/ { print $0 }'
#      | sed -e 's/\(\S.*\).Action/actions\1/g'

#



