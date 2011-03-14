#!/bin/bash
#
# Copyright © 2011 blakawk <blakawk@gentooist.com>
# All rights reserved.  Released under GPLv3 License.
#
# This program is free software: you can redistribute
# it  and/or  modify  it under  the  terms of the GNU.
# General  Public  License  as  published by the Free
# Software Foundation, version 3 of the License.
#
# This  program  is  distributed  in the hope that it
# will be useful, but  WITHOUT  ANY WARRANTY; without
# even  the  implied  warranty  of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.
#
# See   the  GNU  General  Public  License  for  more
# details. You should have received a copy of the GNU
# General Public License along with this program.  If
# not, see <http://www.gnu.org/licenses/>.

exec 2>/dev/null

max="${1:-8}"
batch="${2:-32}"
interval="${3:-60}"
declare -a clients

client ${batch} &>/dev/null &
master=$!

echo "master pid is $master"
sleep ${interval}

while [ ${#clients[@]} -lt $max ]
do
    client ${batch} &>/dev/null &
    lastpid=$!
    clients[${#clients[@]}]=${lastpid}
    echo "launched ${#clients[@]}, last pid was ${lastpid}"
    sleep ${interval}
done

count=${#clients[@]}
while [ ${count} -gt 0 ]
do
    while true
    do
        toKill="$((RANDOM % ${#clients[@]}))"
        if [ -n "${clients[${toKill}]}" ]
        then
            break
        fi
    done
    echo "killing client #${toKill} ${clients[${toKill}]}, remaining ${#clients[@]}"
    kill ${clients[${toKill}]}
    clients[${toKill}]=""
    ((count--))
    sleep ${interval}
done

kill ${master}

echo "done"
