#!/bin/bash
echo "Fixing secclass issue..."
FILENAME="scripts/selinux/genheaders/genheaders.c"
grep -v  "<sys/socket.h>" $FILENAME > "${FILENAME}_BAK" ; mv "${FILENAME}_BAK" $FILENAME

FILENAME="scripts/selinux/mdp/mdp.c"
grep -v  "<sys/socket.h>" $FILENAME > "${FILENAME}_BAK" ; mv "${FILENAME}_BAK" $FILENAME

FILENAME="security/selinux/include/classmap.h"
sed 's/include <linux\/capability.h>/include <linux\/capability.h>\n#include <linux\/socket.h>/' $FILENAME > "${FILENAME}_BAK"; mv "${FILENAME}_BAK" $FILENAME

echo "Updating git repo. This may take some time..."
rm -rf .git
git init
git add .
git commit -m "First commit"
echo "All is done"