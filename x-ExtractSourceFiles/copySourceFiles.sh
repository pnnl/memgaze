#!/bin/bash

SOURCE="${BASH_SOURCE[0]}"
while [ -h "$SOURCE" ]; do
  DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"
  SOURCE="$(readlink "$SOURCE")"
  [[ $SOURCE != /* ]] && SOURCE="$DIR/$SOURCE"
done
DIR="$( cd -P "$( dirname "$SOURCE" )" && pwd )"

case $# in
1) srcFile=$1;;
*) echo "Specify the name of the XML file to be parsed."; exit;;
esac

tmp1=`mktemp`
egrep -e "<(A f)|(F n)=" ${srcFile} | cut -d"\"" -f2 | sort | uniq > ${tmp1}

tmp2=`mktemp`
cat ${tmp1} | while read line
do 
   if [ -f ${line} ]
   then
#      if [[ ${line} == ../* ]]
#      then
#         fname=`${DIR}/canonicalize ${line}`
#      else
         fname=${line}
#      fi
      echo `dirname ${fname}`"/,"${fname};
   fi
done > ${tmp2}

keys=`cat ${tmp2} | sed -e 's,^/,,g' | cut -d"/" -f1 | sort -u`
echo ${tmp2}
params=""
for ff in ${keys}
do
   echo ">${ff}<"
   key=`echo ${ff} | sed -e 's,\.,\\\.,g'`
   if [ "X${ff}" == "X." ]
   then
      fname="sys"
   else
      fname=${ff}
   fi
   egrep -e "^/?${key}" ${tmp2} | cut -d"," -f2- > zyx_${fname}.ll
   params="${params} zyx_${fname}.ll"
done

unlink ${tmp1}
unlink ${tmp2}

${DIR}/collectSourceFiles.sh ${params}
