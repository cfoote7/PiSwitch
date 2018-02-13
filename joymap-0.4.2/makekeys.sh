#!/bin/sh
cat << _EOF > validkeys.h
struct keymap_struct {
    char key[32];
    int value;
};

extern struct keymap_struct keymap[];
_EOF

echo '#include "validkeys.h"' > validkeys.c
echo 'struct keymap_struct keymap[]={' >> validkeys.c
cat keys.txt | sed 's/\\/__slash__/g' | (while read key value; do
echo "    {\"$key\", $value}," | sed 's/__slash__/\\\\/g' >> validkeys.c
done)
echo '    {"", -1},' >> validkeys.c
echo '};' >> validkeys.c
