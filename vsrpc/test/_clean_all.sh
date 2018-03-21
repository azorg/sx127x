#! /bin/sh

c="_clean.sh"
p=`pwd`

for d in *
do
  test -d "$d" || continue
  cd "$d"
  if [ -x "$c" ]
  then
    echo "clean in $d"
    sh "$c"  
  fi
  cd "$p"
done
