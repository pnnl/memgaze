#!/bin/bash

case $# in
0) echo "Usage: $0 list_file1.txt [ list_file2.txt ...]"; exit;;
esac

exedir=`dirname $0`

rezdir=src
mkdir -p ${rezdir}

cat > script_updatePaths.sh << EOF
#!/bin/bash

infile=\$1
tempfile=tmp_\$1
echo "Processing file \${infile}"
EOF
echo -n "sed" >> script_updatePaths.sh

for ff in $*
do
   cprefix=`cat ${ff} | ${exedir}/commonPrefix`
   canonPrefix=`${exedir}/canonicalize ${cprefix}`
   destdir=`echo ${ff} | cut -d"." -f1 | cut -d"_" -f2-`
   mkdir -p ${rezdir}/${destdir}
   sedcprefix=`echo ${cprefix} | sed 's/\./\\\./g'`
   echo "${cprefix}  ->  ${rezdir}/${destdir}"
   echo  "  -e \"s,\(<A f=\\\"\)${sedcprefix}/\?,\1${rezdir}/${destdir}/,g;s,\(<F n=\\\"\)${sedcprefix}/\?,\1${rezdir}/${destdir}/,g\" \\" >> script_updatePaths.sh
#   echo -n "-e \"s,${canonPrefix},${rezdir}/${destdir},g\" " >> script_updatePaths.sh
   plen=`echo -n ${cprefix} | wc -c | tr -d " "`
   plen=`expr ${plen} + 1`
   echo "plen=${plen}"
   cat ${ff} |{
     while read line
     do
#        line=`${exedir}/canonicalize ${line}`
        destfile=${rezdir}/${destdir}`echo -n ${line} | cut -c${plen}-`
        dname=`dirname ${destfile}`
        mkdir -p "${dname}"
        cp "${line}" "${destfile}"
     done
   }
done

echo '  ${infile} > ${tempfile}' >> script_updatePaths.sh
echo 'mv ${tempfile} ${infile}' >> script_updatePaths.sh
chmod +x script_updatePaths.sh
