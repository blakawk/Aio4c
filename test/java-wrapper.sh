#!/bin/sh

java="$1"
args="$2"
class="`echo $3 | sed -e 's@.class@@' -e 's@^.*/\([^/]*\)$@\1@'`"

echo $java $args $class
exec -- $java $args $class
