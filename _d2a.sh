#!/bin/sh
#
# "Archive directory to tarball or zip/7zip"
# Version: 0.5b
# File: "_d2a.sh"
# Author: Alex Zorg <azorg@mail.ru>
# Last update: 2015.02.08

dir=`pwd`
prj=`basename ${dir}`
out="${prj}-$(date +%Y%m%d_%H%M)"
tmp=`mktemp || echo "/tmp/___tmp_d2a.$(out)"`
rm -f "${tmp}"

cd ..

[ -n "$1" ] && fmt="$1" || fmt="tgz"

usage()
{
  echo "usage: `basename $0` tar|tar.gz|tgz|tar.bz2|zip|pzip|7z|p7z|tz|ptz|t7|pt7"
}

case "$fmt" in
  tar)                          tar cvpf  "$tmp" "$prj" ;;
  tar.gz|tgz)                   tar cvzpf "$tmp" "$prj" ;;
  tar.bz2|tbz*)  fmt="tar.bz2"; tar cvjpf "$tmp" "$prj" ;;
 
  zip)            zip -9 -r -y    "$tmp" "$prj" ;;
  pz*) fmt="zip"; zip -9 -r -y -e "$tmp" "$prj" ;;
  
  7*)            7z a -t7z -mx=9 -ms=on -r    "$tmp" "$prj" ;;
  p7*) fmt="7z"; 7z a -t7z -mx=9 -ms=on -r -p "$tmp" "$prj" ;;
  
  tar.zip|tz|ptz)
    [ "$fmt" = "ptz" ] && opt="-e" || opt=
    fmt="tar.zip"
    mkdir    "${tmp}.dir"
    tar cvpf "${tmp}.dir/${prj}.tar" "$prj"
    cd       "${tmp}.dir"
    zip -9 $opt "${tmp}.zip" "${prj}.tar"
    rm                       "${prj}.tar"
    cd -
    rmdir    "${tmp}.dir"
    tmp="${tmp}.zip"
    ;;
  
  tar.7z*|t7*|pt7)
    [ "$fmt" = "pt7" ] && opt="-p" || opt=
    fmt="tar.7z"
    mkdir    "${tmp}.dir"
    tar cvpf "${tmp}.dir/${prj}.tar" "$prj"
    cd       "${tmp}.dir"
    7z a -t7z -mx=9 -ms=on $opt "${tmp}.7z" "${prj}.tar" 
    rm                                      "${prj}.tar"
    cd -
    rmdir    "${tmp}.dir"
    tmp="${tmp}.7z"
    ;;
  
  --help|-h)
    usage
    cd "${dir}"
    exit
    ;;
  
  *)
    usage >&2
    cd "${dir}"
    exit
    ;;
esac

cd "${dir}"

of="${out}.${fmt}"

if [ -f "${tmp}" ]
then
  mv "${tmp}" "${of}"
else
  echo "error: Can't create \"${of}\" archive!" >&2
fi

#*** end of "_d2a.sh" file ***#

